#include "Hub.h"
#include "../Utility/Debug.h"

namespace ControlServer
{
	Hub::Hub()
	{
		_currentLobbyCount = 0;
		isOn = false;
	}

	Hub::~Hub()
	{
		isOn = false;
	}

	void Hub::Construct(std::string ip, int serverPort, int redisPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity, int packetQueueCapacity)
	{
		_controlServerIp = ip;
		_controlServerPort = serverPort;
		_acceptedCapacity = acceptedCapacity;

		_networkManager.Construct
		(	
			Network::SenderType::CONTROL_SERVER,
			overlappedQueueMax,
			[this](ULONG_PTR completionKey, Network::CustomOverlapped* overlapped) {this->ProcessIocp(completionKey, overlapped);},
			[this](ULONG_PTR completionKey) {this->ProcessDisconnect(completionKey);}
		);

		_networkManager.SetupListenSocket(serverPort, prepareSocketMax, iocpThreadCount);

		for (int i = 0;i < prepareSocketMax;++i)
		{
			_networkManager.PrepareAcceptSocket(Network::SenderType::DEFAULT);
		}

		_packetQueue.Construct(packetQueueCapacity);
		_jobQueue.Construct(overlappedQueueMax);//TODO

		std::string log =
			"IP: " + ip +
			", ControlServerPort: " + std::to_string(serverPort) +
			", IOCPThreadCount: " + std::to_string(prepareSocketMax) +
			", IocpThreadCount: " + std::to_string(iocpThreadCount) +
			", OverlappedQueueMax: " + std::to_string(overlappedQueueMax) +
			", AcceptedCapacity: " + std::to_string(acceptedCapacity) +
			", PacketQueueCapacity: " + std::to_string(packetQueueCapacity);


		_redisWorker = new Database::RedisWorker();
		_redisWorker->Construct(ip, redisPort);
		_lobbyManager.Construct(_redisWorker, 60);

		isOn = true;

		Utility::Log("Hub", "Construct", log);

	}

	void Hub::InitializeSubThread(int receiveThreadCount, int jobThreadCount)
	{
		for (int i = 0;i < receiveThreadCount;++i)
		{
			std::thread receiveThread([this]() { this->ReceiveThread(); });
			receiveThread.detach();
		}

		for (int i = 0;i < jobThreadCount;++i)
		{
			std::thread jobThread([this]() { this->JobThread(); });
			jobThread.detach();
		}

		std::thread networkThread([this]() { this->_networkManager.ProcessCompletionHandler(); });
		networkThread.detach();

		std::string log =
			"ReceiveThreadCount: " + std::to_string(receiveThreadCount) +
			", JobThreadCount: " + std::to_string(jobThreadCount) +
			" Completed";

		Utility::Log("Hub", "InitializeSubThread", log);
	}

	void Hub::Start()
	{
		Utility::Log("Hub", "Start", (isOn? "On" : "OFF"));

		while (isOn)
		{

		}
	}

	void Hub::LobbySeverInfoSetting(std::vector<std::string> lobbyKeys, std::vector<int> lobbyPorts, int lobbyThreadCount, int lobbyPreCreateSocketCount, int lobbyAcceptSocketMax, int lobbyOverlappedQueueSizemax)
	{
		_lobbyKeys = lobbyKeys;
		_lobbyPorts = lobbyPorts;
		_lobbyThreadCount = lobbyThreadCount;
		_lobbyPreCreateSocketCount = lobbyPreCreateSocketCount;
		_lobbyAcceptSocketMax = lobbyAcceptSocketMax;
		_lobbyOverlappedQueueSizemax = lobbyOverlappedQueueSizemax;

		Utility::Log("Hub", "LobbySeverInfoSetting", "로비서버정보 저장 완료");
	}

	void Hub::LobbyServerStart(int count)
	{
		if (_lobbyKeys.size() < _currentLobbyCount + count)
		{
			Utility::Log("Hub", "LobbyServerStart", "저장된 로비 키 목록보다 많은 수 호출");
			return;
		}

		if (_lobbyKeys.size() != _lobbyPorts.size())
		{
			Utility::Log("Hub", "LobbyServerStart", "저장된 로비 키 목록과 포트번호 목록의 숫자가 일치하지 않음");
			return;
		}

		std::string lobbyKey = "";
		int port = 0;
		for (int i = _currentLobbyCount;i < count;++i)
		{
			lobbyKey = _lobbyKeys[i];
			port = _lobbyPorts[i];

			std::string lobbyExecutable = "Server_Lobby.exe";

			std::string commandLine = lobbyExecutable + " "
				+ lobbyKey + " "
				+ std::to_string(port) + " "
				+ std::to_string(_lobbyThreadCount) + " "
				+ std::to_string(_lobbyPreCreateSocketCount) + " "
				+ std::to_string(_lobbyAcceptSocketMax) + " "
				+ std::to_string(_lobbyOverlappedQueueSizemax) + " "
				+ _controlServerIp + " "
				+ std::to_string(_controlServerPort) + " "
				+ std::to_string(100);

			STARTUPINFOA si = { sizeof(si) };
			PROCESS_INFORMATION pi;

			BOOL result = CreateProcessA(
				NULL,
				const_cast<char*>(commandLine.c_str()), // 직접 전달된 커맨드라인
				NULL, NULL, FALSE,
				CREATE_NEW_CONSOLE,
				NULL, NULL,
				&si, &pi
			);

			if (!result) 
			{
				DWORD err = GetLastError();
				Utility::Log("Hub", "LobbyServerStart ",std::to_string(err));
			}
			else 
			{
				Utility::Log("Hub", "LobbyServerStart", "Lobby 서버 실행 성공!");
			}

			Sleep(1000);
		}

		_currentLobbyCount += count;//실패해도 실패한대로 넘어간다.
	}

	void Hub::ProcessIocp(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped)
	{
		Network::OperationType operationType = overlapped->GetOperation();

		switch (operationType)
		{
			case Network::OperationType::OP_ACCEPT:
			{
				break;
			}

			case Network::OperationType::OP_CONNECT:
			{
				break;
			}

			case Network::OperationType::OP_RECV:
			{
				Network::MessageHeader* receivedHeader = reinterpret_cast<Network::MessageHeader*>(overlapped->Wsabuf[0].buf);
				std::shared_ptr<Network::Packet> packet = std::make_shared< Network::Packet>();

				int bodySize = ntohl(receivedHeader->BodySize);

				packet->CompletionKey = completionKey;
				packet->SenderType = ntohl(receivedHeader->SenderType);
				packet->ContentsType = ntohl(receivedHeader->ContentsType);
				packet->Buffer = std::string(overlapped->Wsabuf[1].buf, bodySize);// string 값복사 전달을 통해 buffer초기화시에도 안정성을 강화.

				_packetQueue.push(std::move(packet));
				break;
			}

			case Network::OperationType::OP_SEND:
			{
				break;
			}

			case Network::OperationType::OP_DEFAULT:
			{
				break;
			}
		}
	}

	void Hub::ProcessDisconnect(ULONG_PTR completionKey)
	{

	}

	void Hub::RequestSendMessage(Protocol::JobOutput output)
	{
		_networkManager.SendRequest(output.SocketPtr, output.ContentsType, output.Buffer, output.BodySize);
	}

	// 여러 쓰레드
	void Hub::JobThread()
	{
		Protocol::JobOutput output;

		while (isOn)
		{
			std::unique_lock<std::mutex> lock(_lockJobThread);
			_jobThreadConditionValue.wait
			(
				lock, [this]()
				{
					return !this->_jobQueue.empty() || !isOn;
				}
			);

			if (!isOn)
			{
				return;
			}

			while (!_jobQueue.empty())
			{
				//std::shared_ptr< Protocol::Job>job = nullptr;
				//_jobQueue.try_pop(job);
				auto job = _jobQueue.pop();

				job->Execute(output);

				if (output.IsSend)
				{
					RequestSendMessage(output);
				}
			}
			
		}
	}

	//IOCP Worker쓰레드 -> Receive단일쓰레드로 메세지모아서 처리.
	void Hub::ReceiveThread()
	{
		while (isOn)
		{
			if (_packetQueue.empty())
				continue;

			//std::shared_ptr<Network::Packet> packet;
			//_packetQueue.try_pop(packet);

			auto packet = _packetQueue.pop();

			protocol::MESSAGETYPE messageType = static_cast<protocol::MESSAGETYPE>(packet->ContentsType);
			Network::SenderType sender = static_cast<Network::SenderType>(packet->SenderType);

			switch (messageType)
			{
				case protocol::MESSAGETYPE_REQUEST_LOBBYINFO://인증서버가 로비서버정보를 묻는 메세지
				{
					auto request = flatbuffers::GetRoot<protocol::REQUEST_LOBBYINFO>(packet->Buffer.c_str());

					auto job = std::make_shared<Protocol::JOB_REQUEST_LOBBYINFO>(
						packet->CompletionKey,
						[this](std::string& key, int& port, bool success) {this->_lobbyManager.ResponseLobbyInfo(key, port, success);}
						);

					_jobQueue.push(std::move(job));

					break;
				}

				case protocol::MESSAGETYPE_RESPONSE_LOBBYINFO:
				{
					// NO USE
					break;
				}

				case protocol::MESSAGETYPE_NOTICE_LOBBYREADY:
				{
					auto notice = flatbuffers::GetRoot<protocol::NOTICE_LOBBYREADY>(packet->Buffer.c_str());
					std::string lobbyKey = notice->lobby_key()->str();
					int port = notice->port();
					int capacity = notice->capacity();
					bool active = notice->active();
					auto job = std::make_shared<Protocol::JOB_NOTICE_LOBBYREADY>(
						packet->CompletionKey,
						lobbyKey,
						port,
						capacity,
						active,
						[this](std::string& key, int& port, int& capacity, bool active) {this->_lobbyManager.SaveLobbyInfo(key, port, capacity, active);}
					);

					_jobQueue.push(std::move(job));

					Utility::Log("Hub", "MESSAGETYPE_NOTICE_LOBBYREADY", "Recv");

					break;
				}

				case protocol::MESSAGETYPE_NOTICE_LOBBYINFO:
				{
					auto notice = flatbuffers::GetRoot<protocol::NOTICE_LOBBYINFO>(packet->Buffer.c_str());
					std::string lobbyKey = notice->lobby_key()->str();
					int current = notice->current();
					int remain = notice->reamain();
					bool active = notice->active();

					auto job = std::make_shared<Protocol::NOTICE_LOBBYINFO>(
						packet->CompletionKey,
						lobbyKey,
						current,
						remain,
						active,
						[this](std::string& key, int& current, int& remain, bool active) {this->_lobbyManager.UpdateLobbyInfo(key, current, remain, active);}
					);

					break;
				}

				case protocol::MESSAGETYPE_NOTICE_SECRETKEY:
				{

					break;
				}

				case protocol::MESSAGETYPE_NOTICE_RESPONSESPEED:
				{

					break;
				}
			}

			_jobThreadConditionValue.notify_one();
		}
	}
}

