#pragma once
#include "flatbuffers/flatbuffers.h"
#include <basetsd.h>

namespace Protocol
{
	struct JobOutput
	{
		std::string Buffer;
		int BodySize;
		uint32_t ContentsType;
		ULONG_PTR SocketPtr;
	};

	class Job
	{
	public:
		Job() = default;
		virtual ~Job() = default;

		void virtual Execute(JobOutput& output) = 0;

		ULONG_PTR SocketPtr;
	};
}