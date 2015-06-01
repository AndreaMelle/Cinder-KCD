#include "KCDBodyStage.h"
#include <exception>
#include <algorithm>

using namespace kcd;

BodyStage::BodyStage() :
mDeviceSrc(NULL),
mBodyDataSrc(NULL),
mLatestUserDistance(0),
trackIfInferred(true)
{
	mObservations[JointType_HandLeft] = Observation(JointType_HandLeft);
	mObservations[JointType_HandRight] = Observation(JointType_HandRight);
	mObservations[JointType_Head] = Observation(JointType_Head);
	mObservations[JointType_SpineBase] = Observation(JointType_SpineBase);
}

BodyStage::~BodyStage() { }

void BodyStage::update()
{
	
	if (mHasNewBodyJointEvents)
	{
		std::vector<BodyJointEvent> temp;
		mBodyJointEventBufferMutex.lock();
		temp = std::vector<BodyJointEvent>(mBodyJointEventBuffer);
		mBodyJointEventBuffer.clear();
		mBodyJointEventBufferMutex.unlock();
		mHasNewBodyJointEvents = false;

		std::vector<BodyJointEvent>::iterator it;
		for (it = temp.begin(); it != temp.end(); ++it)
		{
			if (it->eventType == BODY_JOINT_DISAPPEAR)
			{
				this->notify(*it);
				if (mBodyJointEventPolling.find(it->jointId) != mBodyJointEventPolling.end())
				{
					mBodyJointEventPolling.erase(it->jointId);
				}
			}
			else if (it->eventType == BODY_JOINT_APPEAR)
			{
				if (mBodyJointEventPolling.find(it->jointId) == mBodyJointEventPolling.end())
				{
					mBodyJointEventPolling.insert(BodyJointEventPair(it->jointId, *it));
				}
				else
				{
					mBodyJointEventPolling[it->jointId].eventType = BODY_JOINT_APPEAR;
					mBodyJointEventPolling[it->jointId].screenSpacePosition = it->screenSpacePosition;
				}
			}
			else if (it->eventType == BODY_JOINT_MOVE)
			{
				if (mBodyJointEventPolling.find(it->jointId) == mBodyJointEventPolling.end())
				{
					BodyJointEvent evt = *it;
					evt.eventType = BODY_JOINT_APPEAR;
					mBodyJointEventPolling.insert(BodyJointEventPair(it->jointId, evt));
				}
				else
				{
					mBodyJointEventPolling[it->jointId].eventType = BODY_JOINT_MOVE;
					mBodyJointEventPolling[it->jointId].screenSpacePosition = it->screenSpacePosition;
				}
			}
		}

	}
	
	std::map<JointType, BodyJointEvent>::iterator itmap;
	for (itmap = mBodyJointEventPolling.begin(); itmap != mBodyJointEventPolling.end(); ++itmap)
	{
		this->notify(itmap->second);
	}

}

HRESULT BodyStage::thread_setup()
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT BodyStage::thread_teardown()
{
	return S_OK;
}

HRESULT BodyStage::thread_process()
{
	HRESULT hr = S_OK;
	
	IMultiSourceFrame* multiSourceFrame = mDeviceSrc->getLatestFrame();
	ICoordinateMapper* coordinateMapper = mDeviceSrc->getCoordinateMapper();
	BodyData bodyData = mBodyDataSrc->getLatestBodyData();

	if (!bodyData.hasActiveUser)
	{
		hr = E_FAIL;
		this->markObservationsUnseen();
	}

	if (bodyData.body == NULL || multiSourceFrame == NULL || coordinateMapper == NULL)
	{
		hr = E_FAIL;
	}

	Joint joints[JointType_Count];

	if (SUCCEEDED(hr))
	{
		hr = bodyData.body->GetJoints(_countof(joints), joints);
	}

	if (SUCCEEDED(hr))
	{
		TrackingState ts = TrackingState_NotTracked;
		JointType jt = JointType_Count;

		this->markObservationsUnseen();

		for (int j = 0; j < _countof(joints); ++j)
		{
			ts = joints[j].TrackingState;

			if (ts == TrackingState_Tracked || (ts == TrackingState_Inferred && trackIfInferred))
			{
				jt = joints[j].JointType;

				if (jt == JointType_HandLeft ||
					jt == JointType_HandRight ||
					jt == JointType_Head ||
					jt == JointType_SpineBase)
				{
					Observation* obs = &mObservations[jt];
					CameraSpacePoint csp = joints[j].Position;

					coordinateMapper->MapCameraPointToColorSpace(csp, &(obs->xy));

					mBodyJointEventBufferMutex.lock();
					if (!obs->seen)
					{
						mBodyJointEventBuffer.push_back(eventFromObservation(BODY_JOINT_APPEAR, *obs));
					}
					else
					{
						mBodyJointEventBuffer.push_back(eventFromObservation(BODY_JOINT_MOVE, *obs));
					}
					mBodyJointEventBufferMutex.unlock();

					obs->isValid = true;
					obs->seen = true;

					mHasNewBodyJointEvents = true;

					if (jt == JointType_SpineBase)
					{
						mLatestUserDistance = ci::Vec3f(csp.X, csp.Y, csp.Z).distanceSquared(ci::Vec3f::zero());
					}
				}
			}
		}
	}

	this->processObservationsUnseen();

	return hr;
}

void BodyStage::markObservationsUnseen()
{
	ObservationIt it;
	for (it = mObservations.begin(); it != mObservations.end(); ++it)
	{
		(*it).second.seen = false;
	}
}

void BodyStage::processObservationsUnseen()
{
	ObservationIt it;
	for (it = mObservations.begin(); it != mObservations.end(); ++it)
	{
		if (!(*it).second.seen && (*it).second.isValid)
		{
			mBodyJointEventBufferMutex.lock();
			mBodyJointEventBuffer.push_back(eventFromObservation(BODY_JOINT_DISAPPEAR, (*it).second));
			mBodyJointEventBufferMutex.unlock();
			mHasNewBodyJointEvents = true;
			(*it).second.isValid = false;
		}
	}
}

BodyJointEvent BodyStage::eventFromObservation(BodyJointEventType eventType, const Observation& obs)
{
	return BodyJointEvent(eventType, obs.id, ci::Vec2f(obs.xy.X, obs.xy.Y));
}

//HRESULT BodyStage::post_thread_process()
//{
//	return S_OK;
//}

void BodyStage::setDeviceSource(IDeviceSourceRef deviceSrc)
{
	mDeviceSrc = deviceSrc;
}

void BodyStage::setBodyDataSource(IBodyDataSourceRef bodyDataSrc)
{
	mBodyDataSrc = bodyDataSrc;
}

float BodyStage::getLatestDistance()
{
	return mLatestUserDistance;
}