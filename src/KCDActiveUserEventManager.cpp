#include "KCDDevice.h"

using namespace kcd;

KCDActiveUserEventManager::KCDActiveUserEventManager()
{

}

KCDActiveUserEventManager::~KCDActiveUserEventManager()
{

}

void KCDActiveUserEventManager::setup(std::weak_ptr<KCDDevice> device)
{

}

void KCDActiveUserEventManager::update(std::weak_ptr<KCDDevice> device)
{
	auto devicePtr = device.lock();
	if (!devicePtr)
		return;

	/*
	* make a copy: yes, waste memory and time,
	* but I don't know what the hell is the app going to do in notify
	*/

	devicePtr->mActiveUserEventBufferMutex.lock();
	std::vector<NUIActiveUserEvent> eventsCopy = std::vector<NUIActiveUserEvent>(devicePtr->mActiveUserEventBuffer);
	devicePtr->mActiveUserEventBuffer.clear();
	devicePtr->mActiveUserEventBufferMutex.unlock();

	std::vector<NUIActiveUserEvent>::iterator it;
	for (it = eventsCopy.begin(); it != eventsCopy.end(); ++it)
	{
		this->notify((*it));
	}
}

void KCDActiveUserEventManager::teardown(std::weak_ptr<KCDDevice> device)
{

}