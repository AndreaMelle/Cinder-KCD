#include "KCDApp.h"
#include "cinder\Json.h"
#include "cinder\Display.h"
#include "NUIManager.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace kcd;

void KCDApp::setup()
{
	mFrameRate = 0.0f;
	mHasUser = false;

	mDrawBodyJoints[JointType_HandLeft] = false;
	mDrawBodyJoints[JointType_HandRight] = false;
	mDrawBodyJoints[JointType_Head] = false;
	mDrawBodyJoints[JointType_SpineBase] = false;

	sprite = gl::Texture::create(loadImage(loadAsset("cow.png")));
	
	try
	{
		mMaskRGBshader = gl::GlslProg::create(loadAsset("passThru_vert.glsl"), loadAsset("maskrgb_frag.glsl"));
	}
	catch (gl::GlslProgCompileExc &exc)
	{
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
		throw exc;
	}
	catch (...)
	{
		std::cout << "Unable to load shader" << std::endl;
		throw std::exception("Unable to load shader");
	}

#if DEBUG_DRAW
	mParams = params::InterfaceGl::create("Params", Vec2i(200, 100));
	mParams->addParam("Frame rate", &mFrameRate, "", true);

#if PROFILE_KINECT_FPS
	mParams->addParam("Kinect fps", &mLatestFpsInfo, "", true);
	//mParams->addParam("Kinect time", &mLatestTimeInfo, "", true);
#endif

#endif

	NUIManager::DefaultManager().setup();
	NUIManager::AttachActiveUserObserver(*this);
	NUIManager::AttachBodyJointObserver(*this);

}

void KCDApp::shutdown()
{
	NUIManager::DefaultManager().teardown();
}

void KCDApp::mouseDown(MouseEvent evt)
{
}

void KCDApp::mouseMove(ci::app::MouseEvent evt)
{
}

void KCDApp::keyDown(KeyEvent evt)
{
	if (evt.getCode() == KeyEvent::KEY_q)
	{
		quit();
	}
}

void KCDApp::onEvent(kcd::ActiveUserEvent what, const Subject<ActiveUserEvent>& sender)
{
	if (what == kcd::ACTIVE_USER_NEW)
	{
		mHasUser = true;
	}
	else if (what == kcd::ACTIVE_USER_LOST)
	{
		mHasUser = false;
	}
}

void KCDApp::onEvent(kcd::BodyJointEvent what, const Subject<kcd::BodyJointEvent>& sender)
{
	if (what.jointId == JointType_HandLeft
		|| what.jointId == JointType_HandRight
		|| what.jointId == JointType_Head
		|| what.jointId == JointType_SpineBase)
	{
		if (what.eventType == BODY_JOINT_APPEAR || what.eventType == BODY_JOINT_MOVE)
		{
			mDrawBodyJoints[what.jointId] = true;
			mBodyJointsPos[what.jointId] = what.screenSpacePosition;
		}
		else if (what.eventType == BODY_JOINT_DISAPPEAR)
		{
			mDrawBodyJoints[what.jointId] = false;
		}
	}
}

void KCDApp::update()
{
	mFrameRate = getAverageFps();

	//" FPS = %0.2f    Time = %I64d", fps, (nTime - m_nStartTime));
#if PROFILE_KINECT_FPS
	kcd::PerformanceQueryData perf = NUIManager::GetPerformaceQueryData();
	mLatestFpsInfo = perf.fps;
	//mLatestTimeInfo = static_cast<double>(perf.elapsedTime) / NANO100_TO_ONE_SECOND;
#endif

	
}

void KCDApp::draw()
{
	gl::setViewport(getWindowBounds());
	gl::clear(ColorAf(1.0f, 0.0f, 1.0f, 1.0f), true);
	gl::setMatricesWindow(getWindowSize());
	gl::enableAlphaBlending();

	gl::color(ColorA::white());

	mColorTextureRef = NUIManager::GetColorTextureRef();
	mMaskTextureRef = NUIManager::GetMaskTextureRef();

	if (mColorTextureRef)
	{
		//gl::draw(mColorTextureRef, getWindowBounds());

		//gl::draw(sprite, getWindowCenter() - sprite->getSize() / 2.0f);

		if (!mMaskTextureRef)
		{
			gl::draw(mColorTextureRef, getWindowBounds());
		}
		else
		{
			mMaskRGBshader->bind();
			mColorTextureRef->bind(0);
			mMaskTextureRef->bind(1);

			mMaskRGBshader->uniform("tex0", 0);
			mMaskRGBshader->uniform("tex1", 1);
			gl::drawSolidRect(getWindowBounds());

			mColorTextureRef->unbind();
			mMaskTextureRef->unbind();
			mMaskRGBshader->unbind();
		}
	}

	if (mDrawBodyJoints[JointType_HandLeft])
	{
		gl::color(ColorA(0.0f, 1.0f, 0, 1.0f));
		gl::drawSolidCircle(mGlobalScale * mBodyJointsPos[JointType_HandLeft], 10.0f);
	}

	if (mDrawBodyJoints[JointType_HandRight])
	{
		gl::color(ColorA(0.0f, 0.0f, 1.0f, 1.0f));
		gl::drawSolidCircle(mGlobalScale * mBodyJointsPos[JointType_HandRight], 10.0f);
	}

	if (mDrawBodyJoints[JointType_SpineBase])
	{
		gl::color(ColorA(1.0f, 0.0f, 1.0f, 1.0f));
		gl::drawSolidCircle(mGlobalScale * mBodyJointsPos[JointType_SpineBase], 10.0f);
	}

	if (mDrawBodyJoints[JointType_Head])
	{
		gl::color(ColorA(1.0f, 0.0f, 0.0f, 1.0f));
		gl::drawSolidCircle(mGlobalScale * mBodyJointsPos[JointType_Head], 10.0f);
	}

	/*if (mHasUser)
	{
		gl::color(ColorA(1.0f, 0, 0, 1.0f));
		gl::drawSolidCircle(getWindowCenter(), 20.0f);
	}*/

#if DEBUG_DRAW
	mParams->draw();
#endif

}

void KCDApp::prepareSettings(Settings* settings)
{
	mGlobalScale = 0.5f;

	int32_t width = static_cast<int32_t>(1920.0f * mGlobalScale);
	int32_t height = (int32_t)((float)width * 9.0f / 16.0f);

	settings->prepareWindow(Window::Format().size(width, height)
		.title("KCD App")
		.borderless(true)
		.pos(-1920,0));
	settings->setFrameRate(60.0f);

}

CINDER_APP_NATIVE(KCDApp, RendererGl(2))
