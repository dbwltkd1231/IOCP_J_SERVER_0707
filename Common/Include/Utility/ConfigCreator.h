#pragma once
#include <iostream>
#include <fstream>
#include "../Library/nlohmann/json.hpp"


namespace Utility
{
	void CreateServerSettingFiles(std::string fileName)
	{
		nlohmann::json config;

		config["ControlServer"]["IP"] = "127.0.0.1";
		config["ControlServer"]["Port"] = 9090;
		config["ControlServer"]["ThreadCount"] = 10;
		config["ControlServer"]["PreCreateSocketCount"] = 10;
		config["ControlServer"]["AcceptSocketMax"] = 30;
		config["ControlServer"]["OverlappedQueueSizemax"] = 100;

		config["REDIS"]["PORT"] = 6379;

		config["LobbyServer"]["Keys"] = nlohmann::json::array({ "Lobby1", "Lobby2", "Lobby3"});
		config["LobbyServer"]["Ports"] = nlohmann::json::array({ 9091, 9092, 9093 });
		config["LobbyServer"]["ThreadCount"] = 10;
		config["LobbyServer"]["PreCreateSocketCount"] = 10;
		config["LobbyServer"]["AcceptSocketMax"] = 30;
		config["LobbyServer"]["OverlappedQueueSizemax"] = 100;

		std::ofstream file(fileName);
		file << config.dump(4);  // 4�� �鿩���� ����

		std::string log = "[Utility] [JsonCreator] File CreateSuccess !";
		std::cout << log << std::endl;
	}

	nlohmann::json LoadSettingFiles(std::string fileName)
	{
		std::ifstream file(fileName);  // JSON ���� ����
		if (!file)
		{
			std::string log = "[Utility] [JsonCreator] File Find Fail !!";
			std::cout << log << std::endl;

			return NULL;
		}

		nlohmann::json config;
		file >> config;  // ���Ͽ��� JSON ��ü �б�

		return config;
	}
}
