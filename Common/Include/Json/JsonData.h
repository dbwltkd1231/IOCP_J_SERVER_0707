#pragma once
#include <string>
#include <nlohmann/json.hpp> 

namespace Json
{
	struct LobbyInfo
	{
		std::string Key;
		int Port;
		int Current;
		int Remain;
		bool Active;
		float ResponseSpeed;

        LobbyInfo(const nlohmann::json& jsonStr)
        {
            Key = jsonStr["key"];
            Port = jsonStr["port"];
            Current = jsonStr["current"];
            Remain = jsonStr["remain"];
            Active = jsonStr["active"];
            ResponseSpeed = jsonStr["response_speed"];
        }

        nlohmann::json static DataToJson(std::string key, int port, int current, int remain, int active, float responseSpeed)//std::time_t last_update
        {
            nlohmann::json json =
            {
                 {"key", key},
                 {"port", port},
                 {"current", current},
                 {"remain", remain},
                 {"active", active},
                 {"response_speed", responseSpeed}
            };

            return json;
        }
	};
}