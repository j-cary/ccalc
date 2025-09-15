/***************************************************************************************************
Purpose: Parse parameters from the input text
***************************************************************************************************/
#include <ctype.h> //isdigit
#include <stdlib.h> //atof
#include "param_shared.h"

/***************************************************************************************************
										Private Variables
***************************************************************************************************/

static const char
* const operators = "+-*/%^()[]=",
* const meaningful_operators = "+-*/%^()[]m", //no equals, explicit unary minus
* const conditional_operators = "&|=<>!",
* const conditional_standins = "drelgn"; /* Must match the op_* vars exactly */
static const char op_bad = 'q';
static bool is_conditional = false; /* Set if conditional operators are used */

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

static inline bool IsArithOp(const char x)
{
	for (const char* c = operators; *c; c++)
		if (*c == x)
			return true;

	return false;
}

/* Check to see if the param starts off with the start of a cond op*/
static inline bool IsCondOp(const char x)
{
	for (const char* c = conditional_operators; *c; c++)
		if (*c == x)
			return true;

	return false;
}

/* Check to see if the param ends with the corresponding cond op */
static inline char CondOp2(const char first, const char next)
{
	int idx;
	const char* c;
	char right_next; /* What we're expecting */

	for (c = conditional_operators; *c; c++)
		if (*c == first)
			break;

	idx = (int)(c - conditional_operators);

	/* Must mirror conditional operators */
	switch (idx)
	{
	case 0: right_next = '&'; break;
	case 1: right_next = '|'; break;
	case 2: right_next = '='; break;
	case 3: right_next = '='; break;
	case 4: right_next = '='; break;
	case 5: right_next = '='; break;

	default: Exit(ERR_PROGRAM);
	}

	return next == right_next ? conditional_standins[idx] : op_bad;
}

static inline bool IsRealOp(const char x)
{
	for (const char* c = meaningful_operators; *c; c++)
		if (*c == x)
			return true;

	return false;
}

#define IsVarChar(x) (isalpha(x) || x == '_')

#pragma region PARAM_SET

void SetParamVal(param_t* this, const void* val)
{
	this->type = PARAM_TYPE_NUM;
	this->val = *(double*)val;
}

void SetParamVar(param_t* this, const void* var)
{
	this->type = PARAM_TYPE_VAR;
	this->var = *(const var_t**)var; //Double dereference - NEW_PARAM needs the address of the pointer
}

void SetParamOp(param_t* this, const void* op)
{
	this->type = PARAM_TYPE_OP;
	this->op = *(char*)op;
}

#pragma endregion 


#pragma region PARSE

static param_t* EvalNum(const char** iter)
{
	double val;
	param_t* param;

	//HACK: strtod -shouldn't- change **iter, just cast it
	val = strtod(*iter, (char**)iter);

	NEW_PARAM(param, val);

	return param;
}

static param_t* EvalVar(const char** iter)
{
	const char* start = *iter;
	param_t* param;

	for (; **iter; (*iter)++)
		if (!IsVarChar(**iter))
			break;

	const var_t* var = GetVar(start, *iter);
	NEW_PARAM(param, var);

	return param;
}

static param_t* EvalOp(const char** iter, _Bool* minus_is_unary)
{
	const bool maybe_conditional = IsCondOp(**iter);
	const char next = *((*iter) + 1);
	param_t* param;
	static const char unary = 'm';

	if (maybe_conditional)
	{ /* See if the second char matches */
		char op = CondOp2(**iter, next);

		if (op != op_bad)
		{
			is_conditional = true;
			NEW_PARAM(param, op);
			(*iter) += 2;
			return param;
		}
		/* Still works as an arith op, however */
	}

	if ((*minus_is_unary) && (**iter == '-'))
		NEW_PARAM(param, unary);
	else
		NEW_PARAM(param, **iter);

	(*iter)++;
	return param;
}

//ERROR - input
static param_t* ParseParam(const char* param, _Bool* minus_is_unary, param_t** last)
{
	param_t* head = NULL, * prev = NULL; //set prev to NULL to shut up compiler

	for (const char* c = param; *c; )
	{
		if (IsArithOp(*c) || IsCondOp(*c))
		{
			*last = EvalOp(&c, minus_is_unary);
			*minus_is_unary = 1;
		}
		else if (IsVarChar(*c))
		{
			*last = EvalVar(&c);
			*minus_is_unary = 0;
		}
		else if (isdigit(*c))
		{
			*last = EvalNum(&c);
			*minus_is_unary = 0;
		}
		else if (isspace(*c))
		{
			c++;
		}
		else
		{
			Error(ERR_INPUT, NULL);
		}

		if (!head) //first iteration
			head = *last;
		else
			prev->next = *last;
		prev = *last;
	}

	return head;
}

param_t* ParseParams(int count, const char* args[], bool* const conditional)
{
	param_t* head = NULL, * prev = NULL;
	_Bool minus_is_unary = 1; //after the start, or any operator. Must be preserved between calls to ParseParam
	is_conditional = false;

	for (int i = 0; i < count; i++)
	{
		const char* arg = args[i];
		param_t* localhead, * last = NULL;

		localhead = ParseParam(arg, &minus_is_unary, &last);

		if (GetErrno() != ERR_NONE)
			return NULL;

		if (!head) //first iteration
			head = localhead;
		else
			prev->next = localhead;
		prev = last;
	}

	return head;
}

#pragma endregion