#include "EngineOCT.h"
#include "EngineRenderer.h" 
#include "UI.h"

#include <clFFT.h>

#define NUM_PLATFORMS 1
#define NUM_DEVICES 2
#define SELECTED_PLATFORM 0
#define MAX_PROGRAM_FILE_LENGTH 10240000
#define DEVICE_INFO_RETURN_SIZE 1024
#define NUM_DEVICES_TO_BUILD_FOR 1	// Number of devices to build a program for - should be 1, just build for the main GPU
#define INTERPOLATION_ORDER 2		// Enable second order interpolation
//
// Define the window type constants
//
#define WINDOW_TYPE_NONE	0
#define WINDOW_TYPE_HANN	1
#define WINDOW_TYPE_BLACKMAN 2
#define WINDOW_TYPE_GAUSSIAN 3

#define GPU_DEVICE_INDEX 0 
#include <chrono>
#include <numeric>
#include <functional>
#include <algorithm>
#include <Commdlg.h>

void EngineOCT::LoadFileData(std::string fileName, std::vector<float> &data)
{
	std::string currentLine;
	std::ifstream fileStreamer;

	fileStreamer.open(fileName);

	if (fileStreamer.is_open())
	{
		int splitLoc = 0;

		//Read while EOF
		while (getline(fileStreamer, currentLine)) {
			//Find index of EOL and extract line using substring
			splitLoc = currentLine.find(',', 0);
			data.push_back(std::stod(currentLine.substr(splitLoc + 1)));
		}

		fileStreamer.close();
		std::cout << fileName + " successfully read\n";

	}
	else std::cout << "Unable to open file";
}

std::string GetFileName(const std::string & prompt) {
	const int BUFSIZE = 1024;
	char buffer[BUFSIZE] = { 0 };
	OPENFILENAME ofns = { 0 };
	ofns.lStructSize = sizeof(ofns);
	ofns.lpstrFile = buffer;
	ofns.nMaxFile = BUFSIZE;
	ofns.lpstrTitle = prompt.c_str();
	GetOpenFileName(&ofns);
	return buffer;
}
//TODO REWRITE Spectra bin loading function
std::vector<short> readFile(const char* filename)
{
	// open the file:
	std::streampos fileSize;
	std::ifstream file(filename, std::ios::binary);

	// get its size:
	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// read the data:
	std::vector<short> fileData(fileSize);
	file.read((char*)&fileData[0], fileSize);
	return fileData;
}

void EngineOCT::LoadOCTData()
{
	std::cout << "Loading OCT data\n";

	//Load csv file data
	//LoadFileData(GetFileName("Resampling Table"), ResamplingTableData);

	LoadFileData("resamplingTable.csv", ResamplingTableData);
	LoadFileData("referenceAScan.csv", ReferenceAScanData);
	LoadFileData("referenceSpectrum.csv", ReferenceSpectrumData);

	//std::vector<short> test = readFile("Spectra.bin");
	//HostSpectraData = test;
	//std::cout << "Spectra.bin successfully read\n";
}

//
// Create some global variables
//
// cloct.h
//
clfftSetupData _clfftSetupData;
clfftPlanHandle _clfftPlanHandle;
size_t _fftLength;
char* _kernelPath;

size_t _tempBufferSize;
cl_mem _deviceTemporaryBuffer;
float* _interpolationMatrix;
//
// clfunctions.h
//
cl_platform_id*		_platformIDs;
cl_device_id*		_deviceIDs;
cl_context			_context;
char**				_deviceNameList;	// Array of devices names
cl_uint				_numDevices;
cl_command_queue	_commandQueue;
cl_uint				_numCommandQueues;
cl_uint				_selectedDeviceIndex;		// Device selected - default it 0... only use 1 device forthe moment
size_t              _maxWorkGroupSize;
size_t              _preProcessKernelWorkGroupSize;
size_t              _postProcessKernelWorkGroupSize;

char*               _compilerOptions;

cl_uint _inputSpectrumLength;
cl_uint _outputAScanLength;

cl_uint _windowType;
cl_uint _imageFormatStride;
cl_uint _corrSizeX;
cl_uint _corrSizeY;
cl_uint _corrSizeZ;
cl_uint _offsetX;
cl_uint _offsetY;
cl_uint _offsetZ;

size_t _totalBScans;
size_t _totalAScans;
size_t _totalInputSpectraLength;
size_t _totalOutputAScanLength;
size_t _totalPreProcessedSpectraLength;
size_t _totalAScansPerBScan;
size_t _bitmapBScanVolumeSize;
size_t _bitmapBScanSize;

BOOL _correlationUsesLogBScans;

cl_program			_clOCTProgram;
cl_kernel _preProcessingKernel;
cl_kernel _postProcessingKernel;
cl_kernel _imageKernel;
cl_kernel corrKernel;
cl_kernel filterKernel;
cl_kernel avgKernel;
cl_kernel compositeKernel;
cl_kernel opacityKernel;

cl_mem deviceSpectra;					// Input array of spectra that make up a single B-Scan
cl_mem bScanCorrData;
cl_mem devicePreProcessedSpectra;			// Resampled, windowed, etc, spectra
cl_mem deviceFourierTransform;

cl_mem deviceEnvBScan;
cl_mem deviceCorrelationMap;        // Stored B-Scan correlation maps
cl_mem deviceLogEnvBScan;
cl_mem deviceResamplingTable;
cl_mem deviceInterpolationMatrix;	// Pre-computed matrix of values used for interpolation
cl_mem deviceReferenceSpectrum;
cl_mem deviceReferenceAScan;
cl_mem deviceSum;
cl_mem deviceSAM;
cl_mem deviceAttenuationDepth;
cl_mem deviceBScanBmp;

cl_mem deviceFilterResults;
cl_mem deviceAvgResults;
cl_mem deviceCompositeResults;
cl_mem deviceBscanNoiseList;
cl_mem deviceCorrNoiseList;

char* clOCTLoadKernelSourceFromFile(char* sourceFile, unsigned int* len)
{
	//
	// Load a source cl file into a string.  Len is the length of the returned source file in characters, or negative if an error occurred
	// Return the loaded file as a null terminated string, or NULL if an error ocurred
	//
	int c;
	unsigned int i;
	unsigned int index = 0;
	char* tmpFileContent = (char*)malloc(sizeof(char) * MAX_PROGRAM_FILE_LENGTH);	// Specify maximum file length in character
	char* fileContent;
	FILE *file;
	file = fopen(sourceFile, "r");

	if (file)
	{
		while (((c = getc(file)) != EOF) && (index < MAX_PROGRAM_FILE_LENGTH))	// c is integer because EOF can be signed
		{
			tmpFileContent[index] = (char)c;
			index++;
		}
		fclose(file);


		if (index == MAX_PROGRAM_FILE_LENGTH)
		{
			*len = -2;
			return NULL;
		}

		// Copy to a new array of the correct size

		fileContent = (char*)malloc(sizeof(char)*index);
		for (i = 0; i< index; i++)
		{
			fileContent[i] = tmpFileContent[i];
		}
		fileContent[index] = '\0';	// Terminate string with null character

		free(tmpFileContent);
	}
	else
	{
		*len = -1;
		return NULL;
	}
	*len = index - 1;
	return fileContent;
}

int clInit(
	cl_context* clContext,
	cl_command_queue* clCommandQueue,
	cl_uint deviceIndex,
	char*** deviceNameList,
	cl_uint* numDevicesInList
	)
{
	cl_uint numPlatforms;	// Number of platforms
	cl_uint platformIDSize = sizeof(cl_platform_id);

	cl_uint j;
	cl_int err;

	_selectedDeviceIndex = deviceIndex;	// OpenCL device index in the device list
										//_selectedPlatformIndex = 0;
	_windowType = 0;					// Default to no window - but can be set during call to pre-processing kernel
										//
										// Get the number of platforms
										//

	//Determine the number of installed platforms
	err = clGetPlatformIDs(NULL, NULL, &numPlatforms);
	if (err != CL_SUCCESS) return err;
	// Create an array to hold the platforms list and populate the cl_platform_id structs
	_platformIDs = (cl_platform_id*)malloc(sizeof(cl_platform_id) * numPlatforms);
	err = clGetPlatformIDs(numPlatforms, _platformIDs, NULL);
	if (err != CL_SUCCESS) return err;
	
	///
	// Assume that we can use the first platform in the list.  Most systems will have only one platform installed, i.e. NVIDIA
	// although it's possible that the Intel SDK could also be installed.
	//
	if (numPlatforms < 1)
		return -1;

	// Get a list of the platform IDs associated with the first platform
	// Get the number of devices
	err = clGetDeviceIDs(_platformIDs[SELECTED_PLATFORM], CL_DEVICE_TYPE_ALL, 
		0, NULL, &_numDevices);	
	if (err < 0)
		return err;
	// Allocate an array for the devices
	_deviceIDs = (cl_device_id*)malloc(_numDevices*sizeof(cl_device_id));	
	// Allocate a list of pointers to device names
	_deviceNameList = (char**)malloc(_numDevices * sizeof(char*));		
	// Populate the array with the device IDs
	err = clGetDeviceIDs(_platformIDs[SELECTED_PLATFORM], CL_DEVICE_TYPE_ALL, 
		_numDevices, _deviceIDs, NULL);	

	if (err < 0)
		return err;
	//
	// Create a list of the device names - mainly for debugging at the moment
	//
	for (j = 0; j<_numDevices; j++)
	{

		//infoRetSize = 1024;	// A large return size variable
		_deviceNameList[j] = (char*)malloc(DEVICE_INFO_RETURN_SIZE);
		err = clGetDeviceInfo(_deviceIDs[j], CL_DEVICE_NAME, DEVICE_INFO_RETURN_SIZE, _deviceNameList[j], NULL);
		if (err < 0)
			return err;


	}
	//
	// Get the maximum local work group size
	// using         //CL_DEVICE_MAX_WORK_GROUP_SIZE
	//
	err = clGetDeviceInfo(_deviceIDs[_selectedDeviceIndex], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &_maxWorkGroupSize, NULL);
	//printf()
	if (err < 0)
		return err;

	//printf("\nMaximum local work group size: %i \n", _maxWorkGroupSize);
	//
	// Also return a reference to the name list to the user
	//
	*deviceNameList = _deviceNameList;
	*numDevicesInList = _numDevices;

	// Create a context for GPU execution 
	*clContext = clCreateContext(NULL, 1, &_deviceIDs[_selectedDeviceIndex], 
		NULL, NULL, &err);
	if (err < 0)
		return err;
	// Create a command queue on which to execute the OCT processing pipeline
	*clCommandQueue = clCreateCommandQueue(*clContext, _deviceIDs[_selectedDeviceIndex], 
		NULL, &err);

	if (err < 0)
		return err;

	return 0;
}

int EngineOCT::clAlloc(
	cl_context context,
	cl_uint inputSpectraLength,		// Specify the actual length of input spectra
	cl_uint outputAScanLength,		// Specify the length of output (outputLength >= inputLength).  If inputlength < outputlength then the input spectra will be zero padded           
	float* hostResamplingTable,		// Resampling table
	float* hostInterpolationMatrix,		// Pre-computed interpolation matrix
	float* hostReferenceSpectrum,	// Reference spectrum to be subtracted (or maybe divided!!) from each spectrum
	float* hostReferenceAScan,		// A reference background A-Scan to be subtracted.. maybe log domain!!
	unsigned int imageFormatStride
	)
{
	//
	// Try to allocate space on the GPU in global memory, this avoids repeating the allocation step for each DFT call
	// Copy constant arrays, i.e. resampling table and spectral reference
	//
	cl_int err = 0;
	cl_uint i;
	//

	//
	// Copy values to global variables
	//
	_inputSpectrumLength = inputSpectraLength;
	_outputAScanLength = outputAScanLength;
	_imageFormatStride = imageFormatStride;
	//
	// Derived quantities used for allocating memory on the GPU
	//
	_totalAScansPerBScan = (size_t)NumAScansPerBScan * (size_t)AScanAverage;
	_totalBScans = (size_t)BScanBatchSize * (size_t)BScanAverage;
	_totalAScans = _totalBScans * _totalAScansPerBScan;
	_totalInputSpectraLength = _totalAScans * (size_t)_inputSpectrumLength;
	_totalOutputAScanLength = _totalAScans * (size_t)_outputAScanLength;
	_totalPreProcessedSpectraLength = _totalOutputAScanLength * (size_t)2;
	_bitmapBScanSize = (size_t)OutputImageHeight * (size_t)_imageFormatStride;
	_bitmapBScanVolumeSize = (size_t)BScanBatchSize * (size_t)BScanAverage * _bitmapBScanSize;

	size_t corrSize = (size_t)(BScanBatchSize + 1) * (size_t)BScanAverage * _bitmapBScanSize;

	//
	_correlationUsesLogBScans = TRUE;       // default is for correlation mapping to use logarithmic bscans
											//
											// Create a buffer on the device for the input spectra to be copied to
											//
	deviceSpectra = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(short) * _totalInputSpectraLength, NULL, &err);
	if (err != CL_SUCCESS)
		return err;

	//Buffer containing BScanData for correlation
	bScanCorrData = clCreateBuffer(context, CL_MEM_READ_ONLY,
		sizeof(unsigned char) * (size_t)(BScanBatchSize + 1) * (size_t)BScanAverage * 
		_bitmapBScanSize, NULL, &err);
	if (err != CL_SUCCESS)
		return err;

	//Buffer containg the Vasculature Results 
	deviceCompositeResults = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*(OutputImageHeight * NumAScansPerBScan * 4 * (BScanBatchSize + 1)), 
		NULL, &err);
	if (err != CL_SUCCESS)
		return err;


	//
	// Copy the resampling table, reference spectrum and reference a-scan immediately (no need to call enqueuewritebuffer)
	// using the CL_MEM_COPY_HOST_PTR flag
	//
	deviceResamplingTable = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * (size_t)_inputSpectrumLength, hostResamplingTable, &err);
	if (err != CL_SUCCESS)
		return err;
	deviceInterpolationMatrix = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * (size_t)_inputSpectrumLength * (size_t)(INTERPOLATION_ORDER + 1) * (size_t)(INTERPOLATION_ORDER + 1), hostInterpolationMatrix, &err);
	if (err != CL_SUCCESS)
		return err;
	deviceReferenceSpectrum = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * (size_t)_inputSpectrumLength, hostReferenceSpectrum, &err);
	if (err != CL_SUCCESS)
		return err;
	deviceReferenceAScan = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * (size_t)_outputAScanLength, hostReferenceAScan, &err);
	if (err != CL_SUCCESS)
		return err;
	//
	// Allocate device memory for the outputs
	//
	// clFFT runs on an array of cl_mem objects, so we'll create the resampled spectra as such an array
	//
	devicePreProcessedSpectra = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * _totalPreProcessedSpectraLength, NULL, &err);	// Complex interleaved pre-processed spectra
	if (err != CL_SUCCESS)
		return err;
	//
	// Likewise create an array of output buffers to hold the transform
	//
	deviceFourierTransform = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * _totalPreProcessedSpectraLength, NULL, &err);	// Complex interleaved
	if (err != CL_SUCCESS)
		return err;

	//
	// Create buffers for each of the outputs
	//
	deviceEnvBScan = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*_totalOutputAScanLength, NULL, &err);	// Envelope of the fft
	if (err != CL_SUCCESS)
		return err;
	deviceLogEnvBScan = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*_totalOutputAScanLength, NULL, &err);	// Logarithmic Envelope of the fft
	if (err != CL_SUCCESS)
		return err;
	deviceCorrelationMap = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*_totalOutputAScanLength, NULL, &err);	// Correlation map
	if (err != CL_SUCCESS)
		return err;
	deviceSum = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * _totalAScans, NULL, &err);	// Sum along each a-scan
	if (err != CL_SUCCESS)
		return err;
	deviceSAM = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * _totalAScans, NULL, &err);	// attenuation measured along each a-scan
	if (err != CL_SUCCESS)
		return err;
	deviceAttenuationDepth = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * _totalAScans, NULL, &err);	// attenuation depth along each a-scan
	if (err != CL_SUCCESS)
		return err;
	deviceBScanBmp = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned char) * _bitmapBScanVolumeSize, NULL, &err);	// BScan bitmap images
	if (err != CL_SUCCESS)
		return err;

	//filterResults
	deviceFilterResults = clCreateBuffer(context, CL_MEM_READ_WRITE, 
		sizeof(float)*(OutputImageHeight * NumAScansPerBScan * (BScanBatchSize + 1)),
		NULL, &err);	// Correlation map
	if (err != CL_SUCCESS)
		return err;

	//AVG Resutls
	deviceAvgResults = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*(OutputImageHeight * NumAScansPerBScan * (BScanBatchSize +1)),NULL, &err);
	if (err != CL_SUCCESS)
		return err;



	deviceBscanNoiseList = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*((BScanBatchSize + 1)), NULL, &err);
	if (err != CL_SUCCESS)
		return err;
	deviceCorrNoiseList = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*((BScanBatchSize + 1)), NULL, &err);
	if (err != CL_SUCCESS)
		return err;

	return err;
}

int clOCTCompileKernels(char* sourceFile, char* build_log,
	size_t* buildLogLength)
{
	//
	// Compile the Kernels used by the OCT processing pipeline
	//
	//	char* sourceFile = "C:\\Users\\oct\\Google Drive\\OCT Control Software - octView\\clOCTKernels\\current\\octKernels.cl\0";//
	//	char* sourceFile = "/Users/phtomlins/Google Drive/OCT Control Software - octView/clOCTKernels/20140520/octKernels.cl\0";
	//char* build_log;
	size_t log_size;
	unsigned int i;

	unsigned int sourceLength;
	cl_int err = 0;


	char* kernelSource = clOCTLoadKernelSourceFromFile(sourceFile, &sourceLength);


	//		return err;		//DEBUGGING!!

	if (sourceLength < 0)
		return sourceLength;

	// Enable the OpenCL library to create a program from the source code
	_clOCTProgram = clCreateProgramWithSource(_context, 1, (const char**)&kernelSource,
		NULL, &err);
	if (err != CL_SUCCESS)
		return err;
	// Build the kernels that reside within the source
	err = clBuildProgram(_clOCTProgram, NUM_DEVICES_TO_BUILD_FOR, 
		&_deviceIDs[_selectedDeviceIndex], _compilerOptions, NULL, NULL);

	if (err != CL_SUCCESS)
	{
		// If there was an error, get the build log
		clGetProgramBuildInfo(_clOCTProgram, _deviceIDs[_selectedDeviceIndex], 
			CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);	// Get the size of the build log
																															//build_log = (char*)malloc(sizeof(char)*log_size + 1);		// Allocate memory for the log
		if (log_size > *buildLogLength)
			build_log = "Insufficient space to store build log.\0";
		else
			clGetProgramBuildInfo(_clOCTProgram, _deviceIDs[_selectedDeviceIndex], 
				CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
		//
		// Terminate with null character
		//
		*buildLogLength = log_size;
		build_log[log_size - 1] = '\0';
		build_log[log_size] = '\0';
		//
		return err;
	}
	else
	{
		*buildLogLength = 0;
		build_log[0] = '\0';
		build_log[1] = '\0';
	}
	//
	// Create specific kernels
	//

	_preProcessingKernel = clCreateKernel(_clOCTProgram, "octPreProcessingKernel", &err);
	if (err != CL_SUCCESS)
		return err;
	_postProcessingKernel = clCreateKernel(_clOCTProgram, "octPostProcessingKernel", &err);
	if (err != CL_SUCCESS)
		return err;
	corrKernel = clCreateKernel(_clOCTProgram, "corrKernel", &err);
	if (err != CL_SUCCESS)
		return err;
	filterKernel = clCreateKernel(_clOCTProgram, "filterKernel", &err);
	if (err != CL_SUCCESS)
		return err; 
	avgKernel = clCreateKernel(_clOCTProgram, "bScanAverage", &err);
	if (err != CL_SUCCESS)
		return err;
	compositeKernel = clCreateKernel(_clOCTProgram, "compositeKernel", &err);
	if (err != CL_SUCCESS)
		return err;
	opacityKernel = clCreateKernel(_clOCTProgram, "opacityKernel", &err);
	if (err != CL_SUCCESS)
		return err;
	//	_imageKernel = clCreateKernel(_clOCTProgram, "octImageKernel", &err);
	//	if (err != CL_SUCCESS)
	//		return err;


	//	free(kernelSource);

	return 0;
}
//
// Build a kernel from a source file
//
int clBuild(char* sourceFile, char* buildLog, size_t* buildLogLength)
{
	//
	// Test code to check the expected output from openCL library functions
	// Enable this to check correct outputs in C# wrapper functions
	//
	//	cl_uint i;
	size_t logLen = 0;
	cl_uint j;
	size_t infoRetSize;
	cl_int err;
	//
	// Compile the kernels
	//
	if (buildLog == NULL)
	{
		logLen = 1000000;
		buildLogLength = &logLen;
		buildLog = (char*)malloc(sizeof(char) * logLen);
	}
	//printf("Comiling kernels...");

	err = clOCTCompileKernels(sourceFile, buildLog, buildLogLength);
	//
	// Clear up internally allocated memory
	//
	if (logLen > 0)
		free(buildLog);



	return err;
}

int EngineOCT::SetPreProcessingKernelParameters()
{
	/*
	__kernel void octPreProcessingKernel(
	0 __global const short* spectra,					// Input arrays - raw spectra
	1 __global const float* resamplingTable,			// resampling table
	2 __global const float* interpolationMatrix,
	3 __global const float* referenceSpectrum,		// reference spectrum for deconvolution
	4 const cl_uint inputSpectraLength,
	5 const cl_uint outputAScanLength,
	6 const cl_uint numBScans,
	7 const cl_uint numAScansPerBScan,
	8 const cl_uint ascanAveragingFactor,
	9 const cl_uint bscanAveragingFactor,
	10 const cl_uint windowType,
	11 __global float* preProcessedCmplxSpectra				// Output is the pre-processed spectra, ready for FFT
	)
	*/
	cl_int err = 0;
	err = clSetKernelArg(_preProcessingKernel, 0, sizeof(cl_mem), &deviceSpectra);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 1, sizeof(cl_mem), &deviceResamplingTable);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 2, sizeof(cl_mem), &deviceInterpolationMatrix);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 3, sizeof(cl_mem), &deviceReferenceSpectrum);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 4, sizeof(cl_uint), &_inputSpectrumLength);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 5, sizeof(cl_uint), &_outputAScanLength);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 6, sizeof(cl_uint), &BScanBatchSize);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 7, sizeof(cl_uint), &this->NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 8, sizeof(cl_uint), &AScanAverage);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 9, sizeof(cl_uint), &BScanAverage);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 10, sizeof(cl_uint), &_windowType);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 11, sizeof(cl_mem), &devicePreProcessedSpectra);	// real part
	if (err != CL_SUCCESS) return err;
	//err = clSetKernelArg(_preProcessingKernel, 11, sizeof(cl_mem), &devicePreProcessedSpectra[1]);	// imaginary part
	//if (err != CL_SUCCESS) return err;

	return err;
}
int EngineOCT::SetPostProcessingKernelParameters()
{
	/*
	__kernel void octPostProcessingKernel(
	__global const float* FourierTransformCmplx,		// 0. Input, the Fourier transform of the pre-processed spectra
	__global const float* referenceAScan,				// 1. Reference a-scan subtracted from envelope of FT
	const unsigned int outputAScanLength,				// 2. Length of the 1D Fourier transform
	const unsigned int numBScans,						// 3. Number of BScans loaded into memory
	const unsigned int numAScansPerBScan,               // 4. Number of A-Scans per BScan
	const unsigned int ascanAveragingFactor,            // 5. AScan averaging factor
	const unsigned int bscanAveragingFactor,            // 6. BScan Averaging factor
	const unsigned int bscanBmpFormatStride,            // 7. Bitmap image row length for bscans
	const float minVal,                                 // 8. Minimum image value for bitmaps
	const float maxVal,                                 // 9. Maximum image value for bitmaps
	__global float* envBScan,							// 10. Envelope of the bscan
	__global float* logEnvBScan,						// 11. Log of envelope
	__global float* Sum,                                // 12. Integral along a-scan
	__global float* SAM,                                // 13. SAM image produced by measuring slope (not implemented yet)
	__global float* AttenuationDepth,                   // 14. Attenuation depth measurement (not yet implemented)
	__global char* bscanBmp                             // 15. BScan Bitmap Array
	)	*/
	cl_int err = 0;
	err = clSetKernelArg(_postProcessingKernel, 0, sizeof(cl_mem), &deviceFourierTransform);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 1, sizeof(cl_mem), &deviceReferenceAScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 2, sizeof(cl_uint), &_outputAScanLength);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 3, sizeof(cl_uint), &BScanBatchSize);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 4, sizeof(cl_uint), &this->NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 5, sizeof(cl_uint), &AScanAverage);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 6, sizeof(cl_uint), &BScanAverage);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 7, sizeof(cl_uint), &_imageFormatStride);	// Envelope of BScan
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 10, sizeof(cl_mem), &deviceEnvBScan);	// Envelope of BScan
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 11, sizeof(cl_mem), &deviceLogEnvBScan);	// Log envelope - usually used for B-Scan images
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 12, sizeof(cl_mem), &deviceSum);	// Log envelope - usually used for B-Scan images
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 13, sizeof(cl_mem), &deviceSAM);	// Log envelope - usually used for B-Scan images
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 14, sizeof(cl_mem), &deviceAttenuationDepth);	// Log envelope - usually used for B-Scan images
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 15, sizeof(cl_mem), &deviceBScanBmp);	// Log envelope - usually used for B-Scan images
	if (err != CL_SUCCESS) return err;

	return err;
}
int EngineOCT::SetCorrelationParameters(cl_uint kernelSizeX,cl_uint kernelsizeY)
{
	cl_int err = 0;
	err = clSetKernelArg(corrKernel, 0, sizeof(cl_mem), &bScanCorrData);
	if (err != CL_SUCCESS) return err;
	size_t totalBScans = BScanBatchSize;
	err = clSetKernelArg(corrKernel, 1, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;

	err = clSetKernelArg(corrKernel, 2, sizeof(cl_uint), &KernelSizeX);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(corrKernel, 3, sizeof(cl_uint), &KernelSizeY);
	if (err != CL_SUCCESS) return err;

	err = clSetKernelArg(corrKernel, 4, sizeof(cl_uint), &this->NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(corrKernel, 5, sizeof(cl_uint), &this->OutputImageHeight);
	if (err != CL_SUCCESS) return err;

	err = clSetKernelArg(corrKernel, 6, sizeof(cl_mem), &deviceCorrelationMap);
	if (err != CL_SUCCESS) return err;

	return err;
}
int EngineOCT::SetFilterParameters()
{
	//Filter Kernel
	cl_int err = 0;
	err = clSetKernelArg(filterKernel, 0, sizeof(cl_mem), &deviceAvgResults);
	if (err != CL_SUCCESS) return err;
	size_t totalBScans = BScanBatchSize;
	err = clSetKernelArg(filterKernel, 3, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(filterKernel, 4, sizeof(cl_uint), &NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(filterKernel, 5, sizeof(cl_uint), &OutputImageHeight);
	if (err != CL_SUCCESS) return err;

	//Avg Kernel Parameters
	err = clSetKernelArg(avgKernel, 0, sizeof(cl_mem), &bScanCorrData);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(avgKernel, 1, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(avgKernel, 2, sizeof(cl_uint), &NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(avgKernel, 3, sizeof(cl_uint), &OutputImageHeight);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(avgKernel, 4, sizeof(cl_mem), &deviceAvgResults);
	if (err != CL_SUCCESS) return err;

	//Composite Kernel Parameters
	err = clSetKernelArg(compositeKernel, 0, sizeof(cl_mem), &deviceAvgResults);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 1, sizeof(cl_mem), &deviceCorrelationMap);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 2, sizeof(cl_mem), &deviceBscanNoiseList);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 3, sizeof(cl_mem), &deviceCorrNoiseList);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 4, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 5, sizeof(cl_mem), &deviceCompositeResults);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 6, sizeof(cl_uint), &NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(compositeKernel, 7, sizeof(cl_uint), &OutputImageHeight);
	if (err != CL_SUCCESS) return err;

	//Opacity Kernel
	err = clSetKernelArg(opacityKernel, 0, sizeof(cl_mem), &deviceCompositeResults);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(opacityKernel, 1, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(opacityKernel, 2, sizeof(cl_uint), &NumAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(opacityKernel, 3, sizeof(cl_uint), &OutputImageHeight);
	if (err != CL_SUCCESS) return err;

	return err;
}

void PreComputeInterpolationCoefficients(float* resamplingTable, float* coefs,
	int ResamplingTableLength)
{
	//
	// Enable a quadratic interpolation
	// Therefore, a curve is defined by three points, requiring the calculation of 9 coefficients.
	//
	int i, j;
	int interpOrder = INTERPOLATION_ORDER;		// 2nd order interpolation
	int numCoefs = (interpOrder + 1) * (interpOrder + 1);
	int offset;
	float x0;
	float x1;
	float x2;
	float x3;
	//
	for (i = 0; i<ResamplingTableLength; i++)
	{
		x0 = resamplingTable[i];
		x1 = floor(x0);
		x2 = ceil(x0);
		x3 = x2 + 1;
		offset = i*numCoefs;

		if (
			(abs(x1 - x2) < 0.5)	// If the first two points are at the same position, then don't interpolate
			||
			(i >= ResamplingTableLength - interpOrder)	// Also don't inerpolate the last interpOrder number of points
			)
		{
			coefs[offset] = 1.0f;
			for (j = 1; j< numCoefs; j++)
				coefs[offset + j] = 0.0f;		// All of the other coefficients are zero
		}
		else	// Otherwise, compute the matrix
		{
			// First row
			coefs[offset] = x2*x3 / ((x1 - x2)*(x1 - x3));
			coefs[offset + 1] = -x1*x3 / ((x1 - x2)*(x2 - x3));
			coefs[offset + 2] = x1*x2 / ((x1 - x3)*(x2 - x3));
			// Second row
			coefs[offset + 3] = -(x2 + x3) / ((x1 - x2)*(x1 - x3));
			coefs[offset + 4] = (x1 + x3) / ((x1 - x2)*(x2 - x3));
			coefs[offset + 5] = -(x1 + x2) / ((x1 - x3)*(x2 - x3));
			// Third row
			coefs[offset + 6] = 1.0f / ((x1 - x2)*(x1 - x3));
			coefs[offset + 7] = -1.0f / ((x1 - x2)*(x2 - x3));
			coefs[offset + 8] = 1.0f / ((x1 - x3)*(x2 - x3));

			/*			if (
			_isnan(coefs[offset]) || !_finite(coefs[offset]) ||
			_isnan(coefs[offset+1]) || !_finite(coefs[offset+1]) ||
			_isnan(coefs[offset+2]) || !_finite(coefs[offset+2]) ||
			_isnan(coefs[offset+3]) || !_finite(coefs[offset+3]) ||
			_isnan(coefs[offset+4]) || !_finite(coefs[offset+4]) ||
			_isnan(coefs[offset+5]) || !_finite(coefs[offset+5]) ||
			_isnan(coefs[offset+6]) || !_finite(coefs[offset+6]) ||
			_isnan(coefs[offset+7]) || !_finite(coefs[offset+7]) ||
			_isnan(coefs[offset+8]) || !_finite(coefs[offset+8])
			)
			{
			printf("------------------------------------------------\r\n");
			printf("| %f\t%f\t%f |\r\n",coefs[offset],coefs[offset+1],coefs[offset+2]);
			printf("| %f\t%f\t%f |\r\n",coefs[offset+3],coefs[offset+4],coefs[offset+5]);
			printf("| %f\t%f\t%f |\r\n",coefs[offset+6],coefs[offset+7],coefs[offset+8]);
			printf("------------------------------------------------\r\n");

			}
			*/
		}
		/*
		printf("------------------------------------------------\r\n");
		printf("| %f\t%f\t%f |\r\n",coefs[offset],coefs[offset+1],coefs[offset+2]);
		printf("| %f\t%f\t%f |\r\n",coefs[offset+3],coefs[offset+4],coefs[offset+5]);
		printf("| %f\t%f\t%f |\r\n",coefs[offset+6],coefs[offset+7],coefs[offset+8]);
		printf("------------------------------------------------\r\n");
		*/
	}
}

int EngineOCT::clOCTInit(cl_uint clDeviceIndex,			// Index in the device list of the OpenCL capable device to use
	cl_uint inputSpectraLength,
	cl_uint outputAScanLength,
	float* hostResamplingTable,		// Resampling table
	float* hostReferenceSpectrum,	// Reference spectrum to be subtracted (or maybe divided!!) from each spectrum
	float* hostReferenceAScan,		// A reference background A-Scan to be subtracted.. maybe log domain!!
	char* kernelPath,
	char* clBuildLog,
	size_t* clBuildLogLength,
	unsigned int imageFormatStride,
	size_t preProcessingkernelWorkgroupSize,
	size_t postProcessingkernelWorkgroupSize,
	char* compilerOptions,
	cl_uint kernelSizeX,
	cl_uint kernelSizeY) {

	clfftStatus status;
	int clErr;
	size_t strideX;
	size_t dist;
	//size_t batchSize;
	char** deviceList;
	cl_uint numDevicesInList;
	//int i;

	//
	// Debugging output
	//
	/*
	printf("\n");
	printf("Resampe Table, Ref. Spectrum, Ref. AScan\n");
	for (i=0; i<10; i++)
	{
	printf("%f8,  %f8,  %f8\n",hostResamplingTable[i],hostReferenceSpectrum[i],hostReferenceAScan[i]);
	}
	*/
	//
	// Set global variable values
	//
	_kernelPath = kernelPath;//"C:\\Users\\oct\\Google Drive\\OCT Control Software - octView\\clOCTKernels\\current\\octProcessingKernels2.cl\0";
	_fftLength = (size_t)outputAScanLength;	// Global FFT length for clFFT to expect
	_numCommandQueues = 1;

	_preProcessKernelWorkGroupSize = preProcessingkernelWorkgroupSize;
	_postProcessKernelWorkGroupSize = postProcessingkernelWorkgroupSize;
	_compilerOptions = compilerOptions;

	strideX = 1;
	dist = _fftLength;
	//batchSize = numBScans*numAScansPerBScan*ascanAveragingFactor*bscanAveragingFactor;  // total number of ascans passed to GPU
	//
	//
	// Initalise the GPU
	//
	clErr = clInit(&_context, &_commandQueue, clDeviceIndex, &deviceList, &numDevicesInList);
	if (clErr != CL_SUCCESS) return(clErr);

	printf("\nCL Selected Device: ");
	printf(deviceList[clDeviceIndex]);
	printf("\n");
	//
	// Pre-compute the interpolation coefficients
	//
	_interpolationMatrix = (float*)malloc(sizeof(float) * (size_t)inputSpectraLength * (INTERPOLATION_ORDER + 1) * (INTERPOLATION_ORDER + 1));
	PreComputeInterpolationCoefficients(hostResamplingTable, _interpolationMatrix, (int)inputSpectraLength);
	//
	//
	// Allocate memory on the GPU device
	//
	clErr = clAlloc(
		_context,
		inputSpectraLength,		// Specify the actual length of input spectra
		outputAScanLength,		// Specify the length of output (outputLength >= inputLength).  If inputlength < outputlength then the input spectra will be zero padded
		hostResamplingTable,		// Resampling table
		_interpolationMatrix,
		hostReferenceSpectrum,	// Reference spectrum to be subtracted (or maybe divided!!) from each spectrum
		hostReferenceAScan,		// A reference background A-Scan to be subtracted.. maybe log domain!!
		imageFormatStride
		);
	if (clErr != CL_SUCCESS) return(clErr);
	//
	// Build the OpenCL kernels
	//
	clErr = clBuild(_kernelPath, clBuildLog, clBuildLogLength);
	if (clErr != CL_SUCCESS) return clErr;
	//
	// Set the pre- and post- processing kernel parameters
	//
	clErr = SetPreProcessingKernelParameters();
	if (clErr != CL_SUCCESS) return clErr;
	clErr = SetPostProcessingKernelParameters();
	if (clErr != CL_SUCCESS) return clErr;
	clErr = SetCorrelationParameters(kernelSizeX,kernelSizeY);
	if (clErr != CL_SUCCESS) return clErr;
	clErr = SetFilterParameters();
	if (clErr != CL_SUCCESS) return clErr;
	//clErr = SetImageKernelParameters();
	//if (clErr != CL_SUCCESS) return clErr;
	//
	// Determine the preferred workgroup size for each kernel
	// using CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE
	/*
	clErr = clGetKernelWorkGroupInfo (_preProcessingKernel,
	_deviceIDs[_selectedDeviceIndex],
	CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
	sizeof(size_t),
	&_preProcessKernelWorkGroupSize,
	NULL);
	*/
	//printf("\nPreferred preProcessing work group size: %i\n", _preProcessKernelWorkGroupSize);
	//
	//
	// Configure the setup data
	//
	// NOTE:  clfftInitSetupData is defined __inline in the clFFT source
	// This breaks gcc when compiled via xcode
	// Update clfft.h to declare
	// static __inline
	//	static __inline clfftStatus clfftInitSetupData( clfftSetupData* setupData )
	//
	status = clfftInitSetupData(&_clfftSetupData);
	if (status != CLFFT_SUCCESS)
		return((int)status);

	//
	// Setup clFFT
	//
	status = clfftSetup(&_clfftSetupData);
	if (status != CLFFT_SUCCESS)
		return((int)status);

	//
	// Create an FFT plan using the CL context that was initialised earlier
	//
	status = clfftCreateDefaultPlan(&_clfftPlanHandle, _context, CLFFT_1D, &_fftLength);
	if (status != CLFFT_SUCCESS)
		return((int)status);
	//
	// Set the plan parameters (see example at http://dournac.org/info/fft_gpu)
	//
	status = clfftSetResultLocation(_clfftPlanHandle, CLFFT_OUTOFPLACE);
	status = clfftSetLayout(_clfftPlanHandle, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);	// Currently complex to complex transform... 
	status = clfftSetPlanBatchSize(_clfftPlanHandle, _totalAScans);
	status = clfftSetPlanPrecision(_clfftPlanHandle, CLFFT_SINGLE);

	//status = clfftSetPlanInStride(_clfftPlanHandle, CLFFT_1D, &strideX);
	//status = clfftSetPlanOutStride(_clfftPlanHandle, CLFFT_1D, &strideX);
	//status = clfftSetPlanDistance(_clfftPlanHandle, _fftLength, _fftLength);
	//
	// Finalise the plan (apparently we can specify it a bit more... will look into this later)
	//
	status = clfftBakePlan(_clfftPlanHandle, _numCommandQueues, &_commandQueue, NULL, NULL);
	if (status != CLFFT_SUCCESS)
		return((int)status);
	//
	// Create a temporary buffer for the plan
	//
	status = clfftGetTmpBufSize(_clfftPlanHandle, &_tempBufferSize);
	if (status != CLFFT_SUCCESS) return status;

	if (_tempBufferSize > 0)
	{
		_deviceTemporaryBuffer = clCreateBuffer(_context, CL_MEM_READ_WRITE, _tempBufferSize, 0, &clErr);
		if (clErr != CL_SUCCESS) return clErr;
	}

	return 0;
}

int PreProcess(short* hostSpectra, int windowType)
{
	//int i;
	cl_int err;
	size_t numWorkItemsPerGroup = _preProcessKernelWorkGroupSize;
	//    size_t numWorkGroups;
	size_t totalWorkItems = (size_t)_totalAScans;
	size_t spectraSize = _totalInputSpectraLength * sizeof(short);

	//    numWorkGroups = (size_t)ceil((double)totalWorkItems / (double)numWorkItemsPerGroup);
	//
	_windowType = windowType;	// Set the window type before calling the kernel...
								//
								// Update the windowtype in the kernel argument
								//
	err = clSetKernelArg(_preProcessingKernel, 10, sizeof(unsigned int), &_windowType);
	if (err != CL_SUCCESS) return err;

	//
	// Copy the raw spectra from the host to the device
	//
	err = clEnqueueWriteBuffer(_commandQueue,
		deviceSpectra,
		CL_FALSE,
		0,
		spectraSize,
		hostSpectra,
		0,
		NULL,
		NULL);
	if (err != CL_SUCCESS)
		return err;

	//
	// Enqueue the pre processing kernel on the device
	//
	err = clEnqueueNDRangeKernel(_commandQueue, _preProcessingKernel, 1, NULL, &totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	return 0;
}


int SaveBitmapVec(
	unsigned char* pixelArray,       // In bitmap format (including padding)
	unsigned int width,
	unsigned int height,
	unsigned int pixelArraySize,      // length of the above array
	const char* fileName)
{
	//
	// Save BScan to RGB 24bpp device independent bitmap
	//
	float bitmapFileSize;
	unsigned int bpp = 24;
	unsigned int bytesPerPixel = bpp / 8;
	unsigned int headerLen = 14;    // 14 byte bitmap header
	unsigned int dibHeaderLen = 12; // BITMAPCOREHEADER
	unsigned int offset = headerLen + dibHeaderLen;
	unsigned int planes = 1;

	unsigned int bytePixel;
	unsigned int i;
	unsigned int ii;
	FILE* fbmp;

	bitmapFileSize = headerLen + dibHeaderLen + pixelArraySize;
	//
	// Write the source array to a device independent bitmap file
	//
	fbmp = fopen(fileName, "wb");
	//
	// Write a 14 byte header
	//
	//value="BM";
	fwrite("BM", 1, 2, fbmp);
	fwrite(&bitmapFileSize, 4, 1, fbmp);
	fwrite("0", 1, 4, fbmp);   // Write 4 zero bytes to the reserved section
	fwrite(&offset, 4, 1, fbmp); // Write the offset to the beginning of the image data
								 //
								 // Write DIB header
								 //
	fwrite(&dibHeaderLen, 4, 1, fbmp);
	fwrite(&width, 2, 1, fbmp);
	fwrite(&height, 2, 1, fbmp);
	fwrite(&planes, 2, 1, fbmp);
	fwrite(&bpp, 2, 1, fbmp);
	//
	// Write the pixel array
	//
	for (i = 0; i<pixelArraySize; i++)
	{
		ii = pixelArraySize - i - 1;
		//printf("%hhu grd",pixelArray[ii]);
		fwrite(&pixelArray[ii], 1, 1, fbmp);  // Red
	}

	fclose(fbmp);

	return EXIT_SUCCESS;

}

void TestBMP(std::vector<float> &data,int bscanNum)
{
	std::vector<std::vector<float>> corrCoeff;
	std::vector<std::vector<float>> corrCoeffRed;

	for (int row = 0; row < 512; row++)
	{
		std::vector<float> rowData00;
		std::vector<float> rowDataRed;

		for (int col = 0; col < (500 * 4); col += 4)
		{
			rowDataRed.push_back(data[col + (500 * row * 4)]);
			rowData00.push_back(data[col + (500 * row * 4) + 1]);
		}
		corrCoeff.push_back(rowData00);
		corrCoeffRed.push_back(rowDataRed);
	}

	int w = 500;
	int h = 512;

	FILE *f = nullptr;
	if (f)
		free(f);

	unsigned char *img = NULL;
	int filesize = 54 + 3 * w*h;  //w is your image width, h is image height, both int
	if (img)
		free(img);
	img = (unsigned char *)malloc(3 * w*h);
	memset(img, 0, sizeof(img));

	for (int xCor = 0; xCor < w; xCor++)
	{
		for (int yCor = 0; yCor < h; yCor++)
		{
			int x = xCor;
			int y = (h - 1) - yCor;
			int r = corrCoeffRed[yCor][xCor];
			int g = corrCoeff[yCor][xCor];
			int b = corrCoeff[yCor][xCor];
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;
			img[(x + y*w) * 3 + 2] = (unsigned char)(r);
			img[(x + y*w) * 3 + 1] = (unsigned char)(g);
			img[(x + y*w) * 3 + 0] = (unsigned char)(b);
		}
	}

	unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
	unsigned char bmppad[3] = { 0,0,0 };

	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w >> 8);
	bmpinfoheader[6] = (unsigned char)(w >> 16);
	bmpinfoheader[7] = (unsigned char)(w >> 24);
	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h >> 8);
	bmpinfoheader[10] = (unsigned char)(h >> 16);
	bmpinfoheader[11] = (unsigned char)(h >> 24);


	std::string str = "./results/aaaaaaaa" +
		std::to_string(bscanNum)
		+ ".bmp";
	const char* gfgfg = str.c_str();

	f = fopen(gfgfg, "wb");
	fwrite(bmpfileheader, 1, 14, f);
	fwrite(bmpinfoheader, 1, 40, f);
	for (int imgIndex = 0; imgIndex < h; imgIndex++)
	{
		fwrite(img + (w*(imgIndex - 1) * 3), 3, w, f);
		fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
	}

	free(img);

	fclose(f);

}

float standard_deviation(std::vector<float> &data)
{
	int n = data.size();

	float mean = 0.0, sum_deviation = 0.0;
	int i;
	for (i = 0; i<n; ++i)
	{
		mean += data[i];
	}
	mean = mean / n;
	for (i = 0; i<n; ++i)
		sum_deviation += (data[i] - mean)*(data[i] - mean);
	return sqrt(sum_deviation / n);
}

float totduration;

int EngineOCT::FilterPostProcess(int batchNum,int batchSize,std::vector<float> &correlationResults,
	std::vector<float> &compositeResults)
{
	printf("Performing Post processing on results \n");

	//Calculate BScan Averages
	cl_int err;
	size_t numWorkItemsPerGroup = 1;
	//    size_t numWorkGroups;
	size_t totalWorkItems = NumAScansPerBScan * OutputImageHeight;

	err = clSetKernelArg(avgKernel, 1, sizeof(cl_uint), &batchSize);

	err = clEnqueueNDRangeKernel(_commandQueue, avgKernel, 1, NULL, 
		&totalWorkItems, NULL, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	std::vector<float> avgResults(NumAScansPerBScan*OutputImageHeight*(batchSize-1));

	err = clEnqueueReadBuffer(_commandQueue, deviceAvgResults, CL_TRUE, 0, 
		sizeof(float)*(NumAScansPerBScan * OutputImageHeight * (batchSize - 1)), 
		avgResults.data(), 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	//Perform median filtering
	err = clSetKernelArg(filterKernel, 3, sizeof(cl_uint), &batchSize);

	err = clEnqueueNDRangeKernel(_commandQueue, filterKernel, 1, NULL,
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	std::vector<float> filterResults(NumAScansPerBScan * OutputImageHeight * (batchSize - 1));

	err = clEnqueueReadBuffer(_commandQueue, deviceAvgResults, CL_TRUE, 0,
		sizeof(float)*(NumAScansPerBScan * OutputImageHeight * (batchSize - 1)), filterResults.data(), 0, NULL, NULL);
	if (err != CL_SUCCESS) 
		return err;


	int topIgnoreSize = 15 * NumAScansPerBScan;
	int dataIgnoreSize = 199 * NumAScansPerBScan;
	int topIgnoreCorr = 14 * NumAScansPerBScan;

	std::vector<float> BScanNoiseList;
	std::vector<float> CorrNoiseList;

	for (int bScanNum = 0; bScanNum < batchSize - 1; bScanNum++)
	{
		//Threshold BScancs
		std::vector<float> avgBScan(filterResults.begin() + 
			(NumAScansPerBScan*OutputImageHeight*bScanNum)
			, filterResults.begin() + (NumAScansPerBScan*OutputImageHeight*(bScanNum+1)));

		std::vector<float> noiseDataBScan;

		for (int i = 0; i < 50; i++)
		{
			noiseDataBScan.insert(noiseDataBScan.end(),
				&avgBScan[((i * NumAScansPerBScan) + topIgnoreSize + dataIgnoreSize)],
				&avgBScan[((i * NumAScansPerBScan) + topIgnoreSize + dataIgnoreSize) 
				+ (NumAScansPerBScan)]);
		}

		//Calculate mean
		double sumBScan = std::accumulate(noiseDataBScan.begin(), noiseDataBScan.end(), 0.0);
		double meanBScan = sumBScan / noiseDataBScan.size();
		//Calculate standard deviation
		std::vector<double> diff(noiseDataBScan.size());
		std::transform(noiseDataBScan.begin(), noiseDataBScan.end(), diff.begin(),
			std::bind2nd(std::minus<double>(), meanBScan));
		double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
		double stdevBScan = std::sqrt(sq_sum / noiseDataBScan.size());
		//3 SD Threshold value
		float noiseFloorBScan = meanBScan - 3 * stdevBScan;
		BScanNoiseList.push_back(noiseFloorBScan);

		//std::replace_if(avgBScan.begin(), avgBScan.end(),
		//	[noiseFloorBScan](float value)
		//{
		//	if (value < noiseFloorBScan)
		//		return true;
		//	else
		//		return false;
		//}, 0);

		//Threshold correlation Map
		std::vector<float> noiseDataCorr;
		std::vector<float> corrResult(correlationResults.begin() + 
			(NumAScansPerBScan * OutputImageHeight * bScanNum)
			, correlationResults.begin() + 
			(NumAScansPerBScan * OutputImageHeight * (bScanNum + 1)));

		for (int i = 0; i < 10; i++)
		{
			noiseDataCorr.insert(noiseDataCorr.end(),
				&corrResult[(i * NumAScansPerBScan) + topIgnoreCorr],
				&corrResult[((i * NumAScansPerBScan) + topIgnoreCorr) + (NumAScansPerBScan)]);
		}


		double sumCorr = std::accumulate(noiseDataCorr.begin(), noiseDataCorr.end(), 0.0f);
		double meanCorr = sumCorr / noiseDataCorr.size();
		CorrNoiseList.push_back(meanCorr);
	}

	//************************************** Composite ************************************** 
	
	//Copy Noise lists to GPU
	err = clSetKernelArg(compositeKernel, 4, sizeof(cl_uint), &batchSize);
	err = clEnqueueWriteBuffer(_commandQueue,
		deviceBscanNoiseList,
		CL_FALSE,
		0,
		BScanNoiseList.size() * sizeof(float),
		BScanNoiseList.data(),
		0,
		NULL,
		NULL);
	err = clEnqueueWriteBuffer(_commandQueue,
		deviceCorrNoiseList,
		CL_FALSE,
		0,
		CorrNoiseList.size() * sizeof(float),
		CorrNoiseList.data(),
		0,
		NULL,
		NULL);

	err = clEnqueueNDRangeKernel(_commandQueue, compositeKernel, 1, NULL,
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	std::vector<float> compositeOutput(NumAScansPerBScan * OutputImageHeight * 4 
		* (batchSize - 1));

	clFlush(_commandQueue);


	//err = clEnqueueReadBuffer(_commandQueue, deviceCompositeResults, CL_TRUE, 0,
		//sizeof(float)*(500 * 512 * 4 * (batchSize - 1)), compositeOutput.data(), 0, NULL, NULL);
	//if (err != CL_SUCCESS) return err;

	//Time clock
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	//Set Opacity
	err = clSetKernelArg(opacityKernel, 1, sizeof(cl_uint), &batchSize);

	err = clEnqueueNDRangeKernel(_commandQueue, opacityKernel, 1, NULL,
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	err = clEnqueueReadBuffer(_commandQueue, deviceCompositeResults, CL_TRUE, 0,
	sizeof(float)*(NumAScansPerBScan * OutputImageHeight * 4 * (batchSize - 1)), 
		compositeOutput.data(), 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	//Read Time
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	float duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	totduration += duration;


	compositeResults.insert(compositeResults.end(),
		compositeOutput.begin(),
		compositeOutput.end());

	return 0;
}

int EngineOCT::ComputeCorrelation(int batchNum, int batchSize,std::vector<float>& correlationResults)
{
	bool saveBMP = true;
	cl_int err;

	size_t numWorkItemsPerGroup = 1;
	size_t totalWorkItems = NumAScansPerBScan * OutputImageHeight;
	
	//Add kernel execute operation to command queue
	err = clEnqueueNDRangeKernel(_commandQueue, corrKernel, 1, NULL, 
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;
	//Array to contain the computed correlation maps
	float* correlationMap;
	correlationMap = (float*)malloc(sizeof(float) * _totalAScans * _outputAScanLength);
	//Populate the array with the results
	err = clEnqueueReadBuffer(_commandQueue, deviceCorrelationMap, CL_TRUE, 0, 
		sizeof(float)*_totalAScans * _outputAScanLength, correlationMap, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	std::vector<float> results(correlationMap, correlationMap + (totalWorkItems*batchSize));

	correlationResults = std::vector<float>(results.begin(),
		results.end() - totalWorkItems);

	if (!saveBMP) {
		for (int bitMapNum = 0; bitMapNum < batchSize - 1; bitMapNum++)
		{

			std::vector<std::vector<float>> corrCoeff;
			for (int row = 0; row < OutputImageHeight; row++)
			{
				std::vector<float> rowData00;

				for (int col = 0; col < NumAScansPerBScan; col += 1)
				{
					rowData00.push_back(results[col + (NumAScansPerBScan * row) +
						(bitMapNum * totalWorkItems)]);
				}
				corrCoeff.push_back(rowData00);
			}

			int ignoreSize = 14 * NumAScansPerBScan;
			std::vector<unsigned char> testVec;
			for (int i = 0; i < (NumAScansPerBScan * OutputImageHeight);i++)
			{
				if (i >= ignoreSize &&
					i < ((200*NumAScansPerBScan)+ignoreSize)) {
					testVec.push_back(results[i + (bitMapNum * totalWorkItems)]);
					testVec.push_back(results[i + (bitMapNum * totalWorkItems)]);
					testVec.push_back(results[i + (bitMapNum * totalWorkItems)]);
				}
			}

			std::string str = "./results/cmOCT" +
				std::to_string(bitMapNum + (batchNum * 1000) + 2)
				+ ".bmp";
			const char* fileName = str.c_str();
			SaveBitmapVec(testVec.data(), NumAScansPerBScan, (200), NumAScansPerBScan * (200) * 3,fileName);

			if (!saveBMP) {

				int w = NumAScansPerBScan - 1;
				int h = OutputImageHeight -1;

				FILE *f = nullptr;
				if (f)
					free(f);

				unsigned char *img = NULL;
				int filesize = 54 + 3 * w*h;  //w is your image width, h is image height, both int
				if (img)
					free(img);
				img = (unsigned char *)malloc(3 * w*h);
				memset(img, 0, sizeof(img));

				for (int xCor = 0; xCor < w; xCor++)
				{
					for (int yCor = 0; yCor < h; yCor++)
					{
						int x = xCor;
						int y = (h - 1) - yCor;
						int r = corrCoeff[yCor][xCor];
						int g = corrCoeff[yCor][xCor];
						int b = corrCoeff[yCor][xCor];
						if (r > 255) r = 255;
						if (g > 255) g = 255;
						if (b > 255) b = 255;
						img[(x + y*w) * 3 + 2] = (unsigned char)(r);
						img[(x + y*w) * 3 + 1] = (unsigned char)(g);
						img[(x + y*w) * 3 + 0] = (unsigned char)(b);
					}
				}

				unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
				unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
				unsigned char bmppad[3] = { 0,0,0 };

				bmpfileheader[2] = (unsigned char)(filesize);
				bmpfileheader[3] = (unsigned char)(filesize >> 8);
				bmpfileheader[4] = (unsigned char)(filesize >> 16);
				bmpfileheader[5] = (unsigned char)(filesize >> 24);

				bmpinfoheader[4] = (unsigned char)(w);
				bmpinfoheader[5] = (unsigned char)(w >> 8);
				bmpinfoheader[6] = (unsigned char)(w >> 16);
				bmpinfoheader[7] = (unsigned char)(w >> 24);
				bmpinfoheader[8] = (unsigned char)(h);
				bmpinfoheader[9] = (unsigned char)(h >> 8);
				bmpinfoheader[10] = (unsigned char)(h >> 16);
				bmpinfoheader[11] = (unsigned char)(h >> 24);


				std::string str = "./results/cResult" +
					std::to_string(bitMapNum + (batchNum * 1000))
					+ ".bmp";
				const char* gfgfg = str.c_str();

				f = fopen(gfgfg, "wb");
				fwrite(bmpfileheader, 1, 14, f);
				fwrite(bmpinfoheader, 1, 40, f);
				for (int imgIndex = 0; imgIndex < h; imgIndex++)
				{
					fwrite(img + (w*(imgIndex - 1) * 3), 3, w, f);
					fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
				}

				free(img);

				fclose(f);
			}
		}
	}

	delete(correlationMap);

	return 0;
}

int InverseTransform()
{
	clfftStatus status;
	cl_int err;

	//
	// For debugging
	//
	//err = clEnqueueWriteBuffer(_commandQueue,
	//							deviceSpectra,
	//							CL_TRUE,
	//							0,
	//							sizeof(short) * _totalAScans * _inputSpectrumLength,
	//							hostSpectra,
	//							0,
	//							NULL,
	//							NULL);
	//if (err != CL_SUCCESS)
	//	return err;



	status = clfftEnqueueTransform(_clfftPlanHandle, CLFFT_BACKWARD, _numCommandQueues, &_commandQueue, 0, NULL, NULL, &devicePreProcessedSpectra, &deviceFourierTransform, _deviceTemporaryBuffer);
	if (status != CLFFT_SUCCESS)
		return((int)status);


	//err = clFinish(_commandQueue);	// Wait for the Transform to complete
	//if (err != CL_SUCCESS) return err;

	err = 0;
	return err;
}

int PostProcess(float minVal, float maxVal)
{
	cl_int err;
	size_t numWorkItemsPerGroup = _postProcessKernelWorkGroupSize;
	size_t totalWorkItems = _totalAScans;
	//
	// Set the kernel min and max value parameters
	//
	err = clSetKernelArg(_postProcessingKernel, 8, sizeof(cl_float), &minVal);	// min threshold
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 9, sizeof(cl_float), &maxVal);	// max threshold
	if (err != CL_SUCCESS) return err;
	//
	// Enqueue the post processing kernel on the device - all of the required data, i.e. the FFT result, is already on the GPU - so nothing to copy
	//
	err = clEnqueueNDRangeKernel(_commandQueue, _postProcessingKernel, 1, NULL, &totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	//err = clFinish(_commandQueue);
	//if (err != CL_SUCCESS)	return err;


	return 0;


}

int Wait()
{
	cl_int err = 0;
	err = clFinish(_commandQueue);
	return err;
}

int CopyPreProcessedSpectraToHost(
	float* preProcessedSpectra
	)
{
	cl_int err;
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, devicePreProcessedSpectra, CL_TRUE, 0, sizeof(float)*_totalAScans * _inputSpectrumLength * 2, preProcessedSpectra, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;
}

int CopyComplexFourierTransformToHost(
	float* cmplx
	//	float* imagPart
	)
{
	cl_int err;
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceFourierTransform, CL_TRUE, 0, sizeof(float)*_totalAScans * _outputAScanLength * 2, cmplx, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;
	//	err = clEnqueueReadBuffer(_commandQueue, deviceFourierTransform[1], CL_TRUE, 0, sizeof(float)*_totalAScans * _outputAScanLength, imagPart, 0, NULL, NULL);
	//	if (err != CL_SUCCESS) return err;

	return err;

}

int CopyLinearEnvelopeToHost(
	float* linearEnvelope
	)
{
	cl_int err;
	/*
	cl_mem devicePreProcessedSpectra[2];			// Resampled, windowed, etc, spectra
	cl_mem deviceFourierTransform[2];
	//cl_mem deviceRealBScan;
	//cl_mem deviceImagBScan;
	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceResamplingTable;
	cl_mem deviceReferenceSpectrum;
	cl_mem deviceReferenceAScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;


	*/
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceEnvBScan, CL_TRUE, 0, sizeof(float)*_totalAScans * _outputAScanLength, linearEnvelope, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}

int CopyLogEnvelopeToHost(
	float* logEnvelope
	)
{
	cl_int err;
	int i;
	/*
	cl_mem devicePreProcessedSpectra[2];			// Resampled, windowed, etc, spectra
	cl_mem deviceFourierTransform[2];
	//cl_mem deviceRealBScan;
	//cl_mem deviceImagBScan;
	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;


	*/
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceLogEnvBScan, CL_TRUE, 0, sizeof(float)*_totalAScans * _outputAScanLength, logEnvelope, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	//printf("\n");
	//printf("Log Env.\n");
	for (i = 0; i<10; i++)
	{
		//printf("%f\n", logEnvelope[i]);
	}

	return err;

}

int CopyCorrelationMapToHost(
	float* correlationMap
	)
{
	cl_int err;
	/*
	cl_mem devicePreProcessedSpectra[2];			// Resampled, windowed, etc, spectra
	cl_mem deviceFourierTransform[2];
	//cl_mem deviceRealBScan;
	//cl_mem deviceImagBScan;
	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;


	*/
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceCorrelationMap, CL_TRUE, 0, sizeof(float)*_totalAScans * _outputAScanLength, correlationMap, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}

int CopySumToHost(
	float* sum
	)
{
	cl_int err;
	/*
	cl_mem devicePreProcessedSpectra[2];			// Resampled, windowed, etc, spectra
	cl_mem deviceFourierTransform[2];
	//cl_mem deviceRealBScan;
	//cl_mem deviceImagBScan;
	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;


	*/
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceSum, CL_TRUE, 0, sizeof(float)*_totalAScans, sum, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}

int CopySAMToHost(
	float* sam
	)
{
	cl_int err;
	/*
	cl_mem devicePreProcessedSpectra[2];			// Resampled, windowed, etc, spectra
	cl_mem deviceFourierTransform[2];
	//cl_mem deviceRealBScan;
	//cl_mem deviceImagBScan;
	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;


	*/
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceSAM, CL_TRUE, 0, sizeof(float)*_totalAScans, sam, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}

int CopyAttenuationDepthToHost(
	float* attenuationDepth
	)
{
	cl_int err;
	/*
	cl_mem devicePreProcessedSpectra[2];			// Resampled, windowed, etc, spectra
	cl_mem deviceFourierTransform[2];
	//cl_mem deviceRealBScan;
	//cl_mem deviceImagBScan;
	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;


	*/
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceAttenuationDepth, CL_TRUE, 0, sizeof(float)*_totalAScans, attenuationDepth, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}


//
// Copy B-Scan Bitmaps back to the host
//
int CopyBScanBitmapsToHost(
	unsigned char* bscanBmp
	)
{
	cl_int err;
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceBScanBmp, CL_TRUE, 0, sizeof(unsigned char)*_bitmapBScanVolumeSize, bscanBmp, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}

//
// Copy a single B-Scan Bitmap back to the host
//
int CopyBScanBitmapToHost(
	unsigned int bscanIndex,
	unsigned char* bscanBmp
	)
{
	cl_int err;
	//
	// Don't block - use Wait function to wait for the buffer to finish copying
	//
	err = clEnqueueReadBuffer(_commandQueue, deviceBScanBmp, CL_TRUE, (size_t)bscanIndex * sizeof(unsigned char)*_bitmapBScanSize, sizeof(unsigned char)*_bitmapBScanSize, bscanBmp, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	return err;

}

int CopyAllResultsToHost(
	float* preProcessedSpectra,
	float* cmplx,
	//float* imagPart,
	float* linearEnvelope,
	float* logEnvelope,
	float* sum,
	float* sam,
	float* attenuationDepth,
	unsigned char* bscanBmp
	)
{
	cl_int err;
	err = CopyPreProcessedSpectraToHost(preProcessedSpectra);
	if (err != CL_SUCCESS) return err;
	err = CopyComplexFourierTransformToHost(cmplx);
	if (err != CL_SUCCESS) return err;
	err = CopyLinearEnvelopeToHost(linearEnvelope);
	if (err != CL_SUCCESS) return err;
	err = CopyLogEnvelopeToHost(logEnvelope);
	if (err != CL_SUCCESS) return err;
	err = CopySumToHost(sum);
	if (err != CL_SUCCESS) return err;
	err = CopySAMToHost(sam);
	if (err != CL_SUCCESS) return err;
	err = CopyAttenuationDepthToHost(attenuationDepth);
	if (err != CL_SUCCESS) return err;
	err = CopyBScanBitmapsToHost(bscanBmp);
	if (err != CL_SUCCESS) return err;
	//Wait();

	return err;
}

int SaveBitmap(
	char* path,
	unsigned char* pixelArray,       // In bitmap format (including padding)
	unsigned int width,
	unsigned int height,
	unsigned int pixelArraySize      // length of the above array
	)
{
	//
	// Save BScan to RGB 24bpp device independent bitmap
	//
	float bitmapFileSize;
	unsigned int bpp = 24;
	unsigned int bytesPerPixel = bpp / 8;
	unsigned int headerLen = 14;    // 14 byte bitmap header
	unsigned int dibHeaderLen = 12; // BITMAPCOREHEADER
	unsigned int offset = headerLen + dibHeaderLen;
	unsigned int planes = 1;

	unsigned int bytePixel;
	unsigned int i;
	unsigned int ii;
	FILE* fbmp;

	bitmapFileSize = headerLen + dibHeaderLen + pixelArraySize;
	//
	// Write the source array to a device independent bitmap file
	//
	fbmp = fopen(path, "wb");
	//
	// Write a 14 byte header
	//
	//value="BM";
	fwrite("BM", 1, 2, fbmp);
	fwrite(&bitmapFileSize, 4, 1, fbmp);
	fwrite("0", 1, 4, fbmp);   // Write 4 zero bytes to the reserved section
	fwrite(&offset, 4, 1, fbmp); // Write the offset to the beginning of the image data
								 //
								 // Write DIB header
								 //
	fwrite(&dibHeaderLen, 4, 1, fbmp);
	fwrite(&width, 2, 1, fbmp);
	fwrite(&height, 2, 1, fbmp);
	fwrite(&planes, 2, 1, fbmp);
	fwrite(&bpp, 2, 1, fbmp);
	//
	// Write the pixel array
	//
	for (i = 0; i<pixelArraySize; i++)
	{
		ii = pixelArraySize - i - 1;
		//printf("%hhu grd",pixelArray[ii]);
		fwrite(&pixelArray[ii], 1, 1, fbmp);  // Red
	}

	fclose(fbmp);

	return EXIT_SUCCESS;

}

int clRelease()
{
	cl_uint i;

	// Release inputs
	clReleaseMemObject(deviceSpectra);
	clReleaseMemObject(deviceResamplingTable);
	clReleaseMemObject(deviceInterpolationMatrix);
	clReleaseMemObject(deviceReferenceSpectrum);
	clReleaseMemObject(deviceReferenceAScan);
	//
	// Release outputs
	//
	//for (i=0; i<_totalAScans; i++)
	//{
	clReleaseMemObject(devicePreProcessedSpectra);
	clReleaseMemObject(deviceFourierTransform);
	//		clReleaseMemObject(devicePreProcessedSpectra[1]);
	//		clReleaseMemObject(deviceFourierTransform[1]);
	//}
	//free(devicePreProcessedSpectra);
	//free(deviceFourierTransform);

	//clReleaseMemObject(deviceRealBScan);
	//clReleaseMemObject(deviceImagBScan);
	clReleaseMemObject(deviceEnvBScan);
	clReleaseMemObject(deviceLogEnvBScan);
	clReleaseMemObject(deviceCorrelationMap);
	clReleaseMemObject(deviceSum);
	clReleaseMemObject(deviceSAM);
	clReleaseMemObject(deviceAttenuationDepth);
	clReleaseMemObject(deviceBScanBmp);
	//
	// Release other resources
	//
	for (i = 0; i<_numDevices; i++)
	{
		free(_deviceNameList[i]);
	}
	//	clReleaseKernel(dftKernel);
	////	clReleaseKernel(testKernel);
	clReleaseKernel(_preProcessingKernel);
	clReleaseKernel(_postProcessingKernel);
	//clReleaseKernel(_imageKernel);
	clReleaseKernel(corrKernel);
	clReleaseKernel(filterKernel);

	clReleaseProgram(_clOCTProgram);


	clReleaseCommandQueue(_commandQueue);
	clReleaseContext(_context);
	//	
	free(_deviceNameList);
	free(_deviceIDs);
	free(_platformIDs);


	return 0;
}

int clOCTDispose()
{
	clfftStatus status;
	//
	// Destroy the plan
	//
	status = clfftDestroyPlan(&_clfftPlanHandle);

	status = clfftTeardown();
	if (status != CLFFT_SUCCESS)
		return((int)status);
	//
	// Release the temporary buffer
	//
	clReleaseMemObject(_deviceTemporaryBuffer);
	//
	// Release the openCL mem objects
	//
	clRelease();
	//
	// Free the interpolation matrix
	//
	free(_interpolationMatrix);

	return 0;
}

void EngineOCT::OpenCLCompute()
{

	cl_uint clDeviceIndex = GPU_DEVICE_INDEX;

	size_t localWorkSize = 2;
	char compilerOptions[] = "-cl-fast-relaxed-math -cl-mad-enable";

	unsigned int inputLen;
	unsigned int outputLen;

	TotalBScans = 500;
	BScanBatchSize = 25;
	NumBScanProcessingIteratations = (unsigned int)floor(TotalBScans / BScanBatchSize);

	MinValue = -155;
	MaxValue = -55;

	this->AScanAverage = 1;
	this->BScanAverage = 1;

	this->NumAScans = 500;

	this->KernelSizeX = 6;
	this->KernelSizeY = 6;
	this->FilterWindowX = 3;
	this->FilterWindowY = 3;

	unsigned int saveBmp = true;

	size_t loglen = 1000000;
	size_t totalBScanSize;
	size_t totalBScanVolumeSize;
	//cl_uint numAScansPerBScan;
	cl_uint totalAScans;
	cl_uint totalInputLen;
	cl_uint totalOutputLen;
	cl_uint totalBatchLength;
	cl_uint stride;

	float* resampTable;
	float* refSpec;
	float* refAScan;

	short* inputSpectra;
	char* buildLog;

	float* referenceSpectrumFileData;
	float* referenceAScanFileData;
	float* resampleTableFileData;
	float* tempArray;

	char* kernelPath;
	float* preProcessed;
	float* cmplx;
	float* linEnv;
	float* logEnv;
	float* sum;
	float* sam;
	float* ad;
	unsigned char* bscanBmp;
	float* correlationMap;

	int res;

	int r;
	int i;
	int j;

	int val = 0;

	FILE* inputSpecFile;
	char* resamplingTablePath;//="/Users/phtomlins/Google Drive/OCT Data/Test/resamplingTable.csv";
	char* spectraPath;//="/Users/phtomlins/Google Drive/OCT Data/Test/Spectra.bin";
	char* referenceSpectrumPath;//="/Users/phtomlins/Google Drive/OCT Data/Test/referenceSpectrum.csv";
	char* referenceAScanPath;//="/Users/phtomlins/Google Drive/OCT Data/Test/referenceAScan.csv";
	char* rootPath;//="/Users/phtomlins/Google Drive/OCT Data/Test/";
	char bmpPath[2048];

	kernelPath = "enginekernels.cl";
	rootPath = "./enginekernels/";

	//
	// Try to read the reference spectrum and resampling table
	//
	inputLen = ReferenceAScanData.size();
	outputLen = ReferenceAScanData.size();

	this->OutputImageHeight = outputLen / 2;

	NumAScansPerBScan = NumAScans * AScanAverage * BScanAverage;
	totalAScans = BScanBatchSize * this->NumAScansPerBScan;

	totalInputLen = inputLen * totalAScans;
	totalOutputLen = outputLen * totalAScans;
	stride = this->NumAScansPerBScan * 3;
	totalBScanSize = (size_t)stride * (size_t)OutputImageHeight;
	totalBScanVolumeSize = totalBScanSize * (size_t)TotalBScans;
	resampTable = (float*)malloc(sizeof(float) * inputLen);
	refSpec = (float*)malloc(sizeof(float) * inputLen);
	refAScan = (float*)malloc(sizeof(float) * inputLen);
	inputSpectra = (short*)malloc(sizeof(short) * totalInputLen);
	buildLog = (char*)malloc(sizeof(char) * loglen);

	preProcessed = (float*)malloc(sizeof(float) * totalOutputLen * 2);
	cmplx = (float*)malloc(sizeof(float) * totalOutputLen * 2);
	//at* real = (float*)malloc(sizeof(float) * totalOutputLen);
	//at* imag = (float*)malloc(sizeof(float) * totalOutputLen);
	linEnv = (float*)malloc(sizeof(float) * totalOutputLen);
	logEnv = (float*)malloc(sizeof(float) * totalOutputLen);
	sum = (float*)malloc(sizeof(float) * totalAScans);
	sam = (float*)malloc(sizeof(float) * totalAScans);
	ad = (float*)malloc(sizeof(float) * totalAScans);
	bscanBmp = (unsigned char*)malloc(sizeof(unsigned char*) * totalBScanVolumeSize);
	correlationMap = (float*)malloc(sizeof(float) * totalOutputLen);

	inputSpecFile = fopen("spectra.bin", "rb");

	for (int i = 0; i<inputLen; i++)
	{
		resampTable[i] = ResamplingTableData[i];
		refSpec[i] = ReferenceSpectrumData[i];
		refAScan[i] = ReferenceAScanData[i];
	}

	res = clOCTInit(
		clDeviceIndex,
		inputLen,
		outputLen,
		resampTable,
		refSpec,
		refAScan,
		kernelPath,
		buildLog,
		&loglen,
		stride,
		localWorkSize,
		localWorkSize,
		compilerOptions,
		KernelSizeX,
		KernelSizeY
		);



	if (res != CL_SUCCESS)
	{
		printf("\nError %d, Build log output:\n", res);
		printf(buildLog);
		printf("\n");
	}
	else
	{

		printf("Processing %i B-Scans in batches of %i, each batch comprising %i A-Scans...\n",
			TotalBScans, BScanBatchSize, totalAScans);

		//NumBScanProcessingIteratations = 2;

		for (i = 0; i < NumBScanProcessingIteratations; i++)
		{
			//
			// Read the next set of BScans from file
			//
			printf(".");
			fseek(inputSpecFile, sizeof(short) * totalInputLen * i, SEEK_SET);
			fread(inputSpectra, sizeof(short), totalInputLen, inputSpecFile);
			printf(".");

			res = PreProcess(inputSpectra, WINDOW_TYPE_BLACKMAN);	// This step includes copying the raw spectra from host to device
			if (res != CL_SUCCESS)
			{
				printf("Failed with error: %i\n", res);

			}

			printf(".");
			res = InverseTransform();
			if (res != CL_SUCCESS)
			{
				printf("Failed with error: %i\n", res);
			}
			//            printf("Done.\n");
			//printf("Transform returned %d.\n", res);
			//            printf("Post-processing...");
			printf(".");

			res = PostProcess(MinValue, MaxValue);
			if (res != CL_SUCCESS)
			{
				printf("Failed with error: %i\n", res);

			}
			res = Wait();

			if (res == CL_SUCCESS)
			{
				res = CopyAllResultsToHost(
					preProcessed,
					cmplx,
					//imag,
					linEnv,
					logEnv,
					sum,
					sam,
					ad,
					bscanBmp
					);


				std::vector<unsigned char> tempBMPDat(bscanBmp,
					bscanBmp + (totalBScanSize * BScanBatchSize));
				BScanResults.insert(BScanResults.end(), tempBMPDat.begin(), tempBMPDat.end());

				printf("\n");
				//for (j = 0; j < 10; j++)
				//{
				//	//printf("%f\n", logEnv[j]);
				//}

				if (!saveBmp) {
					for (j = 0; j < BScanBatchSize; j++)
					{


						sprintf(bmpPath, "%sbscan%0.4i.bmp\0", rootPath, i*BScanBatchSize + j);
						//SaveBitmapFromFloatArray(bmpPath,&logEnv[j*numAScansPerBScan*outputLen], (unsigned short)numAScansPerBScan, outputLen, outputLen/2, bmpMinVal, bmpMaxVal);

						SaveBitmap(bmpPath, &bscanBmp[j*totalBScanSize], NumAScansPerBScan, OutputImageHeight, totalBScanSize);
					}

				}
			}
			printf("%f%% complete\n", (float)i /
				(float)NumBScanProcessingIteratations*100.0f);


		}

		printf("Processing B Scans: SUCCESS \n");
	
		this->ComputeCrossCorrelation();
	}

	fclose(inputSpecFile);
	//clOCTDispose();
	free(inputSpectra);
	free(resampTable);
	free(refSpec);
	free(refAScan);

	free(preProcessed);
	free(cmplx);
	//free(real);
	//free(imag);
	free(linEnv);
	free(logEnv);
	free(correlationMap);
	free(sum);
	free(sam);
	//free(referenceSpectrumFileData);
	//free(referenceAScanFileData);
	//free(resampleTableFileData);
	//free(tempArray);
	free(ad);
	free(bscanBmp);
	free(buildLog);

	printf("B-Scan compute Done.\n");


}

void EngineOCT::ComputeCrossCorrelation()
{
	int res;
	this->CompositeResults.clear();
	this->CorrelationResults.clear();

	totduration = 0.0f;

	for (int batchNum = 0; batchNum < NumBScanProcessingIteratations; batchNum++) {

		printf("Performing Cross Correlation batch %d of %d \n",
			(batchNum + 1), NumBScanProcessingIteratations);

		cl_uint batchSizePlus;
		unsigned char* tempBScanData;
		int volumnSizePlus;
		int size;


		if (batchNum == NumBScanProcessingIteratations - 1)
		{
			//Copy top 50 bScans to GPU
			batchSizePlus = BScanBatchSize;
			volumnSizePlus = NumAScansPerBScan * OutputImageHeight * batchSizePlus * 3;

			size = (sizeof(unsigned char) * volumnSizePlus);
			tempBScanData = (unsigned char*)malloc(size);

			int index00 = (BScanBatchSize * batchNum);
			int index01 = (BScanBatchSize * (batchNum + 1));

			std::copy(BScanResults.begin() + (index00 * OutputImageHeight * NumAScansPerBScan * 3),
				BScanResults.begin() + (index01 * OutputImageHeight * NumAScansPerBScan * 3),
				tempBScanData);

		}
		else
		{
			//Copy top 51 bScans to GPU
			batchSizePlus = BScanBatchSize + 1;
			volumnSizePlus = NumAScansPerBScan * OutputImageHeight * batchSizePlus * 3;

			size = (sizeof(unsigned char) * volumnSizePlus);
			tempBScanData = (unsigned char*)malloc(size);

			int index00 = (BScanBatchSize * batchNum);
			int index01 = (BScanBatchSize * (batchNum + 1)) + 1;

			std::copy(BScanResults.begin() + (index00 * OutputImageHeight * NumAScansPerBScan * 3),
				BScanResults.begin() + (index01 * OutputImageHeight * NumAScansPerBScan * 3),
				tempBScanData);

		}

		res = clSetKernelArg(corrKernel, 1, sizeof(cl_uint), &batchSizePlus);
		res = clSetKernelArg(corrKernel, 2, sizeof(cl_uint), &KernelSizeX);
		res = clSetKernelArg(corrKernel, 3, sizeof(cl_uint), &KernelSizeY);

		res = clEnqueueWriteBuffer(_commandQueue,
			bScanCorrData,
			CL_FALSE,
			0,
			size,
			tempBScanData,
			0,
			NULL,
			NULL);

		res = ComputeCorrelation(batchNum, batchSizePlus, this->CorrelationResults);

		res = clSetKernelArg(filterKernel, 1, sizeof(cl_uint), &FilterWindowX);
		res = clSetKernelArg(filterKernel, 2, sizeof(cl_uint), &FilterWindowY);

		res = FilterPostProcess(batchNum, batchSizePlus, this->CorrelationResults,
			this->CompositeResults);

		delete(tempBScanData);
	}
}





