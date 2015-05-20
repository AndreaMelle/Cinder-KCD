#include "Process.h"

using namespace std;



Process::Process() : mNewData(atomic<bool>(false)), mRunning(atomic<bool>(false)), mThreadCallback(nullptr)
{

}

Process::~Process()
{
	this->stop();
}

void Process::start()
{
	this->stop();

	if (mThreadCallback != nullptr)
	{
		mNewData = false;
		mRunning = true;
		mThread = shared_ptr<thread>(new thread(mThreadCallback));
	}
}

void Process::stop()
{
	mRunning = false;
	if (mThread)
	{
		mThread->join();
		mThread.reset();
	}
}