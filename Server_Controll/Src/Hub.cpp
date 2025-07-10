#include "Hub.h"


namespace ControlServer
{
	Hub::Hub()
	{
		isOn = false;
	}

	Hub::~Hub()
	{
		isOn = false;
	}

	void Hub::Construct(int serverPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity, int packetQueueCapacity)
	{
		_acceptedCapacity = acceptedCapacity;

		_networkManager.Construct
		(	overlappedQueueMax,
			[this](ULONG_PTR key, Network::CustomOverlapped* overlapped) {this->ReceiveMessage(key, overlapped);}
		);

		_networkManager.SetupListenSocket(serverPort, prepareSocketMax, iocpThreadCount);

		for (int i = 0;i < prepareSocketMax;++i)
		{
			_networkManager.PrepareAcceptSocket();
		}

		_packetQueue.Construct(packetQueueCapacity);
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
	}

	void Hub::Start()
	{
		isOn = true;
		while (isOn)
		{

		}
	}

	void Hub::ReceiveMessage(ULONG_PTR completionKey, Network::CustomOverlapped* overlapped)
	{
		Network::MessageHeader* receivedHeader = reinterpret_cast<Network::MessageHeader*>(overlapped->Wsabuf[0].buf);
		std::shared_ptr<Network::Packet> packet = std::make_shared< Network::Packet>();

		int bodySize = ntohl(receivedHeader->BodySize);

		packet->CompletionKey = completionKey;
		packet->ContentsType = ntohl(receivedHeader->ContentsType);
		packet->Buffer = std::string(overlapped->Wsabuf[1].buf, bodySize);// string 값복사 전달을 통해 buffer초기화시에도 안정성을 강화.

		_packetQueue.push(std::move(packet));
	}

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
					!this->_jobQueue.empty() || !isOn;
				}
			);

			if (!isOn)
			{
				return;
			}

			std::shared_ptr<Protocol::Job> job = _jobQueue.front();
			_jobQueue.pop();
			lock.unlock();

			job->Execute(output);

		}
	}

	void Hub::ReceiveThread()
	{
		while (isOn)
		{
			if (_packetQueue.empty())
				continue;

			auto packet = _packetQueue.pop();

			auto messageType = static_cast<protocol::MESSAGETYPE>(packet->ContentsType);

			switch (messageType)
			{
				case protocol::MESSAGETYPE_REQUEST_LOBBYINFO://인증서버가 로비서버정보를 묻는 메세지
				{
					auto requestLobbyInfo = flatbuffers::GetRoot<protocol::REQUEST_LOBBYINFO>(packet->Buffer.c_str());
					std::string lobbyKey = requestLobbyInfo->key()->str();

					auto job = std::make_shared<Protocol::JOB_REQUEST_LOBBYINFO>(packet->CompletionKey, lobbyKey);
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

					break;
				}

				case protocol::MESSAGETYPE_NOTICE_LOBBYINFO:
				{

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

