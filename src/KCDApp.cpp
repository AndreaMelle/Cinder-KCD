#include "KCDApp.h"
#include "cinder\Json.h"
#include "cinder\Display.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void KCDApp::setup()
{
	mFrameRate = 0.0f;

#if DEBUG_DRAW
	mParams = params::InterfaceGl::create("Params", Vec2i(200, 100));
	mParams->addParam("Frame rate", &mFrameRate, "", true);

#if PROFILE_KINECT_FPS
	mParams->addParam("Kinect fps", &mLatestFpsInfo, "", true);
	mParams->addParam("Kinect time", &mLatestTimeInfo, "", true);
#endif

#endif

	mKinectDevice = std::make_shared<kcd::KCDDevice>();
	mKinectRenderer = std::make_shared<kcd::KCDRenderer>();
	mKinectUserEvents = std::make_shared<kcd::KCDActiveUserEventManager>();

	mKinectDevice->setup();
	mKinectDevice->bind(mKinectRenderer);
	mKinectDevice->bind(mKinectUserEvents);
}

void KCDApp::shutdown()
{
	mKinectDevice->unbind(mKinectUserEvents);
	mKinectDevice->unbind(mKinectRenderer);
	mKinectDevice->teardown();
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

void KCDApp::update()
{
	mFrameRate = getAverageFps();

	//" FPS = %0.2f    Time = %I64d", fps, (nTime - m_nStartTime));
#if PROFILE_KINECT_FPS
	mLatestFpsInfo = mKinectDevice->getPerformanceQuery().fps;
	mLatestTimeInfo = static_cast<double>(mKinectDevice->getPerformanceQuery().elapsedTime) / NANO100_TO_ONE_SECOND;
#endif

	
	
}

void KCDApp::draw()
{
	gl::setViewport(getWindowBounds());
	gl::clear(ColorAf(1.0f, 0.0f, 1.0f, 1.0f), true);
	gl::setMatricesWindow(getWindowSize());
	gl::enableAlphaBlending();


	mColorTextureRef = gl::Texture::create(GL_TEXTURE_2D, mKinectRenderer->getColorTextureRef(), kcd::ColorFrameWidth, kcd::ColorFrameHeight, true);

	if (mColorTextureRef)
	{
		gl::draw(mColorTextureRef, getWindowBounds());
	}

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
		.pos(0,0));
	settings->setFrameRate(60.0f);

}

CINDER_APP_NATIVE(KCDApp, RendererGl(2))
