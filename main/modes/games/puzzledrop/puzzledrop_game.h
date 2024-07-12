#pragma once

#include "mode_puzzledrop.h"

void puzzledropDraw(puzzledropVars_t* p);
void puzzledropUpdate(puzzledropVars_t* p, int64_t elapsedUs);
void puzzledropCheck(puzzledropVars_t *p);
void puzzleStartLevel(puzzledropVars_t* p);
