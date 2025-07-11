#pragma once
#include "../Network/Networkmanager.h"
#include "../Utility/Debug.h"

namespace Network
{
	NetworkManager::NetworkManager()
	{
		_isOn = false;
		_senderType = Network::SenderType::DEFAULT;
		_iocpCallback = nullptr;
		_disconnectCallback = nullptr;
	}

	NetworkManager::~NetworkManager()
	{
		_isOn = false;
		_senderType = Network::SenderType::DEFAULT;
		_iocpCallback = nullptr;
		_disconnectCallback = nullptr;

		//  _overlappedQueue 초기화
		// connectedSocketMap 초기화
	}

	void NetworkManager::Construct(Network::SenderType senderType, int overlappedQueueMax, std::function<void(ULONG_PTR, CustomOverlapped*)> iocpCallback, std::function<void(ULONG_PTR)> disconnectCallback)
	{
		_senderType = senderType;

		_overlappedQueue.Construct(overlappedQueueMax);
		for (int i = 0;i < overlappedQueueMax; ++i)
		{
			auto overlapped = new CustomOverlapped();
			_overlappedQueue.push(std::move(overlapped));
		}

		_iocpCallback = std::move(iocpCallback);
		_disconnectCallback = std::move(disconnectCallback);
	}

	void NetworkManager::SetupListenSocket(int serverPort, int prepareSocketMax, int iocpThreadCount)
	{
		_prepareSocketMax = prepareSocketMax;
		_iocpThreadCount = iocpThreadCount;

		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
		_listenSocket = new SOCKET(socket);

		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(serverPort);
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		bind(*_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
		listen(*_listenSocket, SOMAXCONN);

		auto socketKey = (ULONG_PTR)_listenSocket;

		_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, _iocpThreadCount);
		CreateIoCompletionPort((HANDLE)*_listenSocket, _iocp, socketKey, _iocpThreadCount);

		GUID guidAcceptEx = WSAID_ACCEPTEX;
		DWORD bytesReceived;
		if (WSAIoctl(*_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof(guidAcceptEx), &_acceptExPointer, sizeof(_acceptExPointer), &bytesReceived, NULL, NULL) == SOCKET_ERROR)
		{
			Utility::Log("NetworkManager", "SetupListenSocket", "WSAIoctl failed");
			return;
		}

		_preparedSocketQueue.Construct(_prepareSocketMax);
		FillSocketQueue();

		Utility::Log("NetworkManager", "SetupListenSocket", "Completed !");

	}

	void NetworkManager::FillSocketQueue()
	{
		for (int i = 0;i < _prepareSocketMax;++i)
		{
			SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			SOCKET* socketPtr = new SOCKET(newSocket);
			CreateIoCompletionPort((HANDLE)*socketPtr, _iocp, (ULONG_PTR)socketPtr, _iocpThreadCount);
			_preparedSocketQueue.push(std::move(socketPtr));
		}

		Utility::Log("NetworkManager", "FillSocketQueue", "Completed !");
	}

	//- AcceptEx는 다수의 연결을 미리 걸어둘 수 있음 → 접속 폭주 상황에도 여유롭게 처리 가능. IOCP 워커 스레드가 비동기적으로 연결 완료를 받아들이고 수신 준비까지 빠르게 진행 가능. 즉, 동기 accept 쓰레드 구조보다 AcceptEx + IOCP 비동기 구조가 고속 대기열 처리에 유리.

	void NetworkManager::PrepareAcceptSocket(Network::SenderType senderType)
	{
		if (_preparedSocketQueue.size() < 1)
		{
			FillSocketQueue();
		}

		SOCKET* targetSocket = _preparedSocketQueue.pop();

		int errorCode;
		int errorCodeSize = sizeof(errorCode);

		getsockopt(*targetSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			std::string log = "Getsockopt 실패 : " + std::to_string(errorCode);
			Utility::Log("NetworkManager", "PrepareAcceptSocket", log);
			return;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		targetOverlapped->AcceptSetting(targetSocket, senderType);

		DWORD bytesReceived = 0;
		BOOL result = _acceptExPointer(
			*_listenSocket, *targetSocket,
			targetOverlapped->Wsabuf[1].buf, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			&bytesReceived, (OVERLAPPED*)targetOverlapped
		);

		int lastError = WSAGetLastError();
		if (result == FALSE && lastError != WSA_IO_PENDING)
		{
			std::string log = "AcceptEx 실패! 오류 코드: " + std::to_string(lastError);
			Utility::Log("NetworkManager", "PrepareAcceptSocket", log);
			return;
		}

		Utility::Log("PrepareAcceptSocket", "PrepareAcceptSocket", "AcceptEx 호출 성공");
	}

	void NetworkManager::SetupConnectSocket(std::string targetServerIp, int targetServerPort, Network::SenderType senderType)
	{
		SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);

		// 로컬 주소 바인딩
		sockaddr_in  localAddr = { 0 };
		localAddr.sin_family = AF_INET;
		localAddr.sin_addr.s_addr = INADDR_ANY;
		localAddr.sin_port = 0;  // 자동 할당

		if (bind(socket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
		{
			Utility::Log("NetworkManager", "SetupConnectSocket", "클라이언트 bind 실패");
			closesocket(socket);
			return;
		}

		SOCKET* connectSocket = new SOCKET(socket);

		GUID connectExGuid = WSAID_CONNECTEX;
		LPFN_CONNECTEX connectEx = NULL;
		DWORD bytes;
		if (WSAIoctl(*connectSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&connectExGuid, sizeof(connectExGuid),
			&connectEx, sizeof(connectEx),
			&bytes, NULL, NULL))
		{
			Utility::Log("NetworkManager", "SetupConnectSocket", "ConnectEx 가져오기 실패");
			closesocket(*connectSocket);
			return;
		}

		sockaddr_in targetServerAddr{};
		targetServerAddr.sin_family = AF_INET;
		targetServerAddr.sin_port = htons(targetServerPort);
		inet_pton(AF_INET, targetServerIp.c_str(), &targetServerAddr.sin_addr);

		auto socketKey = (ULONG_PTR)connectSocket;

		CreateIoCompletionPort((HANDLE)*connectSocket, _iocp, socketKey, _iocpThreadCount);

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		targetOverlapped->ConnectSetting(connectSocket, senderType);

		BOOL result = connectEx(*connectSocket, (sockaddr*)&targetServerAddr, sizeof(targetServerAddr), nullptr, 0, nullptr, &*targetOverlapped);
		int lastError = WSAGetLastError();
		if (result == FALSE && lastError != WSA_IO_PENDING)
		{
			std::string log = "AcceptEx 실패! 오류 코드: " + std::to_string(lastError);
			Utility::Log("NetworkManager", "SetupConnectSocket", log);
			return;
		}

		Utility::Log("NetworkManager", "SetupConnectSocket", "Completed");
	}

	int NetworkManager::GetCurrentAcceptedSocket()
	{
		return _connectedSocketMap.size();
	}

	void NetworkManager::ProcessCompletionHandler()
	{
		_isOn = true;
		CustomOverlapped* overlapped = nullptr;
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;
		int errorCode = 0;

		while (_isOn)
		{
			BOOL result = GetQueuedCompletionStatus(_iocp, &bytesTransferred, &completionKey, reinterpret_cast<LPOVERLAPPED*>(&overlapped), INFINITE);

			if (!result)
			{
				errorCode = WSAGetLastError();
				switch (errorCode)
				{
					case WSAECONNRESET:
						HandleDisconnectComplete(completionKey, "WSAECONNRESET");
						break;
					case WSAECONNABORTED:
						HandleDisconnectComplete(completionKey, "WSAECONNABORTED");
						break;
					case WSAENETRESET:
						HandleDisconnectComplete(completionKey, "WSAENETRESET");
						break;
					case WSAETIMEDOUT:
						HandleDisconnectComplete(completionKey, "WSAETIMEDOUT");
						break;
					case WSAENOTCONN:
						HandleDisconnectComplete(completionKey, "WSAENOTCONN");
						break;
					case WSAESHUTDOWN:
						HandleDisconnectComplete(completionKey, "WSAESHUTDOWN");
						break;
					case ERROR_NETNAME_DELETED:
						HandleDisconnectComplete(completionKey, "ERROR_NETNAME_DELETED");
						break;
				}
			}
			else
			{
				switch (overlapped->GetOperation())
				{
					case OperationType::OP_CONNECT:
					{
						HandleConnectComplete(overlapped);
						break;
					}
					case OperationType::OP_ACCEPT:
					{
						HandleAcceptComplete(overlapped); //- AcceptEx() 완료 시 IOCP로 들어오는 이벤트에서 completionKey는 리슨 소켓의 키가 들어온다.
						break;
					}
					case OperationType::OP_RECV:
					{
						HandleReceiveComplete(completionKey, overlapped, bytesTransferred);
						break;
					}
					case OperationType::OP_SEND:
					{
						HandleSendComplete(completionKey, overlapped);
						break;
					}
					case OperationType::OP_DEFAULT:
					{
						Utility::Log("NetworkManager", "ProcessCompletionHandler", "Default Type Message");
						break;
					}
				}
			}

			overlapped->Clear();
			_overlappedQueue.push(std::move(overlapped));
		}
	}

	void NetworkManager::SendRequest(ULONG_PTR& targetSocket, uint32_t& contentType, std::string& stringBuffer, int& bodySize)
	{
		auto finder = _connectedSocketMap.find(targetSocket);
		if (finder == _connectedSocketMap.end())
		{
			Utility::Log("NetworkManager", "SendRequest", "Socket Not Find");
			return;
		}

		auto socket = finder->second;

		auto newOverlappedPtr = _overlappedQueue.pop();
		newOverlappedPtr->Clear();

		auto responseBodySize = htonl(bodySize);
		auto responseContentType = htonl(contentType);
		MessageHeader messageHeader(_senderType, responseBodySize, responseContentType);

		newOverlappedPtr->SendSetting(socket, messageHeader, stringBuffer.c_str(), bodySize);

		DWORD flags = 0;
		int result = WSASend(*socket, newOverlappedPtr->Wsabuf, 2, nullptr, flags, &*newOverlappedPtr, nullptr);
		int errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			std::string log = "WSASend 실패! 오류 코드: " + std::to_string(errorCode);
			Utility::Log("NetworkManager", "SendRequest", log);
			return;
		}

		Utility::Log("NetworkManager", "SendRequest", "클라이언트 WSASend 호출");
	}

	bool NetworkManager::CallReceiveReady(SOCKET* targetSocket)
	{
		int errorCode;
		int errorCodeSize = sizeof(errorCode);
		getsockopt(*targetSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			std::string log = "Getsockopt Fail : " + std::to_string(errorCode);
			Utility::Log("NetworkManager", "ReceiveReady", log);
			return false;
		}

		CustomOverlapped* targetOverlapped = _overlappedQueue.pop();
		targetOverlapped->Clear();
		targetOverlapped->ReceiveSetting(targetSocket);

		DWORD flags = 0;
		int result = WSARecv(*targetSocket, targetOverlapped->Wsabuf, 2, nullptr, &flags, &*targetOverlapped, nullptr);

		errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
		{
			std::string log = "WSARecv 실패! 오류 코드: " + std::to_string(errorCode);
			Utility::Log("NetworkManager", "CallReceiveReady", log);
			return false;
		}
		else
		{
			std::string log = " 클라이언트 WSARecv 호출";
			Utility::Log("NetworkManager", "CallReceiveReady", log);
			return true;
		}
	}

	bool NetworkManager::ProcessDisconnect(SOCKET* targetSocket)
	{
		if (targetSocket == nullptr || *targetSocket == INVALID_SOCKET)
			return false;

		if (shutdown(*targetSocket, SD_BOTH) == SOCKET_ERROR) 
		{
			int err = WSAGetLastError();
			Utility::Log("ProcessDisconnect", "Shutdown 실패", std::to_string(err));
		}

		closesocket(*targetSocket);
		*targetSocket = INVALID_SOCKET;

		delete targetSocket;
		return true;
	}

	void NetworkManager::HandleAcceptComplete(CustomOverlapped* overlapped)
	{
		SOCKET* targetSocket = overlapped->GetSocketPtr();
		ULONG_PTR completionKey = (ULONG_PTR)targetSocket;
		_connectedSocketMap.insert(std::make_pair(completionKey, targetSocket));

		_iocpCallback(completionKey, overlapped);

		bool result = CallReceiveReady(targetSocket);
		std::string feedback = (result ? "Success" : "Fail");
		Utility::Log("NetworkManager", "HandleAcceptComplete", "Success And ReceiveReady " + feedback);
	}

	void NetworkManager::HandleConnectComplete(CustomOverlapped* overlapped)
	{
		SOCKET* targetSocket = overlapped->GetSocketPtr();
		ULONG_PTR completionKey = (ULONG_PTR)targetSocket;
		_connectedSocketMap.insert(std::make_pair(completionKey, targetSocket));

		_iocpCallback(completionKey, overlapped);

		bool result = CallReceiveReady(targetSocket);
		std::string feedback = (result ? "Success" : "Fail");
		Utility::Log("NetworkManager", "HandleConnectComplete", "Success And ReceiveReady " + feedback);
	}

	void NetworkManager::HandleReceiveComplete(ULONG_PTR completionKey, CustomOverlapped* overlapped, DWORD bytes)
	{
		if (bytes <= 0)
		{
			Utility::Log("NetworkManager", "HandleReceiveComplete", "Recv 0 bytes ");
			HandleDisconnectComplete(completionKey, "WSARecv 0 bytes");
			return;
		}

		SOCKET* targetSokcet = overlapped->GetSocketPtr();

		_iocpCallback(completionKey, overlapped);

		bool result = CallReceiveReady(targetSokcet);
		std::string feedback = (result ? "Success" : "Fail");
		Utility::Log("NetworkManager", "HandleReceiveComplete", "Success And ReceiveReady " + feedback);
	}

	void NetworkManager::HandleSendComplete(ULONG_PTR completionKey, CustomOverlapped* overlapped)
	{
		_iocpCallback(completionKey, overlapped);
		Utility::Log("NetworkManager", "HandleSendComplete", "메세지 송신 완료");
	}

	void NetworkManager::HandleDisconnectComplete(ULONG_PTR key, std::string error)
	{
		auto finder = _connectedSocketMap.find(key);
		if (finder == _connectedSocketMap.end())
		{
			Utility::Log("NetworkManager", "HandleDisconnectComplete", "등록되지 않은 소켓Key");
			return;
		}

		SOCKET* targetSokcet = finder->second;
		bool result = ProcessDisconnect(targetSokcet);
		if (result)
		{
			_connectedSocketMap.unsafe_erase(key);
		}

		std::string feedback = (result ? "Success" : "Fail");
		std::string log = "Disconnect ? " + feedback + " Error Detail : " + error;
		Utility::Log("NetworkManager", "HandleDisconnectComplete", log);

		_disconnectCallback(key);
	}
}
