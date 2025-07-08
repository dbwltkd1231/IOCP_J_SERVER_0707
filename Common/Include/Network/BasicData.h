#pragma once
#include <cstdint>
#include <winsock2.h>

namespace Network
{
    enum OperationType
    {
        OP_DEFAULT = 0,
        OP_ACCEPT = 1,
        OP_RECV = 2,
        OP_SEND = 3,
        OP_DISCONNECT = 4
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
        void ReceiveSetting();
        void SendSetting(const MessageHeader& headerData, const char* bodyBuffer, ULONG bodyLen); // send Àü¿ë.
        OperationType GetOperation() const;
        void Clear();
    };
}