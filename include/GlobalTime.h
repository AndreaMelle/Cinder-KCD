#ifndef __GLOBAL_TIME_H__
#define __GLOBAL_TIME_H__

#include "cinder\app\App.h"

class GlobalTime
{
public:
	static GlobalTime& DefaultGlobalTime()
	{
		static GlobalTime _instance;
		return _instance;
	}
	
	virtual ~GlobalTime();

	double getElapsedSeconds();
	double getElapsedFrames();
	double getDeltaSeconds();

	static double ElapsedSeconds();
	static double ElapsedFrames();
	static double DeltaSeconds();


private:
	GlobalTime();
	void update();

	boost::signals2::connection mUpdateConnection;

	double mElapsedSeconds;
	double mElapsedFrames;
	double mDelta;
};

#endif //__GLOBAL_TIME_H__