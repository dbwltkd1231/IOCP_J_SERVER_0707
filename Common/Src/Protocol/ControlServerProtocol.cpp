#include "../Protocol/ControlServerProtocol.h"

namespace Protocol
{
	JOB_REQUEST_LOBBYINFO::JOB_REQUEST_LOBBYINFO(ULONG_PTR socketPtr, std::string key)
	{
		SocketPtr = socketPtr;
		Key = key;
	}

	JOB_REQUEST_LOBBYINFO::~JOB_REQUEST_LOBBYINFO()
	{

	}

	void JOB_REQUEST_LOBBYINFO::Execute(JobOutput& output)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto keyOffset = builder.CreateSharedString(Key);
		auto requestConnect = protocol::CreateREQUEST_LOBBYINFO(builder, keyOffset);
		builder.Finish(requestConnect);

		output.BodySize = static_cast<int>(builder.GetSize());
		output.Buffer.assign(reinterpret_cast<const char*>(builder.GetBufferPointer()), output.BodySize);
		output.ContentsType = protocol::MESSAGETYPE_REQUEST_LOBBYINFO;
		output.SocketPtr = SocketPtr;
	}
}