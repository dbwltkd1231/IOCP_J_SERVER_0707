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
		//���� �׸��� ��� �����ؾ������� �����׸� ���� atomic�� ���°ͺ��� lock�� �� ������.


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