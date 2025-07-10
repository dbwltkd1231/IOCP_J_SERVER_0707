#pragma once
#include "flatbuffers/flatbuffers.h"
#include <basetsd.h>

namespace Protocol
{
	struct JobOutput
	{
		ULONG_PTR SocketPtr;
		int BodySize;
		uint32_t ContentsType;
		std::string Buffer;
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