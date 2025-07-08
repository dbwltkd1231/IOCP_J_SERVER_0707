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

	private:
		SOCKET _listenSocket;
		HANDLE _handle = INVALID_HANDLE_VALUE;
		LPFN_ACCEPTEX _acceptExPointer = nullptr;

	private:
		int _preCreateSocketCount = 0;
		int _acceptedSocketMax = 0;
		int _threadCount = 0;
		int _overlappedQueueMax = 0;

	public:
		void Construct(int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax, std::function<void(ULONG_PTR socketPtr, Network::OperationType, Network::CustomOverlapped*)> messageCallback);

	private:
		Utility::LockFreeCircleQueue<Network::CustomOverlapped*> _overlappedQueue;
		Utility::LockFreeCircleQueue<SOCKET*> _preparedSocketQueue;
		tbb::concurrent_map<ULONG_PTR, SOCKET*> _acceptedSocketMap;

	private:
		void OverlappedQueueSetting();
		void PrepareSocket();
		void RequestNewAccept();

	public:
		void RequestSend(ULONG_PTR socketPtr, const MessageHeader header, std::string& stringBuffer, int& bodySize);

	private:
		void ReceiveReady(ULONG_PTR socketPtr);
		void Disconnect(ULONG_PTR socketPtr);
		void ProcessSend(SOCKET* targetSocket, const MessageHeader header, std::string& stringBuffer, int& bodySize);

	private:
		std::function<void(ULONG_PTR socketPtr, Network::OperationType, Network::CustomOverlapped*)> _messageCallback;

	public:
		void ReturnOverlapped(Network::CustomOverlapped* customOverlapped);

	private:
		bool _networkOn;

	private:
		void Process();

	public:
		void ConnectToControlServer(std::string targetServerIp, int targetServerPort);

	private:
		SOCKET* _controlServerSocket;

	};
}