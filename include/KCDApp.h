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
#include "KCDPipeline.h"
#include "KCDDeviceStage.h"
#include "Subject.h"

#define DEBUG_DRAW 1
#define NANO100_TO_ONE_SECOND 10000000.0
#define PROFILE_KINECT_FPS 1

class KCDApp : public ci::app::AppNative, public Observer<kcd::ActiveUserEvent>, public Observer<kcd::BodyJointEvent>
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

	virtual void onEvent(kcd::ActiveUserEvent what, const Subject<kcd::ActiveUserEvent>& sender);
	virtual void onEvent(kcd::BodyJointEvent what, const Subject<kcd::BodyJointEvent>& sender);

private:

	float						mFrameRate;
	double mLatestFpsInfo;
	double  mLatestTimeInfo;

	bool						mFullScreen;
	bool mHasUser;
	
	std::map<JointType, bool> mDrawBodyJoints;
	std::map<JointType, ci::Vec2f> mBodyJointsPos;

	ci::gl::TextureRef mColorTextureRef;
	ci::gl::TextureRef mMaskTextureRef;
	ci::gl::GlslProgRef mMaskRGBshader;
	ci::gl::TextureRef sprite;

#if DEBUG_DRAW
	ci::params::InterfaceGlRef	mParams;
#endif

	ci::ColorAf mBackgroundColor;

	float mGlobalScale;
};

#endif //__KCD_SAMPLE_APP_H__