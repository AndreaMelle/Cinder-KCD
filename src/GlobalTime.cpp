#include "GlobalTime.h"

using namespace ci;
using namespace ci::app;

GlobalTime::GlobalTime() : mElapsedSeconds(0), mElapsedFrames(0), mDelta(0)
{
	App* mainApp = App::get();
	
	if (mainApp)
	{
		mElapsedSeconds = getElapsedSeconds();
		mElapsedFrames = getElapsedFrames();
		mUpdateConnection = mainApp->getSignalUpdate().connect(std::bind(&GlobalTime::update, this));
	}
	else
	{
		throw "GlobalTime cannot be invoked before App creation";
	}

}

GlobalTime::~GlobalTime()
{
	mUpdateConnection.disconnect();
}

void GlobalTime::update()
{
	double elapsedSeconds = getElapsedSeconds();
	
	mDelta = elapsedSeconds - mElapsedSeconds;

	mElapsedFrames = getElapsedFrames();
	mElapsedSeconds = elapsedSeconds;
}

double GlobalTime::getElapsedSeconds()
{
	return mElapsedSeconds;
}

double GlobalTime::getElapsedFrames()
{
	return mElapsedFrames;
}

double GlobalTime::getDeltaSeconds()
{
	return mDelta;
}

double GlobalTime::ElapsedSeconds()
{
	return GlobalTime::DefaultGlobalTime().getElapsedSeconds();
}

double GlobalTime::ElapsedFrames()
{
	return GlobalTime::DefaultGlobalTime().getElapsedFrames();
}

double GlobalTime::DeltaSeconds()
{
	return GlobalTime::DefaultGlobalTime().getDeltaSeconds();
}