#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <thread>
#include <memory>
#include <atomic>
#include <functional>

class Process
{
public:
	Process();
	virtual ~Process();

	void start();
	void stop();

	std::function<void()> mThreadCallback;
	std::atomic_bool mNewData;
	std::atomic_bool mRunning;
	std::shared_ptr<std::thread> mThread;


};

#endif //__PROCESS_H__