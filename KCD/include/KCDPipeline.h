#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include <vector>
#include "Process.h"
#include "cinder\app\App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include <Kinect.h>
#include "Subject.h"
#include <map>

#define THREAD_SLEEP_DURATION 30L
#define QUERY_FRAME_DESCRIPTION 0

/*
* Pipeline: create a thread for sensor data to be processed
* Holds reference to pipeline Stages and calls their update methods
* Order matters
* Destructors are called automatically when pipeline is stopped
* Pipeline binds itself to update loop of Cinder main app (not sure this is a good choice)
* Stage callback order:
* setup() -> thread_setup() -> pre_thread_process() -> thread_process() -> post_thread_process() -> thread_teardown() -> teardown()
* update() is called in parallel
*/

namespace kcd
{
	// TODO: move IBody to weak_ptr
	struct BodyData
	{
		bool hasActiveUser;
		IBody* body;
		UINT activeBodyIndex;
		UINT64 activeUserTrackingId;
		float latestUserDistance;
	};

	struct MaskData
	{
		BYTE* maskBuffer;
		bool hasMask;
	};

	struct PerformanceQueryData
	{
		double fps;
		INT64 elapsedTime;
	};

	typedef enum ActiveUserEvent
	{
		ACTIVE_USER_NEW,
		ACTIVE_USER_LOST
	};

	typedef enum BodyJointEventType
	{
		BODY_JOINT_APPEAR,
		BODY_JOINT_MOVE,
		BODY_JOINT_DISAPPEAR
	};

	struct BodyJointEvent
	{
	public:
		BodyJointEvent() {}
		BodyJointEvent(BodyJointEventType _eventType,
			JointType _jointType,
			ci::Vec2f _screenSpacePosition) :
			eventType(_eventType),
			jointId(_jointType),
			screenSpacePosition(_screenSpacePosition){}
		BodyJointEventType eventType;
		JointType jointId;
		ci::Vec2f screenSpacePosition;
	};

	typedef std::pair<JointType, BodyJointEvent> BodyJointEventPair;

	class IStage
	{
	public:
		IStage() {}
		virtual ~IStage() {}
		
		virtual HRESULT thread_setup() { return S_OK; }
		virtual HRESULT pre_thread_process() { return S_OK;  }
		virtual HRESULT thread_process() = 0;
		virtual HRESULT post_thread_process() { return S_OK; }
		virtual HRESULT thread_teardown() { return S_OK; }

		virtual void setup() {}
		virtual void update() {};
		virtual void teardown() {}
	};

	typedef std::shared_ptr<IStage> IStageRef;
	typedef std::vector<IStageRef>::iterator IStageRefIter;

	class Pipeline
	{
	public:
		Pipeline();
		virtual ~Pipeline();

		void start();
		void update();
		void stop();

		void addStage(IStageRef stage);
		void removeStage(IStageRef stage);
		void removeAllStages();

	private:
		Process mProcess;
		std::vector<IStageRef> mStages;
		boost::signals2::connection mUpdateConnection;

		HRESULT thread_setup();
		HRESULT thread_update();
		HRESULT thread_teardown();
	};

	/*
	* Definitions for Pipeline Sources
	*/

	class IDeviceSource
	{
	public:
		virtual IMultiSourceFrame* getLatestFrame() = 0;
		virtual ICoordinateMapper* getCoordinateMapper() = 0;
	};
	
	class IBodyDataSource
	{
	public:
		virtual BodyData getLatestBodyData() = 0;
	};

	class ITimeSource
	{
	public:
		virtual INT64 getLatestTime() = 0;
		virtual bool hasTimeMeasurement() = 0;
		virtual void invalidateTimeMeasurement() = 0;

	};

	class IActiveUserDistanceSource
	{
	public:
		virtual float getLatestDistance() = 0;
	};

	class IMaskBufferSource
	{
	public:
		virtual MaskData getLatestMaskBuffer() = 0;
		virtual void invalidateLatestMaskBuffer() = 0;
	};

	/*
	* Definitions for Pipeline Outputs
	*/

	class ITextureOutput
	{
	public:
		virtual ci::gl::TextureRef getTextureReference() = 0;
	};

	class IPerformanceOutput
	{
	public:
		virtual const PerformanceQueryData& getPerformanceQuery() = 0;
	};

	typedef std::shared_ptr<Pipeline> PipelineRef;
	typedef std::shared_ptr<IBodyDataSource> IBodyDataSourceRef;
	typedef std::shared_ptr<IDeviceSource> IDeviceSourceRef;
	typedef std::shared_ptr<ITimeSource> ITimeSourceRef;
	typedef std::shared_ptr<IActiveUserDistanceSource> IActiveUserDistanceSourceRef;
	typedef std::shared_ptr<IMaskBufferSource> IMaskBufferSourceRef;
	typedef std::shared_ptr<ITextureOutput> ITextureOutputRef;
	typedef std::shared_ptr<IPerformanceOutput> IPerformanceOutputRef;

	typedef Subject<ActiveUserEvent> IActiveUserOutput;
	typedef std::shared_ptr<IActiveUserOutput> IActiveUserOutputRef;

	typedef Subject<BodyJointEvent> IBodyJointOutput;
	typedef std::shared_ptr<IBodyJointOutput> IBodyJointOutputRef;
};


#endif //__PIPELINE_H__