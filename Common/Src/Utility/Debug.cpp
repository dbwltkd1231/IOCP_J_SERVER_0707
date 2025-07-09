#include "../Utility/Debug.h"

namespace Utility
{
	void Log(const std::string scriptName, const std::string funcName, const std::string log)
	{
		std::string message = "[" + scriptName + "] " + "[" + funcName + "]" + log + "\n";
		std::cout << message;
	}
}