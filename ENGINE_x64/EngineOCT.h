#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iterator>

#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/opencl.h>


class EngineOCT
{
public:
	std::vector<float> ResamplingTableData;
	std::vector<float> ReferenceAScanData;
	std::vector<float> ReferenceSpectrumData;
	std::vector<short int> HostSpectraData;

	std::vector<float> CorrelationResults;
	std::vector<float> BScanResults;
	std::vector<float> CompositeResults;

	const int INPUTSPECTRALENGTH = 1024;
	const int OUTPUTASCANLENGTH = 1024;
	const int BSCANCOUNT = 10;
	const int ASCANBSCANRATIO = 500;
	const int ASCANAVERAGINGFACTOR = 1;
	const int BSCANAVERAGINGFACTOR = 1;

	int KernelSizeX;
	int KernelSizeY;
	int FilterWindowX;
	int FilterWindowY;
	int numBScanProcessingIteratations;

	void LoadOCTData();
	void OpenCLCompute();
	void ComputeCrossCorrelation();

private:
	void LoadFileData(std::string fileName, std::vector<float> &data);
};
