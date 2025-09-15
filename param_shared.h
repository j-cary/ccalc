/***************************************************************************************************
Purpose: Provide common functionality between the parameter modules
***************************************************************************************************/
#pragma once
#include <stdbool.h>
#include "param.h"
#include "var.h"
#include "error.h"

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

#define NEW_PARAM(param, set)	do {\
								param = malloc_s(sizeof(param_t)); \
								param->next = NULL; \
								_Generic((set), \
									double : SetParamVal(param, &set),	\
									const double : SetParamVal(param, &set),\
									const var_t* : SetParamVar(param, &set),\
									const var_t* const : SetParamVar(param, &set),\
									char : SetParamOp(param, &set), \
									const char : SetParamOp(param, &set)); \
								} while(0)

/***************************************************************************************************
									   Interface Functions
***************************************************************************************************/

void SetParamVal(param_t * this, const void* val);

void SetParamVar(param_t * this, const void* var);

void SetParamOp(param_t * this, const void* op);