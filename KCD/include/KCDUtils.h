#ifndef __KCD_UTILS_H__
#define __KCD_UTILS_H__

#include <Kinect.h>

template<class Interface>
inline void __safe_release(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

#endif //__KCD_UTILS_H__