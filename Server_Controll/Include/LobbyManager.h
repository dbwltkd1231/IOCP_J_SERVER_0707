#pragma once
#include <string>
#include <memory>
#include <map>
#include "oneapi/tbb/concurrent_map.h"
#include "../Database/RedisWorker.h"

namespace ControlServer
{
	struct Lobby
	{
		std::string Key;
		int Port;
		int Capacity;
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

		void Construct(Database::RedisWorker* redisWorker, int expireTime);
		void SaveLobbyInfo(std::string key, int port, int capacity, bool active);
		void UpdateLobbyInfo(std::string key, int current, int remain, bool active);
		void UpdateLobbyReponseSpeed(std::string key, float speed);
		void ResponseLobbyInfo(std::string& key, int& port, bool success);

	private:
		void SearchBestLobby(std::string& key, int& port);

		Database::RedisWorker* _redisWorker;
		tbb::concurrent_map<std::string, std::shared_ptr<Lobby>> _lobbyMap;
		int _expireTime;
	};
}

//#include <nlohmann/json.hpp> 