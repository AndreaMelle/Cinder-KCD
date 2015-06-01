#ifndef __KCD_PERFORMANCE_QUERY_STAGE_H__
#define __KCD_PERFORMANCE_QUERY_STAGE_H__

#include <Kinect.h>
#include <mutex>
#include "KCDUtils.h"
#include "KCDPipeline.h"

namespace kcd
{
	

	class PerformanceQueryStage : public IStage, public IPerformanceOutput
	{
	public:
		PerformanceQueryStage();
		virtual ~PerformanceQueryStage();

		void setTimeSource(ITimeSourceRef timeSrc);

		virtual const PerformanceQueryData& getPerformanceQuery();

		virtual HRESULT thread_process();
		virtual HRESULT post_thread_process();
		virtual void update();

		virtual HRESULT thread_setup();
		virtual HRESULT thread_teardown();
		
	private:
		ITimeSourceRef mTimeSrc;

		INT64 mStartTime;
		double mFreq;
		INT64 mLastCounter;
		INT64 mNextStatusTime;
		DWORD mFramesSinceUpdate;
		double mLatestFpsReading;
		INT64 mLatestTimeReading;

		std::mutex mPerformanceQueryMutex;

		PerformanceQueryData mPerformanceQuery;
	};

	typedef std::shared_ptr<PerformanceQueryStage> PerformanceQueryStageRef;
};

#endif //__KCD_PERFORMANCE_QUERY_STAGE_H__