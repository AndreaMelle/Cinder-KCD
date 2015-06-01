#ifndef __KCD_MASK_STAGE_H__
#define __KCD_MASK_STAGE_H__

#include <Kinect.h>
#include <mutex>
#include "KCDUtils.h"
#include "KCDPipeline.h"
#include "opencv2\opencv.hpp"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

namespace kcd
{
	class MaskStage : public IStage, public ITextureOutput //, public IMaskBufferSource
	{
	public:
		MaskStage();
		virtual ~MaskStage();

		void setDeviceSource(IDeviceSourceRef deviceSrc);
		void setBodyDataSource(IBodyDataSourceRef bodyDataSrc);

		virtual ci::gl::TextureRef getTextureReference();
		//virtual MaskData getLatestMaskBuffer();
		//virtual void invalidateLatestMaskBuffer();

		virtual void setup();
		//virtual HRESULT thread_setup();
		virtual HRESULT thread_process();
		virtual HRESULT post_thread_process();
		//virtual HRESULT thread_teardown();
		virtual void teardown();

		virtual void update();

	private:
		IDeviceSourceRef mDeviceSrc;
		IBodyDataSourceRef mBodyDataSrc;

		IDepthFrame* depthFrame;
		IBodyIndexFrame* bodyIndexFrame;
		IDepthFrameReference* depthFrameRef;
		IBodyIndexFrameReference* bodyIndexFrameRef;

		DepthSpacePoint* mDepthCoordinates;
		BYTE* mMaskBuffer;
		std::mutex mMaskDataMutex;

		GLuint maskTextureName;
		ci::gl::TextureRef mMaskTextureRef;
		std::atomic<bool> mHasMaskData;
		std::atomic<bool> mHasMaskTextureRef; //Depends on active user!

		//MaskData mLatestMaskData;
		
	};

	typedef std::shared_ptr<MaskStage> MaskStageRef;
};

#endif //__KCD_MASK_STAGE_H__