#pragma once
#include <thread>
#include "../Network/Networkmanager.h"
#include "Business/Job.h"

namespace Business
{
	class ServerControlCenter
	{
	public:
		ServerControlCenter();
		~ServerControlCenter();

	public:
		void Initialize(int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax);
		void MainProcess();
		void JobThread();

	private:
		bool _isServerOn;

	private:
		std::function<void(ULONG_PTR socketPtr, Network::OperationType, Network::CustomOverlapped*)> _messageCallback;

	private:
		void EnqueueJob(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped);

	private:
		Utility::LockFreeCircleQueue<std::shared_ptr<Business::Job>> _jobQueue;

	private:
		Network::NetworkManager _networkManager;
	};
}