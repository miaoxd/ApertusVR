#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filewritestream.h"
#include "ApeFobHeadTrackingPlugin.h"

ApeFobHeadTrackingPlugin::ApeFobHeadTrackingPlugin()
{
	mpSystemConfig = Ape::ISystemConfig::getSingletonPtr();
	mpEventManager = Ape::IEventManager::getSingletonPtr();
	mpEventManager->connectEvent(Ape::Event::Group::NODE, std::bind(&ApeFobHeadTrackingPlugin::eventCallBack, this, std::placeholders::_1));
	mpEventManager->connectEvent(Ape::Event::Group::CAMERA, std::bind(&ApeFobHeadTrackingPlugin::eventCallBack, this, std::placeholders::_1));
	mpScene = Ape::IScene::getSingletonPtr();
	mpFobTracker = nullptr;
	mCameraDoubleQueue = Ape::DoubleQueue<Ape::CameraWeakPtr>();
	mCameras = std::vector<Ape::CameraWeakPtr>();
	mpMainWindow = Ape::IMainWindow::getSingletonPtr();
	mTrackerConfig = Ape::FobHeadTrackingTrackerConfig();
	mDisplayConfigList = Ape::FobHeadTrackingDisplayConfigList();
	mUserNode = Ape::NodeWeakPtr();
	mCamerasNode = Ape::NodeWeakPtr();
	mTrackedViewerPosition = Ape::Vector3();
	mTrackedViewerOrientation = Ape::Quaternion();
	mTrackedViewerOrientationYPR = Ape::Euler();
}

ApeFobHeadTrackingPlugin::~ApeFobHeadTrackingPlugin()
{
	std::cout << "ApeFobHeadTrackingPlugin dtor" << std::endl;
}

void ApeFobHeadTrackingPlugin::eventCallBack(const Ape::Event& event)
{
    if (event.type == Ape::Event::Type::CAMERA_CREATE)
	{
		if (auto camera = std::static_pointer_cast<Ape::ICamera>(mpScene->getEntity(event.subjectName).lock()))
			mCameraDoubleQueue.push(camera);
	}
}

void ApeFobHeadTrackingPlugin::Init()
{
	std::cout << "ApeFobHeadTrackingPlugin::init" << std::endl;
	mCamerasNode = mpScene->createNode("FobHeadTrackingNode");
	std::cout << "ApeFobHeadTrackingPlugin waiting for main window" << std::endl;
	while (mpMainWindow->getHandle() == nullptr)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	std::cout << "ApeFobHeadTrackingPlugin main window was found" << std::endl;
	mpFobTracker = trackdInitTrackerReader(4126);
	
	std::stringstream fileFullPath;
	fileFullPath << mpSystemConfig->getFolderPath() << "\\ApeFobHeadTrackingPlugin.json";
	FILE* apeFobHeadTrackingPluginConfigFile = std::fopen(fileFullPath.str().c_str(), "r");
	char readBuffer[65536];
	if (apeFobHeadTrackingPluginConfigFile)
	{
		rapidjson::FileReadStream jsonFileReaderStream(apeFobHeadTrackingPluginConfigFile, readBuffer, sizeof(readBuffer));
		rapidjson::Document jsonDocument;
		jsonDocument.ParseStream(jsonFileReaderStream);
		if (jsonDocument.IsObject())
		{
			rapidjson::Value& tracker = jsonDocument["tracker"];
			for (rapidjson::Value::MemberIterator trackerMemberIterator = 
					tracker.MemberBegin(); trackerMemberIterator != tracker.MemberEnd(); ++trackerMemberIterator)
			{
				if (trackerMemberIterator->name == "rotation")
				{
					Ape::Degree angle;
					Ape::Vector3 axis;
					for (rapidjson::Value::MemberIterator rotationMemberIterator =
						tracker[trackerMemberIterator->name].MemberBegin();
						rotationMemberIterator != tracker[trackerMemberIterator->name].MemberEnd(); ++rotationMemberIterator)
					{
						if (rotationMemberIterator->name == "angle")
							angle = rotationMemberIterator->value.GetFloat();
						else if (rotationMemberIterator->name == "x")
							axis.x = rotationMemberIterator->value.GetFloat();
						else if (rotationMemberIterator->name == "y")
							axis.y = rotationMemberIterator->value.GetFloat();
						else if (rotationMemberIterator->name == "z")
							axis.z = rotationMemberIterator->value.GetFloat();
					}
					mTrackerConfig.rotation.FromAngleAxis(angle, axis);
				}
				else if (trackerMemberIterator->name == "translate")
				{
					for (rapidjson::Value::MemberIterator translateMemberIterator =
						tracker[trackerMemberIterator->name].MemberBegin();
						translateMemberIterator != tracker[trackerMemberIterator->name].MemberEnd(); ++translateMemberIterator)
					{
						if (translateMemberIterator->name == "x")
							mTrackerConfig.translate.x = translateMemberIterator->value.GetFloat();
						else if (translateMemberIterator->name == "y")
							mTrackerConfig.translate.y = translateMemberIterator->value.GetFloat();
						else if (translateMemberIterator->name == "z")
							mTrackerConfig.translate.z = translateMemberIterator->value.GetFloat();
					}
				}
				else if (trackerMemberIterator->name == "scale")
				{
					for (rapidjson::Value::MemberIterator scaleMemberIterator =
						tracker[trackerMemberIterator->name].MemberBegin();
						scaleMemberIterator != tracker[trackerMemberIterator->name].MemberEnd(); ++scaleMemberIterator)
					{
						if (scaleMemberIterator->name == "x")
							mTrackerConfig.scale.x = scaleMemberIterator->value.GetFloat();
						else if (scaleMemberIterator->name == "y")
							mTrackerConfig.scale.y = scaleMemberIterator->value.GetFloat();
						else if (scaleMemberIterator->name == "z")
							mTrackerConfig.scale.z = scaleMemberIterator->value.GetFloat();
					}
				}
				else if (trackerMemberIterator->name == "eyeSeparation")
				{
					mTrackerConfig.eyeSeparationPerEye = trackerMemberIterator->value.GetFloat() / 2;
				}
			}
			rapidjson::Value& displays = jsonDocument["displays"];
			for (auto& display : displays.GetArray())
			{
				Ape::FobHeadTrackingDisplayConfig fobHeadTrackingDisplayConfig;
				for (rapidjson::Value::MemberIterator displayMemberIterator = 
					display.MemberBegin(); displayMemberIterator != display.MemberEnd(); ++displayMemberIterator)
				{
					if (displayMemberIterator->name == "size")
					{
						for (rapidjson::Value::MemberIterator sizeMemberIterator = 
							display[displayMemberIterator->name].MemberBegin();
							sizeMemberIterator != display[displayMemberIterator->name].MemberEnd(); ++sizeMemberIterator)
						{
							if (sizeMemberIterator->name == "width")
								fobHeadTrackingDisplayConfig.size.x = sizeMemberIterator->value.GetFloat();
							else if (sizeMemberIterator->name == "height")
								fobHeadTrackingDisplayConfig.size.y = sizeMemberIterator->value.GetFloat();
						}
					}
					else if (displayMemberIterator->name == "position")
					{
						for (rapidjson::Value::MemberIterator positionMemberIterator =
							display[displayMemberIterator->name].MemberBegin();
							positionMemberIterator != display[displayMemberIterator->name].MemberEnd(); ++positionMemberIterator)
						{
							if (positionMemberIterator->name == "x")
								fobHeadTrackingDisplayConfig.position.x = positionMemberIterator->value.GetFloat();
							else if (positionMemberIterator->name == "y")
								fobHeadTrackingDisplayConfig.position.y = positionMemberIterator->value.GetFloat();
							else if (positionMemberIterator->name == "z")
								fobHeadTrackingDisplayConfig.position.z = positionMemberIterator->value.GetFloat();
						}
					}
					else if (displayMemberIterator->name == "orientation")
					{
					    Ape::Degree angle;
						Ape::Vector3 axis;
						for (rapidjson::Value::MemberIterator orientationMemberIterator =
							display[displayMemberIterator->name].MemberBegin();
							orientationMemberIterator != display[displayMemberIterator->name].MemberEnd(); ++orientationMemberIterator)
						{
							if (orientationMemberIterator->name == "angle")
								angle = orientationMemberIterator->value.GetFloat();
							else if (orientationMemberIterator->name == "x")
								axis.x = orientationMemberIterator->value.GetFloat();
							else if (orientationMemberIterator->name == "y")
								axis.y = orientationMemberIterator->value.GetFloat();
							else if (orientationMemberIterator->name == "z")
								axis.z = orientationMemberIterator->value.GetFloat();
						}
						fobHeadTrackingDisplayConfig.orientation.FromAngleAxis(angle, axis);
					}
				}
				mDisplayConfigList.push_back(fobHeadTrackingDisplayConfig);
			}
		}
		fclose(apeFobHeadTrackingPluginConfigFile);
	}	
}

void ApeFobHeadTrackingPlugin::Run()
{
    int excpectedCameraCount = mDisplayConfigList.size() * 2;
	int cameraCount = 0;
    while (cameraCount < excpectedCameraCount)
	{
		mCameraDoubleQueue.swap();
		while (!mCameraDoubleQueue.emptyPop())
		{
			if (auto camera = mCameraDoubleQueue.front().lock())
			{
				mCameras.push_back(camera);
				if (!mUserNode.lock())
				{
					mUserNode = camera->getParentNode();
					mCamerasNode.lock()->setParentNode(mUserNode);
				}
				camera->setParentNode(mCamerasNode);
				cameraCount++;
			}
			mCameraDoubleQueue.pop();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	for (int i = 0; i < mDisplayConfigList.size(); i++)
	{
		auto displayConfig = mDisplayConfigList[i];
		if (auto cameraLeft = mCameras[i * 2].lock())
		{
			if (auto cameraRight = mCameras[i * 2 + 1].lock())
			{
				cameraLeft->setOrientation(displayConfig.orientation);
				cameraRight->setOrientation(displayConfig.orientation);
			}
		}
	}
	while (mpFobTracker)
	{
		float positionDataFromTracker[3];
		float orientationDataFromTracker[3];
		if (trackdGetPosition(mpFobTracker, 0, positionDataFromTracker) && trackdGetEulerAngles(mpFobTracker, 0, orientationDataFromTracker))
		{
			mTrackedViewerPosition = (Ape::Vector3(positionDataFromTracker[0], positionDataFromTracker[1], positionDataFromTracker[2]) * mTrackerConfig.scale) + mTrackerConfig.translate;
			mTrackedViewerOrientationYPR = Ape::Euler(Ape::Degree(orientationDataFromTracker[1]).toRadian(),
				Ape::Degree(orientationDataFromTracker[2]).toRadian() - 1.57f,
				Ape::Degree(orientationDataFromTracker[0]).toRadian());
			mTrackedViewerOrientation = mTrackedViewerOrientationYPR.toQuaternion() * mTrackerConfig.rotation;
		}
		if (auto camerasNode = mCamerasNode.lock())
		{
			/*Ape::Quaternion trackedViewerOrientation = Ape::Euler(Ape::Degree(orientationDataFromTracker[1]).toRadian(),
				Ape::Degree(0).toRadian(),
				Ape::Degree(orientationDataFromTracker[0]).toRadian());*/
			//camerasNode->setPosition(mTrackedViewerPosition);
			//camerasNode->setOrientation(mTrackedViewerOrientation);
		}
		//system("cls");
		for (int i = 0; i < mDisplayConfigList.size(); i++)
		{
			auto displayConfig = mDisplayConfigList[i];
			if (auto cameraLeft = mCameras[i * 2].lock())
			{
				if (auto cameraRight = mCameras[i * 2 + 1].lock())
				{
					cameraLeft->setPosition(mTrackedViewerOrientation * Ape::Vector3(-mTrackerConfig.eyeSeparationPerEye, 0, 0));
					cameraRight->setPosition(mTrackedViewerOrientation * Ape::Vector3(mTrackerConfig.eyeSeparationPerEye, 0, 0));

					Ape::Vector3 viewerLeftEyeRelativeToDisplay = displayConfig.orientation.Inverse() * (mTrackedViewerPosition +
						(mTrackedViewerOrientation * Ape::Vector3(-mTrackerConfig.eyeSeparationPerEye, 0, 0)) - displayConfig.position);
					Ape::Vector3 viewerRightEyeRelativeToDisplay = displayConfig.orientation.Inverse() * (mTrackedViewerPosition +
						(mTrackedViewerOrientation * Ape::Vector3(mTrackerConfig.eyeSeparationPerEye, 0, 0)) - displayConfig.position);
					cameraLeft->setFocalLength(viewerLeftEyeRelativeToDisplay.z);
					cameraRight->setFocalLength(viewerRightEyeRelativeToDisplay.z);
					cameraLeft->setFrustumOffset(Ape::Vector2(-viewerLeftEyeRelativeToDisplay.x, -viewerLeftEyeRelativeToDisplay.y));
					cameraRight->setFrustumOffset(Ape::Vector2(-viewerRightEyeRelativeToDisplay.x, -viewerRightEyeRelativeToDisplay.y));
					cameraLeft->setFOVy(2 * atan((displayConfig.size.y / 2) / cameraLeft->getFocalLength()));
					cameraRight->setFOVy(2 * atan((displayConfig.size.y / 2) / cameraRight->getFocalLength()));

					//cameraLeft->setProjection(Ape::Matrix4());
					//cameraRight->setProjection(Ape::Matrix4());
					//std::cout << i << "left" << viewerLeftEyeRelativeToDisplay.x << "," << viewerLeftEyeRelativeToDisplay.y << ", " << viewerLeftEyeRelativeToDisplay.z << std::endl;
					//std::cout << i << "right" << viewerRightEyeRelativeToDisplay.x << "," << viewerRightEyeRelativeToDisplay.y << ", " << viewerRightEyeRelativeToDisplay.z << std::endl;
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	mpEventManager->disconnectEvent(Ape::Event::Group::NODE, std::bind(&ApeFobHeadTrackingPlugin::eventCallBack, this, std::placeholders::_1));
	mpEventManager->disconnectEvent(Ape::Event::Group::CAMERA, std::bind(&ApeFobHeadTrackingPlugin::eventCallBack, this, std::placeholders::_1));
}

void ApeFobHeadTrackingPlugin::Step()
{

}

void ApeFobHeadTrackingPlugin::Stop()
{

}

void ApeFobHeadTrackingPlugin::Suspend()
{

}

void ApeFobHeadTrackingPlugin::Restart()
{

}
