#ifndef __KCD_BODY_STAGE_H__
#define __KCD_BODY_STAGE_H__

#include <Kinect.h>
#include <mutex>
#include "KCDUtils.h"
#include "KCDPipeline.h"
#include "Subject.h"
#include <map>

#define DEFAULT_HANDLEFT_ID 0
#define DEFAULT_HANDRIGHT_ID 1

namespace kcd
{
	struct Observation
	{
	public:
		Observation() : id(JointType_Count) {}
		Observation(JointType _id) : id(_id), isValid(false), seen(false){ xy = { 0 }; }
		// let's use the joint enum as unique id for now
		// limited to a single user, of course, as this whole code is
		JointType id;
		ColorSpacePoint xy;
		bool isValid;
		bool seen;
	};

	typedef std::pair<JointType, Observation> ObservationPair;
	typedef std::map<JointType, Observation>::iterator ObservationIt;

	class BodyStage : public IActiveUserDistanceSource, public IStage, public Subject<BodyJointEvent>
	{
	public:
		BodyStage();
		virtual ~BodyStage();

		void setDeviceSource(IDeviceSourceRef deviceSrc);
		void setBodyDataSource(IBodyDataSourceRef bodyDataSrc);

		virtual float getLatestDistance();

		virtual HRESULT thread_process();
		//virtual HRESULT post_thread_process();
		virtual void update();

		virtual HRESULT thread_setup();
		virtual HRESULT thread_teardown();
		
	private:
		IDeviceSourceRef mDeviceSrc;
		IBodyDataSourceRef mBodyDataSrc;
		
		std::map<JointType, Observation> mObservations;

		std::vector<BodyJointEvent> mBodyJointEventBuffer;
		std::mutex mBodyJointEventBufferMutex;
		std::atomic<bool> mHasNewBodyJointEvents;

		std::map<JointType, BodyJointEvent> mBodyJointEventPolling;

		float mLatestUserDistance;

		bool trackIfInferred;

		void markObservationsUnseen();
		void processObservationsUnseen();
		BodyJointEvent eventFromObservation(BodyJointEventType eventType, const Observation& obs);

	};

	typedef std::shared_ptr<BodyStage> BodyStageRef;
};

#endif //__KCD_BODY_STAGE_H__