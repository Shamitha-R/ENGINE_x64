#include "UI.h"


UI::UI()
{
}


UI::~UI()
{
}

void TW_CALL RunCB(void *clientData)
{
	EngineOCT *oct = (EngineOCT *)(clientData);

	oct->LoadOCTData();
}


void UI::InitialiseUI(int screenWidth,int screenHeight,EngineOCT &oct)
{
	TwBar *bar;
	TwBar *bar2;

	TwInit(TW_OPENGL,NULL);

	// Tell the window size to AntTwfeakBar
	TwWindowSize(screenWidth, screenHeight);

	// Create a tweak bar
	bar = TwNewBar("Functions");
	TwDefine(" GLOBAL help='Test Functions' "); // Message added to the help bar.
	TwAddButton(bar, "Run", RunCB, &oct, " label='Load OCT Data' ");

	bar2 = TwNewBar("Parameters");
	//TwAddVarRO(bar2, "Width", TW_TYPE_INT32, &width,
	//" label='Wnd width' help='count' ");

}

