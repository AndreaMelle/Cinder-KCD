#ifndef __NUI_MANAGER_HEADER_H__
#define __NUI_MANAGER_HEADER_H__

#include "KCDPipeline.h"
#include "KCDDeviceStage.h"
#include "KCDColorStage.h"
#include "KCDActiveUserStage.h"
#include "KCDBodyStage.h"
#include "KCDMaskStage.h"
#include "KCDPerformanceQueryStage.h"

class NUIManager
{
public:
	static const int ColorFrameWidth = 1920;
	static const int ColorFrameHeight = 1080;

	static NUIManager& DefaultManager()
	{
		static NUIManager _instance;
		return _instance;
	}

	virtual ~NUIManager();

	void setup();
	void teardown();
	//void debugDraw();

	kcd::ITextureOutputRef getColorTextureOutput();
	kcd::ITextureOutputRef getMaskTextureOutput();
	kcd::IPerformanceOutputRef getPerformaceOutput();
	kcd::IActiveUserOutputRef getActiveUserOutput();
	kcd::IBodyJointOutputRef getBodyJointOutput();

public:
	/* A number of static convenience methods */
	static ci::gl::TextureRef GetColorTextureRef();
	static ci::gl::TextureRef GetMaskTextureRef();
	static const kcd::PerformanceQueryData& GetPerformaceQueryData();
	static void AttachActiveUserObserver(Observer<kcd::ActiveUserEvent>& observer);
	static void AttachBodyJointObserver(Observer<kcd::BodyJointEvent>& observer);

private:
	NUIManager();
	NUIManager(NUIManager const&);
	void operator=(NUIManager const&);

	void update();

	boost::signals2::connection mUpdateConnection;

	kcd::PipelineRef mPipeline;
	kcd::DeviceStageRef mDevice;
	kcd::ActiveUserStageRef mActiveUser;
	kcd::BodyStageRef mBody;
	kcd::MaskStageRef mMask;
	kcd::ColorStageRef mColor;
	kcd::PerformanceQueryStageRef mPerf;

};

#endif //__NUI_MANAGER_HEADER_H__