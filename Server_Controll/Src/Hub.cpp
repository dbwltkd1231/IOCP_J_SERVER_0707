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

	void Hub::Construct(int serverPort, int prepareSocketMax, int iocpThreadCount, int overlappedQueueMax, int acceptedCapacity)
	{
		_acceptedCapacity = acceptedCapacity;

		_networkManager.Construct(overlappedQueueMax);
		_networkManager.SetupListenSocket(serverPort, prepareSocketMax, iocpThreadCount);


		for (int i = 0;i < prepareSocketMax;++i)
		{
			_networkManager.PrepareAcceptSocket();
		}
	}

	void Hub::MainThread()
	{
		std::thread mainThread([this]() { this->_networkManager.ProcessCompletionHandler(); });
		mainThread.detach();

		isOn = true;
		while (isOn)
		{

		}
	}
}

