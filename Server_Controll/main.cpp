#pragma once
#include <iostream>
#include "Business/ServerControlCenter.h"

int main()
{
	Business::ServerControlCenter controlCenter;
	controlCenter.Initialize(1, 1, 1, 1, 1);
	controlCenter.MainProcess();
}