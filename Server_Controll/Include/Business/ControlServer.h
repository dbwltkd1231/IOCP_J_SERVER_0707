#pragma once
#include <vector>
#include <thread>
#include "../Network/Networkmanager.h"
#include "Business/Job.h"

namespace Control
{
	class ControlServer
	{
	public:
		ControlServer();
		~ControlServer();

	public:
		void Initialize(std::string serverIp, int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax);
		void LobbySeverInfoSetting(std::vector<std::string> lobbyKeys, std::vector<int> lobbyPorts, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax);
		void MainProcess();
		void JobThread();

	private:
		std::string _ip;
		int _port;

	private:
		bool _isServerOn;

	private:
		std::vector<std::string> _lobbyKeys;
		std::vector<int> _lobbyPorts;
		int _lobbyThreadCount;
		int _lobbyPreCreateSocketCount;
		int _lobbyAcceptSocketMax;
		int _lobbyOverlappedQueueMax;

	private:
		std::function<void(ULONG_PTR socketPtr, Network::OperationType, Network::CustomOverlapped*)> _messageCallback;

	private:
		void EnqueueJob(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped);

	private:
		Utility::LockFreeCircleQueue<std::shared_ptr<Control::Job>> _jobQueue;

	private:
		Network::NetworkManager _networkManager;

	private:
		void ExecuteLobbyServer();
	};
}