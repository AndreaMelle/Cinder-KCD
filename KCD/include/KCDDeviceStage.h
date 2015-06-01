#ifndef __KCD_DEVICE_STAGE_H__
#define __KCD_DEVICE_STAGE_H__

#include <Kinect.h>
#include <mutex>
#include "KCDUtils.h"
#include "KCDPipeline.h"

#define BGRA_SIZE (4 * sizeof(BYTE))
#define RGBA_SIZE (4 * sizeof(BYTE))
#define MASK_SIZE (sizeof(BYTE))

namespace kcd
{
	class DeviceStage : public IDeviceSource, public IStage
	{
	public:
		static const int DepthFrameWidth = 512;
		static const int DepthFrameHeight = 424;
		static const int ColorFrameWidth = 1920;
		static const int ColorFrameHeight = 1080;

	public:
		DeviceStage();
		virtual ~DeviceStage();

		virtual HRESULT thread_setup();
		virtual HRESULT thread_process();
		virtual HRESULT post_thread_process();
		virtual HRESULT thread_teardown();

		// TODO: use weak_ptr
		virtual IMultiSourceFrame* getLatestFrame();
		virtual ICoordinateMapper* getCoordinateMapper();
		
	private:
		IKinectSensor* mKinectSensor;
		ICoordinateMapper* mCoordinateMapper;
		IMultiSourceFrameReader *mFrameReader;

		IMultiSourceFrame* multiSourceFrame;
		
		ci::Vec2f CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const ci::Vec2f& scale);
		ci::Vec2f CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const int screenwidth, const int screenheight);
		
		void handleKinectError(const HRESULT& error);
	};

	typedef std::shared_ptr<DeviceStage> DeviceStageRef;
};

#endif //__KCD_DEVICE_STAGE_H__