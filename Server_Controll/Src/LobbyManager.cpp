#include "LobbyManager.h"
#include "../Utility/Debug.h"
#include "../Json/JsonData.h"

namespace ControlServer
{
	LobbyManager::LobbyManager()
	{

	}

	LobbyManager::~LobbyManager()
	{
		_redisWorker = nullptr;
	}

	void LobbyManager::Construct(Database::RedisWorker* redisWorker, int expireTime)
	{
		_redisWorker = redisWorker;
		_expireTime = expireTime;
	}

	void LobbyManager::SaveLobbyInfo(std::string key, int port, int capacity, bool active)
	{
		int activeOffset = active ? 1 : 0;
		nlohmann::json result = Json::LobbyInfo::DataToJson(key, port, 0, capacity, activeOffset, 0);
		std::string jsonString = result.dump(); 
		_redisWorker->UpdateData("LobbyInfo", key, jsonString, _expireTime);
		Utility::Log("LobbyManager", "SaveLobbyInfo", "로비 등록 완료: Key=" + key + ", Port=" + std::to_string(port) + ", Capacity=" + std::to_string(capacity) + ", Active=" + (active ? "true" : "false"));
	}

	void LobbyManager::UpdateLobbyInfo(std::string key, int current, int remain, bool active) // port번호추가필요.
	{
		auto finder = _lobbyMap.find(key);
		if (finder == _lobbyMap.end())
		{
			Utility::Log("LobbyManager", "SaveLobbyInfo", "조회되는 로비키 없음");
			return;
		}

		std::shared_ptr<ControlServer::Lobby> targetLobby = finder->second;
		targetLobby->Current = current;
		targetLobby->Remain = remain;
		targetLobby->Active = active;

		Utility::Log("LobbyManager", "UpdateLobbyInfo", "로비 상태 갱신: Key=" + key + ", Current=" + std::to_string(current) + ", Remain=" + std::to_string(remain) + ", Active=" + (active ? "true" : "false"));
	}

	void LobbyManager::UpdateLobbyReponseSpeed(std::string key, float speed)
	{
		auto finder = _lobbyMap.find(key);
		if (finder == _lobbyMap.end())
		{
			Utility::Log("LobbyManager", "SaveLobbyInfo", "조회되는 로비키 없음");
			return;
		}

		std::shared_ptr<ControlServer::Lobby> targetLobby = finder->second;
		targetLobby->ResponseSpeed = speed;

		Utility::Log("LobbyManager", "UpdateLobbyResponseSpeed", "응답 속도 갱신: Key=" + key + ", ResponseSpeed=" + std::to_string(speed));
	}

	void LobbyManager::ResponseLobbyInfo(std::string& key, int& port, bool success)
	{
		std::string bestLobbyKey = "";
		int bestLobbyPort = 0;
		SearchBestLobby(bestLobbyKey, bestLobbyPort);
		if (bestLobbyKey == "")
		{
			success = false;
			Utility::Log("LobbyManager", "SaveLobbyInfo", "입장가능한 로비서버 없음");
		}
		else
		{
			key = bestLobbyKey;
			port = bestLobbyPort;
			success = true;

			Utility::Log("LobbyManager", "ResponseLobbyInfo", "최적 로비 서버 선택됨: Key=" + key + ", Port=" + std::to_string(port));
		}
	}

	void LobbyManager::SearchBestLobby(std::string& bestLobbyKey, int& bestLobbyPort)
	{
		std::map <std::string, std::string> result = _redisWorker->ScanMultipleResult("LobbyInfo");
		float bestRatio = std::numeric_limits<float>::max();
		float bestResponseSpeed = std::numeric_limits<float>::max();

		for (const auto& [key, lobbyInfo] : result)
		{
			nlohmann::json lobbyJson = nlohmann::json::parse(lobbyInfo);
			Json::LobbyInfo targetLobby(lobbyJson);

			if (!targetLobby.Active)
				continue;

			int total = targetLobby.Current + targetLobby.Remain;
			if (total == 0)
				continue;

			float ratio = static_cast<float>(targetLobby.Current) / total;

			if (ratio < bestRatio ||
				(ratio == bestRatio && targetLobby.ResponseSpeed < bestResponseSpeed)) {

				bestRatio = ratio;
				bestResponseSpeed = targetLobby.ResponseSpeed;
				bestLobbyKey = key;
				bestLobbyPort = targetLobby.Port;
			}
		}
	}
}