#pragma once
#include "param.h"

void InitVarList(const char* path);
void ClearVarList();
void PrintVarList();
void CleanupVars();

var_t* GetVar(const char* start, const char* end);
void SetVars(var_t** var_list, double value);