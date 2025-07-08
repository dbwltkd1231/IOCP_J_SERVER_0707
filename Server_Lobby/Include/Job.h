#pragma once
#include "../Network/BasicData.h"

namespace Lobby
{
	struct Job
	{
		Job(ULONG_PTR socketPtr, Network::OperationType operationType, Network::CustomOverlapped* customOverlapped)
		{
			SocketPtr = socketPtr;
			OperationType = operationType;
			CustomOverlapped = customOverlapped;
		}

		ULONG_PTR SocketPtr;
		Network::OperationType OperationType;
		Network::CustomOverlapped* CustomOverlapped;
	};
}