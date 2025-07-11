#pragma once
#include <atomic>
#include <string>
namespace LobbyServer
{
	class LobbyMonitor
	{
	public:
		LobbyMonitor();
		~LobbyMonitor();

		void Construct(int capacity);
		void RegisterLobbyUser();
		void UnregisterLobbyUser();

		int GetCurrentUser();
		int GetRemainCapacity();
	private:
		int _capacity;
		std::atomic<int> _currentUser;
		std::atomic<int> _remainCapacity;
		float _responseSpeed;


	};
}