#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/opencl.h>

class EngineOCT
{
public:
	std::vector<std::string> ResamplingTableData;
	std::vector<std::string> ReferenceAScanData;
	std::vector<std::string> ReferenceSpectrumData;

	const int INPUTSPECTRALENGTH = 1024;
	const int OUTPUTASCANLENGTH = 1024;
	const int BSCANCOUNT = 10;
	const int ASCANBSCANRATIO = 500;
	const int ASCANAVERAGINGFACTOR = 1;
	const int BSCANAVERAGINGFACTOR = 1;

	void LoadOCTData();

private:
	void LoadFileData(std::string fileName, std::vector<std::string> &data);
};
