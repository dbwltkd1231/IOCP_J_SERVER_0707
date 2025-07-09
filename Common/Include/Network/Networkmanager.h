#pragma once
#include <string>
#include <thread>
#include <functional>

#define NOMINMAX
#include <winsock2.h>
#include <MSWSock.h>
#include <windows.h>
#include <ws2tcpip.h>

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

		void Construct(int overlappedQueueMax);

		void SetupListenSocket(int serverPort, int prepareSocketMax, int iocpThreadCount);
		void PrepareAcceptSocket();
		void SetupConnectSocket(std::string targetServerIp, int targetServerPort);
		int GetCurrentAcceptedSocket();
		void ProcessCompletionHandler();


	private:
		void FillSocketQueue();
		bool CallReceiveReady(SOCKET* targetSocket);
		bool ProcessDisconnect(SOCKET* targetSocket);

		void HandleConnectComplete();
		void HandleDisconnectComplete(ULONG_PTR key, std::string error);
		void HandleAcceptComplete(ULONG_PTR key);
		void HandleReceiveComplete(ULONG_PTR key, CustomOverlapped* overlapped, DWORD bytes);
		void HandleSendComplete(ULONG_PTR key, CustomOverlapped* overlapped);

		HANDLE _iocp = INVALID_HANDLE_VALUE;
		SOCKET* _listenSocket;
		SOCKET* _connectSocket;
		LPFN_ACCEPTEX _acceptExPointer = nullptr;

		Utility::LockFreeCircleQueue<Network::CustomOverlapped*> _overlappedQueue;
		tbb::concurrent_map<ULONG_PTR, SOCKET*> _preparedSocketMap;
		Utility::LockFreeCircleQueue<SOCKET*> _preparedSocketQueue;
		tbb::concurrent_map<ULONG_PTR, SOCKET*> _accpetCompletedSocketMap;

		int _prepareSocketMax;
		int _iocpThreadCount;

		bool _isOn;
	};
}