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

	cl_uint OutputImageHeight;
	cl_uint NumAScansPerBScan;
	cl_uint NumAScans;	
	cl_uint AScanAverage;
	cl_uint BScanAverage;

	float MinValue;
	float MaxValue;

	int KernelSizeX;
	int KernelSizeY;
	int FilterWindowX;
	int FilterWindowY;
	int NumBScanProcessingIteratations;
	int TotalBScans;
	int BScanBatchSize;

	void LoadOCTData();
	void OpenCLCompute();
	void ComputeCrossCorrelation();

private:
	void LoadFileData(std::string fileName, std::vector<float> &data);

	int SetPreProcessingKernelParameters();
	int SetPostProcessingKernelParameters();
	int SetCorrelationParameters(cl_uint kernelSizeX, cl_uint kernelsizeY);
	int SetFilterParameters();
	int clOCTInit(cl_uint clDeviceIndex,
		cl_uint inputSpectraLength,
		cl_uint outputAScanLength,
		float* hostResamplingTable,
		float* hostReferenceSpectrum,
		float* hostReferenceAScan,
		char* kernelPath,
		char* clBuildLog,
		size_t* clBuildLogLength,
		unsigned int imageFormatStride,
		size_t preProcessingkernelWorkgroupSize,
		size_t postProcessingkernelWorkgroupSize,
		char* compilerOptions,
		cl_uint kernelSizeX,
		cl_uint kernelSizeY);
	int clAlloc(
		cl_context context,
		cl_uint inputSpectraLength,		
		cl_uint outputAScanLength,		              
		float* hostResamplingTable,		
		float* hostInterpolationMatrix,		
		float* hostReferenceSpectrum,	
		float* hostReferenceAScan,		
		unsigned int imageFormatStride
		);
	int FilterPostProcess(int batchNum, int batchSize,
		std::vector<float> &correlationResults,
		std::vector<float> &compositeResults);
	int ComputeCorrelation(int batchNum, int batchSize, 
		std::vector<float>& correlationResults);
};
