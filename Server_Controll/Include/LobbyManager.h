#pragma once
#include <string>
#include <memory>
#include "oneapi/tbb/concurrent_map.h"

namespace ControlServer
{
	struct Lobby
	{
		std::string Key;
		int Port;
		int Current;
		int Remain;
		bool Active;
		float ResponseSpeed;
	};

	class LobbyManager
	{
	public:
		LobbyManager();
		~LobbyManager();

		void SaveLobbyInfo(std::string key, int port, bool active);
		void UpdateLobbyInfo(std::string key, int current, int remain, bool active);
		void UpdateLobbyReponseSpeed(std::string key, float speed);
		void ResponseLobbyInfo(std::string& key, int& port, bool success);

	private:
		std::string SearchBestLobby();

		tbb::concurrent_map<std::string, std::shared_ptr<Lobby>> _lobbyMap;

	};
}