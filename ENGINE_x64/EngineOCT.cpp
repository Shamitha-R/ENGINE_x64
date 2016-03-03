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

void EngineOCT::LoadFileData(std::string fileName, std::vector<float> &data)
{
	std::string currentLine;
	std::ifstream fileStreamer;

	fileStreamer.open(fileName);

	if (fileStreamer.is_open())
	{
		int splitLoc = 0;

		while (getline(fileStreamer, currentLine)) {
			splitLoc = currentLine.find(',', 0);
			data.push_back(std::stod(currentLine.substr(splitLoc + 1)));
		}

		fileStreamer.close();
		std::cout << fileName + " successfully read\n";

	}
	else std::cout << "Unable to open file";
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
	LoadFileData("resamplingTable.csv", ResamplingTableData);
	LoadFileData("referenceAScan.csv", ReferenceAScanData);
	LoadFileData("referenceSpectrum.csv", ReferenceSpectrumData);

	//std::vector<short> test = readFile("Spectra.bin");
	//HostSpectraData = test;
	//std::cout << "Spectra.bin successfully read\n";
}

//void setupTexture()
//{
//	for (int i = 0; i < 20*20*3;i++)
//	{
//		textureData[i] = ((i*1.0f)/(400*3.0f*1.0f))*255;
//	}
//
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//	glPixelStorei(GL_PACK_ALIGNMENT, 1);
//	glGenTextures(1, &textureName);   // generate a texture handler really reccomanded (mandatory in openGL 3.0)
//	glBindTexture(GL_TEXTURE_2D, textureName); // tell openGL that we are using the texture 
//
//	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB,
//		GL_UNSIGNED_BYTE, (GLvoid*)textureData); // send the texture data
//
//	 // Set up the texture
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//
//	glEnable(GL_TEXTURE_2D);
//}
//
//void updateTexture()
//{			
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, /* or GL_BGR, depends on how your video is decoded*/
//		GL_UNSIGNED_BYTE, textureData);
//
//	glBindTexture(GL_TEXTURE_2D, textureName);
//	glBegin(GL_QUADS);  // draw something with the texture on
//	glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
//	glTexCoord2f(1.0f, 0.0f); glVertex2f(0.5f, -0.5f);
//	glTexCoord2f(1.0f, 1.0f); glVertex2f(0.5f, 0.5f);
//	glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f, 0.5f);
//	glEnd();
//}

void EngineRendering(EngineOCT &oct)
{	
	//	// Set up vertex data (and buffer(s)) and attribute pointers
	//	GLfloat vertices[] = {
	//		// Positions          // Colors           // Texture Coords
	//		0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right
	//		0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Bottom Right
	//		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Bottom Left
	//		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // Top Left 
	//	};
	//	GLuint indices[] = {  // Note that we start from 0!
	//		0, 1, 3, // First Triangle
	//		1, 2, 3  // Second Triangle
	//	};
	//	GLuint VBO, VAO, EBO;
	//	glGenVertexArrays(1, &VAO);
	//	glGenBuffers(1, &VBO);
	//	glGenBuffers(1, &EBO);

	//	glBindVertexArray(VAO);

	//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//	// Position attribute
	//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	//	glEnableVertexAttribArray(0);
	//	// Color attribute
	//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	//	glEnableVertexAttribArray(1);
	//	// TexCoord attribute
	//	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	//	glEnableVertexAttribArray(2);

	//	glBindVertexArray(0); // Unbind VAO


	//						  // Load and create a texture 
	//	GLuint texture;
	//	glGenTextures(1, &texture);
	//	glBindTexture(GL_TEXTURE_2D, texture); // All upcoming GL_TEXTURE_2D operations now have effect on this texture object
	//
	//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//	//// Set texture filtering parameters
	//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	//	// Load image, create texture and generate mipmaps
	//	//SDL_Surface* image = engine.LoadSurface("textures/wall.png");

	//	//GLuint textureName;
	//	const GLsizei width = 500;
	//	const GLsizei height = 512;

	//	//std::vector<float> imgData(&testPixelData[0], &testPixelData[500*512]);

	//	GLbyte textureData[width * height * 3];

	//	for (int i = 0; i < width*height*3;i+=3)
	//	{
	//		textureData[i] = testPixelData[i/3];
	//		textureData[i+1] = testPixelData[i / 3];
	//		textureData[i+2] = testPixelData[i / 3];
	//	}

	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
	//	glGenerateMipmap(GL_TEXTURE_2D);

	//	//SDL_FreeSurface(image);
	//	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.


	//	SDL_Event e;
	//	SDL_StartTextInput();
	//	int test;
	//	float val = 0.0f;
	//	bool quit = false;

	//	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	//	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	//	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	//	GLfloat cameraSpeed = 0.05f;
	//	SDL_Keycode currentCode;

	//	bool firstMouse = true;
	//	int xpos, ypos;

	//	GLfloat yaw = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
	//	GLfloat pitch = 0.0f;
	//	GLfloat lastX = 800 / 2.0;
	//	GLfloat lastY = 600 / 2.0;

	//	float angle = 0.0f;

	//	while (!quit)
	//	{
	//		if (SDL_PollEvent(&e) != 0)
	//		{
	//			//test = TwEventSDL20(&e, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
	//			test = TwEventSDL20(&e);

	//			if (!test) {

	//				if (e.type == SDL_QUIT)
	//				{
	//					quit = true;
	//				}
	//				else if (e.type == SDL_KEYDOWN && e.key.repeat != 0)
	//				{
	//					currentCode = e.key.keysym.sym;

	//					if (currentCode == SDLK_w)
	//						cameraPos += cameraSpeed * cameraFront;
	//					if (currentCode == SDLK_s)
	//						cameraPos -= cameraSpeed * cameraFront;
	//					if (currentCode == SDLK_a)
	//						cameraPos -= glm::normalize(
	//							glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	//					if (currentCode == SDLK_d)
	//						cameraPos += glm::normalize(
	//							glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	//				}
	//			}


	//			SDL_GetMouseState(&xpos, &ypos);

	//			if (firstMouse)
	//			{
	//				lastX = xpos;
	//				lastY = ypos;
	//				firstMouse = false;
	//			}
	//			GLfloat xoffset = xpos - lastX;
	//			GLfloat yoffset = lastY - ypos;

	//			lastX = xpos;
	//			lastY = ypos;

	//			GLfloat sensitivity = 0.05;
	//			xoffset *= sensitivity;
	//			yoffset *= sensitivity;

	//			yaw += xoffset;
	//			pitch += yoffset;

	//			// Make sure that when pitch is out of bounds, screen doesn't get flipped
	//			if (pitch > 89.0f)
	//				pitch = 89.0f;
	//			if (pitch < -89.0f)
	//				pitch = -89.0f;

	//			glm::vec3 front;
	//			front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	//			front.y = sin(glm::radians(pitch));
	//			front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	//			//cameraFront = glm::normalize(front);
	//		}

	//		// Render
	//		// Clear the color buffer
	//		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	//		glClear(GL_COLOR_BUFFER_BIT);

	//		// Bind Textures using texture units
	//		glActiveTexture(GL_TEXTURE0);
	//		glBindTexture(GL_TEXTURE_2D, texture);
	//		glUniform1i(glGetUniformLocation(ourShader.Program, "ourTexture1"), 0);

	//		// Activate shader
	//		ourShader.Enable();

	//		// Create transformations
	//		glm::mat4 model;
	//		glm::mat4 view;
	//		glm::mat4 projection;

	//		model = glm::rotate(model, -45.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	//		angle = (400.0f - xpos);
	//		angle = angle / 360.0f * M_PI * 1.0f;
	//		model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

	//		view = glm::translate(view, glm::vec3(0.0f, 0.0f, -2.0f));
	//		projection = glm::perspective(70.0f, (GLfloat)800 / (GLfloat)600, 0.1f, 100.0f);
	//		// Get their uniform location
	//		GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
	//		GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
	//		GLint projLoc = glGetUniformLocation(ourShader.Program, "projection");
	//		// Pass them to the shaders
	//		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	//		// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
	//		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//		// Draw container
	//		glBindVertexArray(VAO);
	//		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	//		glBindVertexArray(0);

	//		TwDraw();

	//		SDL_GL_SwapWindow(engine.EngineWindow);

	//		//engine.Render();
	//	}


	//	SDL_StopTextInput();
	//	// Properly de-allocate all resources once they've outlived their purpose
	//	glDeleteVertexArrays(1, &VAO);
	//	glDeleteBuffers(1, &VBO);
	//}


	//// Terminate AntTweakBar
	//TwTerminate();

	//engine.~EngineRenderer();

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
cl_uint _outputImageHeight;
cl_uint _numBScans;
cl_uint _numAScansPerBScan;
cl_uint _ascanAveragingFactor;
cl_uint _bscanAveragingFactor;
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
	err = clGetPlatformIDs(NULL, NULL, &numPlatforms);
	if (err != CL_SUCCESS) return err;
	//
	// Create an array to hold the platforms list
	//
	_platformIDs = (cl_platform_id*)malloc(sizeof(cl_platform_id) * numPlatforms);
	err = clGetPlatformIDs(numPlatforms, _platformIDs, NULL);
	if (err != CL_SUCCESS) return err;
	///
	// Assume that we can use the first platform in the list.  Most systems will have only one platform installed, i.e. NVIDIA
	// although it's possible that the Intel SDK could also be installed.
	//
	if (numPlatforms < 1)
		return -1;
	//
	// Get a list of the platform IDs associated with the first platform
	//
	err = clGetDeviceIDs(_platformIDs[SELECTED_PLATFORM], CL_DEVICE_TYPE_ALL, 0, NULL, &_numDevices);	// Get the number of devices
	if (err < 0)
		return err;
	_deviceIDs = (cl_device_id*)malloc(_numDevices*sizeof(cl_device_id));	// Allocate an array for the devices
	_deviceNameList = (char**)malloc(_numDevices * sizeof(char*));		// Allocate a list of pointers to device names
	err = clGetDeviceIDs(_platformIDs[SELECTED_PLATFORM], CL_DEVICE_TYPE_ALL, _numDevices, _deviceIDs, NULL);	// Populate the array with the device IDs
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
	//
	// Create a context for GPU execution - use the first platform for now
	//
	*clContext = clCreateContext(NULL, 1, &_deviceIDs[_selectedDeviceIndex], NULL, NULL, &err);
	if (err < 0)
		return err;
	//
	// Create a command queue on which to execute the OCT processing pipeline
	//
	*clCommandQueue = clCreateCommandQueue(*clContext, _deviceIDs[_selectedDeviceIndex], 0, &err);
	if (err < 0)
		return err;

	return 0;
}

int clAlloc(
	cl_context context,
	cl_uint inputSpectraLength,		// Specify the actual length of input spectra
	cl_uint outputAScanLength,		// Specify the length of output (outputLength >= inputLength).  If inputlength < outputlength then the input spectra will be zero padded
	cl_uint numBScans,               // Number of B-Scans processed in one go
	cl_uint numAScansPerBScan,
	cl_uint ascanAveragingFactor,
	cl_uint bscanAveragingFactor,
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
	_outputImageHeight = _outputAScanLength / 2;
	_numBScans = numBScans;
	_numAScansPerBScan = numAScansPerBScan;
	_ascanAveragingFactor = ascanAveragingFactor;
	_bscanAveragingFactor = bscanAveragingFactor;
	_imageFormatStride = imageFormatStride;
	//
	// Derived quantities used for allocating memory on the GPU
	//
	_totalAScansPerBScan = (size_t)numAScansPerBScan * (size_t)ascanAveragingFactor;
	_totalBScans = (size_t)_numBScans * (size_t)_bscanAveragingFactor;
	_totalAScans = _totalBScans * _totalAScansPerBScan;
	_totalInputSpectraLength = _totalAScans * (size_t)_inputSpectrumLength;
	_totalOutputAScanLength = _totalAScans * (size_t)_outputAScanLength;
	_totalPreProcessedSpectraLength = _totalOutputAScanLength * (size_t)2;
	_bitmapBScanSize = (size_t)_outputImageHeight * (size_t)_imageFormatStride;
	_bitmapBScanVolumeSize = (size_t)_numBScans * (size_t)_bscanAveragingFactor * _bitmapBScanSize;

	size_t corrSize = (size_t)(_numBScans + 1) * (size_t)_bscanAveragingFactor * _bitmapBScanSize;

	//
	_correlationUsesLogBScans = TRUE;       // default is for correlation mapping to use logarithmic bscans
											//
											// Create a buffer on the device for the input spectra to be copied to
											//
	deviceSpectra = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(short) * _totalInputSpectraLength, NULL, &err);
	if (err != CL_SUCCESS)
		return err;

	//BScanData for correlation
	bScanCorrData = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
		sizeof(unsigned char) * corrSize, NULL, &err);
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
	deviceFilterResults = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*(512 * 500 * (numBScans + 1)),
		NULL, &err);	// Correlation map
	if (err != CL_SUCCESS)
		return err;

	//filterResults
	deviceAvgResults = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*(512 * 500*(numBScans+1)),NULL, &err);	
	if (err != CL_SUCCESS)
		return err;

	//Composite Results 
	deviceCompositeResults = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*(512 * 500 * 4 * (numBScans + 1)), NULL, &err);	
	if (err != CL_SUCCESS)
		return err;
	deviceBscanNoiseList = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*((numBScans + 1)), NULL, &err);	
	if (err != CL_SUCCESS)
		return err;
	deviceCorrNoiseList = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float)*((numBScans + 1)), NULL, &err);
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
	//
	// Enable the openCL library to create a program from the source code
	//
	_clOCTProgram = clCreateProgramWithSource(_context, 1, (const char**)&kernelSource, NULL, &err);
	if (err != CL_SUCCESS)
		return err;

	//
	// Build the kernels that reside within the source
	//
	//err = clBuildProgram(_clOCTProgram, _numDevices, _deviceIDs, NULL, NULL, NULL); 
	//   const char options[] = "-Werror -cl-std=CL1.1";
	//    error = clBuildProgram(program, 1, &device, options, NULL, NULL);
	err = clBuildProgram(_clOCTProgram, NUM_DEVICES_TO_BUILD_FOR, &_deviceIDs[_selectedDeviceIndex], _compilerOptions, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		//
		// If there was an error, get the build log
		//
		clGetProgramBuildInfo(_clOCTProgram, _deviceIDs[_selectedDeviceIndex], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);	// Get the size of the build log
																															//build_log = (char*)malloc(sizeof(char)*log_size + 1);		// Allocate memory for the log
		if (log_size > *buildLogLength)
			build_log = "Insufficient space to store build log.\0";
		else
			clGetProgramBuildInfo(_clOCTProgram, _deviceIDs[_selectedDeviceIndex], CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
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

int SetPreProcessingKernelParameters()
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
	err = clSetKernelArg(_preProcessingKernel, 6, sizeof(cl_uint), &_numBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 7, sizeof(cl_uint), &_numAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 8, sizeof(cl_uint), &_ascanAveragingFactor);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 9, sizeof(cl_uint), &_bscanAveragingFactor);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 10, sizeof(cl_uint), &_windowType);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_preProcessingKernel, 11, sizeof(cl_mem), &devicePreProcessedSpectra);	// real part
	if (err != CL_SUCCESS) return err;
	//err = clSetKernelArg(_preProcessingKernel, 11, sizeof(cl_mem), &devicePreProcessedSpectra[1]);	// imaginary part
	//if (err != CL_SUCCESS) return err;

	return err;


}
int SetPostProcessingKernelParameters()
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
	err = clSetKernelArg(_postProcessingKernel, 3, sizeof(cl_uint), &_numBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 4, sizeof(cl_uint), &_numAScansPerBScan);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 5, sizeof(cl_uint), &_ascanAveragingFactor);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(_postProcessingKernel, 6, sizeof(cl_uint), &_bscanAveragingFactor);
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
int SetCorrelationParameters(cl_uint kernelSizeX,cl_uint kernelsizeY)
{
	cl_int err = 0;
	err = clSetKernelArg(corrKernel, 0, sizeof(cl_mem), &bScanCorrData);
	if (err != CL_SUCCESS) return err;
	size_t totalBScans = _numBScans;
	err = clSetKernelArg(corrKernel, 1, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;

	err = clSetKernelArg(corrKernel, 2, sizeof(cl_uint), &kernelSizeX);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(corrKernel, 3, sizeof(cl_uint), &kernelsizeY);
	if (err != CL_SUCCESS) return err;

	err = clSetKernelArg(corrKernel, 4, sizeof(cl_mem), &deviceCorrelationMap);
	if (err != CL_SUCCESS) return err;

	return err;
}
int SetFilterParameters()
{
	//Filter Kernel
	cl_int err = 0;
	err = clSetKernelArg(filterKernel, 0, sizeof(cl_mem), &deviceAvgResults);
	if (err != CL_SUCCESS) return err;
	size_t totalBScans = _numBScans;
	err = clSetKernelArg(filterKernel, 3, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;

	//Avg Kernel Parameters
	err = clSetKernelArg(avgKernel, 0, sizeof(cl_mem), &bScanCorrData);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(avgKernel, 1, sizeof(cl_uint), &totalBScans);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(avgKernel, 2, sizeof(cl_mem), &deviceAvgResults);
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

	//Opacity Kernel
	err = clSetKernelArg(opacityKernel, 0, sizeof(cl_mem), &deviceCompositeResults);
	if (err != CL_SUCCESS) return err;
	err = clSetKernelArg(opacityKernel, 1, sizeof(cl_uint), &totalBScans);
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

int clOCTInit(cl_uint clDeviceIndex,			// Index in the device list of the OpenCL capable device to use
	cl_uint inputSpectraLength,
	cl_uint outputAScanLength,
	cl_uint numBScans,
	cl_uint numAScansPerBScan,
	cl_uint ascanAveragingFactor,
	cl_uint bscanAveragingFactor,
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
		numBScans,
		numAScansPerBScan,
		ascanAveragingFactor,
		bscanAveragingFactor,
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

//void TestBMP(std::vector<float> &data)
//{
//	std::vector<std::vector<float>> corrCoeff;
//	for (int row = 0; row < 512; row++)
//	{
//		std::vector<float> rowData00;
//
//		for (int col = 0; col < (500 * 3); col += 3)
//		{
//			rowData00.push_back(data[col + (500 * row * 3)]);
//		}
//		corrCoeff.push_back(rowData00);
//	}
//
//	int w = 500;
//	int h = 512;
//
//	FILE *f = nullptr;
//	if (f)
//		free(f);
//
//	unsigned char *img = NULL;
//	int filesize = 54 + 3 * w*h;  //w is your image width, h is image height, both int
//	if (img)
//		free(img);
//	img = (unsigned char *)malloc(3 * w*h);
//	memset(img, 0, sizeof(img));
//
//	for (int xCor = 0; xCor < w; xCor++)
//	{
//		for (int yCor = 0; yCor < h; yCor++)
//		{
//			int x = xCor;
//			int y = (h - 1) - yCor;
//			int r = corrCoeff[yCor][xCor];
//			int g = corrCoeff[yCor][xCor];
//			int b = corrCoeff[yCor][xCor];
//			if (r > 255) r = 255;
//			if (g > 255) g = 255;
//			if (b > 255) b = 255;
//			img[(x + y*w) * 3 + 2] = (unsigned char)(r);
//			img[(x + y*w) * 3 + 1] = (unsigned char)(g);
//			img[(x + y*w) * 3 + 0] = (unsigned char)(b);
//		}
//	}
//
//	unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
//	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
//	unsigned char bmppad[3] = { 0,0,0 };
//
//	bmpfileheader[2] = (unsigned char)(filesize);
//	bmpfileheader[3] = (unsigned char)(filesize >> 8);
//	bmpfileheader[4] = (unsigned char)(filesize >> 16);
//	bmpfileheader[5] = (unsigned char)(filesize >> 24);
//
//	bmpinfoheader[4] = (unsigned char)(w);
//	bmpinfoheader[5] = (unsigned char)(w >> 8);
//	bmpinfoheader[6] = (unsigned char)(w >> 16);
//	bmpinfoheader[7] = (unsigned char)(w >> 24);
//	bmpinfoheader[8] = (unsigned char)(h);
//	bmpinfoheader[9] = (unsigned char)(h >> 8);
//	bmpinfoheader[10] = (unsigned char)(h >> 16);
//	bmpinfoheader[11] = (unsigned char)(h >> 24);
//
//
//	std::string str = "./results/aaaaaaaa" +
//		std::to_string(1)
//		+ ".bmp";
//	const char* gfgfg = str.c_str();
//
//	f = fopen(gfgfg, "wb");
//	fwrite(bmpfileheader, 1, 14, f);
//	fwrite(bmpinfoheader, 1, 40, f);
//	for (int imgIndex = 0; imgIndex < h; imgIndex++)
//	{
//		fwrite(img + (w*(imgIndex - 1) * 3), 3, w, f);
//		fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
//	}
//
//	free(img);
//
//	fclose(f);
//
//}

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

int FilterPostProcess(int batchNum,int batchSize,std::vector<float> &correlationResults,
	std::vector<float> &compositeResults)
{
	printf("Performing Post processing on results \n");

	//Calculate BScan Averages
	cl_int err;
	size_t numWorkItemsPerGroup = 1;
	//    size_t numWorkGroups;
	size_t totalWorkItems = 500 * 512;

	err = clSetKernelArg(avgKernel, 1, sizeof(cl_uint), &batchSize);

	err = clEnqueueNDRangeKernel(_commandQueue, avgKernel, 1, NULL, 
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	std::vector<float> avgResults(500*512*(batchSize-1));

	err = clEnqueueReadBuffer(_commandQueue, deviceAvgResults, CL_TRUE, 0, 
		sizeof(float)*(500 * 512 * (batchSize - 1)), avgResults.data(), 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;


	//Perform median filtering
	err = clSetKernelArg(filterKernel, 3, sizeof(cl_uint), &batchSize);

	err = clEnqueueNDRangeKernel(_commandQueue, filterKernel, 1, NULL,
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	std::vector<float> filterResults(500 * 512 * (batchSize - 1));

	err = clEnqueueReadBuffer(_commandQueue, deviceAvgResults, CL_TRUE, 0,
		sizeof(float)*(500 * 512 * (batchSize - 1)), filterResults.data(), 0, NULL, NULL);
	if (err != CL_SUCCESS) 
		return err;


	int topIgnoreSize = 15 * 500;
	int dataIgnoreSize = 199 * 500;
	int topIgnoreCorr = 14 * 500;

	std::vector<float> BScanNoiseList;
	std::vector<float> CorrNoiseList;

	for (int bScanNum = 0; bScanNum < batchSize - 1; bScanNum++)
	{
		//Threshold BScancs
		std::vector<float> avgBScan(filterResults.begin() + (500*512*bScanNum)
			, filterResults.begin() + (500*512*(bScanNum+1)));

		std::vector<float> noiseDataBScan;

		for (int i = 0; i < batchSize; i++)
		{
			noiseDataBScan.insert(noiseDataBScan.end(),
				&avgBScan[((i * 500) + topIgnoreSize + dataIgnoreSize)],
				&avgBScan[((i * 500) + topIgnoreSize + dataIgnoreSize) + (500)]);
		}

		double sumBScan = std::accumulate(noiseDataBScan.begin(), noiseDataBScan.end(), 0.0);
		double meanBScan = sumBScan / noiseDataBScan.size();

		std::vector<double> diff(noiseDataBScan.size());
		std::transform(noiseDataBScan.begin(), noiseDataBScan.end(), diff.begin(),
			std::bind2nd(std::minus<double>(), meanBScan));
		double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
		double stdevBScan = std::sqrt(sq_sum / noiseDataBScan.size());

		float test = standard_deviation(noiseDataBScan);

		float noiseFloorBScan = meanBScan + 3 * stdevBScan;
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
		std::vector<float> corrResult(correlationResults.begin() + (500 * 512 * bScanNum)
			, correlationResults.begin() + (500 * 512 * (bScanNum + 1)));

		for (int i = 0; i < 10; i++)
		{
			noiseDataCorr.insert(noiseDataCorr.end(),
				&corrResult[(i * 500) + topIgnoreCorr],
				&corrResult[((i * 500) + topIgnoreCorr) + (500)]);
		}


		double sumCorr = std::accumulate(noiseDataCorr.begin(), noiseDataCorr.end(), 0.0f);
		double meanCorr = sumCorr / noiseDataCorr.size();
		CorrNoiseList.push_back(meanCorr);

		//std::replace_if(corrResult.begin(), corrResult.end(),
		//	[meanCorr](float value)
		//{
		//	if (value < meanCorr)
		//		return true;
		//	else
		//		return false;
		//}
		//, 0);

		////Now use the B-Scan to filter only the areas of the correlation map
		////that are contributed to by actual OCT signal rather than  noise.
		//std::vector<float> cmOCTFiltered = corrResult;
		//for (int i = 0; i < cmOCTFiltered.size(); i++)
		//{
		//	if (i < (400 * 500)) {
		//		if (avgBScan[i+(500)] == 0)
		//			cmOCTFiltered[i] = 0;
		//	}
		//}

		//The filtered cmOCT gives the location of the vasculature
		//Now extract the corresponding regions from the original OCT data
		//std::vector<float> originalBScan(filterResults.begin() + (500 * 512 * bScanNum)
		//	, filterResults.begin() + (500 * 512 * (bScanNum + 1)));

		//std::vector<float> vasculature(500 * 512 * 3);

		//for (int i = 0; i < cmOCTFiltered.size(); i++)
		//{
		//	vasculature[i * 3] = originalBScan[i];
		//	vasculature[(i * 3) + 1] = originalBScan[i];
		//	vasculature[(i * 3) + 2] = originalBScan[i];

		//	if (i >= 500) {
		//		if (cmOCTFiltered[i - 500] == 0) {
		//			vasculature[i * 3] = 0;
		//			vasculature[(i * 3) + 1] = 0;
		//			vasculature[(i * 3) + 1] = 0;
		//		}
		//	}
		//}

		//int ignoreSize = 16 * 500;
		//std::vector<unsigned char> testVec;
		//for (int i = 0; i < (500 * 512); i++)
		//{
		//	if (i >= ignoreSize &&
		//		i < ((200 * 500) + ignoreSize)) {
		//		testVec.push_back(vasculature[i*3]);
		//		//testVec.push_back(vasculature[i*3]);
		//		//testVec.push_back(vasculature[i*3]);
		//	}
		//}

		//std::string str = "./results/vas" +
		//	std::to_string(0 + (0 * 1000) + 2)
		//	+ ".bmp";
		//const char* fileName = str.c_str();
		//SaveBitmapVec(testVec.data(), 500, (200), 500 * (200) * 3, fileName);

		////*********************************** Compositing *******************************************
		//std::vector<float> composite(500 * 512 * 4);

		//for (int i = 0; i < (500 * 512); i++)
		//{
		//	if (vasculature[i * 3] > 0) {
		//		composite[i * 4] = vasculature[i*3];
		//		composite[(i * 4) + 1] = 0;
		//		composite[(i * 4) + 2] = 0;
		//	}
		//	else
		//	{
		//		composite[i * 4] = originalBScan[i];
		//		composite[(i * 4) + 1] = originalBScan[i];
		//		composite[(i * 4) + 2] = originalBScan[i];
		//	}

		//	if (originalBScan[i] < 50 || i < (15*500))
		//		composite[(i * 4) + 3] = 0;
		//	else
		//		composite[(i * 4) + 3] = originalBScan[i];
		//}

		//compositeResults.insert(compositeResults.end(),
		//	composite.begin(),composite.end());

		/*std::vector<float> dsds(composite.begin(),
			composite.begin() + (500 * 512 * 4));
		TestBMP(dsds,bScanNum+(batchNum*1000));*/
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

	std::vector<float> compositeOutput(500 * 512 * 4 * (batchSize - 1));

	//err = clEnqueueReadBuffer(_commandQueue, deviceCompositeResults, CL_TRUE, 0,
		//sizeof(float)*(500 * 512 * 4 * (batchSize - 1)), compositeOutput.data(), 0, NULL, NULL);
	//if (err != CL_SUCCESS) return err;

	//Set Opacity
	err = clSetKernelArg(opacityKernel, 1, sizeof(cl_uint), &batchSize);

	err = clEnqueueNDRangeKernel(_commandQueue, opacityKernel, 1, NULL,
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);
	if (err != CL_SUCCESS)
		return err;

	err = clEnqueueReadBuffer(_commandQueue, deviceCompositeResults, CL_TRUE, 0,
	sizeof(float)*(500 * 512 * 4 * (batchSize - 1)), compositeOutput.data(), 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	compositeResults.insert(compositeResults.end(),
		compositeOutput.begin(),
		compositeOutput.end());

	//for (int bScanNum = 0; bScanNum < batchSize - 1; bScanNum++)
	//{
	//	std::vector<float> dsds(compositeOutput.begin() + (bScanNum * (500*512*4)),
	//		compositeOutput.begin() + (500 * 512 * 4) + (bScanNum * (500 * 512 * 4)));
	//	TestBMP(dsds,bScanNum+(batchNum*1000));
	//}

	return 0;
}

int ComputeCorrelation(int batchNum, int batchSize,std::vector<float>& correlationResults)
{
	bool saveBMP = true;
	cl_int err;
	size_t numWorkItemsPerGroup = 1;
	size_t totalWorkItems = 500 * 512;

	err = clEnqueueNDRangeKernel(_commandQueue, corrKernel, 1, NULL, 
		&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);

	if (err != CL_SUCCESS)
		return err;

	float* correlationMap;
	correlationMap = (float*)malloc(sizeof(float) * _totalAScans * _outputAScanLength);

	err = clEnqueueReadBuffer(_commandQueue, deviceCorrelationMap, CL_TRUE, 0, sizeof(float)*_totalAScans * _outputAScanLength, correlationMap, 0, NULL, NULL);
	if (err != CL_SUCCESS) return err;

	std::vector<float> results(correlationMap, correlationMap + (totalWorkItems*batchSize));

	correlationResults = std::vector<float>(results.begin(),
		results.end() - totalWorkItems);

	if (!saveBMP) {
		for (int bitMapNum = 0; bitMapNum < batchSize - 1; bitMapNum++)
		{

			std::vector<std::vector<float>> corrCoeff;
			for (int row = 0; row < 512; row++)
			{
				std::vector<float> rowData00;

				for (int col = 0; col < 500; col += 1)
				{
					rowData00.push_back(results[col + (500 * row) +
						(bitMapNum * totalWorkItems)]);
				}
				corrCoeff.push_back(rowData00);
			}

			int ignoreSize = 14 * 500;
			std::vector<unsigned char> testVec;
			for (int i = 0; i < (500 * 512);i++)
			{
				if (i >= ignoreSize &&
					i < ((200*500)+ignoreSize)) {
					testVec.push_back(results[i + (bitMapNum * totalWorkItems)]);
					testVec.push_back(results[i + (bitMapNum * totalWorkItems)]);
					testVec.push_back(results[i + (bitMapNum * totalWorkItems)]);
				}
			}

			std::string str = "./results/cmOCT" +
				std::to_string(bitMapNum + (batchNum * 1000) + 2)
				+ ".bmp";
			const char* fileName = str.c_str();
			SaveBitmapVec(testVec.data(), 500, (200), 500 * (200) * 3,fileName);

			if (!saveBMP) {

				int w = 499;
				int h = 511;

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

int CalculateCoeff(int i1p1, int i1p2, int i1p3, int i1p4, float i1Mean,
	int i2p1, int i2p2, int i2p3, int i2p4, float i2Mean)
{
	int result = 0;

	int numerator = 0;
	int denominator00 = 0;
	int denominator01 = 0;

	float diffI1 = 0;
	float diffI2 = 0;

	diffI1 = i1p1 - i1Mean;
	diffI2 = i2p1 - i2Mean;

	numerator += diffI1 * diffI2;
	denominator00 += diffI1 * diffI1;
	denominator01 += diffI2 * diffI2;

	diffI1 = i1p2 - i1Mean;
	diffI2 = i2p2 - i2Mean;

	numerator += diffI1 * diffI2;
	denominator00 += diffI1 * diffI1;
	denominator01 += diffI2 * diffI2;

	diffI1 = i1p3 - i1Mean;
	diffI2 = i2p3 - i2Mean;

	numerator += diffI1 * diffI2;
	denominator00 += diffI1 * diffI1;
	denominator01 += diffI2 * diffI2;

	diffI1 = i1p4 - i1Mean;
	diffI2 = i2p4 - i2Mean;

	numerator += diffI1 * diffI2;
	denominator00 += diffI1 * diffI1;
	denominator01 += diffI2 * diffI2;

	float coeff = numerator / (sqrt(denominator00) * sqrt(denominator01));

	if (numerator == 0 || denominator00 == 0 || denominator01 == 0)
	{
		result = 0;
	}
	else
		result = abs(1 - coeff) * 255;

	return result;
}

void EngineOCT::OpenCLCompute()
{
	cl_uint clDeviceIndex = GPU_DEVICE_INDEX;

	size_t localWorkSize = 2;
	char compilerOptions[] = "-cl-fast-relaxed-math -cl-mad-enable";

	unsigned int inputLen;
	unsigned int outputLen;
	unsigned int outputImageHeight;

	unsigned int totalBScans = 500;
	unsigned int numBScansPerBatch = 25;
	unsigned int numBScanProcessingIteratations = (unsigned int)floor(totalBScans / numBScansPerBatch);
	unsigned int numAScans = 500;
	unsigned int ascanAve = 1;
	unsigned int bscanAve = 1;

	float bmpMinVal = -155;
	float bmpMaxVal = -55;

	this->KernelSizeX = 6;
	this->KernelSizeY = 6;
	this->FilterWindowX = 3;
	this->FilterWindowY = 3;

	unsigned int saveBmp = true;

	size_t loglen = 1000000;
	size_t totalBScanSize;
	size_t totalBScanVolumeSize;
	cl_uint numAScansPerBScan;
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

	kernelPath = "test.cl";
	rootPath = "./test/";

	//
	// Try to read the reference spectrum and resampling table
	//
	inputLen = ReferenceAScanData.size();
	outputLen = ReferenceAScanData.size();

	outputImageHeight = outputLen / 2;

	numAScansPerBScan = numAScans * ascanAve * bscanAve;
	totalAScans = numBScansPerBatch * numAScansPerBScan;
	totalInputLen = inputLen * totalAScans;
	totalOutputLen = outputLen * totalAScans;
	stride = 1500;
	totalBScanSize = (size_t)stride * (size_t)outputImageHeight;
	totalBScanVolumeSize = totalBScanSize * (size_t)totalBScans;
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
		numBScansPerBatch,
		numAScans,
		ascanAve,
		bscanAve,
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
			totalBScans, numBScansPerBatch, totalAScans);

		//numBScanProcessingIteratations = 4;

		for (i = 0; i < numBScanProcessingIteratations; i++)
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

			res = PostProcess(bmpMinVal, bmpMaxVal);
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
					bscanBmp + (totalBScanSize * numBScansPerBatch));
				BScanResults.insert(BScanResults.end(), tempBMPDat.begin(), tempBMPDat.end());

				printf("\n");
				//for (j = 0; j < 10; j++)
				//{
				//	//printf("%f\n", logEnv[j]);
				//}

				if (!saveBmp) {
					for (j = 0; j < numBScansPerBatch; j++)
					{


						sprintf(bmpPath, "%sbscan%0.4i.bmp\0", rootPath, i*numBScansPerBatch + j);
						//SaveBitmapFromFloatArray(bmpPath,&logEnv[j*numAScansPerBScan*outputLen], (unsigned short)numAScansPerBScan, outputLen, outputLen/2, bmpMinVal, bmpMaxVal);

						SaveBitmap(bmpPath, &bscanBmp[j*totalBScanSize], numAScansPerBScan, outputImageHeight, totalBScanSize);
						sprintf(bmpPath, "%scorrelationMap%0.4i.bmp\0", rootPath, i*numBScansPerBatch + j);
					}

				}
			}
			printf("%f%% complete\n", (float)i /
				(float)numBScanProcessingIteratations*100.0f);


		}

		printf("Processing B Scans: SUCCESS \n");

		this->numBScanProcessingIteratations = numBScanProcessingIteratations;
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

	printf("CL compute Done.\n");
}

void EngineOCT::ComputeCrossCorrelation()
{
	int res;
	this->CompositeResults.clear();
	this->CompositeResults.clear();

	for (int batchNum = 0; batchNum < numBScanProcessingIteratations; batchNum++) {

		printf("Performing Cross Correlation batch %d of %d \n",
			(batchNum + 1), numBScanProcessingIteratations);

		cl_uint batchSizePlus;
		unsigned char* tempBScanData;
		int volumnSizePlus;
		int size;


		if (batchNum == numBScanProcessingIteratations - 1)
		{
			//Copy top 50 bScans to GPU
			batchSizePlus = _numBScans;
			volumnSizePlus = 500 * 512 * batchSizePlus * 3;

			size = (sizeof(unsigned char) * volumnSizePlus);
			tempBScanData = (unsigned char*)malloc(size);

			int index00 = (_numBScans * batchNum);
			int index01 = (_numBScans * (batchNum + 1));

			std::copy(BScanResults.begin() + (index00 * 512 * 500 * 3),
				BScanResults.begin() + (index01 * 512 * 500 * 3),
				tempBScanData);

		}
		else
		{
			//Copy top 51 bScans to GPU
			batchSizePlus = _numBScans + 1;
			volumnSizePlus = 500 * 512 * batchSizePlus * 3;

			size = (sizeof(unsigned char) * volumnSizePlus);
			tempBScanData = (unsigned char*)malloc(size);

			int index00 = (_numBScans * batchNum);
			int index01 = (_numBScans * (batchNum + 1)) + 1;

			std::copy(BScanResults.begin() + (index00 * 512 * 500 * 3),
				BScanResults.begin() + (index01 * 512 * 500 * 3),
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





