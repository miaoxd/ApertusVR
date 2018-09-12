/*MIT License

Copyright (c) 2018 MTA SZTAKI

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/


#ifndef APE_SCENESESSIONIMPL_H
#define APE_SCENESESSIONIMPL_H

#ifdef _WIN32
#ifdef APE_SCENEMANAGER_DLL_EXPORT
#define APE_SCENEMANAGER_DLL_EXPORT __declspec(dllexport)
#else
#define APE_SCENEMANAGER_DLL_EXPORT __declspec(dllimport)
#endif
#else
#define APE_SCENEMANAGER_DLL_EXPORT 
#endif

#include <iostream>
#include <thread>
#include <memory>
#include <chrono>
#include "managers/ApeISceneSession.h"
#include "managers/ApeISystemConfig.h"
#include "managers/ApeIPluginManager.h"
#include "managers/ApeIEventManager.h"
#include "managers/ApeISceneManager.h"
#include "ApeLobbyManager.h"
#include "ApePointCloudImpl.h"
#include "GetTime.h"
#include "Rand.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "FullyConnectedMesh2.h"
#include "Kbhit.h"
#include "RakSleep.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "SocketLayer.h"
#include "ReplicaManager3.h"
#include "NetworkIDManager.h"
#include "Gets.h"
#include "Itoa.h"
#include "NatPunchthroughClient.h"
#include "TCPInterface.h"
#include "ReadyEvent.h"
#include "PacketLogger.h"
#include "RPC4Plugin.h"
#include "HTTPConnection2.h"
#include "DS_List.h"

namespace Ape
{
	class APE_SCENEMANAGER_DLL_EXPORT SceneSessionImpl : public Ape::ISceneSession
	{
	private:
		RakNet::RakPeerInterface* mpRakReplicaPeer;

		RakNet::RakPeerInterface* mpRakStreamPeer;
		
		std::shared_ptr<RakNet::ReplicaManager3> mpReplicaManager3;
		
		RakNet::NetworkIDManager* mpNetworkIDManager;

		RakNet::NatPunchthroughClient* mpNatPunchthroughClient;

		RakNet::RakNetGUID mGuid;

		RakNet::SystemAddress mAddress;

		RakNet::RakNetGUID mHostGuid;

		RakNet::SystemAddress mHostAddress;

		std::string mNATServerIP;

		std::string mNATServerPort;

		RakNet::SystemAddress mNATServerAddress;

		std::string mLobbyServerIP;

		std::string mLobbyServerPort;

		std::string mLobbyServerSessionName;

		Ape::SceneSession::ParticipantType mParticipantType;

		bool mIsConnectedToNATServer;
		
		bool mIsHost;
		
		Ape::ISystemConfig* mpSystemConfig;

		Ape::IPluginManager* mpPluginManager;

		Ape::IEventManager* mpEventManager;

		Ape::ISceneManager* mpSceneManager;

		std::vector<Ape::Replica*> mStreamReplicas;

		void eventCallBack(const Ape::Event& event);

		bool mIsConnectedToHost;

		LobbyManager* mpLobbyManager;

		void init();

		void runReplicaPeerListen();

		void listenReplicaPeer();

	public:
		SceneSessionImpl();

		~SceneSessionImpl();

		void create();

		void destroy();

		void connect(SceneSessionUniqueID sceneSessionUniqueID);

		void leave();

		SceneSessionUniqueID getGUID();

		Ape::SceneSession::ParticipantType getParticipantType() override;

		bool isHost();

		std::weak_ptr<RakNet::ReplicaManager3>  getReplicaManager();

		void setScene(Ape::ISceneManager* scene);
	};
}

#endif