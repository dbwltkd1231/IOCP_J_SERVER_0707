#pragma once
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

    struct MessageHeader
    {
        uint32_t BodySize;
        uint32_t ContentsType;

        MessageHeader(uint32_t bodySize, uint32_t contentsType) : BodySize(bodySize), ContentsType(contentsType)
        {
        }

        MessageHeader(const MessageHeader& other) : BodySize(other.BodySize), ContentsType(other.ContentsType)
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
}