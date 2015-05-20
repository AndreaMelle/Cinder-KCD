#include "KCDDevice.h"
#include <exception>
#include <algorithm>

using namespace kcd;
using namespace cv;

static const long long kThreadSleepDuration = 30L;

KCDDevice::KCDDevice() : mStartTime(0),
	mLastCounter(0),
	mFramesSinceUpdate(0),
	mFreq(0),
	mNextStatusTime(0LL),
	mKinectSensor(NULL),
	mCoordinateMapper(NULL),
	mFrameReader(NULL),
	mDepthCoordinates(NULL),
	mColorBuffer(NULL),
	mHasActiveUser(false),
	mUserEngagedThreshold(6.25f),
	mUserLostThreshold(6.25f),
	mActiveBodyIndex(0),
	mActiveUserTrackingId(0),
	mLatestUserDistance(std::numeric_limits<float>::max()),
	mScreenSpaceScale(ci::Vec2f::one())
{
	//TODO thread-safe setter
	//mScreenSpaceScale = Vec2f(getWindowSize()) / Vec2f(mChannelBodyIndex.getSize());

	LARGE_INTEGER qpf = { 0 };
	if (QueryPerformanceFrequency(&qpf))
	{
		mFreq = double(qpf.QuadPart);
	}
}

KCDDevice::~KCDDevice()
{
	this->teardown();
}

void KCDDevice::setup()
{
	ci::app::App* mainApp = ci::app::App::get();
	if (!mainApp)
	{
		throw "KCDDevice cannot be invoked before App creation";
	}

	int depthFrameArea = DepthFrameWidth * DepthFrameHeight;
	int colorFrameArea = ColorFrameWidth * ColorFrameHeight;

	mDepthCoordinates = new DepthSpacePoint[depthFrameArea];

	mColorBuffer = new BYTE[colorFrameArea * BGRA_SIZE];
	memset(mColorBuffer, 255, colorFrameArea * BGRA_SIZE);

	mMaskBuffer = new BYTE[colorFrameArea * MASK_SIZE];
	memset(mMaskBuffer, 0, colorFrameArea * MASK_SIZE);

	mProcess.mThreadCallback = [&]()
	{
		HRESULT hr;
		hr = _thread_setup();

		if (FAILED(hr))
		{
			_thread_teardown();
			return;
		}

		while (mProcess.mRunning)
		{
			hr = _thread_update();

			if (FAILED(hr))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(kThreadSleepDuration));
				continue;
			}
		}

		_thread_teardown();
	};

	mUpdateConnection = mainApp->getSignalUpdate().connect(std::bind(&KCDDevice::update, this));

	mProcess.start();
}

void KCDDevice::teardown()
{
	mUpdateConnection.disconnect();

	mProcess.stop();

	if (mColorBuffer)
	{
		delete[] mColorBuffer;
		mColorBuffer = NULL;
	}

	if (mMaskBuffer)
	{
		delete[] mMaskBuffer;
		mMaskBuffer = NULL;
	}

	if (mDepthCoordinates)
	{
		delete[] mDepthCoordinates;
		mDepthCoordinates = NULL;
	}
}

HRESULT KCDDevice::_thread_setup()
{
	HRESULT hr;
	hr = GetDefaultKinectSensor(&mKinectSensor);

	if (FAILED(hr))
	{
		this->handleKinectError(hr);
		return hr;
	}

	if (mKinectSensor)
	{
		if (SUCCEEDED(hr))
		{
			hr = mKinectSensor->get_CoordinateMapper(&mCoordinateMapper);
		}

		hr = mKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = mKinectSensor->OpenMultiSourceFrameReader(
				FrameSourceTypes::FrameSourceTypes_Depth |
				FrameSourceTypes::FrameSourceTypes_Color |
				FrameSourceTypes::FrameSourceTypes_BodyIndex |
				FrameSourceTypes::FrameSourceTypes_Body, &mFrameReader);
		}
	}

	if (!mKinectSensor || FAILED(hr))
	{
		this->handleKinectError(hr);
		return E_FAIL;
	}

	return hr;
}

HRESULT KCDDevice::_thread_teardown()
{
	__safe_release(mFrameReader);
	__safe_release(mCoordinateMapper);

	if (mKinectSensor)
	{
		mKinectSensor->Close();
	}

	__safe_release(mKinectSensor);

	return S_OK;
}

void KCDDevice::update()
{
	// lock and copy "small" variables such as performance queries
	mPerformanceQueryMutex.lock();
	mPerformanceQuery.fps = mLatestFpsReading;
	mPerformanceQuery.elapsedTime = mLatestTimeReading;
	mPerformanceQueryMutex.unlock();	

	// update Kalman or passthrough filter results for 60 fps

	// 1. prediction

	// 2. measurement
	std::vector<NUIObservation> measurements;
	mObservationMutex.lock();
	measurements = std::vector<NUIObservation>(mObservations); //copy. TODO: maybe not in the future
	mObservations.clear(); // after reading the observations, we clear
	mObservationMutex.unlock();

	// 3. update

	// fill the results of body parts tracking, and update latest distance

	bindIt it;
	for (it = mBindables.begin(); it != mBindables.end(); ++it)
	{
		(*it)->update(shared_from_this());
	}

}

HRESULT KCDDevice::_thread_color_subupdate(IMultiSourceFrame*& multiSourceFrame, BYTE* maskBuffer)
{
	IColorFrame* colorFrame = NULL;
	IColorFrameReference* colorFrameRef = NULL;
	HRESULT hr = S_OK;

	hr = multiSourceFrame->get_ColorFrameReference(&colorFrameRef);

	if (SUCCEEDED(hr))
	{
		hr = colorFrameRef->AcquireFrame(&colorFrame);
	}

	__safe_release(colorFrameRef);
	
	if (SUCCEEDED(hr))
	{
#if QUERY_FRAME_DESCRIPTION
		IFrameDescription* colorFrameDescription = NULL;
		int colorWidth = 0;
		int colorHeight = 0;
		unsigned int colorBytesPerPixel = 0;
#endif

		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT colorBufferSize = 0;
		BYTE* colorBuffer = NULL;

#if PROFILE_KINECT_FPS
		INT64 colorTime = 0;
		HRESULT hrTime = colorFrame->get_RelativeTime(&colorTime);
		if (SUCCEEDED(hrTime))
		{
			this->profile(colorTime);
		}
#endif

#if QUERY_FRAME_DESCRIPTION
		hr = colorFrame->get_FrameDescription(&colorFrameDescription);
		if (SUCCEEDED(hr)) { hr = colorFrameDescription->get_Width(&colorWidth); }
		if (SUCCEEDED(hr)) { hr = colorFrameDescription->get_Height(&colorHeight); }
		if (SUCCEEDED(hr)) { hr = colorFrameDescription->get_BytesPerPixel(&colorBytesPerPixel); }
#endif

		if (SUCCEEDED(hr))
		{
			hr = colorFrame->get_RawColorImageFormat(&imageFormat);
		}

		mImageDataMutex.lock();

		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = colorFrame->AccessRawUnderlyingBuffer(&colorBufferSize, &colorBuffer);
			}
			else if (mColorBuffer)
			{
				colorBufferSize = ColorFrameWidth * ColorFrameHeight * BGRA_SIZE;
				colorBuffer = mColorBuffer;

				hr = colorFrame->CopyConvertedFrameDataToArray(colorBufferSize, colorBuffer, ColorImageFormat_Bgra);

				if (maskBuffer)
				{
					BYTE* src = NULL;
					BYTE value = 0;

					for (register int i = 0; i < (ColorFrameWidth * ColorFrameHeight); ++i)
					{
						src = mColorBuffer + (BGRA_SIZE * i);
						value = *(maskBuffer + i);
						if (value >= 128)
							*(src + 3) = value;
						else
							*(src + 3) = 0;
					}
				}	
			}
			else
			{
				hr = E_FAIL;
			}
		}

		mImageDataMutex.unlock();

#if QUERY_FRAME_DESCRIPTION
		__safe_release(colorFrameDescription);
#endif
	}

	__safe_release(colorFrame);

	return hr;
}

HRESULT KCDDevice::_thread_activeuser_subupdate(IMultiSourceFrame*& multiSourceFrame)
{
	IBodyFrame* bodyFrame = NULL;
	IBodyFrameReference* bodyFrameRef = NULL;
	HRESULT hr = S_OK;

	hr = multiSourceFrame->get_BodyFrameReference(&bodyFrameRef);

	if (mCoordinateMapper && SUCCEEDED(hr))
	{
		hr = bodyFrameRef->AcquireFrame(&bodyFrame);
	}

	__safe_release(bodyFrameRef);

	if (SUCCEEDED(hr))
	{
		IBody* bodies[BODY_COUNT] = { 0 };

		if (SUCCEEDED(hr))
		{
			hr = bodyFrame->GetAndRefreshBodyData(_countof(bodies), bodies);
		}

		if (SUCCEEDED(hr))
		{
			bool seen = false;

			for (register int i = 0; i < BODY_COUNT; ++i)
			{
				IBody* body = bodies[i];
				if (body)
				{
					BOOLEAN tracked = false;
					hr = body->get_IsTracked(&tracked);

					if (tracked)
					{
						UINT64 trackingId = 0;
						hr = body->get_TrackingId(&trackingId);

						if (SUCCEEDED(hr))
						{
							if (mHasActiveUser)
							{
								if (trackingId == mActiveUserTrackingId)
								{
									mActiveBodyIndex = i;
									this->_thread_bodytracking_subupdate(body);

									if (mLatestUserDistance <= mUserLostThreshold)
									{
										seen = true;
										break;
									}
								}
							}
							else
							{
								Joint joints[JointType_Count];
								hr = body->GetJoints(_countof(joints), joints);

								if (SUCCEEDED(hr))
								{
									for (int j = 0; j < _countof(joints); ++j)
									{
										if (joints[j].JointType == JointType_SpineBase && joints[j].TrackingState == TrackingState_Tracked)
										{
											CameraSpacePoint pSpineBase = joints[j].Position;
											float distance = ci::Vec3f(pSpineBase.X, pSpineBase.Y, pSpineBase.Z).distanceSquared(ci::Vec3f::zero());

											if (distance <= mUserEngagedThreshold)
											{
												mActiveUserTrackingId = trackingId;
												mActiveBodyIndex = i;
												mHasActiveUser = true;

												mObservationMutex.lock();
												mObservations.clear(); // new user, clear old observations if any
												mObservationMutex.unlock();

												this->_thread_bodytracking_subupdate(body);

												mActiveUserEventBufferMutex.lock();
												mActiveUserEventBuffer.push_back(NUIActiveUserEvent::ACTIVE_USER_NEW);
												mActiveUserEventBufferMutex.unlock();

												seen = true;
												break;
											}
										}
									}
								}

						
							}
						}
					}
				}
			}

			// check active user not seen
			if (mHasActiveUser && !seen)
			{
				mHasActiveUser = false;
				mActiveBodyIndex = 0;
				mActiveUserTrackingId = 0;

				mObservationMutex.lock();
				mObservations.clear(); // assume that if a user is lost, observations are not valid anymore
				mObservationMutex.unlock();

				mActiveUserEventBufferMutex.lock();
				mActiveUserEventBuffer.push_back(NUIActiveUserEvent::ACTIVE_USER_LOST);
				mActiveUserEventBufferMutex.unlock();
			}
		}

		for (int i = 0; i < _countof(bodies); ++i)
		{
			__safe_release(bodies[i]);
		}
	}

	__safe_release(bodyFrame);

	return hr;
}

HRESULT KCDDevice::_thread_bodytracking_subupdate(IBody*& body)
{
	Joint joints[JointType_Count];
	HRESULT hr = body->GetJoints(_countof(joints), joints);

	if (SUCCEEDED(hr))
	{
		TrackingState ts = TrackingState_NotTracked;
		JointType jt = JointType_Count;

		for (int j = 0; j < _countof(joints); ++j)
		{
			ts = joints[j].TrackingState;

			if (ts == TrackingState_Tracked || ts == TrackingState_Inferred)
			{
				jt = joints[j].JointType;
				
				if (jt == JointType_HandLeft ||
					jt == JointType_HandRight ||
					jt == JointType_Head ||
					jt == JointType_SpineBase)
				{
					NUIObservation obs;
					DepthSpacePoint dp = { 0 };
					CameraSpacePoint csp = joints[j].Position;

					mCoordinateMapper->MapCameraPointToDepthSpace(csp, &dp);

					obs.id = JointType_HandLeft;
					obs.x = dp.X * mScreenSpaceScale.x;
					obs.y = dp.Y * mScreenSpaceScale.y;

					mObservationMutex.lock();
					mObservations.push_back(obs);
					mObservationMutex.unlock();

					if (jt == JointType_SpineBase)
					{
						mObservationMutex.lock();
						mLatestUserDistance = ci::Vec3f(csp.X, csp.Y, csp.Z).distanceSquared(ci::Vec3f::zero());
						mObservationMutex.unlock();
					}
				}
			}
		}
	}

	return hr;
}

HRESULT KCDDevice::_thread_bodymask_subupdate(IMultiSourceFrame*& multiSourceFrame, const UINT& userIdx)
{
	IDepthFrame* depthFrame = NULL;
	IBodyIndexFrame* bodyIndexFrame = NULL;
	HRESULT hr = S_OK;
	
	IDepthFrameReference* depthFrameRef = NULL;
	hr = multiSourceFrame->get_DepthFrameReference(&depthFrameRef);
	if (SUCCEEDED(hr))
	{
		hr = depthFrameRef->AcquireFrame(&depthFrame);
	}
	__safe_release(depthFrameRef);

	if (SUCCEEDED(hr))
	{
		IBodyIndexFrameReference* bodyIndexFrameRef = NULL;
		hr = multiSourceFrame->get_BodyIndexFrameReference(&bodyIndexFrameRef);
		if (SUCCEEDED(hr))
		{
			hr = bodyIndexFrameRef->AcquireFrame(&bodyIndexFrame);
		}
		__safe_release(bodyIndexFrameRef);
	}

	if (SUCCEEDED(hr))
	{
#if QUERY_FRAME_DESCRIPTION
		IFrameDescription* depthFrameDescription = NULL;
		int depthWidth = 0;
		int depthHeight = 0;
		IFrameDescription* bodyIndexFrameDescription = NULL;
		int bodyIndexWidth = 0;
		int bodyIndexHeight = 0;
#endif

		UINT depthBufferSize = 0;
		UINT16* depthBuffer = NULL;
		
		UINT bodyIndexBufferSize = 0;
		BYTE* bodyIndexBuffer = NULL;

#if QUERY_FRAME_DESCRIPTION
		if (SUCCEEDED(hr)) { hr = depthFrame->get_FrameDescription(&depthFrameDescription); }
		if (SUCCEEDED(hr)) { hr = depthFrameDescription->get_Width(&depthWidth); }
		if (SUCCEEDED(hr)) { hr = depthFrameDescription->get_Height(&depthHeight); }
		if (SUCCEEDED(hr)) { hr = bodyIndexFrame->get_FrameDescription(&bodyIndexFrameDescription); }
		if (SUCCEEDED(hr)) { hr = bodyIndexFrameDescription->get_Width(&bodyIndexWidth); }
		if (SUCCEEDED(hr)) { hr = bodyIndexFrameDescription->get_Height(&bodyIndexHeight); }
#endif

		if (SUCCEEDED(hr))
		{
			hr = depthFrame->AccessUnderlyingBuffer(&depthBufferSize, &depthBuffer);
		}

		if (SUCCEEDED(hr))
		{
			hr = bodyIndexFrame->AccessUnderlyingBuffer(&bodyIndexBufferSize, &bodyIndexBuffer);
		}

		if (SUCCEEDED(hr))
		{
			// process frame now!! ;)
			
			// simple approach
			// write the user id map as alpha channel of the colorbuffer, prior to upload to GPU

			// more complex
			// write the alpha channel to a separate opencv matrix for refinement, then combine with color buffer
			// look into OpenCV API: surely non-contiguous processing is expensive
			// but looping for channel ricombination? is there a faster way?

			if (mCoordinateMapper && mDepthCoordinates && depthBuffer && mMaskBuffer && bodyIndexBuffer
#if QUERY_FRAME_DESCRIPTION
				&& (depthWidth == DepthFrameWidth) && (depthHeight == DepthFrameHeight)
				&& (colorWidth == ColorFrameWidth) && (colorHeight == ColorFrameHeight)
				&& (bodyIndexWidth == DepthFrameWidth) && (bodyIndexHeight == DepthFrameHeight)
#endif
				)
			{
				hr = mCoordinateMapper->MapColorFrameToDepthSpace(DepthFrameWidth * DepthFrameHeight, depthBuffer, ColorFrameWidth * ColorFrameHeight, mDepthCoordinates);

				if (SUCCEEDED(hr))
				{
					for (register int i = 0; i < (ColorFrameWidth * ColorFrameHeight); ++i)
					{
						BYTE* src = mMaskBuffer + i;
						*src = 0;

						DepthSpacePoint p = mDepthCoordinates[i];

						if (p.X != -std::numeric_limits<float>::infinity() && p.Y != -std::numeric_limits<float>::infinity())
						{
							int depthX = static_cast<int>(p.X + 0.5f);
							int depthY = static_cast<int>(p.Y + 0.5f);

							if ((depthX >= 0 && depthX < DepthFrameWidth) && (depthY >= 0 && depthY < DepthFrameHeight))
							{
								BYTE player = bodyIndexBuffer[depthX + (depthY * DepthFrameWidth)];

								if (player == userIdx)
								{
									*src = 255;
								}
							}
						}
					}

					Mat maskMat;
					maskMat = Mat(Size(ColorFrameWidth, ColorFrameHeight), CV_8UC1, mMaskBuffer);
					Mat element = getStructuringElement(MORPH_RECT, Size(3 * 2 + 1, 3 * 2 + 1), Point(-1, 1));
					morphologyEx(maskMat, maskMat, MORPH_OPEN, element);
					//morphologyEx(src, dst, MORPH_CLOSE, element);
					blur(maskMat, maskMat, Size(11, 11));
				}
			}

		}
#if QUERY_FRAME_DESCRIPTION
		__safe_release(depthFrameDescription);
		__safe_release(bodyIndexFrameDescription);
#endif
	}

	__safe_release(depthFrame);
	__safe_release(bodyIndexFrame);
	
	return hr;
}

HRESULT KCDDevice::_thread_update()
{
	if (!mFrameReader)
	{
		return E_FAIL;
	}

	IMultiSourceFrame* multiSourceFrame = NULL;
	HRESULT hr = mFrameReader->AcquireLatestFrame(&multiSourceFrame);

	if (FAILED(hr))
	{
		return E_FAIL;
	}

	bool hasMask = false;

	hr = this->_thread_activeuser_subupdate(multiSourceFrame);

	if (SUCCEEDED(hr))
	{
		if (mHasActiveUser)
		{
			hr = this->_thread_bodymask_subupdate(multiSourceFrame, mActiveBodyIndex);

			if (SUCCEEDED(hr))
			{
				hasMask = true;
			}
		}
	}

	if (hasMask)
	{
		hr = this->_thread_color_subupdate(multiSourceFrame, mMaskBuffer);
	}
	else
	{
		hr = this->_thread_color_subupdate(multiSourceFrame, NULL);
	}
	
	__safe_release(multiSourceFrame);

	return hr;
}

void KCDDevice::profile(INT64 time)
{
	if (!mStartTime)
	{
		mStartTime = time;
	}

	LARGE_INTEGER qpcNow = { 0 };

	mPerformanceQueryMutex.lock();
	double temp = mLatestFpsReading;
	mPerformanceQueryMutex.unlock();

	if (mFreq)
	{
		if (QueryPerformanceCounter(&qpcNow))
		{
			if (mLastCounter)
			{
				mFramesSinceUpdate++;
				temp = mFreq * mFramesSinceUpdate / double(qpcNow.QuadPart - mLastCounter);
			}
		}
	}

	mPerformanceQueryMutex.lock();
	mLatestFpsReading = temp;
	mLatestTimeReading = time - mStartTime;
	mPerformanceQueryMutex.unlock();

	mLastCounter = qpcNow.QuadPart;
	mFramesSinceUpdate = 0;
}

const KinectPerformanceQuery& KCDDevice::getPerformanceQuery()
{
	return mPerformanceQuery;
}

void KCDDevice::handleKinectError(const HRESULT& error)
{
	throw std::exception("Could not open Kinect Device.");
}

/*
* these two methods are not "secure" as they do not check whether mCoordinateMapper is not NULL
* they are also not thread safe
*/

ci::Vec2f KCDDevice::CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const ci::Vec2f& scale)
{
	DepthSpacePoint dp = { 0 };
	mCoordinateMapper->MapCameraPointToDepthSpace(csp, &dp);
	return (ci::Vec2f(dp.X, dp.Y) * scale);
}

ci::Vec2f KCDDevice::CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const int screenwidth, const int screenheight)
{
	DepthSpacePoint dp = { 0 };
	mCoordinateMapper->MapCameraPointToDepthSpace(csp, &dp);
	float x = static_cast<float>(dp.X * screenwidth) / DepthFrameWidth;
	float y = static_cast<float>(dp.Y * screenheight) / DepthFrameHeight;
	return ci::Vec2f(x, y);
}

void KCDDevice::bind(IKCDBindableRef b)
{
	bindIt it = std::find(mBindables.begin(), mBindables.end(), b);

	if (it == mBindables.end())
	{
		b->setup(shared_from_this());
		mBindables.push_back(b);
	}
}

void KCDDevice::unbind(IKCDBindableRef b)
{
	bindIt it = std::find(mBindables.begin(), mBindables.end(), b);

	if (it != mBindables.end())
	{
		(*it)->teardown(shared_from_this());
		mBindables.erase(it);
	}
}

void KCDDevice::unbindAll()
{
	bindIt it;

	for (it = mBindables.begin(); it != mBindables.end(); ++it)
	{
		(*it)->teardown(shared_from_this());
	}

	mBindables.clear();
}

//void KCDDevice::setUserEngagedThreshold(float t)
//{
//	mUserEngagedThreshold = t * t;
//}
//
//void KCDDevice::setUserLostThreshold(float t)
//{
//	mUserLostThreshold = t * t;
//}