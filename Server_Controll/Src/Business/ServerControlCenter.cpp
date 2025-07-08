#include <iostream>
#include "Business/ServerControlCenter.h"
#include "../Utility/Debug.h"

namespace Business
{
	ServerControlCenter::ServerControlCenter()
	{
		_isServerOn = false;
	}

	ServerControlCenter::~ServerControlCenter()
	{
		_isServerOn = false;
	}

	void ServerControlCenter::Initialize(int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax)
	{
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
			" - ServerPort          : " + std::to_string(serverPort) + " " +
			" - ThreadCount         : " + std::to_string(threadCount) + " " +
			" - PreCreateSocketCount: " + std::to_string(preCreateSocketCount) + " " +
			" - AcceptSocketMax     : " + std::to_string(acceptSocketMax) + " " +
			" - OverlappedQueueMax  : " + std::to_string(overlappedQueueMax);
		Utility::Log("Server_Control", "Server", "Initialize", log);
	}

	void ServerControlCenter::EnqueueJob(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped)
	{
		std::shared_ptr<Business::Job> newJob = std::make_shared<Business::Job>(socketPtr, operationType, customOverlapped);

		_jobQueue.push(std::move(newJob));
	}

	void ServerControlCenter::MainProcess()
	{
		_isServerOn = true;

		for (int i = 0;i < 1;++i)//jobthreadcount
		{
			std::thread jobThread([this]() { this->JobThread(); });
			jobThread.detach();
		}

		std::string command = "";
		while (_isServerOn)
		{
			std::cout << "서버 명령어 입력: ";
			std::getline(std::cin, command);

			if (command == "Exit")
			{
				_isServerOn = false;
			}
		}
	}

	//이벤트쓰레드도 있으면 좋을듯. 이게메인?

	//로비생성 시퀀스 : 로비생성 -> notice를 준비끝난로비가 보내면 그때 map같은곳에 객체로 만들어 저장.
	//시크릿키는 처음, 그리고 바뀔때 인증서버와 로비서버에 각각보내준다.
	//시크릿키에 유효시간포함시킬것.
	//시크릿키에대한 검증도 여기서? 아니다 시크릿키를 이미보내줬는데 굳이인듯.

	void ServerControlCenter::JobThread()
	{
		while (_isServerOn)
		{
			if (_jobQueue.size() < 1)
				continue;

			std::shared_ptr<Business::Job> job = _jobQueue.pop();

			Network::OperationType operationType = job->OperationType;
			switch (operationType)
			{
				case Network::OperationType::OP_ACCEPT:
				{
					break;
				}
				case Network::OperationType::OP_RECV:
				{
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

					break;
				}
				case Network::OperationType::OP_DISCONNECT:
				{

				}
				case Network::OperationType::OP_DEFAULT:
				{

					break;
				}
			}

			_networkManager.ReturnOverlapped(job->CustomOverlapped);
		}
	}
}