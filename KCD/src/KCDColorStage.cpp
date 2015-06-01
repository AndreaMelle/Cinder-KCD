#include "KCDColorStage.h"
#include <exception>
#include <algorithm>
#include "KCDDeviceStage.h"

using namespace kcd;

ColorStage::ColorStage() :
mDeviceSrc(NULL),
mMaskSrc(NULL),
mColorFrame(NULL),
mColorFrameRef(NULL),
mColorBuffer(NULL),
mColorTime(0),
colorTextureName(0),
mHasNewColorData(false)
{

}

ColorStage::~ColorStage() { }

void ColorStage::setup()
{
	int colorFrameArea = DeviceStage::ColorFrameWidth * DeviceStage::ColorFrameHeight;
	mColorBuffer = new BYTE[colorFrameArea * BGRA_SIZE];
	memset(mColorBuffer, 255, colorFrameArea * BGRA_SIZE);

	glGenTextures(1, &colorTextureName);
	glBindTexture(GL_TEXTURE_2D, colorTextureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, DeviceStage::ColorFrameWidth);

	if (mColorBuffer)
	{
		mColorDataMutex.lock();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, mColorBuffer);
		mColorDataMutex.unlock();
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	}
	
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ColorStage::teardown()
{
	glDeleteTextures(1, &colorTextureName);

	if (mColorBuffer)
	{
		delete[] mColorBuffer;
		mColorBuffer = NULL;
	}
}

void ColorStage::update()
{
	if (mHasNewColorData)
	{
		if (glIsTexture(colorTextureName))
		{
			glBindTexture(GL_TEXTURE_2D, colorTextureName);
			mColorDataMutex.lock();
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, GL_BGRA, GL_UNSIGNED_BYTE, mColorBuffer);
			mColorDataMutex.unlock();
			mHasNewColorData = false;
			glBindTexture(GL_TEXTURE_2D, 0);

			mColorTextureRef = ci::gl::Texture::create(GL_TEXTURE_2D, colorTextureName, DeviceStage::ColorFrameWidth, DeviceStage::ColorFrameHeight, true);

		}
	}
}

//HRESULT ColorStage::thread_setup()
//{
//	HRESULT hr = S_OK;
//	return hr;
//}

//HRESULT ColorStage::thread_teardown()
//{
//	return S_OK;
//}

HRESULT ColorStage::thread_process()
{
	
	HRESULT hr = S_OK;
	
	IMultiSourceFrame* multiSourceFrame = mDeviceSrc->getLatestFrame();

	if (multiSourceFrame == NULL)
	{
		hr = E_FAIL;
	}

	mColorFrame = NULL;
	mColorFrameRef = NULL;
	
	if (SUCCEEDED(hr))
	{
		hr = multiSourceFrame->get_ColorFrameReference(&mColorFrameRef);
	}

	if (SUCCEEDED(hr))
	{
		hr = mColorFrameRef->AcquireFrame(&mColorFrame);
	}

	if (SUCCEEDED(hr))
	{
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT colorBufferSize = 0;
		BYTE* colorBuffer = NULL;

		INT64 colorTime = 0;
		HRESULT hrTime = mColorFrame->get_RelativeTime(&colorTime);
		if (SUCCEEDED(hrTime))
		{
			mColorTime = colorTime;
			mHasColorTime = true;
		}

		if (SUCCEEDED(hr))
		{
			hr = mColorFrame->get_RawColorImageFormat(&imageFormat);
		}

		mColorDataMutex.lock();

		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = mColorFrame->AccessRawUnderlyingBuffer(&colorBufferSize, &colorBuffer);
				mHasNewColorData = true;
			}
			else if (mColorBuffer)
			{
				colorBufferSize = DeviceStage::ColorFrameWidth * DeviceStage::ColorFrameHeight * BGRA_SIZE;
				colorBuffer = mColorBuffer;

				hr = mColorFrame->CopyConvertedFrameDataToArray(colorBufferSize, colorBuffer, ColorImageFormat_Bgra);

				//MaskData maskData = mMaskSrc->getLatestMaskBuffer();

				//if (maskData.hasMask && maskData.maskBuffer)
				//{
				//	BYTE* src = NULL;
				//	BYTE value = 0;

				//	for (register int i = 0; i < (DeviceStage::ColorFrameWidth * DeviceStage::ColorFrameHeight); ++i)
				//	{
				//		src = mColorBuffer + (BGRA_SIZE * i);
				//		value = *(maskData.maskBuffer + i);
				//		if (value >= 128)
				//			*(src + 3) = value;
				//		else
				//			*(src + 3) = 0;
				//	}

				//	//mMaskSrc->invalidateLatestMaskBuffer();
				//}

				mHasNewColorData = true;
			}
			else
			{
				hr = E_FAIL;
			}
		}

		mColorDataMutex.unlock();
	}

	return hr;
}

HRESULT ColorStage::post_thread_process()
{
	__safe_release(mColorFrame);
	__safe_release(mColorFrameRef);
	return S_OK;
}

void ColorStage::setDeviceSource(IDeviceSourceRef deviceSrc)
{
	mDeviceSrc = deviceSrc;
}

INT64 ColorStage::getLatestTime()
{
	return mColorTime;
}

ci::gl::TextureRef ColorStage::getTextureReference()
{
	return mColorTextureRef;
}

void ColorStage::setMaskSource(IMaskBufferSourceRef maskSrc)
{
	mMaskSrc = maskSrc;
}

bool ColorStage::hasTimeMeasurement()
{
	return mHasColorTime;
}

void ColorStage::invalidateTimeMeasurement()
{
	mHasColorTime = false;
}