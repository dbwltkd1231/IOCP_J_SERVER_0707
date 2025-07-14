#include "LobbyHub.h"
#include "../Utility/Debug.h"

namespace LobbyServer
{
	LobbyHub::LobbyHub()
	{
		isOn = false;
		_controlServerCompletionKey = 0;
	}

	LobbyHub::~LobbyHub()
	{
		isOn = false;
		_controlServerCompletionKey = 0;
	}

	void LobbyHub::Construct(std::string lobbyKey, int lobbyPort, int iocpThreadCount, int preparedSocketMax, int acceptexSocketMax, int overlappedQueueMax, int packetQueueCapacity, int lobbyCapacity)
	{
		_lobbyKey = lobbyKey;
		_lobbyPort = lobbyPort;
		_icopThreadCount = iocpThreadCount;
		_preparedSocketMax = preparedSocketMax;
		_accpetexSocketMax = acceptexSocketMax;
		_overlappedQueueMax = overlappedQueueMax;
		_packetQueueCapacity = packetQueueCapacity;

		_lobbyMonitor.Construct(lobbyCapacity);
		_jobQueue.Construct(overlappedQueueMax);//TODO

		_networkManager.Construct
		(
			Network::SenderType::LOBBY_SERVER,
			overlappedQueueMax,
			[this](ULONG_PTR completionKey, Network::CustomOverlapped* overlapped) {this->ProcessIocp(completionKey, overlapped);},
			[this](ULONG_PTR completionKey) {this->ProcessDisconnect(completionKey);}
		);

		_networkManager.SetupListenSocket(_lobbyPort, _preparedSocketMax, _icopThreadCount);

		for (int i = 0;i < _preparedSocketMax;++i)
		{
			_networkManager.PrepareAcceptSocket(Network::SenderType::CLIENT);
		}

		_packetQueue.Construct(_packetQueueCapacity);

		isOn = true;

		std::string log =
			"LobbyKey: " + _lobbyKey +
			", LobbyPort: " + std::to_string(_lobbyPort) +
			", IOCPThreadCount: " + std::to_string(_icopThreadCount) +
			", PreparedSocketMax: " + std::to_string(_preparedSocketMax) +
			", AcceptExSocketMax: " + std::to_string(_accpetexSocketMax) +
			", OverlappedQueueMax: " + std::to_string(_overlappedQueueMax) +
			", PacketQueueCapacity: " + std::to_string(_packetQueueCapacity) +
			", LobbyCapacity: " + std::to_string(lobbyCapacity);

		Utility::Log("LobbyHub", "Construct", log);
	}

	void LobbyHub::InitializeSubThread(int receiveThreadCount, int jobThreadCount)
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

		Utility::Log("LobbyHub", "InitializeSubThread", log);
	}

	void LobbyHub::Start()
	{
		Utility::Log("LobbyHub", "Start", (isOn ? "On" : "OFF"));

		while (isOn)
		{

		}
	}

	void LobbyHub::ControlSeverInfoSetting(std::string controlServerIp, int controlServerPort)
	{
		_controlServerIp = controlServerIp;
		_controlServerPort = controlServerPort;
	}

	void LobbyHub::ConnectControlServer()
	{
		_networkManager.SetupConnectSocket(_controlServerIp, _controlServerPort, Network::SenderType::CONTROL_SERVER);
	}

	// Hub의 코드가 계속 늘어나긴할텐데... 일단 해보고 나중에판단하는것으로..
	void LobbyHub::ProcessIocp(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped)
	{
		Network::OperationType operationType = overlapped->GetOperation();
		Network::SenderType senderType = Network::SenderType::DEFAULT;

		switch (operationType)
		{
			case Network::OperationType::OP_ACCEPT:
			{
				senderType = overlapped->GetSenderType();

				if (senderType == Network::SenderType::CLIENT)
				{
					Utility::Log("LobbyHub", "ProcessIocp", "Client Accept");
					_lobbyMonitor.RegisterLobbyUser();

					std::string key = _lobbyKey;
					int current = _lobbyMonitor.GetCurrentUser();
					int remain = _lobbyMonitor.GetRemainCapacity();
					bool active = isOn;
					auto job = std::make_shared<Protocol::JOB_NOTICE_LOBBYINFO>(
						completionKey,
						key,
						current,
						remain,
						active
					);

					_jobQueue.push(std::move(job));
					_jobThreadConditionValue.notify_one();
					Utility::Log("LobbyHub", "ProcessIocp", "Control Server Connect");
				}

				break;
			}

			case Network::OperationType::OP_CONNECT:
			{
				senderType = overlapped->GetSenderType();

				if (senderType == Network::SenderType::CONTROL_SERVER)
				{
					_controlServerCompletionKey = completionKey;

					std::string key = _lobbyKey;
					int port = _lobbyPort;
					int capacity = _lobbyMonitor.GetCapacity();
					bool active = isOn;
					auto job = std::make_shared<Protocol::JOB_NOTICE_LOBBYREADY>(
						_controlServerCompletionKey,
						key,
						port,
						capacity,
						active
					);

					_jobQueue.push(std::move(job));
					_jobThreadConditionValue.notify_one();
					Utility::Log("LobbyHub", "ProcessIocp", "Control Server Connect");
				}

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

	void LobbyHub::ProcessDisconnect(ULONG_PTR completionKey)
	{

	}

	void LobbyHub::RequestSendMessage(Protocol::JobOutput output)
	{
		_networkManager.SendRequest(output.SocketPtr, output.ContentsType, output.Buffer, output.BodySize);
		Utility::Log("LobbyHub","RequestSendMessage","Complete");
	}

	// 멀티 쓰레드
	void LobbyHub::JobThread()
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
	void LobbyHub::ReceiveThread()
	{
		while (isOn)
		{
			if (_packetQueue.empty())
				continue;

			auto packet = _packetQueue.pop();
			auto messageType = static_cast<protocol::MESSAGETYPE>(packet->ContentsType);
			Network::SenderType sender = static_cast<Network::SenderType>(packet->SenderType);

			/*
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
				bool active = notice->active();

				auto job = std::make_shared<Protocol::JOB_NOTICE_LOBBYREADY>(
					packet->CompletionKey,
					lobbyKey,
					port,
					active,
					[this](std::string& key, int& port, bool active) {this->_lobbyManager.SaveLobbyInfo(key, port, active);}
				);

				_jobQueue.push(std::move(job));

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
			*/
			_jobThreadConditionValue.notify_one();
		}
	}



}

