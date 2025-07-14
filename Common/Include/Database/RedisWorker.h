#pragma once
#include <string>
#include <map>
#include "hiredis/hiredis.h"

namespace Database
{
	class RedisWorker
	{
	public:
		RedisWorker();
		~RedisWorker();

		void Construct(std::string ip, int redisPort);
		void UpdateData(const std::string table, const std::string key, std::string jsonString, int ttl);
		std::map<std::string, std::string> ScanMultipleResult(std::string tableName);
		void DeleteTable(const std::string& tableName, int scanCount);

	private:
		redisContext* _redis;
	};
}