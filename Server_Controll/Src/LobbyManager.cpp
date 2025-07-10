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
		//���� �׸��� ��� �����ؾ������� �����׸� ���� atomic�� ���°ͺ��� lock�� �� ������.
		//-> �������������� job�����带 ���Ͼ����� �����鼭 lock�� �����ʴ¹������ ����.

		auto finder = _lobbyMap.find(key);
		if (finder != _lobbyMap.end())
		{
			Utility::Log("LobbyManager", "SaveLobbyInfo", "�ߺ��� �κ�Ű ����");
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

		Utility::Log("LobbyManager", "SaveLobbyInfo", "�κ� ��� �Ϸ�: Key=" + key + ", Port=" + std::to_string(port) + ", Active=" + (active ? "true" : "false"));
	}

	void LobbyManager::UpdateLobbyInfo(std::string key, int current, int remain, bool active)
	{
		auto finder = _lobbyMap.find(key);
		if (finder == _lobbyMap.end())
		{
			Utility::Log("LobbyManager", "SaveLobbyInfo", "��ȸ�Ǵ� �κ�Ű ����");
			return;
		}

		std::shared_ptr<ControlServer::Lobby> targetLobby = finder->second;
		targetLobby->Current = current;
		targetLobby->Remain = remain;
		targetLobby->Active = active;

		Utility::Log("LobbyManager", "UpdateLobbyInfo", "�κ� ���� ����: Key=" + key + ", Current=" + std::to_string(current) + ", Remain=" + std::to_string(remain) + ", Active=" + (active ? "true" : "false"));
	}

	void LobbyManager::UpdateLobbyReponseSpeed(std::string key, float speed)
	{
		auto finder = _lobbyMap.find(key);
		if (finder == _lobbyMap.end())
		{
			Utility::Log("LobbyManager", "SaveLobbyInfo", "��ȸ�Ǵ� �κ�Ű ����");
			return;
		}

		std::shared_ptr<ControlServer::Lobby> targetLobby = finder->second;
		targetLobby->ResponseSpeed = speed;

		Utility::Log("LobbyManager", "UpdateLobbyResponseSpeed", "���� �ӵ� ����: Key=" + key + ", ResponseSpeed=" + std::to_string(speed));
	}

	void LobbyManager::ResponseLobbyInfo(std::string& key, int& port, bool success)
	{
		std::string lobbyKey = SearchBestLobby();
		if (lobbyKey == "")
		{
			success = false;
			Utility::Log("LobbyManager", "SaveLobbyInfo", "���尡���� �κ񼭹� ����");
		}
		else
		{
			key = lobbyKey;
			port = _lobbyMap[lobbyKey]->Port;
			success = true;

			Utility::Log("LobbyManager", "ResponseLobbyInfo", "���� �κ� ���� ���õ�: Key=" + key + ", Port=" + std::to_string(port));
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