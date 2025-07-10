#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "../Network/Networkmanager.h"
#include "LobbyManager.h"
#include "../Protocol/ControlServerProtocol.h"

namespace ControlServer
{
	class Hub
	{
	public:
		Hub();
		~Hub();

		void Construct(int serverPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity, int packetQueueCapacity);
		void InitializeSubThread(int receiveThreadCount, int jobThreadCount);
		void Start();
	private:
		Network::NetworkManager _networkManager;

		bool isOn;
		int _acceptedCapacity;

		void ReceiveMessage(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped);
		void RequestSendMessage(Protocol::JobOutput output);
		void ReceiveThread();
		void JobThread();

		ControlServer::LobbyManager _lobbyManager;

		Utility::LockFreeCircleQueue<std::shared_ptr <Network::Packet>> _packetQueue;

		Utility::LockFreeCircleQueue<std::shared_ptr<Protocol::Job>> _jobQueue;
		std::condition_variable _jobThreadConditionValue;
		std::mutex _lockJobThread;

	};
}