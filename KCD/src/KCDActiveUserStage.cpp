#include "KCDActiveUserStage.h"
#include <exception>
#include <algorithm>

using namespace kcd;

ActiveUserStage::ActiveUserStage() :
mDeviceSrc(NULL),
mDistanceSrc(NULL),
mBodyFrame(NULL),
bodyFrameRef(NULL),
mHasNewActiveUserData(false),
mUserEngagedThreshold(6.25f),
mUserLostThreshold(6.25f)
{
	for (int i = 0; i < _countof(bodies); ++i)
	{
		bodies[i] = NULL;
	}

	mLatestBodyData.hasActiveUser = false;
	mLatestBodyData.activeBodyIndex = 0;
	mLatestBodyData.activeUserTrackingId = 0;
	mLatestBodyData.latestUserDistance = std::numeric_limits<float>::max();
	mLatestBodyData.body = NULL;
}

ActiveUserStage::~ActiveUserStage() { }

void ActiveUserStage::update()
{
	/*
	* make a copy: yes, waste memory and time,
	* but I don't know what the hell is the app going to do in notify
	*/

	if (mHasNewActiveUserData)
	{
		mActiveUserEventBufferMutex.lock();
		std::vector<ActiveUserEvent> eventsCopy = std::vector<ActiveUserEvent>(mActiveUserEventBuffer);
		mActiveUserEventBuffer.clear();
		mActiveUserEventBufferMutex.unlock();

		mHasNewActiveUserData = false;

		std::vector<ActiveUserEvent>::iterator it;
		for (it = eventsCopy.begin(); it != eventsCopy.end(); ++it)
		{
			this->notify((*it));
		}
	}
}

//HRESULT ActiveUserStage::thread_setup()
//{
//	HRESULT hr = S_OK;
//	return hr;
//}

HRESULT ActiveUserStage::thread_teardown()
{
	mActiveUserEventBufferMutex.lock();
	mActiveUserEventBuffer.clear();
	mActiveUserEventBufferMutex.unlock();
	return S_OK;
}

HRESULT ActiveUserStage::thread_process()
{
	HRESULT hr = S_OK;
	mLatestBodyData.body = NULL;
	
	IMultiSourceFrame* multiSourceFrame = mDeviceSrc->getLatestFrame();
	ICoordinateMapper* coordinateMapper = mDeviceSrc->getCoordinateMapper();

	if (multiSourceFrame == NULL || coordinateMapper == NULL)
	{
		hr = E_FAIL;
	}

	mBodyFrame = NULL;
	bodyFrameRef = NULL;

	if (SUCCEEDED(hr))
	{
		hr = multiSourceFrame->get_BodyFrameReference(&bodyFrameRef);
	}

	if (SUCCEEDED(hr))
	{
		hr = bodyFrameRef->AcquireFrame(&mBodyFrame);
	}

	if (SUCCEEDED(hr))
	{
		for (int i = 0; i < _countof(bodies); ++i)
		{
			bodies[i] = NULL;
		}

		if (SUCCEEDED(hr))
		{
			hr = mBodyFrame->GetAndRefreshBodyData(_countof(bodies), bodies);
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
							if (mLatestBodyData.hasActiveUser)
							{
								if (trackingId == mLatestBodyData.activeUserTrackingId)
								{
									mLatestBodyData.activeBodyIndex = i;
									mLatestBodyData.latestUserDistance = mDistanceSrc->getLatestDistance();
									mLatestBodyData.body = body;

									if (mLatestBodyData.latestUserDistance <= mUserLostThreshold)
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
												mLatestBodyData.activeUserTrackingId = trackingId;
												mLatestBodyData.activeBodyIndex = i;
												mLatestBodyData.hasActiveUser = true;
												mLatestBodyData.latestUserDistance = distance;
												mLatestBodyData.body = body;

												//mObservationMutex.lock();
												//mObservations.clear(); // new user, clear old observations if any
												//mObservationMutex.unlock();

												mActiveUserEventBufferMutex.lock();
												mActiveUserEventBuffer.push_back(ActiveUserEvent::ACTIVE_USER_NEW);
												mActiveUserEventBufferMutex.unlock();
												mHasNewActiveUserData = true;
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
			if (mLatestBodyData.hasActiveUser && !seen)
			{
				mLatestBodyData.hasActiveUser = false;
				mLatestBodyData.activeBodyIndex = 0;
				mLatestBodyData.activeUserTrackingId = 0;
				mLatestBodyData.body = NULL;

				//mObservationMutex.lock();
				//mObservations.clear(); // assume that if a user is lost, observations are not valid anymore
				//mObservationMutex.unlock();

				mActiveUserEventBufferMutex.lock();
				mActiveUserEventBuffer.push_back(ActiveUserEvent::ACTIVE_USER_LOST);
				mActiveUserEventBufferMutex.unlock();
				mHasNewActiveUserData = true;
			}
		}
	}

	return hr;
}

HRESULT ActiveUserStage::post_thread_process()
{
	__safe_release(mBodyFrame);
	__safe_release(bodyFrameRef);

	for (int i = 0; i < _countof(bodies); ++i)
	{
		__safe_release(bodies[i]);
	}

	return S_OK;
}

void ActiveUserStage::setDeviceSource(IDeviceSourceRef deviceSrc)
{
	mDeviceSrc = deviceSrc;
}

void ActiveUserStage::setActiveUserDistanceSource(IActiveUserDistanceSourceRef distanceSrc)
{
	mDistanceSrc = distanceSrc;
}

BodyData ActiveUserStage::getLatestBodyData()
{
	return mLatestBodyData;
}