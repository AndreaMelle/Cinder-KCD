#include "KCDPipeline.h"

using namespace kcd;

Pipeline::Pipeline()
{

}

Pipeline::~Pipeline()
{
	this->stop();
}

void Pipeline::start()
{
	ci::app::App* mainApp = ci::app::App::get();
	if (!mainApp)
	{
		throw "KCD Pipeline cannot be invoked before App creation";
	}

	mProcess.mThreadCallback = [&]()
	{
		HRESULT hr;
		hr = thread_setup();

		if (FAILED(hr))
		{
			thread_teardown();
			return;
		}

		while (mProcess.mRunning)
		{
			hr = thread_update();

			if (FAILED(hr))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_DURATION));
				continue;
			}
		}

		thread_teardown();
	};

	IStageRefIter it;
	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->setup();
	}

	mUpdateConnection = mainApp->getSignalUpdate().connect(std::bind(&Pipeline::update, this));

	mProcess.start();

}

void Pipeline::stop()
{
	mUpdateConnection.disconnect();
	mProcess.stop();

	IStageRefIter it;
	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->teardown();
	}

	mStages.clear();
}

void Pipeline::update()
{
	IStageRefIter it;
	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->update();
	}
}

HRESULT Pipeline::thread_setup()
{
	IStageRefIter it;
	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->thread_setup();
	}

	return S_OK;
}

HRESULT Pipeline::thread_update()
{
	IStageRefIter it;

	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->pre_thread_process();
	}

	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->thread_process();
	}

	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->post_thread_process();
	}

	return S_OK;
}

HRESULT Pipeline::thread_teardown()
{
	IStageRefIter it;
	for (it = mStages.begin(); it != mStages.end(); ++it)
	{
		(*it)->thread_teardown();
	}

	return S_OK;
}

void Pipeline::addStage(IStageRef stage)
{
	if (mProcess.mRunning)
	{
		throw "Pipeline alteration while running not supported yet";
	}

	IStageRefIter it = std::find(mStages.begin(), mStages.end(), stage);
	if (it == mStages.end())
	{
		mStages.push_back(stage);
	}
}

void Pipeline::removeStage(IStageRef stage)
{
	if (mProcess.mRunning)
	{
		throw "Pipeline alteration while running not supported yet";
	}

	IStageRefIter it = std::find(mStages.begin(), mStages.end(), stage);
	if (it != mStages.end())
	{
		mStages.erase(it);
	}
}

void Pipeline::removeAllStages()
{
	if (mProcess.mRunning)
	{
		throw "Pipeline alteration while running not supported yet";
	}

	mStages.clear();
}