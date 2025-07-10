#pragma once
#include "../Protocol/Job.h"
#include "../Protocol/CONTROL_SERVER_PROTOCOL_generated.h"

namespace Protocol
{
	class JOB_REQUEST_LOBBYINFO : public Job
	{
	public:
		JOB_REQUEST_LOBBYINFO(ULONG_PTR socketPtr, std::string key);
		~JOB_REQUEST_LOBBYINFO() override;

		void Execute(JobOutput& output) override;

		std::string Key;
	};

	/*
	class RESPONSE_LOBBYINFO : Job
	{
	public:
		RESPONSE_LOBBYINFO();
		~RESPONSE_LOBBYINFO() override;

		void Execute(JobOutput& output) override;
	};

	class NOTICE_LOBBYREADY : Job
	{
	public:
		NOTICE_LOBBYREADY();
		~NOTICE_LOBBYREADY() override;

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