#ifndef __KCD_COLOR_STAGE_H__
#define __KCD_COLOR_STAGE_H__

#include <Kinect.h>
#include <mutex>
#include "KCDUtils.h"
#include "KCDPipeline.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

/*
* TODO: get rid of this awful coupling between color and mask
* Possible solutions:
* 
* a. The mask stage holds a new gl texture for the mask (waistful)
* 
* b. The color stage issues an event to the mask stage when it has data,
* the mask stage is then in charge of copying the mask data to the color data
*/

namespace kcd
{
	class ColorStage : public ITimeSource, public IStage, public ITextureOutput
	{
	public:
		ColorStage();
		virtual ~ColorStage();

		void setDeviceSource(IDeviceSourceRef deviceSrc);
		void setMaskSource(IMaskBufferSourceRef maskSrc);

		virtual INT64 getLatestTime();
		virtual bool hasTimeMeasurement();
		virtual void invalidateTimeMeasurement();
		
		virtual ci::gl::TextureRef getTextureReference();

		virtual void setup();
		//virtual HRESULT thread_setup();
		virtual HRESULT thread_process();
		virtual HRESULT post_thread_process();
		//virtual HRESULT thread_teardown();
		virtual void teardown();
		
		virtual void update();
		
	private:
		IDeviceSourceRef mDeviceSrc;
		IMaskBufferSourceRef mMaskSrc;
		IColorFrame* mColorFrame;
		IColorFrameReference* mColorFrameRef;

		BYTE* mColorBuffer;
		std::mutex mColorDataMutex;
		INT64 mColorTime;
		bool mHasColorTime;
		std::atomic<bool> mHasNewColorData;

		GLuint colorTextureName;
		ci::gl::TextureRef mColorTextureRef;
	};

	typedef std::shared_ptr<ColorStage> ColorStageRef;
};

#endif //__KCD_COLOR_STAGE_H__