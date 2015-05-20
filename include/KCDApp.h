#ifndef __KCD_SAMPLE_APP_H__
#define __KCD_SAMPLE_APP_H__

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/qtime/QuickTime.h"
#include <vector>
#include "KCDDevice.h"

#define DEBUG_DRAW 1
#define NANO100_TO_ONE_SECOND 10000000.0


class KCDApp : public ci::app::AppNative
{
public:
	void setup();
	void mouseDown(ci::app::MouseEvent evt);
	void mouseMove(ci::app::MouseEvent evt);
	void keyDown(ci::app::KeyEvent evt);
	void update();
	void draw();
	void prepareSettings(ci::app::AppBasic::Settings* settings);
	void shutdown();

private:

	kcd::KCDDeviceRef mKinectDevice;
	kcd::KCDRendererRef mKinectRenderer;
	kcd::KCDActiveUserEventManagerRef mKinectUserEvents;

	float						mFrameRate;
	double mLatestFpsInfo;
	double  mLatestTimeInfo;

	bool						mFullScreen;

	ci::gl::TextureRef mColorTextureRef;

#if DEBUG_DRAW
	ci::params::InterfaceGlRef	mParams;
#endif

	ci::ColorAf mBackgroundColor;

	float mGlobalScale;
};

#endif //__KCD_SAMPLE_APP_H__