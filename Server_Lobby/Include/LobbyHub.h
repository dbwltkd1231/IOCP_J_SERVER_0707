#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

#include "LobbyMonitor.h"
#include "../Network/Networkmanager.h"
#include "../Protocol/LobbyServerProtocol.h"

namespace LobbyServer
{
	class LobbyHub
	{
	public:
		LobbyHub();
		~LobbyHub();

		void Construct(std::string lobbyKey, int lobbyPort, int iocpThreadCount, int preparedSocketMax, int acceptexSocketMax, int overlappedQueueMax, int packetQueueCapacity, int lobbyCapacity);
		void InitializeSubThread(int receiveThreadCount, int jobThreadCount);
		void Start();

		void ControlSeverInfoSetting(std::string controlServerIp, int controlServerPort);
		void ConnectControlServer();
	private:
		Network::NetworkManager _networkManager;
		LobbyServer::LobbyMonitor _lobbyMonitor;

		bool isOn;
		
		std::string _lobbyKey;
		int _lobbyPort;
		int _icopThreadCount;
		int _preparedSocketMax;
		int _accpetexSocketMax;
		int _overlappedQueueMax;
		int _packetQueueCapacity;

		std::string _controlServerIp;
		int _controlServerPort;
		ULONG_PTR _controlServerCompletionKey;


		void ProcessIocp(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped);
		void ProcessDisconnect(ULONG_PTR completionKey);
		void RequestSendMessage(Protocol::JobOutput output);
		void ReceiveThread();
		void JobThread();

		Utility::LockFreeCircleQueue<std::shared_ptr <Network::Packet>> _packetQueue;

		Utility::LockFreeCircleQueue<std::shared_ptr<Protocol::Job>> _jobQueue;
		std::condition_variable _jobThreadConditionValue;
		std::mutex _lockJobThread;




	};
}