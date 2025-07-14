#pragma once
#include <functional>
#include "../Protocol/Job.h"
#include "../Protocol/SERVER_PROTOCOL_generated.h"

namespace Protocol
{
	class JOB_REQUEST_LOBBYINFO : public Job
	{
	public:
		JOB_REQUEST_LOBBYINFO(ULONG_PTR socketPtr, std::function<void(std::string& key, int& port, bool success)> requestLobbyInfo);
		~JOB_REQUEST_LOBBYINFO() override;

		void Execute(JobOutput& output) override;

	private:
		std::function<void(std::string& key, int& port, bool success)> _requestLobbyInfo;
	};

	class JOB_NOTICE_LOBBYREADY : public Job
	{
	public:
		JOB_NOTICE_LOBBYREADY(ULONG_PTR socketPtr, std::string lobbyKey, int port, int capacity, bool active, std::function<void(std::string& key, int& port, int& capacity, bool success)> saveLobbyInfo);
		~JOB_NOTICE_LOBBYREADY() override;

		void Execute(JobOutput& output) override;

	private:
		std::function<void(std::string& key, int& port, int& capacity, bool& success)> _saveLobbyInfo;

		std::string _lobbyKey;
		int _port;
		int _capacity;
		bool _active;
	};

	class NOTICE_LOBBYINFO : Job
	{
	public:
		NOTICE_LOBBYINFO(ULONG_PTR socketPtr, std::string LobbyKey, int current, int remain, bool active, std::function<void(std::string& key, int& current, int& remain, bool& active)> updateLobbyInfo);
		~NOTICE_LOBBYINFO() override;

		void Execute(JobOutput& output) override;

	private:
		std::function<void(std::string& key, int& current, int& remain, bool& active)> _updateLobbyInfo;

		std::string _lobbyKey;
		int _current;
		int _remain;
		bool _active;

	};

	/*
	class RESPONSE_LOBBYINFO : Job
	{
	public:
		RESPONSE_LOBBYINFO();
		~RESPONSE_LOBBYINFO() override;

		void Execute(JobOutput& output) override;
	};

	

	class NOTICE_LOBBYINFO : Job
	{
	public:
		NOTICE_LOBBYINFO();
		~NOTICE_LOBBYINFO() override;

		void Execute(JobOutput& output) override;
	};

	class NOTICE_SECRETKEY : Job
	{
	public:
		NOTICE_SECRETKEY();
		~NOTICE_SECRETKEY() override;

		void Execute(JobOutput& output) override;
	};

	class NOTICE_RESPONSESPEED : Job
	{
	public:
		NOTICE_RESPONSESPEED();
		~NOTICE_RESPONSESPEED() override;

		void Execute(JobOutput& output) override;
	};
	*/
}