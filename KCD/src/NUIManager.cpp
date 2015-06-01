#include "NUIManager.h"

using namespace kcd;

NUIManager::NUIManager()
{

}

NUIManager::~NUIManager()
{

}

void NUIManager::setup()
{
	ci::app::App* mainApp = ci::app::App::get();
	if (!mainApp)
	{
		throw "KCD Pipeline cannot be invoked before App creation";
	}

	mPipeline = PipelineRef(new Pipeline());

	mDevice = DeviceStageRef(new DeviceStage());
	mActiveUser = ActiveUserStageRef(new ActiveUserStage());
	mBody = BodyStageRef(new BodyStage());
	mMask = MaskStageRef(new MaskStage());
	mColor = ColorStageRef(new ColorStage());
	mPerf = PerformanceQueryStageRef(new PerformanceQueryStage());

	mColor->setDeviceSource(mDevice);
	mActiveUser->setDeviceSource(mDevice);
	mActiveUser->setActiveUserDistanceSource(mBody);
	mBody->setDeviceSource(mDevice);
	mBody->setBodyDataSource(mActiveUser);
	mMask->setDeviceSource(mDevice);
	mMask->setBodyDataSource(mActiveUser);
	mPerf->setTimeSource(mColor);

	mPipeline->addStage(mDevice);
	mPipeline->addStage(mActiveUser);
	mPipeline->addStage(mBody);
	mPipeline->addStage(mMask);
	mPipeline->addStage(mColor);
	mPipeline->addStage(mPerf);

	mUpdateConnection = mainApp->getSignalUpdate().connect(std::bind(&NUIManager::update, this));

	mPipeline->start();
}

void NUIManager::teardown()
{
	mUpdateConnection.disconnect();
	mPipeline->stop();
}

void NUIManager::update()
{

}

ITextureOutputRef NUIManager::getColorTextureOutput()
{
	return this->mColor;
}

ITextureOutputRef NUIManager::getMaskTextureOutput()
{
	return this->mMask;
}

IPerformanceOutputRef NUIManager::getPerformaceOutput()
{
	return this->mPerf;
}

kcd::IActiveUserOutputRef NUIManager::getActiveUserOutput()
{
	return this->mActiveUser;
}

kcd::IBodyJointOutputRef NUIManager::getBodyJointOutput()
{
	return this->mBody;
}

ci::gl::TextureRef NUIManager::GetColorTextureRef()
{
	return NUIManager::DefaultManager().getColorTextureOutput()->getTextureReference();
}

ci::gl::TextureRef NUIManager::GetMaskTextureRef()
{
	return NUIManager::DefaultManager().getMaskTextureOutput()->getTextureReference();
}

const PerformanceQueryData& NUIManager::GetPerformaceQueryData()
{
	return NUIManager::DefaultManager().getPerformaceOutput()->getPerformanceQuery();
}

void NUIManager::AttachActiveUserObserver(Observer<ActiveUserEvent>& observer)
{
	NUIManager::DefaultManager().getActiveUserOutput()->attach(observer);
}

void NUIManager::AttachBodyJointObserver(Observer<BodyJointEvent>& observer)
{
	NUIManager::DefaultManager().getBodyJointOutput()->attach(observer);
}

