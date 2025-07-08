#include "../Network/BasicData.h"

#define BUFFER_SIZE_MAX 1024

namespace Network
{
	CustomOverlapped::CustomOverlapped()
	{
		_socketPtr = 0;
		_operationType = OperationType::OP_DEFAULT;
		this->hEvent = NULL;

		Wsabuf[0].buf = new char[sizeof(MessageHeader)];
		Wsabuf[0].len = sizeof(MessageHeader);

		Wsabuf[1].buf = new char[BUFFER_SIZE_MAX];
		Wsabuf[1].len = BUFFER_SIZE_MAX;
	}

	CustomOverlapped::~CustomOverlapped()
	{
		_socketPtr = 0;
		_operationType = OperationType::OP_DEFAULT;
		this->hEvent = NULL;

		delete[] Wsabuf[0].buf;
		Wsabuf[0].len = 0;

		delete[] Wsabuf[1].buf;
		Wsabuf[1].len = 0;
	}

	CustomOverlapped::CustomOverlapped(const CustomOverlapped& other)
	{
		_socketPtr = other._socketPtr;
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

	void CustomOverlapped::AcceptSetting(ULONG_PTR socketPtr)
	{
		_operationType = Network::OperationType::OP_ACCEPT;
		_socketPtr = socketPtr;
	}

	void CustomOverlapped::SendSetting(const MessageHeader& headerData, const char* bodyBuffer, ULONG bodyLen)
	{
		_operationType = Network::OperationType::OP_SEND;

		memset(Wsabuf[0].buf, 0, Wsabuf[0].len);
		memset(Wsabuf[1].buf, 0, bodyLen);

		auto header = new MessageHeader(headerData);
		Wsabuf[0].buf = reinterpret_cast<char*>(header);
		Wsabuf[0].len = sizeof(MessageHeader);

		memcpy(Wsabuf[1].buf, bodyBuffer, bodyLen);
		Wsabuf[1].len = bodyLen;
	}

	void CustomOverlapped::ReceiveSetting()
	{
		_operationType = Network::OperationType::OP_RECV;
	}

	OperationType CustomOverlapped::GetOperation() const
	{
		return _operationType;
	}

	void CustomOverlapped::Clear()
	{
		memset(Wsabuf[0].buf, 0, sizeof(MessageHeader));
		Wsabuf[0].len = sizeof(MessageHeader);
		memset(Wsabuf[1].buf, 0, Wsabuf[1].len);
		Wsabuf[1].len = BUFFER_SIZE_MAX;

		_operationType = OperationType::OP_DEFAULT;
		_socketPtr = 0;
		this->hEvent = NULL;
	}
}

