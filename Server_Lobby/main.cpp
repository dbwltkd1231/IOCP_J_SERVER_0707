#pragma once
#include "LobbyServer.h"

int main(int argc, char* argv[])
{
	std::string serverKey;
	int serverPort;
	int threadCount;
	int preCreateSocketCount;
	int acceptSocketMax;
	int overlappedQueueMax;
	std::string controlServerIp;
	int controlServerPort;

	if (argc > 8) 
	{
		try 
		{
			serverKey = argv[1];
			serverPort = std::stoi(argv[2]);
			threadCount = std::stoi(argv[3]);
			preCreateSocketCount = std::stoi(argv[4]);
			acceptSocketMax = std::stoi(argv[5]);
			overlappedQueueMax = std::stoi(argv[6]);
			controlServerIp = argv[7];
			controlServerPort = std::stoi(argv[8]);
		}
		catch (...) {
			std::cerr << "Invalid instance index: " << argv[1] << std::endl;
			return -1;
		}
	}
	else
	{
		serverKey = "Lobby1";
		serverPort = 9091;
		threadCount = 10;
		preCreateSocketCount = 10;
		acceptSocketMax = 10;
		overlappedQueueMax = 100;
		controlServerIp = "127.0.0.1";
		controlServerPort = 9090;
	}
	
	Lobby::LobbyServer lobbyServer;
	lobbyServer.Initialize(serverKey, serverPort, threadCount, preCreateSocketCount, acceptSocketMax, overlappedQueueMax, controlServerIp, controlServerPort);
	lobbyServer.MainProcess();

}