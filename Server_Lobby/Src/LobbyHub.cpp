#include "LobbyHub.h"
#include "../Utility/Debug.h"

namespace LobbyServer
{
	LobbyHub::LobbyHub()
	{
		isOn = false;
	}

	LobbyHub::~LobbyHub()
	{
		isOn = false;
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
		_lobbyCapacity = lobbyCapacity;

		_networkManager.Construct
		(overlappedQueueMax,
			[this](ULONG_PTR key, Network::CustomOverlapped* overlapped) {this->ReceiveMessage(key, overlapped);}
		);

		_networkManager.SetupListenSocket(_lobbyPort, _preparedSocketMax, _icopThreadCount);

		for (int i = 0;i < _preparedSocketMax;++i)
		{
			_networkManager.PrepareAcceptSocket();
		}

		_packetQueue.Construct(_packetQueueCapacity);

		isOn = true;
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
	}

	void LobbyHub::Start()
	{
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
		_networkManager.SetupConnectSocket(_controlServerIp, _controlServerPort);
	}

	void LobbyHub::ReceiveMessage(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped)
	{
		Network::MessageHeader* receivedHeader = reinterpret_cast<Network::MessageHeader*>(overlapped->Wsabuf[0].buf);
		std::shared_ptr<Network::Packet> packet = std::make_shared< Network::Packet>();

		int bodySize = ntohl(receivedHeader->BodySize);

		packet->CompletionKey = completionKey;
		packet->ContentsType = ntohl(receivedHeader->ContentsType);
		packet->Buffer = std::string(overlapped->Wsabuf[1].buf, bodySize);// string 값복사 전달을 통해 buffer초기화시에도 안정성을 강화.

		_packetQueue.push(std::move(packet));
	}

	void LobbyHub::RequestSendMessage(Protocol::JobOutput output)
	{
		_networkManager.SendRequest(output.SocketPtr, output.ContentsType, output.Buffer, output.BodySize);
	}

	// 여러 쓰레드
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

