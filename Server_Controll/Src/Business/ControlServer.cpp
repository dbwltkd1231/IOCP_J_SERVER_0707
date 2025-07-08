#pragma once
#include "Business/ControlServer.h"
#include "../Utility/Debug.h"

namespace Control
{
	ControlServer::ControlServer()
	{
		_isServerOn = false;
	}

	ControlServer::~ControlServer()
	{
		_isServerOn = false;
	}

	void ControlServer::Initialize(std::string serverIp, int serverPort, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax)
	{
		_ip = serverIp;
		_port = serverPort;

		_messageCallback = std::function<void(ULONG_PTR, Network::OperationType, Network::CustomOverlapped*)>
			(
				[this]
				(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped)
				{
					this->EnqueueJob(socketPtr, operationType, customOverlapped);
				}
			);

		_networkManager.Construct(_port, threadCount, preCreateSocketCount, acceptSocketMax, overlappedQueueMax, _messageCallback);

		std::string log =
			" - ServerPort : " + std::to_string(_port) + " " +
			" - ThreadCount : " + std::to_string(threadCount) + " " +
			" - PreCreateSocketCount : " + std::to_string(preCreateSocketCount) + " " +
			" - AcceptSocketMax : " + std::to_string(acceptSocketMax) + " " +
			" - OverlappedQueueMax : " + std::to_string(overlappedQueueMax);
		Utility::Log("Server_Control", "Server", "Initialize", log);
	}

	void ControlServer::LobbySeverInfoSetting(std::vector<std::string> lobbyKeys, std::vector<int> lobbyPorts, int threadCount, int preCreateSocketCount, int acceptSocketMax, int overlappedQueueMax)
	{
		_lobbyKeys = lobbyKeys;
		_lobbyPorts = lobbyPorts;
		_lobbyThreadCount = threadCount;
		_lobbyPreCreateSocketCount = preCreateSocketCount;
		_lobbyAcceptSocketMax = acceptSocketMax;
		_lobbyOverlappedQueueMax = overlappedQueueMax;
	}

	void ControlServer::EnqueueJob(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped)
	{
		std::shared_ptr<Control::Job> newJob = std::make_shared<Control::Job>(socketPtr, operationType, customOverlapped);

		_jobQueue.push(std::move(newJob));
	}

	void ControlServer::MainProcess()
	{
		_isServerOn = true;

		for (int i = 0;i < 1;++i)//jobthreadcount
		{
			std::thread jobThread([this]() { this->JobThread(); });
			jobThread.detach();
		}

		//ExecuteLobbyServer();

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

	void ControlServer::JobThread()
	{
		while (_isServerOn)
		{
			if (_jobQueue.size() < 1)
				continue;

			std::shared_ptr<Control::Job> job = _jobQueue.pop();

			Network::OperationType operationType = job->OperationType;
			switch (operationType)
			{
				case Network::OperationType::OP_ACCEPT:
				{
					Utility::Log("Server_Control", "Server", "JobThread", "Accept");
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
					Utility::Log("Server_Control", "Server", "JobThread", "Recv");
	
					break;
				}
				case Network::OperationType::OP_SEND:
				{
					Utility::Log("Server_Control", "Server", "JobThread", "Send");
					break;
				}
				case Network::OperationType::OP_DISCONNECT:
				{
					Utility::Log("Server_Control", "Server", "JobThread", "Disconnect");
				}
				case Network::OperationType::OP_DEFAULT:
				{
					Utility::Log("Server_Control", "Server", "JobThread", "Default");
					break;
				}
			}

			_networkManager.ReturnOverlapped(job->CustomOverlapped);
		}
	}

	void ControlServer::ExecuteLobbyServer()
	{
		std::string key = _lobbyKeys.front();  
		_lobbyKeys.erase(_lobbyKeys.begin());

		int port = _lobbyPorts.front();
		_lobbyPorts.erase(_lobbyPorts.begin());

		STARTUPINFOA si = { sizeof(si) };
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOW;

		PROCESS_INFORMATION pi;

		// ������ EXE ��ο� ����
		std::string commandLine = "Server_Lobby.exe " + key + " " +
			std::to_string(port) + " " +
			std::to_string(_lobbyThreadCount) + " " +
			std::to_string(_lobbyPreCreateSocketCount) + " " +
			std::to_string(_lobbyAcceptSocketMax) + " " +
			std::to_string(_lobbyOverlappedQueueMax) + " " +
			_ip + " " +
			std::to_string(_port);

		// ���ڿ��� ���� �����ϰ� ����� ���� ����
		char cmdLineBuffer[512];
		strcpy_s(cmdLineBuffer, commandLine.c_str());

		BOOL result = CreateProcessA(
			NULL,               // ���ø����̼� �̸�
			cmdLineBuffer,      // ����� (���� ����)
			NULL, NULL,         // ���� �Ӽ�
			FALSE,              // �ڵ� ��� ����
			0,                  // ���� �÷���
			NULL,               // ȯ�� ����
			NULL,               // ���� ���͸�
			&si, &pi            // ���μ��� ����
		);

		if (result)
		{
			std::cout << "�ٸ� ������Ʈ ���� ����!\n";
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			std::cerr << "���� ����: " << GetLastError() << "\n";
		}


	}
}