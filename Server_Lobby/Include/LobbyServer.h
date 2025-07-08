#pragma once
#include "../Network/Networkmanager.h"
#include "Job.h"

namespace Lobby
{
	class LobbyServer
	{
	public:
		LobbyServer();
		~LobbyServer();

	public:
		void Initialize(std::string serverKey, int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax, std::string targetServerIp, int targetServerPort);
		void MainProcess();
		void JobThread();

	private:
		std::string _serverKey;
		bool _isServerOn;

	private:
		std::function<void(ULONG_PTR socketPtr, Network::OperationType, Network::CustomOverlapped*)> _messageCallback;

	private:
		void EnqueueJob(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped);

	private:
		Utility::LockFreeCircleQueue<std::shared_ptr<Lobby::Job>> _jobQueue;

	private:
		Network::NetworkManager _networkManager;
	};

	
}