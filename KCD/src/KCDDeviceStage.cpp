#include "KCDDeviceStage.h"
#include <exception>
#include <algorithm>

using namespace kcd;

DeviceStage::DeviceStage() :
	mKinectSensor(NULL),
	mCoordinateMapper(NULL),
	mFrameReader(NULL),
	multiSourceFrame(NULL)
{

}

DeviceStage::~DeviceStage()
{
	
}

HRESULT DeviceStage::thread_setup()
{
	HRESULT hr = S_OK;
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

HRESULT DeviceStage::thread_teardown()
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

HRESULT DeviceStage::thread_process()
{
	HRESULT hr = S_OK;

	if (!mFrameReader)
	{
		return E_FAIL;
	}

	multiSourceFrame = NULL;
	hr = mFrameReader->AcquireLatestFrame(&multiSourceFrame);
	return hr;
}

HRESULT DeviceStage::post_thread_process()
{
	__safe_release(multiSourceFrame);
	return S_OK;
}

void DeviceStage::handleKinectError(const HRESULT& error)
{
	throw std::exception("Could not open Kinect Device.");
}

IMultiSourceFrame* DeviceStage::getLatestFrame()
{
	return multiSourceFrame;
}

ICoordinateMapper* DeviceStage::getCoordinateMapper()
{
	return mCoordinateMapper;
}

/*
* these two methods are not "secure" as they do not check whether mCoordinateMapper is not NULL
* they are also not thread safe
*/

ci::Vec2f DeviceStage::CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const ci::Vec2f& scale)
{
	DepthSpacePoint dp = { 0 };
	mCoordinateMapper->MapCameraPointToDepthSpace(csp, &dp);
	return (ci::Vec2f(dp.X, dp.Y) * scale);
}

ci::Vec2f DeviceStage::CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const int screenwidth, const int screenheight)
{
	DepthSpacePoint dp = { 0 };
	mCoordinateMapper->MapCameraPointToDepthSpace(csp, &dp);
	float x = static_cast<float>(dp.X * screenwidth) / DepthFrameWidth;
	float y = static_cast<float>(dp.Y * screenheight) / DepthFrameHeight;
	return ci::Vec2f(x, y);
}