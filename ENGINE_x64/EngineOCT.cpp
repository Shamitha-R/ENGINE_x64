#include "EngineOCT.h"
#include "EngineRenderer.h" 
#include "UI.h"

#include <clFFT.h>

#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)
#define INTERPOLATION_ORDER 2

void EngineOCT::LoadFileData(std::string fileName,std::vector<float> &data)
{
	std::string currentLine;
	std::ifstream fileStreamer;

	fileStreamer.open(fileName);

	if(fileStreamer.is_open())
	{
		int splitLoc = 0;

		while (getline(fileStreamer, currentLine)) {
			splitLoc = currentLine.find(',', 0);
			data.push_back(std::stod(currentLine.substr(splitLoc + 1)));
		}

		fileStreamer.close();
		std::cout << fileName + " successfully read\n";

	}else std::cout << "Unable to open file";
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

	std::vector<short> test = readFile("Spectra.bin");
	HostSpectraData = test;
	std::cout << "Spectra.bin successfully read\n";
}

int TW_CALL TwEventSDL20(const void *sdlEvent)
{
	int handled = 0;
	const SDL_Event *event = (const SDL_Event *)sdlEvent;

	if (event == NULL)
		return 0;

	switch (event->type)
	{
	case SDL_KEYDOWN:
		if (event->key.keysym.sym & SDLK_SCANCODE_MASK) {
			int key = 0;
			switch (event->key.keysym.sym) {
			case SDLK_UP:
				key = TW_KEY_UP;
				break;
			case SDLK_DOWN:
				key = TW_KEY_DOWN;
				break;
			case SDLK_RIGHT:
				key = TW_KEY_RIGHT;
				break;
			case SDLK_LEFT:
				key = TW_KEY_LEFT;
				break;
			case SDLK_INSERT:
				key = TW_KEY_INSERT;
				break;
			case SDLK_HOME:
				key = TW_KEY_HOME;
				break;
			case SDLK_END:
				key = TW_KEY_END;
				break;
			case SDLK_PAGEUP:
				key = TW_KEY_PAGE_UP;
				break;
			case SDLK_PAGEDOWN:
				key = TW_KEY_PAGE_DOWN;
				break;
			default:
				if (event->key.keysym.sym >= SDLK_F1 &&
					event->key.keysym.sym <= SDLK_F12) {
					key = event->key.keysym.sym + TW_KEY_F1 - SDLK_F1;
				}
				break;
			}
			if (key != 0) {
				handled = TwKeyPressed(key, event->key.keysym.mod);
			}
		}
		else {
			handled = TwKeyPressed(event->key.keysym.sym /*& 0xFF*/,
				event->key.keysym.mod);
		}
		break;
	case SDL_MOUSEMOTION:
		handled = TwMouseMotion(event->motion.x, event->motion.y);
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		if (event->type == SDL_MOUSEBUTTONDOWN &&
			(event->button.button == 4 || event->button.button == 5)) {
			// mouse wheel
			static int s_WheelPos = 0;
			if (event->button.button == 4)
				++s_WheelPos;
			else
				--s_WheelPos;
			handled = TwMouseWheel(s_WheelPos);
		}
		else {
			handled = TwMouseButton(
				(event->type == SDL_MOUSEBUTTONUP) ?
				TW_MOUSE_RELEASED : TW_MOUSE_PRESSED,
				(TwMouseButtonID)event->button.button);
		}
		break;
	case SDL_WINDOWEVENT:
		if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
			// tell the new size to TweakBar
			TwWindowSize(event->window.data1, event->window.data2);
			// do not set 'handled'
			// SDL_VIDEORESIZE may be also processed by the calling application
		}
		break;
	}

	return handled;
}

void EngineRendering(EngineOCT &oct)
{
	EngineRenderer engine;

	if (!engine.InitializeEngine())
	{
		printf("Engine Initialization Failed\n");
	}
	else
	{
		UI engineUI;
		engineUI.InitialiseUI(engine.SCREENWIDTH, engine.SCREENHEIGHT,oct);

		// Build and compile our shader program
		Shader ourShader("shaders/vertexshader.vs", "shaders/fragmentshader.fs");

		// Set up vertex data (and buffer(s)) and attribute pointers
		GLfloat vertices[] = {
			// Positions          // Colors           // Texture Coords
			0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right
			0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Bottom Right
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Bottom Left
			-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // Top Left 
		};
		GLuint indices[] = {  // Note that we start from 0!
			0, 1, 3, // First Triangle
			1, 2, 3  // Second Triangle
		};
		GLuint VBO, VAO, EBO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
		// TexCoord attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0); // Unbind VAO


							  // Load and create a texture 
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture); // All upcoming GL_TEXTURE_2D operations now have effect on this texture object
											   // Set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// Set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Load image, create texture and generate mipmaps
		SDL_Surface* image = engine.LoadSurface("textures/wall.png");
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
		glGenerateMipmap(GL_TEXTURE_2D);

		SDL_FreeSurface(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.


		SDL_Event e;
		SDL_StartTextInput();
		int test;
		float val = 0.0f;
		bool quit = false;

		while (!quit)
		{

			while (SDL_PollEvent(&e) != 0)
			{
				//test = TwEventSDL20(&e, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
				test = TwEventSDL20(&e);

				if (!test) {

					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					else if (e.type == SDL_TEXTINPUT)
					{
						int x = 0, y = 0;
						SDL_GetMouseState(&x, &y);
						engine.HandleInput(e.text.text[0], x, y);
					}
				}
			}

			// Render
			// Clear the color buffer
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);


			// Bind Textures using texture units
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			glUniform1i(glGetUniformLocation(ourShader.Program, "ourTexture1"), 0);

			// Activate shader
			ourShader.Use();

			// Create transformations
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 projection;

			val += 0.01f;

			model = glm::rotate(model, val, glm::vec3(0.0f, 1.0f, 0.0f));
			view = glm::translate(view, glm::vec3(0.0f, 0.0f, -2.0f));
			projection = glm::perspective(70.0f, (GLfloat)800 / (GLfloat)600, 0.1f, 100.0f);
			// Get their uniform location
			GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
			GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
			GLint projLoc = glGetUniformLocation(ourShader.Program, "projection");
			// Pass them to the shaders
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
			// Note: currently we set the projection matrix each frame, but since the projection matrix rarely changes it's often best practice to set it outside the main loop only once.
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

			// Draw container
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			TwDraw();

			SDL_GL_SwapWindow(engine.EngineWindow);

			//engine.Render();
		}


		SDL_StopTextInput();

		// Properly de-allocate all resources once they've outlived their purpose
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}


	// Terminate AntTweakBar
	TwTerminate();

	engine.~EngineRenderer();
}

void PreComputeInterpolationCoefficients(std::vector<float> &resamplingTable, std::vector<float> &coefs, int ResamplingTableLength)
{
	//
	// Use a quadratic interpolation
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
		}
	}
}

void EngineOCT::OpenCLCompute()
{
	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_mem Amobj = NULL;
	cl_mem Bmobj = NULL;
	cl_mem Cmobj = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret;

	//Proccessing Parameters
	cl_uint inputSpectraLength = ResamplingTableData.size();
	cl_uint outputAScanLength = ReferenceAScanData.size();
	cl_uint bScanCount = 50;
	cl_uint aScanBScanRatio = 500;
	cl_uint aScanAveragingFactor = 1;
	cl_uint bScanAveragingFactor = 1;
	unsigned int outputImageHeight;// = outputLen/2;

	unsigned int totalBScans = 100;
	unsigned int numBScansPerBatch = 50;
	unsigned int numBScanProccesingIterations = (unsigned int)floor(totalBScans / bScanCount);
	unsigned int numAScans = 500;

	size_t localWorkSize = 2;
	size_t totalBScanSize;
	size_t totalBScanVolumeSize;
	size_t fftLength = size_t(outputAScanLength);
	size_t tempBufferSize;
	cl_mem deviceTemporaryBuffer;


	cl_uint numAScansPerBScan;
	cl_uint totalAScans; // = numAScans * numBScans * ascanAve * bscanAve;
	cl_uint totalInputLen; // = inputLen * totalAScans;
	cl_uint totalOutputLen; // = outputLen * totalAScans;
	cl_uint totalBatchLength;    // Number of array elements in a batch of bscans
	cl_uint stride = 1500; //= 3*numAScans*ascanAve;

	cl_uint windowType = 0;
	cl_uint imageFormatStride = stride;

	char string[MEM_SIZE];

	numAScansPerBScan = aScanBScanRatio * aScanAveragingFactor * bScanAveragingFactor;
	totalAScans = bScanCount * numAScansPerBScan;
	totalInputLen = inputSpectraLength * totalAScans;
	totalOutputLen = outputAScanLength * totalAScans;

	outputImageHeight = outputAScanLength / 2;

	numAScansPerBScan = numAScans * aScanAveragingFactor * bScanAveragingFactor;
	totalAScans = numBScansPerBatch * numAScansPerBScan;
	totalInputLen = inputSpectraLength * totalAScans;
	totalOutputLen = outputAScanLength * totalAScans;

	totalBScanSize = (size_t)stride * (size_t)outputImageHeight;

	//resampTable = (float*)malloc(sizeof(float) * inputLen);
	//refSpec = (float*)malloc(sizeof(float) * inputLen);
	//refAScan = (float*)malloc(sizeof(float) * inputLen);
	//inputSpectra = (short*)malloc(sizeof(short) * totalInputLen);
	//buildLog = (char*)malloc(sizeof(char) * loglen);

	totalBScanVolumeSize = totalBScanSize * (size_t)totalBScans;

	FILE *fp;
	char fileName[] = "./test.cl";
	char *source_str;
	size_t source_size;

	/* Load the source code containing the kernel*/
	fp = fopen(fileName, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/* Get Platform and Device Info */
	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, nullptr, &platformIdCount);

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);

	ret = clGetDeviceIDs(platformIds[2], CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	//Device settings
	char* value;
	size_t valueSize;
	clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &valueSize);
	value = (char*)malloc(valueSize);
	clGetDeviceInfo(device_id, CL_DEVICE_NAME, valueSize, value, NULL);
	printf("Selected OpenCL Device: %s\n",  value);
	free(value);

	/* Create OpenCL context */
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

	/* Create Command Queue */
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	//Calcaulate interpolation coefficients
	std::vector<float> interpolationMatrix(sizeof(float) * (size_t)inputSpectraLength * (INTERPOLATION_ORDER + 1) * (INTERPOLATION_ORDER + 1));
	PreComputeInterpolationCoefficients(ResamplingTableData, interpolationMatrix, inputSpectraLength);

	//Allocate GPU Memory
	cl_int err = 0;
	cl_mem deviceSpectra;
	cl_mem deviceResamplingTable;
	cl_mem deviceInterpolationMatrix;
	cl_mem deviceReferenceSpectrum;
	cl_mem deviceReferenceAScan;
	
	cl_mem devicePreProcessedSpectra;	
	cl_mem deviceFourierTransform;

	cl_mem deviceEnvBScan;
	cl_mem deviceLogEnvBScan;
	cl_mem deviceSum;
	cl_mem deviceSAM;
	cl_mem deviceAttenuationDepth;
	cl_mem deviceBScanBmp;

	//Output Memory Buffers
	size_t totalPreProcessedSpectraLength;
	size_t totalOutputAScanLength;
	size_t bitmapBScanVolumeSize;
	size_t bitmapBScanSize;
	size_t totalInputSpectraLength;

	totalInputSpectraLength = totalAScans * (size_t)inputSpectraLength;
	totalOutputAScanLength = totalAScans * (size_t)outputAScanLength;
	totalPreProcessedSpectraLength = totalOutputAScanLength * (size_t)2;

	outputImageHeight = outputAScanLength / 2;
	bitmapBScanSize = (size_t)outputImageHeight * (size_t)imageFormatStride;
	bitmapBScanVolumeSize = (size_t)bScanCount * (size_t)bScanAveragingFactor * bitmapBScanSize;


	//
	FILE* inputSpecFile;
	inputSpecFile = fopen("spectra.bin", "rb");
	short* inputSpectra;
	inputSpectra = (short*)malloc(sizeof(short) * totalInputLen);

	fseek(inputSpecFile, sizeof(short) * totalInputLen * 0, SEEK_SET);
	fread(inputSpectra, sizeof(short), totalInputLen, inputSpecFile);

	deviceSpectra = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
		sizeof(short) * totalInputSpectraLength, &inputSpectra, &err);
	deviceResamplingTable = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
		sizeof(float) * ResamplingTableData.size(), &ResamplingTableData, &err);
	deviceInterpolationMatrix = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
		sizeof(float) * interpolationMatrix.size(),&interpolationMatrix, &err);
	deviceReferenceSpectrum = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
		sizeof(float) * ReferenceSpectrumData.size(), &ReferenceSpectrumData, &err);
	deviceReferenceAScan = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
		sizeof(float) * ReferenceAScanData.size(), &ReferenceAScanData, &err);

	devicePreProcessedSpectra = clCreateBuffer(context, CL_MEM_READ_WRITE, 
		sizeof(float) * totalPreProcessedSpectraLength, NULL, &err);	// Complex interleaved pre-processed spectra
	deviceFourierTransform = clCreateBuffer(context, CL_MEM_READ_WRITE, 
		sizeof(float) * totalPreProcessedSpectraLength, NULL, &err);	// Complex interleaved
	deviceEnvBScan = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
		sizeof(float)*totalOutputAScanLength, NULL, &err);	// Envelope of the fft
	deviceLogEnvBScan = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
		sizeof(float)*totalOutputAScanLength, NULL, &err);	// Logarithmic Envelope of the fft
	deviceSum = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
		sizeof(float) * totalAScans, NULL, &err);	// Sum along each a-scan
	deviceSAM = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
		sizeof(float) * totalAScans, NULL, &err);	// attenuation measured along each a-scan
	deviceAttenuationDepth = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
		sizeof(float) * totalAScans, NULL, &err);	// attenuation depth along each a-scan
	deviceBScanBmp = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
		sizeof(unsigned char) * bitmapBScanVolumeSize, NULL, &err);	// BScan bitmap images


	//Build Kernels from source
	program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &err);
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	//Create each kernel
	cl_kernel preProcessingKernel;
	cl_kernel postProcessingKernel;



	preProcessingKernel = clCreateKernel(program, "octPreProcessingKernel", &err);
	postProcessingKernel = clCreateKernel(program, "octPostProcessingKernel", &err);

	//Set Kernel Parameters
	err = clSetKernelArg(preProcessingKernel, 0, sizeof(cl_mem), &deviceSpectra);
	err = clSetKernelArg(preProcessingKernel, 1, sizeof(cl_mem), &deviceResamplingTable);
	err = clSetKernelArg(preProcessingKernel, 2, sizeof(cl_mem), &deviceInterpolationMatrix);
	err = clSetKernelArg(preProcessingKernel, 3, sizeof(cl_mem), &deviceReferenceSpectrum);
	err = clSetKernelArg(preProcessingKernel, 4, sizeof(cl_uint), &inputSpectraLength);
	err = clSetKernelArg(preProcessingKernel, 5, sizeof(cl_uint), &outputAScanLength);
	err = clSetKernelArg(preProcessingKernel, 6, sizeof(cl_uint), &bScanCount);
	err = clSetKernelArg(preProcessingKernel, 7, sizeof(cl_uint), &aScanBScanRatio);
	err = clSetKernelArg(preProcessingKernel, 8, sizeof(cl_uint), &aScanAveragingFactor);
	err = clSetKernelArg(preProcessingKernel, 9, sizeof(cl_uint), &bScanAveragingFactor);
	err = clSetKernelArg(preProcessingKernel, 10, sizeof(cl_uint), &windowType);
	err = clSetKernelArg(preProcessingKernel, 11, sizeof(cl_mem), &devicePreProcessedSpectra);	// real part

	err = clSetKernelArg(postProcessingKernel, 0, sizeof(cl_mem), &deviceFourierTransform);
	err = clSetKernelArg(postProcessingKernel, 1, sizeof(cl_mem), &deviceReferenceAScan);
	err = clSetKernelArg(postProcessingKernel, 2, sizeof(cl_uint), &outputAScanLength);
	err = clSetKernelArg(postProcessingKernel, 3, sizeof(cl_uint), &bScanCount);
	err = clSetKernelArg(postProcessingKernel, 4, sizeof(cl_uint), &aScanBScanRatio);
	err = clSetKernelArg(postProcessingKernel, 5, sizeof(cl_uint), &aScanAveragingFactor);
	err = clSetKernelArg(postProcessingKernel, 6, sizeof(cl_uint), &bScanAveragingFactor);
	err = clSetKernelArg(postProcessingKernel, 7, sizeof(cl_uint), &imageFormatStride);	// Envelope of BScan
	err = clSetKernelArg(postProcessingKernel, 10, sizeof(cl_mem), &deviceEnvBScan);	// Envelope of BScan
	err = clSetKernelArg(postProcessingKernel, 11, sizeof(cl_mem), &deviceLogEnvBScan);	// Log envelope - usually used for B-Scan images
	err = clSetKernelArg(postProcessingKernel, 12, sizeof(cl_mem), &deviceSum);	// Log envelope - usually used for B-Scan images
	err = clSetKernelArg(postProcessingKernel, 13, sizeof(cl_mem), &deviceSAM);	// Log envelope - usually used for B-Scan images
	err = clSetKernelArg(postProcessingKernel, 14, sizeof(cl_mem), &deviceAttenuationDepth);	// Log envelope - usually used for B-Scan images
	err = clSetKernelArg(postProcessingKernel, 15, sizeof(cl_mem), &deviceBScanBmp);	// Log envelope - usually used for B-Scan images
	
	float bmpMinVal = -155;    // Threshold values for storing log images as bitmaps
	float bmpMaxVal = -55;


	//CL FFT
	clfftStatus status;
	clfftSetupData clfftSetupData;
	clfftPlanHandle clfftPlanHandle;

	status = clfftInitSetupData(&clfftSetupData);
	status = clfftSetup(&clfftSetupData);
	status = clfftCreateDefaultPlan(&clfftPlanHandle, context, CLFFT_1D, &fftLength);

	status = clfftSetResultLocation(clfftPlanHandle, CLFFT_OUTOFPLACE);
	status = clfftSetLayout(clfftPlanHandle, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);	// Currently complex to complex transform... 
	status = clfftSetPlanBatchSize(clfftPlanHandle, totalAScans);
	status = clfftSetPlanPrecision(clfftPlanHandle, CLFFT_SINGLE);
	status = clfftBakePlan(clfftPlanHandle, 1, &command_queue, NULL, NULL);
	status = clfftGetTmpBufSize(clfftPlanHandle, &tempBufferSize);

	if(tempBufferSize > 0)
	{
		deviceTemporaryBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, tempBufferSize, 0, NULL);
	}

	///////////////////// CL Init Done
	size_t  maxWorkGroupSize;
	size_t  preProcessKernelWorkGroupSize = localWorkSize;
	size_t  postProcessKernelWorkGroupSize = localWorkSize;


	//Output data
	float* preProcessed;
	float* cmplx;
	float* linEnv;
	float* logEnv;
	float* sum;
	float* sam;
	float* ad;
	unsigned char* bscanBmp;

	preProcessed = (float*)malloc(sizeof(float) * totalOutputLen * 2);
	cmplx = (float*)malloc(sizeof(float) * totalOutputLen * 2);
	linEnv = (float*)malloc(sizeof(float) * totalOutputLen);
	logEnv = (float*)malloc(sizeof(float) * totalOutputLen);
	sum = (float*)malloc(sizeof(float) * totalAScans);
	sam = (float*)malloc(sizeof(float) * totalAScans);
	ad = (float*)malloc(sizeof(float) * totalAScans);
	bscanBmp = (unsigned char*)malloc(sizeof(unsigned char*) * totalBScanVolumeSize);

	char bmpPath[2048];


	for (int i = 0; i < numBScanProccesingIterations-1;i++)
	{
		//Pre Process
		size_t totalWorkItems = totalAScans;
		size_t spectraSize = totalInputSpectraLength * sizeof(short);
		size_t numWorkItemsPerGroup = preProcessKernelWorkGroupSize;

		windowType = 2;

		err = clSetKernelArg(preProcessingKernel, 10, sizeof(unsigned int), &windowType);

		//std::vector<short>tempSpecData(&HostSpectraData[0],
			//&HostSpectraData[totalInputSpectraLength * sizeof(short)]);
		
		//err = clEnqueueWriteBuffer(command_queue,deviceSpectra,CL_TRUE,0,
			//spectraSize,&inputSpectra,0,NULL,NULL);

		err = clEnqueueNDRangeKernel(command_queue, preProcessingKernel, 1, NULL, 
			&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);

		clFinish(command_queue);

		status = clfftEnqueueTransform(clfftPlanHandle, CLFFT_BACKWARD, 1,
			&command_queue, 0, NULL, NULL,
			&devicePreProcessedSpectra, &deviceFourierTransform, deviceTemporaryBuffer);

		clFinish(command_queue);

		err = clSetKernelArg(postProcessingKernel, 8, sizeof(cl_float), &bmpMinVal);	// min threshold
		err = clSetKernelArg(postProcessingKernel, 9, sizeof(cl_float), &bmpMaxVal);

		clFinish(command_queue);

		err = clEnqueueNDRangeKernel(command_queue, postProcessingKernel, 1, NULL,
			&totalWorkItems, &numWorkItemsPerGroup, 0, NULL, NULL);

		clFinish(command_queue);

		//Copy results to host
		err = clEnqueueReadBuffer(command_queue, devicePreProcessedSpectra, CL_TRUE, 
			0, sizeof(float)*totalAScans * inputSpectraLength * 2, preProcessed, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceFourierTransform, CL_TRUE, 
			0, sizeof(float)*totalAScans * outputAScanLength * 2, cmplx, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceEnvBScan, CL_TRUE, 
			0, sizeof(float)* totalAScans * outputAScanLength, linEnv, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceLogEnvBScan, CL_TRUE, 
			0, sizeof(float)*totalAScans * outputAScanLength, logEnv, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceSum, CL_TRUE, 0, 
			sizeof(float)*totalAScans, sum, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceSAM, CL_TRUE, 0,
			sizeof(float)*totalAScans, sam, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceAttenuationDepth, CL_TRUE, 0,
			sizeof(float)*totalAScans, ad, 0, NULL, NULL);
		err = clEnqueueReadBuffer(command_queue, deviceBScanBmp, CL_TRUE, 0,
			sizeof(unsigned char)*bitmapBScanVolumeSize, bscanBmp, 0, NULL, NULL);

		clFinish(command_queue);

		//Save BMP
		for (int j = 0; j < numBScansPerBatch; j++)
		{
			sprintf(bmpPath, "%sbscan%0.4i.bmp\0", "./test/", i*numBScansPerBatch + j);

			float bitmapFileSize;
			unsigned int bpp = 24;
			unsigned int bytesPerPixel = bpp / 8;
			unsigned int headerLen = 14;    // 14 byte bitmap header
			unsigned int dibHeaderLen = 12; // BITMAPCOREHEADER
			unsigned int offset = headerLen + dibHeaderLen;
			unsigned int planes = 1;

			unsigned int width = numAScansPerBScan;
			unsigned int height = outputImageHeight;

			unsigned int bytePixel;
			unsigned int ii;
			unsigned char* pixelArray;
			pixelArray = &bscanBmp[j*totalBScanSize];

			bitmapFileSize = headerLen + dibHeaderLen + totalBScanSize;

			FILE* fbmp;
			fbmp = fopen(bmpPath, "wb");

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

			for (int k = 0; k<totalBScanSize; k++)
			{
				ii = totalBScanSize - k - 1;
				fwrite(&pixelArray[ii], 1, 1, fbmp);  // Red
			}

			fclose(fbmp);
		}
	}
}



int main(int argc, char *argv[])
{
	EngineOCT engineOCT;
	engineOCT.LoadOCTData();
	engineOCT.OpenCLCompute();

	EngineRendering(engineOCT);

	return 0;
}

