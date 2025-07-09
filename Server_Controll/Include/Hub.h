#pragma once
#include <thread>
#include "../Network/Networkmanager.h"


namespace ControlServer
{
	class Hub
	{
	public:
		Hub();
		~Hub();

		void Construct(int serverPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity);
		void MainThread();
	private:
		Network::NetworkManager _networkManager;

		bool isOn;
		int _acceptedCapacity;
	};
}