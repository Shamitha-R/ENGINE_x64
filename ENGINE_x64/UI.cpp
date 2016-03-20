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
	EngineOCT *oct = (EngineOCT *)(clientData);
	oct->OpenCLCompute();
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

	// Tell the window size to AntTwfeakBar
	TwWindowSize(screenWidth, screenHeight);

	// Create a tweak bar
	bar = TwNewBar("Functions");
	TwDefine(" GLOBAL help='Test Functions' "); // Message added to the help bar.
	TwAddButton(bar, "LoadData", LoadDataFunc, &engine.OCT, " label='Load OCT Data' ");
	TwAddButton(bar, "ExecuteProcessing", ExecuteOCTProccessing, &engine.OCT, " label='Execute Processing' ");
	TwAddButton(bar, "OutputBScans", ExecuteOCTProccessing, &engine.OCT, " label='Output BScans' ");
	TwAddButton(bar, "OutputCResults", ExecuteOCTProccessing, &engine.OCT, " label='Output Correlation Results' ");
	TwAddButton(bar, "OutputComposite", ExecuteOCTProccessing, &engine.OCT, " label='Output Vasculature' ");
	TwAddButton(bar, "ComputeCorrelation", ComputeCorrelation, &engine, " label='Compute Correlation' ");


	parametersBar = TwNewBar("Render Parameters");
	TwAddVarRW(parametersBar, "kernelSizeX", TW_TYPE_INT32, &engine.OCT.KernelSizeX,
		" label='Kernel size X'");
	TwAddVarRW(parametersBar, "kernelSizeY", TW_TYPE_INT32, &engine.OCT.KernelSizeY,
		" label='Kernel size Y'");
	TwAddVarRW(parametersBar, "FilterWindowX", TW_TYPE_INT32, &engine.OCT.FilterWindowX,
		" label='Filter Window X'");
	TwAddVarRW(parametersBar, "FilterWindowY", TW_TYPE_INT32, &engine.OCT.FilterWindowY,
		" label='Filter Window Y'");

	computeParameters = TwNewBar("Compute Parameters");
	TwAddVarRW(computeParameters, "AScanCount", TW_TYPE_INT32, &engine.OCT.NumAScans,
		" label='A-Scan Count'");
	TwAddVarRW(computeParameters, "MaxValue", TW_TYPE_FLOAT, &engine.OCT.MaxValue,
		" label='Max Value'");
	TwAddVarRW(computeParameters, "MinValue", TW_TYPE_FLOAT, &engine.OCT.MinValue,
		" label='Min Value'");
}

