#include "../Protocol/LobbyServerProtocol.h"
#include "../Utility/Debug.h"
namespace Protocol
{
	JOB_NOTICE_LOBBYREADY::JOB_NOTICE_LOBBYREADY(ULONG_PTR socketPtr, std::string key, int port, bool active)
	{
		SocketPtr = socketPtr;
		_key = key;
		_port = port;
		_active = active;
	}

	JOB_NOTICE_LOBBYREADY::~JOB_NOTICE_LOBBYREADY()
	{

	}

	void JOB_NOTICE_LOBBYREADY::Execute(JobOutput& output)
	{

		flatbuffers::FlatBufferBuilder builder;
		auto lobbyKeyOffset = builder.CreateString(_key);
		auto noticeLobbyReady = protocol::CreateNOTICE_LOBBYREADY(builder, lobbyKeyOffset, _port, _active);
		builder.Finish(noticeLobbyReady);

		output.BodySize = static_cast<int>(builder.GetSize());
		output.Buffer.assign(reinterpret_cast<const char*>(builder.GetBufferPointer()), output.BodySize);
		output.ContentsType = protocol::MESSAGETYPE_NOTICE_LOBBYREADY;
		output.SocketPtr = SocketPtr;

		output.IsSend = true;

		Utility::Log("Protocol", "JOB_NOTICE_LOBBYREADY", "JOB_NOTICE_LOBBYREADY");
	}

	////////////
}