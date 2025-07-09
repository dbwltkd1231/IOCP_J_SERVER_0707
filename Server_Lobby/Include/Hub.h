#pragma once
#include "../Network/Networkmanager.h"

namespace LobbyServer
{
	class Hub
	{
	public:
		Hub();
		~Hub();

		void Construct(int serverPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity);
		void ConnectToControlServer(std::string targetServerIp, int targetServerPort);
		void MainThread();
	private:
		Network::NetworkManager _networkManager;

		bool isOn;
		int _acceptedCapacity;
	};
}