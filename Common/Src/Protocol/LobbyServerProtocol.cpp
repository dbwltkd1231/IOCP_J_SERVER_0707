#include "../Protocol/LobbyServerProtocol.h"
#include "../Utility/Debug.h"
namespace Protocol
{
	JOB_NOTICE_LOBBYREADY::JOB_NOTICE_LOBBYREADY(ULONG_PTR socketPtr, std::string key, int port, int capacity, bool active)
	{
		SocketPtr = socketPtr;
		_key = key;
		_port = port;
		_capacity = capacity;
		_active = active;
	}

	JOB_NOTICE_LOBBYREADY::~JOB_NOTICE_LOBBYREADY()
	{

	}

	void JOB_NOTICE_LOBBYREADY::Execute(JobOutput& output)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto lobbyKeyOffset = builder.CreateString(_key);
		auto noticeLobbyReady = protocol::CreateNOTICE_LOBBYREADY(builder, lobbyKeyOffset, _port, _capacity, _active);
		builder.Finish(noticeLobbyReady);

		output.BodySize = static_cast<int>(builder.GetSize());
		output.Buffer.assign(reinterpret_cast<const char*>(builder.GetBufferPointer()), output.BodySize);
		output.ContentsType = protocol::MESSAGETYPE_NOTICE_LOBBYREADY;
		output.SocketPtr = SocketPtr;

		output.IsSend = true;
	}

	////////////

	JOB_NOTICE_LOBBYINFO::JOB_NOTICE_LOBBYINFO(ULONG_PTR socketPtr, std::string key, int current, int remain, bool active)
	{
		SocketPtr = socketPtr;
		_key = key;
		_current = current;
		_remain = remain;
		_active = active;
	}

	JOB_NOTICE_LOBBYINFO::~JOB_NOTICE_LOBBYINFO()
	{

	}

	void JOB_NOTICE_LOBBYINFO::Execute(JobOutput& output)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto lobbyKeyOffset = builder.CreateString(_key);
		auto noticeLobbyInfo = protocol::CreateNOTICE_LOBBYINFO(builder, lobbyKeyOffset, _current, _remain, _active);
		builder.Finish(noticeLobbyInfo);

		output.BodySize = static_cast<int>(builder.GetSize());
		output.Buffer.assign(reinterpret_cast<const char*>(builder.GetBufferPointer()), output.BodySize);
		output.ContentsType = protocol::MESSAGETYPE_NOTICE_LOBBYINFO;
		output.SocketPtr = SocketPtr;

		output.IsSend = true;
	}
}