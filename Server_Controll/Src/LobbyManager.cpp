#include "LobbyManager.h"
#include "../Utility/Debug.h"

namespace ControlServer
{
	LobbyManager::LobbyManager()
	{

	}

	LobbyManager::~LobbyManager()
	{

	}

	void LobbyManager::SaveLobbyInfo(std::string key, int port, bool active)
	{
		//여러 항목을 묶어서 갱신해야함으로 개별항목에 대해 atomic을 쓰는것보다 lock이 더 안전함.
		//-> 관리서버임으로 job쓰레드를 단일쓰레도 돌리면서 lock을 쓰지않는방식으로 진행.

		auto finder = _lobbyMap.find(key);
		if (finder != _lobbyMap.end())
		{
			Utility::Log("LobbyManager", "SaveLobbyInfo", "중복된 로비키 수신");
			return;
		}

		std::shared_ptr<ControlServer::Lobby> newLobby = std::make_shared<ControlServer::Lobby>();
		newLobby->Active = active;
		newLobby->Key = key;
		newLobby->Port = port;
		newLobby->ResponseSpeed = std::numeric_limits<float>::max();
		newLobby->Current = 0;
		newLobby->Remain = 0;

		_lobbyMap.insert(std::make_pair(newLobby->Key, newLobby));

		Utility::Log("LobbyManager", "SaveLobbyInfo", "로비 등록 완료: Key=" + key + ", Port=" + std::to_string(port) + ", Active=" + (active ? "true" : "false"));
	}

	void LobbyManager::UpdateLobbyInfo(std::string key, int current, int remain, bool active)
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
		std::string lobbyKey = SearchBestLobby();
		if (lobbyKey == "")
		{
			success = false;
			Utility::Log("LobbyManager", "SaveLobbyInfo", "입장가능한 로비서버 없음");
		}
		else
		{
			key = lobbyKey;
			port = _lobbyMap[lobbyKey]->Port;
			success = true;

			Utility::Log("LobbyManager", "ResponseLobbyInfo", "최적 로비 서버 선택됨: Key=" + key + ", Port=" + std::to_string(port));
		}
	}

	std::string LobbyManager::SearchBestLobby()
	{
		std::string bestLobbyKey = "";
		float bestRatio = std::numeric_limits<float>::max();
		float bestResponseSpeed = std::numeric_limits<float>::max();

		for (const auto& [key, lobby] : _lobbyMap)
		{
			if (!lobby->Active)
				continue;

			int total = lobby->Current + lobby->Remain;
			if (total == 0)
				continue;

			float ratio = static_cast<float>(lobby->Current) / total;

			if (ratio < bestRatio ||
				(ratio == bestRatio && lobby->ResponseSpeed < bestResponseSpeed)) {

				bestRatio = ratio;
				bestResponseSpeed = lobby->ResponseSpeed;
				bestLobbyKey = key;
			}
		}

		return bestLobbyKey;
	}
}