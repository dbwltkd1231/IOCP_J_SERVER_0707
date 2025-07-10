#include "../Protocol/ControlServerProtocol.h"

namespace Protocol
{
	JOB_REQUEST_LOBBYINFO::JOB_REQUEST_LOBBYINFO(ULONG_PTR socketPtr, std::function<void(std::string& key, int& port, bool success)> lobbyInfo)
	{
		SocketPtr = socketPtr;
		LobbyInfo = lobbyInfo;
	}

	JOB_REQUEST_LOBBYINFO::~JOB_REQUEST_LOBBYINFO()
	{

	}

	void JOB_REQUEST_LOBBYINFO::Execute(JobOutput& output)
	{
		bool success = false;
		std::string lobbyKey = "";
		int lobbyPort = 0;

		LobbyInfo(lobbyKey, lobbyPort, success);

		flatbuffers::FlatBufferBuilder builder;
		auto lobbyKeyOffset = builder.CreateString(lobbyKey);
		auto responseLobbyInfo = protocol::CreateRESPONSE_LOBBYINFO(builder, success, lobbyKeyOffset, lobbyPort);
		builder.Finish(responseLobbyInfo);

		output.BodySize = static_cast<int>(builder.GetSize());
		output.Buffer.assign(reinterpret_cast<const char*>(builder.GetBufferPointer()), output.BodySize);
		output.ContentsType = protocol::MESSAGETYPE_RESPONSE_LOBBYINFO;
		output.SocketPtr = SocketPtr;
	}
}