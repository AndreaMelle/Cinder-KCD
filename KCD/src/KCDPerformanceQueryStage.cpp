#include "KCDPerformanceQueryStage.h"
#include <exception>
#include <algorithm>
#include "KCDDeviceStage.h"

using namespace kcd;

PerformanceQueryStage::PerformanceQueryStage() :
mStartTime(0),
mFreq(0),
mLastCounter(0),
mNextStatusTime(0LL),
mFramesSinceUpdate(0),
mLatestFpsReading(0),
mLatestTimeReading(0)
{
	LARGE_INTEGER qpf = { 0 };
	if (QueryPerformanceFrequency(&qpf))
	{
		mFreq = double(qpf.QuadPart);
	}
}

PerformanceQueryStage::~PerformanceQueryStage() { }

void PerformanceQueryStage::update()
{
	mPerformanceQueryMutex.lock();
	mPerformanceQuery.fps = mLatestFpsReading;
	//mPerformanceQuery.elapsedTime = mLatestTimeReading;
	mPerformanceQueryMutex.unlock();
}

HRESULT PerformanceQueryStage::thread_setup()
{
	HRESULT hr = S_OK;
	
	return hr;
}

HRESULT PerformanceQueryStage::thread_teardown()
{
	return S_OK;
}

HRESULT PerformanceQueryStage::thread_process()
{
	HRESULT hr = S_OK;

	if (mTimeSrc->hasTimeMeasurement())
	{
		INT64 time = mTimeSrc->getLatestTime();

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
		//mLatestTimeReading = time - mStartTime;
		mPerformanceQueryMutex.unlock();

		mLastCounter = qpcNow.QuadPart;
		mFramesSinceUpdate = 0;

		mTimeSrc->invalidateTimeMeasurement();
	}
	
	return hr;
}

HRESULT PerformanceQueryStage::post_thread_process()
{
	return S_OK;
}

void PerformanceQueryStage::setTimeSource(ITimeSourceRef timeSrc)
{
	mTimeSrc = timeSrc;
}

const PerformanceQueryData& PerformanceQueryStage::getPerformanceQuery()
{
	return mPerformanceQuery;
}

