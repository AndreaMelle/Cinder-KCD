#include "KCDMaskStage.h"
#include <exception>
#include <algorithm>
#include "KCDDeviceStage.h"

using namespace kcd;
using namespace cv;

MaskStage::MaskStage() :
mDeviceSrc(NULL),
mBodyDataSrc(NULL),
depthFrame(NULL),
bodyIndexFrame(NULL),
depthFrameRef(NULL),
bodyIndexFrameRef(NULL),
mDepthCoordinates(NULL),
mMaskBuffer(NULL),
maskTextureName(0),
mHasMaskData(false)
{
	//mLatestMaskData.hasMask = false;
	//mLatestMaskData.maskBuffer = NULL;
}

MaskStage::~MaskStage() { }

void MaskStage::setup()
{
	int depthFrameArea = DeviceStage::DepthFrameWidth * DeviceStage::DepthFrameHeight;
	int colorFrameArea = DeviceStage::ColorFrameWidth * DeviceStage::ColorFrameHeight;

	mDepthCoordinates = new DepthSpacePoint[colorFrameArea];

	mMaskBuffer = new BYTE[colorFrameArea * MASK_SIZE ];
	memset(mMaskBuffer, 0, colorFrameArea * MASK_SIZE);

	glGenTextures(1, &maskTextureName);
	glBindTexture(GL_TEXTURE_2D, maskTextureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, DeviceStage::ColorFrameWidth);

	if (mMaskBuffer)
	{
		mMaskDataMutex.lock();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, mMaskBuffer);
		mMaskDataMutex.unlock();
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void MaskStage::teardown()
{
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

	glDeleteTextures(1, &maskTextureName);
}

void MaskStage::update()
{
	if (mHasMaskData)
	{
		if (glIsTexture(maskTextureName))
		{
			glBindTexture(GL_TEXTURE_2D, maskTextureName);
			mMaskDataMutex.lock();
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, GL_RED, GL_UNSIGNED_BYTE, mMaskBuffer);
			mMaskDataMutex.unlock();
			mHasMaskData = false;
			glBindTexture(GL_TEXTURE_2D, 0);

			mMaskTextureRef = ci::gl::Texture::create(GL_TEXTURE_2D, maskTextureName, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, true);

		}
	}
	
}

//HRESULT MaskStage::thread_setup()
//{
//	HRESULT hr = S_OK;
//	return hr;
//}
//
//HRESULT MaskStage::thread_teardown()
//{
//	return S_OK;
//}

HRESULT MaskStage::thread_process()
{
	HRESULT hr = S_OK;
	//mLatestMaskData.hasMask = false;
	//mLatestMaskData.maskBuffer = NULL;
	
	IMultiSourceFrame* multiSourceFrame = mDeviceSrc->getLatestFrame();
	ICoordinateMapper* coordinateMapper = mDeviceSrc->getCoordinateMapper();
	BodyData bodyData = mBodyDataSrc->getLatestBodyData();

	if (!bodyData.hasActiveUser)
	{
		hr = E_FAIL;
		//mLatestMaskData.hasMask = false;
		//mLatestMaskData.maskBuffer = NULL;
		mHasMaskTextureRef = false;
	}

	if (multiSourceFrame == NULL || coordinateMapper == NULL)
	{
		hr = E_FAIL;
	}

	depthFrame = NULL;
	bodyIndexFrame = NULL;
	depthFrameRef = NULL;
	if (SUCCEEDED(hr))
	{
		hr = multiSourceFrame->get_DepthFrameReference(&depthFrameRef);
	}

	if (SUCCEEDED(hr))
	{
		hr = depthFrameRef->AcquireFrame(&depthFrame);
	}
	
	if (SUCCEEDED(hr))
	{
		bodyIndexFrameRef = NULL;
		hr = multiSourceFrame->get_BodyIndexFrameReference(&bodyIndexFrameRef);
		if (SUCCEEDED(hr))
		{
			hr = bodyIndexFrameRef->AcquireFrame(&bodyIndexFrame);
		}
	}

	if (SUCCEEDED(hr))
	{
		UINT depthBufferSize = 0;
		UINT16* depthBuffer = NULL;
		UINT bodyIndexBufferSize = 0;
		BYTE* bodyIndexBuffer = NULL;

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

			int depthX = 0;
			int depthY = 0;
			BYTE player = 0;
			BYTE* src = NULL;
			BYTE* src1 = NULL;
			BYTE* src2 = NULL;
			BYTE* src3 = NULL;

			if (coordinateMapper && mDepthCoordinates && depthBuffer && mMaskBuffer && bodyIndexBuffer)
			{
				hr = coordinateMapper->MapColorFrameToDepthSpace(DeviceStage::DepthFrameWidth * DeviceStage::DepthFrameHeight, depthBuffer, DeviceStage::ColorFrameWidth * DeviceStage::ColorFrameHeight, mDepthCoordinates);

				if (SUCCEEDED(hr))
				{
					mMaskDataMutex.lock();

					for (register int i = 0; i < (DeviceStage::ColorFrameWidth * DeviceStage::ColorFrameHeight); ++i)
					{
						src = mMaskBuffer + i;

						*src = 0;

						DepthSpacePoint p = mDepthCoordinates[i];

						if (p.X != -std::numeric_limits<float>::infinity() && p.Y != -std::numeric_limits<float>::infinity())
						{
							depthX = static_cast<int>(p.X + 0.5f);
							depthY = static_cast<int>(p.Y + 0.5f);

							if ((depthX >= 0 && depthX < DeviceStage::DepthFrameWidth) && (depthY >= 0 && depthY < DeviceStage::DepthFrameHeight))
							{
								player = bodyIndexBuffer[depthX + (depthY * DeviceStage::DepthFrameWidth)];

								if (player == bodyData.activeBodyIndex)
								{
									*src = 255;
								}
							}
						}
					}

#ifdef NDEBUG
					Mat maskMat;
					maskMat = Mat(Size(DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight), CV_8UC1, mMaskBuffer);
					Mat element = getStructuringElement(MORPH_RECT, Size(3 * 2 + 1, 3 * 2 + 1), Point(-1, 1));
					morphologyEx(maskMat, maskMat, MORPH_OPEN, element);
					//morphologyEx(src, dst, MORPH_CLOSE, element);
					blur(maskMat, maskMat, Size(11, 11));
#endif

					//mLatestMaskData.hasMask = true;
					//mLatestMaskData.maskBuffer = mMaskBuffer;

					mMaskDataMutex.unlock();
					mHasMaskData = true;
					mHasMaskTextureRef = true;
				}
			}

		}
	}

	return hr;
}

//void MaskStage::invalidateLatestMaskBuffer()
//{
//	mLatestMaskData.hasMask = false;
//	mLatestMaskData.maskBuffer = NULL;
//}

HRESULT MaskStage::post_thread_process()
{
	__safe_release(depthFrame);
	__safe_release(bodyIndexFrame);
	__safe_release(depthFrameRef);
	__safe_release(bodyIndexFrameRef);

	return S_OK;
}

void MaskStage::setDeviceSource(IDeviceSourceRef deviceSrc)
{
	mDeviceSrc = deviceSrc;
}

void MaskStage::setBodyDataSource(IBodyDataSourceRef bodyDataSrc)
{
	mBodyDataSrc = bodyDataSrc;
}

//MaskData MaskStage::getLatestMaskBuffer()
//{
//	return mLatestMaskData;
//}

ci::gl::TextureRef MaskStage::getTextureReference()
{
	if (mHasMaskTextureRef)
		return mMaskTextureRef;
	else
		return NULL;
}