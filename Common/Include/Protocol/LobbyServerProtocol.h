#pragma once
#include <functional>
#include "../Protocol/Job.h"
#include "../Protocol/SERVER_PROTOCOL_generated.h"

namespace Protocol
{
	class JOB_NOTICE_LOBBYREADY : public Job
	{
	public:
		JOB_NOTICE_LOBBYREADY(ULONG_PTR socketPtr, std::string key, int port, bool active);
		~JOB_NOTICE_LOBBYREADY() override;

		void Execute(JobOutput& output) override;

	private:
		std::string _key;
		int _port;
		bool _active;

	};

	class JOB_NOTICE_LOBBYINFO : public Job
	{
	public:
		JOB_NOTICE_LOBBYINFO(ULONG_PTR socketPtr, std::string key, int current, int remain, bool active);
		~JOB_NOTICE_LOBBYINFO() override;

		void Execute(JobOutput& output) override;

	private:
		std::string _key;
		int _current;
		int _remain;
		bool _active;

	};



}