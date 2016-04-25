#include "UI.h"

UI::UI()
{
}

UI::~UI()
{
}

void TW_CALL LoadDataFunc(void *clientData)
{
	EngineOCT *oct = (EngineOCT *)(clientData);
	oct->LoadOCTData();
}

void TW_CALL ExecuteOCTProccessing(void *clientData)
{
	Engine *engine = (Engine *)(clientData);
	engine->OCT.OpenCLCompute();
	engine->PassRenderData();
}

void TW_CALL ComputeCorrelation(void *clientData)
{
	Engine *engine = (Engine *)(clientData);
	engine->OCT.ComputeCrossCorrelation();
	engine->PassRenderData();
}

void UI::InitialiseUI(int screenWidth,int screenHeight, Engine &engine)
{
	TwBar *bar;
	TwBar *parametersBar;
	TwBar *computeParameters;

	TwInit(TW_OPENGL,NULL);

	// Tell the window size to AntTweakBar
	TwWindowSize(screenWidth, screenHeight);

	//UI Layout

	//Functions Tab
	bar = TwNewBar("Functions");
	TwDefine(" GLOBAL help='Test Functions' "); // Message added to the help bar.
	TwAddButton(bar, "LoadData", LoadDataFunc, &engine.OCT, " label='Load OCT Data' ");
	TwAddButton(bar, "ExecuteProcessing", ExecuteOCTProccessing, &engine, " label='Execute Processing' ");
	TwAddButton(bar, "ComputeCorrelation", ComputeCorrelation, &engine, " label='Compute Correlation' ");

	//Parameters Tab
	parametersBar = TwNewBar("Render Parameters");
	TwAddVarRW(parametersBar, "kernelSizeX", TW_TYPE_INT32, &engine.OCT.KernelSizeX,
		" label='Kernel size X'");
	TwAddVarRW(parametersBar, "kernelSizeY", TW_TYPE_INT32, &engine.OCT.KernelSizeY,
		" label='Kernel size Y'");
	TwAddVarRW(parametersBar, "FilterWindowX", TW_TYPE_INT32, &engine.OCT.FilterWindowX,
		" label='Filter Window X'");
	TwAddVarRW(parametersBar, "FilterWindowY", TW_TYPE_INT32, &engine.OCT.FilterWindowY,
		" label='Filter Window Y'");

	//Compute Parameters Tab
	computeParameters = TwNewBar("Compute Parameters");
	TwAddVarRW(computeParameters, "AScanCount", TW_TYPE_INT32, &engine.OCT.NumAScans,
		" label='A-Scan Count'");
	TwAddVarRW(computeParameters, "BScanCount", TW_TYPE_INT32, &engine.OCT.TotalBScans,
		" label='B-Scan Count'");
	TwAddVarRW(computeParameters, "BScanBatchSize", TW_TYPE_INT32, &engine.OCT.BScanBatchSize,
		" label='B-Scan Batch Size'");
	TwAddVarRW(computeParameters, "MaxValue", TW_TYPE_FLOAT, &engine.OCT.MaxValue,
		" label='Max Value'");
	TwAddVarRW(computeParameters, "MinValue", TW_TYPE_FLOAT, &engine.OCT.MinValue,
		" label='Min Value'");
}

