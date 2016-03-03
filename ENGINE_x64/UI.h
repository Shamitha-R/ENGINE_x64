#pragma once

#include <AntTweakBar.h>
#include "Engine.h"


class Engine;

class UI
{
public:
	UI();
	~UI();

	void InitialiseUI(int screenWidth, int screenHeight,
		Engine &engine);
};

