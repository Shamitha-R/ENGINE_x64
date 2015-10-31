#include <iostream>
#include <vector>
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/opencl.h>


int main()
{
	//Platform(driver) config
	cl_uint platformIdCount;
	clGetPlatformIDs(0, nullptr, &platformIdCount);

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);
	
	int i;
	std::cin >> i;

	return 0;
}

