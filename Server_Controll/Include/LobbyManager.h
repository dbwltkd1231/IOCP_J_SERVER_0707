#pragma once
#include <string>
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
		void UpdateLobbyInfo(std::string key, int current, int remain);
		void UpdateLobbyReponseSpeed(std::string key, float speed);

		void ResponseLobbyInfo(std::string& key, int& port, bool success);

	private:
		tbb::concurrent_map<std::string, Lobby> _lobbyMap;




	};
}