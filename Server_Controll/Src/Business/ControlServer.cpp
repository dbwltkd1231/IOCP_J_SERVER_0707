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
					//// 여기서 풀링을하면 메모리를 새로생성해야함으로 오버헤드발생가능 -> 풀링은 따로 콜백을 받고, 여기서는 그대로 전달하자.
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

		// 실행할 EXE 경로와 인자
		std::string commandLine = "Server_Lobby.exe " + key + " " +
			std::to_string(port) + " " +
			std::to_string(_lobbyThreadCount) + " " +
			std::to_string(_lobbyPreCreateSocketCount) + " " +
			std::to_string(_lobbyAcceptSocketMax) + " " +
			std::to_string(_lobbyOverlappedQueueMax) + " " +
			_ip + " " +
			std::to_string(_port);

		// 문자열을 수정 가능하게 만들기 위해 복사
		char cmdLineBuffer[512];
		strcpy_s(cmdLineBuffer, commandLine.c_str());

		BOOL result = CreateProcessA(
			NULL,               // 애플리케이션 이름
			cmdLineBuffer,      // 명령줄 (인자 포함)
			NULL, NULL,         // 보안 속성
			FALSE,              // 핸들 상속 여부
			0,                  // 생성 플래그
			NULL,               // 환경 변수
			NULL,               // 현재 디렉터리
			&si, &pi            // 프로세스 정보
		);

		if (result)
		{
			std::cout << "다른 프로젝트 실행 성공!\n";
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			std::cerr << "실행 실패: " << GetLastError() << "\n";
		}


	}
}