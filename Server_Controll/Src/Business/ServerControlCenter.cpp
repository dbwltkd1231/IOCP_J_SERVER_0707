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
			std::cout << "���� ��ɾ� �Է�: ";
			std::getline(std::cin, command);

			if (command == "Exit")
			{
				_isServerOn = false;
			}
		}
	}

	//�̺�Ʈ�����嵵 ������ ������. �̰Ը���?

	//�κ���� ������ : �κ���� -> notice�� �غ񳡳��κ� ������ �׶� map�������� ��ü�� ����� ����.
	//��ũ��Ű�� ó��, �׸��� �ٲ� ���������� �κ񼭹��� ���������ش�.
	//��ũ��Ű�� ��ȿ�ð����Խ�ų��.
	//��ũ��Ű������ ������ ���⼭? �ƴϴ� ��ũ��Ű�� �̹̺�����µ� �����ε�.

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
					//// ���⼭ Ǯ�����ϸ� �޸𸮸� ���λ����ؾ������� �������߻����� -> Ǯ���� ���� �ݹ��� �ް�, ���⼭�� �״�� ��������.
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