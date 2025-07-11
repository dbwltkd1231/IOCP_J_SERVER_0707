#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

#include "../Network/Networkmanager.h"
#include "LobbyManager.h"
#include "../Protocol/ControlServerProtocol.h"
//#include "oneapi/tbb/concurrent_queue.h"

namespace ControlServer
{
	class Hub
	{
	public:
		Hub();
		~Hub();

		void Construct(std::string ip, int serverPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity, int packetQueueCapacity);
		void InitializeSubThread(int receiveThreadCount, int jobThreadCount);
		void Start();

		void LobbySeverInfoSetting(std::vector<std::string> lobbyKeys, std::vector<int> lobbyPorts, int lobbyThreadCount, int lobbyPreCreateSocketCount, int lobbyAcceptSocketMax, int lobbyOverlappedQueueSizemax);
		void LobbyServerStart(int count);
	private:
		Network::NetworkManager _networkManager;

		bool isOn;
		int _acceptedCapacity;

		void ProcessIocp(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped);
		void ProcessDisconnect(ULONG_PTR completionKey);
		void RequestSendMessage(Protocol::JobOutput output);
		void ReceiveThread();
		void JobThread();

		ControlServer::LobbyManager _lobbyManager;

		Utility::LockFreeCircleQueue<std::shared_ptr <Network::Packet>> _packetQueue;
		//tbb::concurrent_queue<std::shared_ptr<Network::Packet>>_packetQueue;
		//tbb::concurrent_queue<std::shared_ptr<Protocol::Job>>_jobQueue;
		Utility::LockFreeCircleQueue<std::shared_ptr<Protocol::Job>> _jobQueue;

		std::condition_variable _jobThreadConditionValue;
		std::mutex _lockJobThread;

		std::string _controlServerIp;
		int _controlServerPort;

		int _currentLobbyCount;
		std::vector<std::string> _lobbyKeys;
		std::vector<int> _lobbyPorts;
		int _lobbyThreadCount;
		int _lobbyPreCreateSocketCount;
		int _lobbyAcceptSocketMax;
		int _lobbyOverlappedQueueSizemax;

	};
}