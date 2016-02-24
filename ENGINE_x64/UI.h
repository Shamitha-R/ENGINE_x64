#pragma once

#include <AntTweakBar.h>
#include "EngineOCT.h"
#include "EngineRenderer.h"

class UI
{
public:
	UI();
	~UI();

	void InitialiseUI(int screenWidth, int screenHeight,EngineOCT &oct,EngineRenderer &renderer);
};

