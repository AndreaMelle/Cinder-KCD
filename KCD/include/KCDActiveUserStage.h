#ifndef __KCD_ACTIVE_USER_STAGE_H__
#define __KCD_ACTIVE_USER_STAGE_H__

#include <Kinect.h>
#include <mutex>
#include "KCDUtils.h"
#include "KCDPipeline.h"
#include "Subject.h"

namespace kcd
{
	class ActiveUserStage : public Subject<ActiveUserEvent>, public IBodyDataSource, public IStage
	{
	public:
		ActiveUserStage();
		virtual ~ActiveUserStage();

		void setDeviceSource(IDeviceSourceRef deviceSrc);
		void setActiveUserDistanceSource(IActiveUserDistanceSourceRef distanceSrc);

		virtual BodyData getLatestBodyData();

		virtual HRESULT thread_process();
		virtual HRESULT post_thread_process();
		virtual void update();

		//virtual HRESULT thread_setup();
		virtual HRESULT thread_teardown();
		
	private:
		IDeviceSourceRef mDeviceSrc;
		IActiveUserDistanceSourceRef mDistanceSrc;

		IBodyFrame* mBodyFrame;
		IBodyFrameReference* bodyFrameRef;
		IBody* bodies[BODY_COUNT];

		BodyData mLatestBodyData;

		float mUserEngagedThreshold; // meters, from camera
		float mUserLostThreshold; // meters, from camera

		std::vector<ActiveUserEvent> mActiveUserEventBuffer;
		std::mutex mActiveUserEventBufferMutex;
		std::atomic<bool> mHasNewActiveUserData;
	};

	typedef std::shared_ptr<ActiveUserStage> ActiveUserStageRef;
};

#endif //__KCD_ACTIVE_USER_STAGE_H__