#include "LobbyMonitor.h"

namespace LobbyServer
{
	LobbyMonitor::LobbyMonitor()
	{

	}

	LobbyMonitor::~LobbyMonitor()
	{

	}


	void LobbyMonitor::Construct(int capacity)
	{
		_capacity = capacity;
		_currentUser.store(0, std::memory_order_release);
		_remainCapacity.store(_capacity, std::memory_order_release);
	}

	void LobbyMonitor::RegisterLobbyUser()
	{
		_currentUser.fetch_add(1, std::memory_order_acq_rel);
		_remainCapacity.fetch_sub(1, std::memory_order_acq_rel);

	}

	void LobbyMonitor::UnregisterLobbyUser()
	{
		_currentUser.fetch_sub(1, std::memory_order_acq_rel);
		_remainCapacity.fetch_add(1, std::memory_order_acq_rel);
	}

	int LobbyMonitor::GetCapacity()
	{
		return _capacity;
	}

	int LobbyMonitor::GetCurrentUser()
	{
		return _currentUser.load(std::memory_order_acquire);
	}

	int LobbyMonitor::GetRemainCapacity()
	{
		return _remainCapacity.load(std::memory_order_acquire);
	}

}