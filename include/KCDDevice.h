#ifndef __KCD_DEVICE_H__
#define __KCD_DEVICE_H__

#include <Kinect.h>
#include "cinder\gl\gl.h"
#include "cinder\Surface.h"
#include "Process.h"
#include <mutex>
#include "opencv2\opencv.hpp"
#include "KCDUtils.h"
#include "cinder\app\App.h"
#include "Subject.h"

#define PROFILE_KINECT_FPS 1
#define MULTI_THREADED 1
#define QUERY_FRAME_DESCRIPTION 0
#define DEFAULT_HANDLEFT_ID 0
#define DEFAULT_HANDRIGHT_ID 1

/*
 * Simple Kinect sensor access, in separate thread
 * Reason for multi thread is, Kinect is 30fps, UI loop is 60 fps
 * Note that CUDA might not be possible if we don't compile for x64, as they dropped support
 */

//static const long long kThreadSleepDuration = 30L;

namespace kcd
{
	class IKCDBindable;
	class KCDRenderer;
	class KCDActiveUserEventManager;

	typedef std::shared_ptr<IKCDBindable> IKCDBindableRef;
	typedef std::shared_ptr<KCDRenderer> KCDRendererRef;
	typedef std::shared_ptr<KCDActiveUserEventManager> KCDActiveUserEventManagerRef;

	class KCDDevice : public std::enable_shared_from_this<KCDDevice>
	{
		friend class IKCDBindable;
		friend class KCDRenderer;
		friend class KCDActiveUserEventManager;

	public:
		KCDDevice();
		virtual ~KCDDevice();

		void setup();
		void update();
		void teardown();

		const KinectPerformanceQuery& getPerformanceQuery();

		void bind(IKCDBindableRef b);
		void unbind(IKCDBindableRef b);
		void unbindAll();

		// TODO: cannot just do setters yet, because of threading and locks
		//void setUserEngagedThreshold(float t);
		//void setUserLostThreshold(float t);
		
	private:

		Process mProcess;
		
		// locks
		std::mutex mPerformanceQueryMutex;
		std::mutex mImageDataMutex;
		std::mutex mObservationMutex;
		std::mutex mActiveUserEventBufferMutex;

		// Kinect Device
		IKinectSensor* mKinectSensor;
		ICoordinateMapper* mCoordinateMapper;
		DepthSpacePoint* mDepthCoordinates;
		IMultiSourceFrameReader *mFrameReader;
		
		// Time management
		INT64 mStartTime;
		double mFreq;
		INT64 mLastCounter;
		INT64 mNextStatusTime;
		DWORD mFramesSinceUpdate;
		double mLatestFpsReading;
		INT64 mLatestTimeReading;
		KinectPerformanceQuery mPerformanceQuery;

		//color buffer
		BYTE* mColorBuffer;
		BYTE* mMaskBuffer;

		// active user
		UINT mActiveBodyIndex;
		UINT64 mActiveUserTrackingId;
		bool mHasActiveUser;
		float mUserEngagedThreshold; // meters, from camera
		float mUserLostThreshold; // meters, from camera
		float mLatestUserDistance;
		std::vector<NUIActiveUserEvent> mActiveUserEventBuffer;

		ci::Vec2f mScreenSpaceScale;

		std::vector<NUIObservation> mObservations;		

		// multi-threading
		HRESULT _thread_setup();
		HRESULT _thread_update();
		HRESULT _thread_teardown();

		// sub-updates
		HRESULT _thread_color_subupdate(IMultiSourceFrame*& multiSourceFrame, BYTE* maskBuffer = NULL);
		HRESULT _thread_activeuser_subupdate(IMultiSourceFrame*& multiSourceFrame);
		HRESULT _thread_bodytracking_subupdate(IBody*& body);
		HRESULT _thread_bodymask_subupdate(IMultiSourceFrame*& multiSourceFrame, const UINT& userIdx);

		//helpers
		ci::Vec2f CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const ci::Vec2f& scale);
		ci::Vec2f CameraSpaceToScreenSpace(const CameraSpacePoint& csp, const int screenwidth, const int screenheight);
		
		void handleKinectError(const HRESULT& error);
		void profile(INT64 time);

		// bindables
		boost::signals2::connection mUpdateConnection;
		std::vector<IKCDBindableRef> mBindables;
		typedef std::vector<IKCDBindableRef>::iterator bindIt;
	};

	typedef std::shared_ptr<KCDDevice> KCDDeviceRef;

	class IKCDBindable
	{
	public:
		IKCDBindable() {}
		virtual ~IKCDBindable() {}

		virtual void setup(std::weak_ptr<KCDDevice> device) = 0;
		virtual void update(std::weak_ptr<KCDDevice> device) = 0;
		virtual void teardown(std::weak_ptr<KCDDevice> device) = 0;
	};

	class KCDRenderer : public IKCDBindable
	{
	public:
		KCDRenderer();
		virtual ~KCDRenderer();
		GLuint getColorTextureRef();

		virtual void setup(std::weak_ptr<KCDDevice> device);
		virtual void update(std::weak_ptr<KCDDevice> device);
		virtual void teardown(std::weak_ptr<KCDDevice> device);

	protected:
		GLuint colorTextureName;
	};

	class KCDActiveUserEventManager : public IKCDBindable, public Subject<NUIActiveUserEvent>
	{
	public:
		KCDActiveUserEventManager();
		virtual ~KCDActiveUserEventManager();

		virtual void setup(std::weak_ptr<KCDDevice> device);
		virtual void update(std::weak_ptr<KCDDevice> device);
		virtual void teardown(std::weak_ptr<KCDDevice> device);
	};

};

#endif //__KCD_DEVICE_H__

// BUG: glMapBuffer is async
// therefore when unlocking mColorBuffer, we haven't copied the data yet
// we should copy the data on kinect thread to avoid writing and reading at the same time

//void * ptr = glMapBuffer(GL_TEXTURE_2D, GL_WRITE_ONLY);
//if (ptr != nullptr)
//{
//	memcpy(ptr, colorBuffer, colorWidth * colorHeight * BGRA_SIZE);
//}
//
//glUnmapBuffer(GL_TEXTURE_2D);