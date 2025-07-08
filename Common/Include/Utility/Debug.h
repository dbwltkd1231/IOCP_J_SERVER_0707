#pragma once
#include <iostream>
#include <string>


namespace Utility
{
	void Log(const std::string projectName, const std::string scriptName, const std::string funcName, const std::string log)
	{
		std::string message = "[" + projectName + "] [" + scriptName + "] " + "[" + funcName + "]" +log + "\n";
		std::cout << message;
	}
}