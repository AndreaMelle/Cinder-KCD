#ifndef __KCD_UTILS_H__
#define __KCD_UTILS_H__

#include <Kinect.h>
#include "cinder\Vector.h"

#define BGRA_SIZE (4 * sizeof(BYTE))
#define RGBA_SIZE (4 * sizeof(BYTE))
#define MASK_SIZE (sizeof(BYTE))

template<class Interface>
inline void __safe_release(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

namespace kcd
{
	typedef enum NUIActiveUserEvent
	{
		ACTIVE_USER_NEW,
		ACTIVE_USER_LOST
	};

	static const int DepthFrameWidth = 512;
	static const int DepthFrameHeight = 424;
	static const int ColorFrameWidth = 1920;
	static const int ColorFrameHeight = 1080;

	struct KinectPerformanceQuery
	{
		double fps;
		INT64 elapsedTime;
	};

	struct NUIObservation
	{
		// let's use the joint enum as unique id for now
		// limited to a single user, of course, as this whole code is
		JointType id;
		float x; // this is window screen space
		float y; // this is window screen space
	};

};

#endif //__KCD_UTILS_H__