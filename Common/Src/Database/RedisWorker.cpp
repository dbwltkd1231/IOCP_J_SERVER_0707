#include "RedisWorker.h"
#include <sstream>
#include "../Utility/Debug.h"

namespace Database
{
	RedisWorker::RedisWorker()
	{

	}

	RedisWorker::~RedisWorker()
	{

	}

	void RedisWorker::Construct(std::string ip, int redisPort)
	{
		_redis = redisConnect(ip.c_str(), redisPort);
		if (_redis == NULL || _redis->err)
		{
			Utility::Log("RedisWorker", "Construct", "Redis Connect Fail");
			return;
		}

		Utility::Log("RedisWorker", "Construct", "Redis Connect Success");
	}

    void RedisWorker::UpdateData(const std::string table, const std::string key, std::string jsonString, int ttl)
    {
        std::string cacheKey = "table:" + table + ":" + key;
        redisReply* reply = (redisReply*)redisCommand(_redis, "SET %s %s EX %d", cacheKey.c_str(), jsonString.c_str(), ttl);

        std::string log = "Set Cached Data : " + cacheKey + " - " + jsonString;
        Utility::Log("RedisWorker", "UpdateData", log);
    }

	std::map<std::string, std::string> RedisWorker::ScanMultipleResult(std::string tableName)
	{
        std::map<std::string, std::string> result;

        unsigned long long cursor = 0;
        do {
            redisReply* reply = (redisReply*)redisCommand(_redis, "SCAN %d MATCH table:%s:*", cursor, tableName.c_str());
            if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements < 2) {
                std::cerr << "Redis SCAN ����!" << std::endl;
                return result;
            }

            cursor = std::stoi(reply->element[0]->str); // ���� SCAN Ŀ�� ������Ʈ
            redisReply* keyList = reply->element[1];

            if (keyList->type == REDIS_REPLY_ARRAY)
            {
                for (size_t i = 0; i < keyList->elements; i++)
                {
                    if (keyList->element[i] && keyList->element[i]->str)
                    {
                        std::string key = keyList->element[i]->str;
                        redisReply* replyData = (redisReply*)redisCommand(_redis, "GET %s", key.c_str());

                        if (replyData && replyData->type == REDIS_REPLY_STRING)
                        {
                            std::string value = replyData->str;

                            result[key] = value;
                        }
                    }
                }
            }
        } while (cursor != 0);  // cursor�� 0�̸� SCAN ����

        return result;
	}

    void RedisWorker::DeleteTable(const std::string& tableName, int scanCount)
    {
        std::string pattern = "table:" + tableName + ":*";
        unsigned long long cursor = 0;

        do {
            redisReply* scanReply = (redisReply*)redisCommand(_redis,
                "SCAN %llu MATCH %s COUNT %d", cursor, pattern.c_str(), scanCount);

            if (!scanReply || scanReply->type != REDIS_REPLY_ARRAY || scanReply->elements < 2)
            {
                Utility::Log("RedisWorker", "DeleteTableEntries", "Redis SCAN ���� -> ���̺� ���� ����");
                if (scanReply) 
                    freeReplyObject(scanReply);

                return;
            }

            cursor = std::stoull(scanReply->element[0]->str);
            redisReply* keyList = scanReply->element[1];

            if (keyList->type == REDIS_REPLY_ARRAY) {
                for (size_t i = 0; i < keyList->elements; ++i) {
                    if (keyList->element[i] && keyList->element[i]->str) {
                        std::string keyToDelete = keyList->element[i]->str;
                        redisReply* delReply = (redisReply*)redisCommand(_redis, "DEL %s", keyToDelete.c_str());

                        if (delReply && delReply->type == REDIS_REPLY_INTEGER && delReply->integer == 1) {
                            std::cout << "[���� �Ϸ�] " << keyToDelete << std::endl;
                        }
                        else {
                            std::cerr << "[���� ����] " << keyToDelete << std::endl;
                        }

                        if (delReply) freeReplyObject(delReply);
                    }
                }
            }

            if (scanReply) freeReplyObject(scanReply);

        } while (cursor != 0);

        std::string log = "Deleted Cached Table : " + tableName;
        Utility::Log("RedisWorker", "DeleteTableEntries", log);
    }
}