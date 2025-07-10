#pragma once
#include "Hub.h"
#include "../Utility/ConfigCreator.h"

//#define SettingMode 

int main()
{
#if defined(SettingMode)
	Utility::CreateServerSettingFiles("ServerSetting_config.json");
#else

	auto config = Utility::LoadSettingFiles("ServerSetting_config.json");
	if (config == NULL)
	{
		return 0;
	}

    // ControlServer 값 불러오기
    std::string controlIP = config["ControlServer"]["IP"];
    int controlPort = config["ControlServer"]["Port"];
    int controlThreadCount = config["ControlServer"]["ThreadCount"];
    int controlPreCreateSocketCount = config["ControlServer"]["PreCreateSocketCount"];
    int controlAcceptSocketMax = config["ControlServer"]["AcceptSocketMax"];
    int controlOverlappedQueueSizemax = config["ControlServer"]["OverlappedQueueSizemax"];

    // LobbyServer 배열 값 불러오기
    std::vector<std::string> lobbyKeys = config["LobbyServer"]["Keys"].get<std::vector<std::string>>();
    std::vector<int> lobbyPorts = config["LobbyServer"]["Ports"].get<std::vector<int>>();
    int lobbyThreadCount = config["LobbyServer"]["ThreadCount"];
    int lobbyPreCreateSocketCount = config["LobbyServer"]["PreCreateSocketCount"];
    int lobbyAcceptSocketMax = config["LobbyServer"]["AcceptSocketMax"];
    int lobbyOverlappedQueueSizemax = config["LobbyServer"]["OverlappedQueueSizemax"];

    ControlServer::Hub hub;
    hub.Construct(controlIP, controlPort, controlPreCreateSocketCount, controlThreadCount, controlOverlappedQueueSizemax, controlAcceptSocketMax, 100);
    hub.InitializeSubThread(1, 5);

    hub.LobbySeverInfoSetting(lobbyKeys, lobbyPorts, lobbyThreadCount, lobbyPreCreateSocketCount, lobbyAcceptSocketMax, lobbyOverlappedQueueSizemax);
    hub.LobbyServerStart(2);

    hub.Start();

#endif

	return 0;
}