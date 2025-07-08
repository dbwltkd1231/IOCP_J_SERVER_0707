#pragma once
#include "LobbyServer.h"
#include "../Utility/Debug.h"

namespace Lobby
{
	LobbyServer::LobbyServer()
	{
		_isServerOn = false;
	}

	LobbyServer::~LobbyServer()
	{
		_isServerOn = false;
	}

	void LobbyServer::Initialize(std::string serverKey, int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax, std::string targetServerIp, int targetServerPort)
	{
		_serverKey = serverKey;

		_messageCallback = std::function<void(ULONG_PTR, Network::OperationType, Network::CustomOverlapped*)>
			(
				[this]
				(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped)
				{
					this->EnqueueJob(socketPtr, operationType, customOverlapped);
				}
			);

		_networkManager.Construct(serverPort, threadCount, preCreateSocketCount, acceptSocketMax, overlappedQueueMax, _messageCallback);

		std::string log =
			" - ServerPort : " + std::to_string(serverPort) + " " +
			" - ThreadCount : " + std::to_string(threadCount) + " " +
			" - PreCreateSocketCount : " + std::to_string(preCreateSocketCount) + " " +
			" - AcceptSocketMax : " + std::to_string(acceptSocketMax) + " " +
			" - OverlappedQueueMax : " + std::to_string(overlappedQueueMax);
		Utility::Log("LobbyServer", "Server", "Initialize", log);

		_networkManager.ConnectToControlServer(targetServerIp, targetServerPort);
	}

	void LobbyServer::EnqueueJob(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped)
	{
		std::shared_ptr<Lobby::Job> newJob = std::make_shared<Lobby::Job>(socketPtr, operationType, customOverlapped);

		_jobQueue.push(std::move(newJob));
	}

	void LobbyServer::MainProcess()
	{
		_isServerOn = true;

		for (int i = 0;i < 1;++i)//jobthreadcount
		{
			std::thread jobThread([this]() { this->JobThread(); });
			jobThread.detach();
		}

		while (_isServerOn)
		{
			
		}
	}

	void LobbyServer::JobThread()
	{
		while (_isServerOn)
		{
			if (_jobQueue.size() < 1)
				continue;

			std::shared_ptr<Lobby::Job> job = _jobQueue.pop();

			Network::OperationType operationType = job->OperationType;
			switch (operationType)
			{
				case Network::OperationType::OP_CONNECT:
				{
					Utility::Log("Server_Lobby", "LobbyServer", "JobThread", "Connect");
					break;
				}
				case Network::OperationType::OP_ACCEPT:
				{
					Utility::Log("Server_Lobby", "LobbyServer", "JobThread", "Accept");
					break;
				}
				case Network::OperationType::OP_RECV:
				{
					Utility::Log("Server_Lobby", "LobbyServer", "JobThread", "Recv");
					//// 여기서 풀링을하면 메모리를 새로생성해야함으로 오버헤드발생가능 -> 풀링은 따로 콜백을 받고, 여기서는 그대로 전달하자.
					//MessageHeader* receivedHeader = reinterpret_cast<MessageHeader*>(overlapped->Wsabuf[0].buf);
					//int requestBodySize = ntohl(receivedHeader->BodySize);
					//uint32_t requestContentsType = ntohl(receivedHeader->ContentsType);
					//
					//std::string bufferString = std::string(overlapped->Wsabuf[1].buf, requestBodySize);


					break;
				}
				case Network::OperationType::OP_SEND:
				{
					Utility::Log("Server_Lobby", "LobbyServer", "JobThread", "Send");
					break;
				}
				case Network::OperationType::OP_DISCONNECT:
				{
					Utility::Log("Server_Lobby", "LobbyServer", "JobThread", "Disconnect");
				}
				case Network::OperationType::OP_DEFAULT:
				{
					Utility::Log("Server_Lobby", "LobbyServer", "JobThread", "Default");
					break;
				}
			}

			_networkManager.ReturnOverlapped(job->CustomOverlapped);
		}
	}
}