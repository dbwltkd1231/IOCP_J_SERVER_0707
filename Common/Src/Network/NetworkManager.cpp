#include "../Network/Networkmanager.h"

namespace Network
{
	NetworkManager::NetworkManager()
	{
		_networkOn = false;
	}

	NetworkManager::~NetworkManager()
	{
		_networkOn = false;
	}

	void NetworkManager::Construct(int serverPort, int threadCount)
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			//Utility::LogError("Network", "IOCP", "WSAStartup failed");
			return;
		}

		_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_listenSocket == INVALID_SOCKET)
		{
			//Utility::LogError("Network", "IOCP", "listenSocket Create failed");
			WSACleanup();
			return;
		}

		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(serverPort);
		if (bind(_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			//Utility::LogError("Network", "IOCP", "bind failed");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}

		//Utility::Log("Network", "IOCP", "bind success");
		if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			//Utility::LogError("Network", "IOCP", "listen failed");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}

		//Utility::Log("Network", "IOCP", "listen success");

		_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, threadCount);
		if (_handle == NULL)
		{
			//Utility::LogError("Network", "IOCP", "CreateIoCompletionPort failed");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}
		//Utility::Log("Network", "IOCP", "IOCP Handle Ready");

		if (!CreateIoCompletionPort((HANDLE)_listenSocket, _handle, 0, threadCount))
		{
			//Utility::LogError("Network", "IOCP", "CreateIoCompletionPort failed");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}
		//Utility::Log("Network", "IOCP", "CreateIoCompletionPort Success");

		GUID guidAcceptEx = WSAID_ACCEPTEX;
		DWORD bytesReceived;
		if (WSAIoctl(_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &_acceptExPointer, sizeof(_acceptExPointer), &bytesReceived, NULL, NULL) == SOCKET_ERROR)
		{
			//Utility::Log("Network", "IOCP", "WSAIoctl failed");
			return;
		}

	}

	void NetworkManager::OverlappedQueueSetting(int overlappedCount)
	{
		_overlappedQueue.Construct(overlappedCount);
		for (int i = 0;i < overlappedCount; ++i)
		{
			auto overlapped = new CustomOverlapped();
			_overlappedQueue.push(std::move(overlapped));
		}

		//Utility::Log("Network", "NetworkManager", "OverlappedQueue Ready Success");
	}

	void NetworkManager::PrepareSocket(int acceptReadyCount, int threadCount)
	{
		for (int index = 0;index < acceptReadyCount; ++index)
		{
			SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			SOCKET* socketPtr = new SOCKET(newSocket);
			CreateIoCompletionPort((HANDLE)newSocket, _handle, (ULONG_PTR)socketPtr, threadCount);

			_preparedSocketQueue.push(std::move(socketPtr));
		}

		//std::string log = "소켓 IOCP 연결 : " + std::to_string(acceptReadyCount);
		//Utility::Log("Network", "NetworkManager", log);
	}

	void NetworkManager::Activate()
	{

	}

	ULONG_PTR NetworkManager::RequestAccept()
	{
		if (_preparedSocketQueue.size() < 1)
		{
			PrepareSocket(1, 1);//캐싱필요.
		}

		int errorCode;
		int errorCodeSize = sizeof(errorCode);

		SOCKET* targetSocket = _preparedSocketQueue.pop();
		getsockopt(*targetSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			//std::cerr << "Socket error detected: " << errorCode << std::endl;
			return 0;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		ULONG_PTR socketKey = (ULONG_PTR)targetSocket;
		targetOverlapped->AcceptSetting(socketKey);

		DWORD bytesReceived = 0;
		bool result = _acceptExPointer(_listenSocket, *targetSocket, targetOverlapped->Wsabuf[1].buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytesReceived, (CustomOverlapped*)&(*targetOverlapped));

		std::string log;
		errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
		{
			log = "AcceptEx 실패! 오류 코드: " + std::to_string(errorCode);
			return 0;
		}
		else
		{
			log = "클라이언트 AcceptEx 호출";
		}

		_acceptedSocketMap.insert(std::make_pair(socketKey, targetSocket));
		//	Utility::Log("Network", "Client", log);

		return socketKey;
	}

	void NetworkManager::RequestSend(ULONG_PTR socketPtr, const MessageHeader header, std::string& stringBuffer, int& bodySize)
	{
		auto finder = _acceptedSocketMap.find(socketPtr);
		if (finder == _acceptedSocketMap.end())
			return;

		SOCKET* socket = finder->second;

		ProcessSend(socket, header, stringBuffer, bodySize);
	}


	void NetworkManager::ReceiveReady(ULONG_PTR socketPtr)
	{
		auto finder = _acceptedSocketMap.find(socketPtr);
		if (finder == _acceptedSocketMap.end())
			return;

		SOCKET* targetSocket = finder->second;

		int errorCode;
		int errorCodeSize = sizeof(errorCode);
		getsockopt(*targetSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			std::cerr << "Socket error detected: " << errorCode << std::endl;
			return;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		targetOverlapped->ReceiveSetting();

		DWORD flags = 0;
		int result = WSARecv(*targetSocket, targetOverlapped->Wsabuf, 2, nullptr, &flags, &*targetOverlapped, nullptr);

		std::string log;
		errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
		{
			log = "WSARecv 실패! 오류 코드: " + std::to_string(errorCode);
		}
		else
		{
			log = " 클라이언트 WSARecv 호출";
		}

		//Utility::Log("Network", "Client", log);
	}

	void NetworkManager::ProcessSend(SOCKET* targetSocket, const MessageHeader header, std::string& stringBuffer, int& bodySize)
	{
		if (targetSocket == nullptr || *targetSocket == INVALID_SOCKET)
		{
			//Utility::Log("Network", "Client", "Invalid Socket Pointer");
			return;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();

		targetOverlapped->SendSetting(header, stringBuffer.c_str(), bodySize);

		DWORD flags = 0;
		int result = WSASend(*targetSocket, targetOverlapped->Wsabuf, 2, nullptr, flags, &*targetOverlapped, nullptr);
		int errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			std::string log = "WSASend 실패! 오류 코드: " + std::to_string(errorCode);
			//Utility::Log("Network", "Client", log);
			return;
		}

		//Utility::Log("Network", "Client", "클라이언트 WSASend 호출");
	}

	void NetworkManager::Process()
	{
		_networkOn = true;

		CustomOverlapped* overlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;


		while (_networkOn)
		{
			BOOL result = GetQueuedCompletionStatus(_handle, &bytesTransferred, &completionKey, reinterpret_cast<LPOVERLAPPED*>(&overlapped), INFINITE);

			if (!result)
			{

			}
		}
	}

}