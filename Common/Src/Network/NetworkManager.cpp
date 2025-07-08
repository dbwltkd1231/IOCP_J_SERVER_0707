#include "../Network/Networkmanager.h"
#include "../Utility/Debug.h"

namespace Network
{
	NetworkManager::NetworkManager()
	{
		_networkOn = false;
		_messageCallback = nullptr;
	}

	NetworkManager::~NetworkManager()
	{
		_networkOn = false;
		_messageCallback = nullptr;
	}

	void NetworkManager::Construct(int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax, std::function<void(ULONG_PTR socketPtr, Network::OperationType, Network::CustomOverlapped*)> messageCallback)
	{
		_threadCount = threadCount;
		_preCreateSocketCount = preCreateSocketCount;
		_acceptedSocketMax = acceptSocketMax;
		_overlappedQueueMax = overlappedQueueMax;
		_messageCallback = messageCallback;

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "WSAStartup failed");
			return;
		}
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "WSAStartup Success");

		_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_listenSocket == INVALID_SOCKET)
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "Listen Socket Create Fail");
			WSACleanup();
			return;
		}
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "Listen Socket Create Success");

		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(serverPort);
		if (bind(_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "Listen Socket Bind Fail");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "Listen Socket Bind Success");

		//Utility::Log("Network", "IOCP", "bind success");
		if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "Listen Socket Listen Fail");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "Listen Socket Listen Success");

		_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _threadCount);
		if (_handle == NULL)
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "IOCP Handle Create Fail");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "IOCP Handle Create Success");

		if (!CreateIoCompletionPort((HANDLE)_listenSocket, _handle, 0, _threadCount))
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "IOCP Handle - ListenSocket Connect Fail");
			closesocket(_listenSocket);
			WSACleanup();
			return;
		}
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "CreateIoCompletionPort Success");

		GUID guidAcceptEx = WSAID_ACCEPTEX;
		DWORD bytesReceived;
		if (WSAIoctl(_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &_acceptExPointer, sizeof(_acceptExPointer), &bytesReceived, NULL, NULL) == SOCKET_ERROR)
		{
			Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", "WSAIoctl failed");
			return;
		}

		_preparedSocketQueue.Construct(_preCreateSocketCount);
		OverlappedQueueSetting();
		PrepareSocket();
		Utility::Log("Server_Controll", "NetworkManager", "Construct", "Completed");
	}

	void NetworkManager::OverlappedQueueSetting()
	{
		_overlappedQueue.Construct(_overlappedQueueMax);
		for (int i = 0;i < _overlappedQueueMax; ++i)
		{
			auto overlapped = new CustomOverlapped();
			_overlappedQueue.push(std::move(overlapped));
		}

		std::string log = "Overlapped객체풀 생성 : " + std::to_string(_overlappedQueueMax);
		Utility::Log("Server_Controll", "NetworkManager", "OverlappedQueueSetting", log);
	}

	void NetworkManager::PrepareSocket()
	{
		for (int index = 0;index < _preCreateSocketCount; ++index)
		{
			SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			SOCKET* socketPtr = new SOCKET(newSocket);
			CreateIoCompletionPort((HANDLE)newSocket, _handle, (ULONG_PTR)socketPtr, _threadCount);

			_preparedSocketQueue.push(std::move(socketPtr));

		}
		
		std::string log = "소켓 IOCP 연결 : " + std::to_string(_preCreateSocketCount);
		Utility::Log("Server_Controll", "NetworkManager", "PrepareSocket", log);
	}

	void NetworkManager::RequestNewAccept()
	{
		std::string log = "";

		if (_preparedSocketQueue.size() < 1)
		{
			PrepareSocket();//캐싱필요.
		}

		int errorCode;
		int errorCodeSize = sizeof(errorCode);

		SOCKET* targetSocket = _preparedSocketQueue.pop();
		getsockopt(*targetSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			log = "Getsockopt Fail : " + std::to_string(errorCode);
			Utility::Log("Server_Controll", "NetworkManager", "RequestNewAccept", log);
			return;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		ULONG_PTR socketKey = (ULONG_PTR)targetSocket;
		targetOverlapped->AcceptSetting(socketKey);

		DWORD bytesReceived = 0;
		bool result = _acceptExPointer(_listenSocket, *targetSocket, targetOverlapped->Wsabuf[1].buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytesReceived, (CustomOverlapped*)&(*targetOverlapped));

		errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
		{
			log = "AcceptEx 실패! 오류 코드: " + std::to_string(errorCode);
			return;
		}
		else
		{
			log = "클라이언트 AcceptEx 호출";
		}

		_acceptedSocketMap.insert(std::make_pair(socketKey, targetSocket));
		Utility::Log("Server_Controll", "NetworkManager", "RequestNewAccept", log);
	}

	void NetworkManager::ReceiveReady(ULONG_PTR socketPtr)
	{
		auto finder = _acceptedSocketMap.find(socketPtr);
		if (finder == _acceptedSocketMap.end())
			return;

		SOCKET* targetSocket = finder->second;
		std::string log = "";
		int errorCode;
		int errorCodeSize = sizeof(errorCode);
		getsockopt(*targetSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			log = "Getsockopt Fail : " + std::to_string(errorCode);
			Utility::Log("Server_Controll", "NetworkManager", "ReceiveReady", log);
			return;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		targetOverlapped->ReceiveSetting();

		DWORD flags = 0;
		int result = WSARecv(*targetSocket, targetOverlapped->Wsabuf, 2, nullptr, &flags, &*targetOverlapped, nullptr);

		errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
		{
			log = "WSARecv 실패! 오류 코드: " + std::to_string(errorCode);
		}
		else
		{
			log = " 클라이언트 WSARecv 호출";
		}

		Utility::Log("Server_Controll", "NetworkManager", "ReceiveReady", log);
	}

	void NetworkManager::Disconnect(ULONG_PTR socketPtr)
	{
		auto finder = _acceptedSocketMap.find(socketPtr);
		if (finder == _acceptedSocketMap.end())
			return;

		SOCKET* targetSocket = finder->second;

		//TODO
	}

	void NetworkManager::RequestSend(ULONG_PTR socketPtr, const MessageHeader header, std::string& stringBuffer, int& bodySize)
	{
		auto finder = _acceptedSocketMap.find(socketPtr);
		if (finder == _acceptedSocketMap.end())
			return;

		SOCKET* socket = finder->second;

		ProcessSend(socket, header, stringBuffer, bodySize);
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

	void NetworkManager::ReturnOverlapped(Network::CustomOverlapped* customOverlapped)
	{
		customOverlapped->Clear();
		_overlappedQueue.push(std::move(customOverlapped));
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
				int errorCode = WSAGetLastError();

				switch (errorCode)
				{
					case WSAECONNRESET:
					case WSAECONNABORTED:
					case WSAENETRESET:
					case WSAETIMEDOUT:
					case WSAENOTCONN:
					case WSAESHUTDOWN:
					case ERROR_NETNAME_DELETED:
						_messageCallback(completionKey, OperationType::OP_DISCONNECT, nullptr);
						//Disconnect(completionKey);
						break;
				}

				switch (overlapped->GetOperation())
				{
					case OperationType::OP_ACCEPT:
					{
						if (_acceptedSocketMap.size() < _acceptedSocketMax)
						{
							RequestNewAccept();
						}

						ReceiveReady(completionKey);
						_messageCallback(completionKey, OperationType::OP_ACCEPT, nullptr);
						break;
					}
					case OperationType::OP_RECV:
					{
						if (bytesTransferred <= 0)
						{
							_messageCallback(completionKey, OperationType::OP_DISCONNECT, overlapped);
							//Disconnect(completionKey);
							//_disconnectCallback(overlapped, completionKey, bytesTransferred, 0);
						}

					//// 여기서 풀링을하면 메모리를 새로생성해야함으로 오버헤드발생가능 -> 풀링은 따로 콜백을 받고, 여기서는 그대로 전달하자.
					//MessageHeader* receivedHeader = reinterpret_cast<MessageHeader*>(overlapped->Wsabuf[0].buf);
					//int requestBodySize = ntohl(receivedHeader->BodySize);
					//uint32_t requestContentsType = ntohl(receivedHeader->ContentsType);
					//
					//std::string bufferString = std::string(overlapped->Wsabuf[1].buf, requestBodySize);
		
						_messageCallback(completionKey, OperationType::OP_RECV, overlapped);
						
						break;
					}
					case OperationType::OP_SEND:
					{
						_messageCallback(completionKey, OperationType::OP_SEND, overlapped);
						break;
					}
					case OperationType::OP_DEFAULT:
					{
						_messageCallback(completionKey, OperationType::OP_DEFAULT, nullptr);
						break;
					}
				}
			}
			else
			{

			}
		}
	}

}