#include "HCrashReport.h"
#include <iostream>


int main()
{
	HCrashReport::RegisterCrashReporter();
	
	int* A = new int();
	delete A;
	std::cout <<*A;
	
	return 0;
}