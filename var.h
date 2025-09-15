/***************************************************************************************************
Purpose: Handle the creation, modification, and loading of variables
***************************************************************************************************/
#pragma once
#include "param.h"

/***************************************************************************************************
									   Interface Functions
***************************************************************************************************/

/* Open up the saved variable file */
void InitVarList(const char* path);

/* Clear the active variable list. */
void ClearVarList();

/* Print the active variable list */
void PrintVarList();

/* Save variables to file; free the variable list */
void CleanupVars();

/* Get a variable by name. 'start' and 'end' are the bounds of the search string */
var_t* GetVar(const char* start, const char* end);

/* Set a list of variables to 'value' */
void SetVars(var_t** var_list, double value);