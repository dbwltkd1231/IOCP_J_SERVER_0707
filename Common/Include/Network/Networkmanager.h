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

		void Construct(Network::SenderType senderType, int overlappedQueueMax, std::function<void(ULONG_PTR, CustomOverlapped*)> iocpCallback, std::function<void(ULONG_PTR)> disconnectCallback);
		void SetupListenSocket(int serverPort, int prepareSocketMax, int iocpThreadCount);
		void PrepareAcceptSocket(Network::SenderType senderType);
		void SetupConnectSocket(std::string targetServerIp, int targetServerPort, Network::SenderType senderType);
		int GetCurrentAcceptedSocket();
		void ProcessCompletionHandler();

		void SendRequest(ULONG_PTR& targetSocket, uint32_t& contentType, std::string& stringBuffer, int& bodySize);

	private:
		void FillSocketQueue();
		bool CallReceiveReady(SOCKET* targetSocket);
		bool ProcessDisconnect(SOCKET* targetSocket);

		void HandleAcceptComplete(CustomOverlapped* overlapped);
		void HandleConnectComplete(CustomOverlapped* overlapped);
		void HandleDisconnectComplete(ULONG_PTR completionKey, std::string error);
		void HandleReceiveComplete(ULONG_PTR completionKey, CustomOverlapped* overlapped, DWORD bytes);
		void HandleSendComplete(ULONG_PTR completionKey, CustomOverlapped* overlapped);

		HANDLE _iocp = INVALID_HANDLE_VALUE;
		SOCKET* _listenSocket;
		LPFN_ACCEPTEX _acceptExPointer = nullptr;

		Utility::LockFreeCircleQueue<Network::CustomOverlapped*> _overlappedQueue;
		Utility::LockFreeCircleQueue<SOCKET*> _preparedSocketQueue;
		tbb::concurrent_map<ULONG_PTR, SOCKET*> _connectedSocketMap;

		std::function<void(ULONG_PTR)> _disconnectCallback;
		std::function<void(ULONG_PTR, CustomOverlapped*)> _iocpCallback;

		Network::SenderType _senderType;

		int _prepareSocketMax;
		int _iocpThreadCount;

		bool _isOn;
	};
}