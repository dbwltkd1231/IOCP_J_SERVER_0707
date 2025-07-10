#include "LobbyManager.h"

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


	}

	void LobbyManager::UpdateLobbyInfo(std::string key, int current, int remain)
	{

	}

	void LobbyManager::UpdateLobbyReponseSpeed(std::string key, float speed)
	{

	}

	void LobbyManager::ResponseLobbyInfo(std::string& key, int& port, bool success)
	{

	}
}