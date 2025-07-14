#include "../Protocol/ControlServerProtocol.h"

namespace Protocol
{
	JOB_REQUEST_LOBBYINFO::JOB_REQUEST_LOBBYINFO(ULONG_PTR socketPtr, std::function<void(std::string& key, int& port, bool success)> requestLobbyInfo)
	{
		SocketPtr = socketPtr;
		_requestLobbyInfo = requestLobbyInfo;
	}

	JOB_REQUEST_LOBBYINFO::~JOB_REQUEST_LOBBYINFO()
	{

	}

	void JOB_REQUEST_LOBBYINFO::Execute(JobOutput& output)
	{
		bool success = false;
		std::string lobbyKey = "";
		int lobbyPort = 0;

		_requestLobbyInfo(lobbyKey, lobbyPort, success);

		flatbuffers::FlatBufferBuilder builder;
		auto lobbyKeyOffset = builder.CreateString(lobbyKey);
		auto responseLobbyInfo = protocol::CreateRESPONSE_LOBBYINFO(builder, success, lobbyKeyOffset, lobbyPort);
		builder.Finish(responseLobbyInfo);

		output.BodySize = static_cast<int>(builder.GetSize());
		output.Buffer.assign(reinterpret_cast<const char*>(builder.GetBufferPointer()), output.BodySize);
		output.ContentsType = protocol::MESSAGETYPE_RESPONSE_LOBBYINFO;
		output.SocketPtr = SocketPtr;

		output.IsSend = true;
	}

	////////////

	JOB_NOTICE_LOBBYREADY::JOB_NOTICE_LOBBYREADY(ULONG_PTR socketPtr, std::string lobbyKey, int port, int capacity, bool active, std::function<void(std::string& key, int& port, int& capacity, bool success)> saveLobbyInfo)
	{
		SocketPtr = socketPtr;
		_lobbyKey = lobbyKey;
		_port = port;
		_capacity = capacity;
		_active = active;

		_saveLobbyInfo = saveLobbyInfo;
	}

	JOB_NOTICE_LOBBYREADY::~JOB_NOTICE_LOBBYREADY()
	{

	}

	void JOB_NOTICE_LOBBYREADY::Execute(JobOutput& output)
	{
		_saveLobbyInfo(_lobbyKey, _port, _capacity, _active);
		output.IsSend = false;
	}


	////////////

	NOTICE_LOBBYINFO::NOTICE_LOBBYINFO(ULONG_PTR socketPtr, std::string lobbyKey, int current, int remain, bool active, std::function<void(std::string& key, int& current, int& remain, bool& active)> updateLobbyInfo)
	{
		SocketPtr = socketPtr;
		_lobbyKey = lobbyKey;
		_current = current;
		_remain = remain;
		_active = active;
		_updateLobbyInfo = updateLobbyInfo;
	}

	NOTICE_LOBBYINFO::~NOTICE_LOBBYINFO()
	{

	}

	void NOTICE_LOBBYINFO::Execute(JobOutput& output)
	{
		_updateLobbyInfo(_lobbyKey, _current, _remain, _active);
		output.IsSend = false;
	}
}