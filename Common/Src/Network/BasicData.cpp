#include "../Network/BasicData.h"

#define BUFFER_SIZE_MAX 1024

namespace Network
{
	CustomOverlapped::CustomOverlapped()
	{
		_operationType = OperationType::OP_DEFAULT;
		_senderType = Network::SenderType::DEFAULT;
		this->hEvent = NULL;
		_socket = nullptr;

		Wsabuf[0].buf = new char[sizeof(MessageHeader)];
		Wsabuf[0].len = sizeof(MessageHeader);

		Wsabuf[1].buf = new char[BUFFER_SIZE_MAX];
		Wsabuf[1].len = BUFFER_SIZE_MAX;
	}

	CustomOverlapped::~CustomOverlapped()
	{
		_operationType = OperationType::OP_DEFAULT;
		_senderType = Network::SenderType::DEFAULT;
		this->hEvent = NULL;

		delete[] Wsabuf[0].buf;
		Wsabuf[0].len = 0;

		delete[] Wsabuf[1].buf;
		Wsabuf[1].len = 0;
	}

	CustomOverlapped::CustomOverlapped(const CustomOverlapped& other)
	{
		this->hEvent = other.hEvent;
		_operationType = other._operationType;

		if (other.Wsabuf[0].len > 0)
		{
			Wsabuf[0].buf = other.Wsabuf[0].buf;
			Wsabuf[0].len = other.Wsabuf[0].len;
		}
		else
		{
			Wsabuf[0].buf = nullptr;
			Wsabuf[0].len = 0;
		}

		if (other.Wsabuf[1].len > 0)
		{
			Wsabuf[1].buf = other.Wsabuf[1].buf;
			Wsabuf[1].len = other.Wsabuf[1].len;
		}
		else
		{
			Wsabuf[1].buf = nullptr;
			Wsabuf[1].len = 0;
		}

	}

	void CustomOverlapped::ConnectSetting(SOCKET* socket, Network::SenderType senderType)
	{
		_operationType = Network::OperationType::OP_CONNECT;
		_socket = socket;
		_senderType = senderType;
	}

	void CustomOverlapped::AcceptSetting(SOCKET* socket, Network::SenderType senderType)
	{
		_operationType = Network::OperationType::OP_ACCEPT;
		_socket = socket;
		_senderType = senderType;
	}

	void CustomOverlapped::SendSetting(SOCKET* socket, const MessageHeader& headerData, const char* bodyBuffer, ULONG bodyLen)
	{
		_socket = socket;
		_operationType = Network::OperationType::OP_SEND;

		memset(Wsabuf[0].buf, 0, Wsabuf[0].len);
		memset(Wsabuf[1].buf, 0, bodyLen);

		auto header = new MessageHeader(headerData);
		Wsabuf[0].buf = reinterpret_cast<char*>(header);
		Wsabuf[0].len = sizeof(MessageHeader);

		memcpy(Wsabuf[1].buf, bodyBuffer, bodyLen);
		Wsabuf[1].len = bodyLen;
	}

	void CustomOverlapped::ReceiveSetting(SOCKET* socket)
	{
		_operationType = Network::OperationType::OP_RECV;
		_socket = socket;
	}

	OperationType CustomOverlapped::GetOperation() const
	{
		return _operationType;
	}

	Network::SenderType CustomOverlapped::GetSenderType() const
	{
		return _senderType;
	}

	SOCKET* CustomOverlapped::GetSocketPtr() const
	{
		return _socket;
	}

	void CustomOverlapped::Clear()
	{
		memset(Wsabuf[0].buf, 0, sizeof(MessageHeader));
		Wsabuf[0].len = sizeof(MessageHeader);
		memset(Wsabuf[1].buf, 0, Wsabuf[1].len);
		Wsabuf[1].len = BUFFER_SIZE_MAX;

		_operationType = OperationType::OP_DEFAULT;
		_senderType = Network::SenderType::DEFAULT;
		_socket = nullptr;
		this->hEvent = NULL;
	}
}

