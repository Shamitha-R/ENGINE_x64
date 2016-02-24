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
	EngineOCT *oct = (EngineOCT *)(clientData);
	oct->ComputeCrossCorrelation();
}

void UI::InitialiseUI(int screenWidth,int screenHeight,EngineOCT &oct,EngineRenderer &renderer)
{
	TwBar *bar;
	TwBar *parametersBar;

	TwInit(TW_OPENGL,NULL);

	// Tell the window size to AntTwfeakBar
	TwWindowSize(screenWidth, screenHeight);

	// Create a tweak bar
	bar = TwNewBar("Functions");
	TwDefine(" GLOBAL help='Test Functions' "); // Message added to the help bar.
	TwAddButton(bar, "LoadData", LoadDataFunc, &oct, " label='Load OCT Data' ");
	TwAddButton(bar, "ExecuteProcessing", ExecuteOCTProccessing, &oct, " label='Execute Processing' ");
	TwAddButton(bar, "OutputBScans", ExecuteOCTProccessing, &oct, " label='Output BScans' ");
	TwAddButton(bar, "OutputCResults", ExecuteOCTProccessing, &oct, " label='Output Correlation Results' ");
	TwAddButton(bar, "OutputComposite", ExecuteOCTProccessing, &oct, " label='Output Vasculature' ");
	TwAddButton(bar, "ComputeCorrelation", ComputeCorrelation, &oct, " label='Compute Correlation' ");

	int speed;

	parametersBar = TwNewBar("Parameters");
	TwAddVarRW(parametersBar, "kernelSizeX", TW_TYPE_INT32, &oct.KernelSizeX, 
		" label='Kernel size X'");
	TwAddVarRW(parametersBar, "kernelSizeY", TW_TYPE_INT32, &oct.KernelSizeY,
		" label='Kernel size Y'");


}

