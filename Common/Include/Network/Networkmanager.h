#pragma once
#include <string>
#include <thread>
#define NOMINMAX
#include <winsock2.h>
#include <MSWSock.h>
#include <windows.h>

#include "oneapi/tbb/concurrent_map.h"
#include "../Utility/LockFreeCircleQueue.h"
#include "../Network/BasicData.h"

namespace Network
{
	class NetworkManager
	{
	public:
		NetworkManager();
		~NetworkManager();

	private:
		SOCKET _listenSocket;
		HANDLE _handle = INVALID_HANDLE_VALUE;
		LPFN_ACCEPTEX _acceptExPointer = nullptr;

	public:
		void Construct(int serverPort, int threadCount);
		void OverlappedQueueSetting(int overlappedCount);
		void PrepareSocket(int acceptReadyCount, int threadCount);
		void Activate();

	public:
		ULONG_PTR RequestAccept();
		void RequestSend(ULONG_PTR socketPtr, const MessageHeader header, std::string& stringBuffer, int& bodySize);

	private:
		void ReceiveReady(ULONG_PTR socketPtr);
		void ProcessSend(SOCKET* targetSocket, const MessageHeader header, std::string& stringBuffer, int& bodySize);

	private:
		Utility::LockFreeCircleQueue<Network::CustomOverlapped*> _overlappedQueue;
		Utility::LockFreeCircleQueue<SOCKET*> _preparedSocketQueue;
		tbb::concurrent_map<ULONG_PTR, SOCKET*> _acceptedSocketMap;

	private:
		bool _networkOn;

	private:
		void Process();
	};
}