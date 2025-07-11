#pragma once
#include <string>
#include <cstdint>
#include <winsock2.h>

namespace Network
{
    enum OperationType
    {
        OP_DEFAULT = 0,
        OP_CONNECT = 1,
        OP_ACCEPT = 2,
        OP_RECV = 3,
        OP_SEND = 4,
        OP_DISCONNECT = 5,
    };

    enum SenderType
    {
        DEFAULT =0,
        CONTROL_SERVER = 1,
        AUTH_SERVER =2,
        LOBBY_SERVER =3,
        CLIENT =4
    };

    struct MessageHeader
    {
        uint32_t SenderType;
        uint32_t BodySize;
        uint32_t ContentsType;

        MessageHeader(uint32_t senderType, uint32_t bodySize, uint32_t contentsType) : SenderType(senderType), BodySize(bodySize), ContentsType(contentsType)
        {
        }

        MessageHeader(const MessageHeader& other) : SenderType(other.SenderType), BodySize(other.BodySize), ContentsType(other.ContentsType)
        {
        }
    };

    struct CustomOverlapped :OVERLAPPED
    {
    public:
        CustomOverlapped();
        ~CustomOverlapped();
        CustomOverlapped(const CustomOverlapped& other);

    public:
        WSABUF Wsabuf[2];

    private:
        OperationType _operationType;
        ULONG_PTR _socketPtr;

    public:
        void AcceptSetting(ULONG_PTR socketPtr);
        void ConnectSetting(ULONG_PTR socketPtr);
        void ReceiveSetting();
        void SendSetting(const MessageHeader& headerData, const char* bodyBuffer, ULONG bodyLen);
        OperationType GetOperation() const;
        ULONG_PTR GetKey() const;
        void Clear();
    };

    struct Packet
    {
        int SenderType;
        ULONG_PTR CompletionKey;
        int ContentsType;
        std::string Buffer;
    };

    /*
    Network::MessageHeader* receivedHeader = reinterpret_cast<Network::MessageHeader*>(overlapped->Wsabuf[0].buf);
		auto requestBodySize = ntohl(receivedHeader->BodySize);
		auto requestContentsType = ntohl(receivedHeader->ContentsType);

		auto bufferString = std::string(overlapped->Wsabuf[1].buf, requestBodySize);// string 값복사 전달을 통해 buffer초기화시에도 안정성을 강화.
		auto messageType = static_cast<protocol::MESSAGETYPE>(requestBodySize);
    */
}