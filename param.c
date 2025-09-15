/***************************************************************************************************
Purpose: Handle parameters
***************************************************************************************************/
#include <stdio.h> //printf
#include <ctype.h> //isdigit
#include <stdlib.h> //atof

#include "param.h"
#include "var.h"
#include "error.h"

/***************************************************************************************************
										Private Variables
***************************************************************************************************/

const char* const operators = "+-*/%^()[]=";
const char* const meaningful_operators = "+-*/%^()[]m"; //no equals, explicit unary minus

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

static inline _Bool IsOp(const char x)
{
	for (const char* c = operators; *c; c++)
		if (*c == x)
			return 1;

	return 0;
}

static inline _Bool IsRealOp(const char x)
{
	for (const char* c = meaningful_operators; *c; c++)
		if (*c == x)
			return 1;

	return 0;
}

#define IsVarChar(x) (isalpha(x) || x == '_')
#define IsLParen(x) (x == '(' || x == '[')
#define IsRParen(x) (x == ')' || x == ']')

#pragma region PARAM_SET

#define NEW_PARAM(param, set)	do {\
								param = malloc_s(sizeof(param_t)); \
								param->next = NULL; \
								_Generic((set), \
									double : SetParamVal(param, &set),	\
									const var_t* : SetParamVar(param, &set),\
									char : SetParamOp(param, &set)); \
								} while(0)

static void SetParamVal(param_t* this, const void* val)
{
	this->type = PARAM_TYPE_NUM;
	this->val = *(double*)val;
}

static void SetParamVar(param_t* this, const void* var)
{
	this->type = PARAM_TYPE_VAR;
	this->var = *(const var_t**)var; //Double dereference - NEW_PARAM needs the address of the pointer
}

static void SetParamOp(param_t* this, const void* op)
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
	param_t* param;
	static const char unary = 'm';

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
		if (IsOp(*c))
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
			c++;
		else
			Error(ERR_INPUT, NULL);

		if (!head) //first iteration
			head = *last;
		else
			prev->next = *last;
		prev = *last;
	}

	return head;
}

param_t* ParseParams(int count, const char* args[])
{
	param_t* head = NULL, * prev = NULL;
	_Bool minus_is_unary = 1; //after the start, or any operator. Must be preserved between calls to ParseParam

	for (int i = 0; i < count; i++)
	{
		const char* arg = args[i];
		param_t* localhead, *last = NULL;

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


void CleanupParamList(param_t* head)
{
	for (param_t* p = head; p;)
	{
		param_t* next = p->next;

		free(p);
		p = next;
	}
}

void PrintParamList(const param_t* head)
{
	for (const param_t* p = head; p; p = p->next)
	{
		switch (p->type)
		{
		case PARAM_TYPE_NUM:
			printf("%f\n", p->val);
			break;
		case PARAM_TYPE_VAR:
			printf("%s\n", p->var->name);
			break;
		case PARAM_TYPE_OP:
			printf("%c\n", p->op);
			break;
		}
	}
}


#pragma region RPN

//Determine what vars, if any, will be assigned to 
//returns - the first param after the '='
//returns - head if no '='
//ERROR - Syntax
static const param_t* SetupAssignments(const param_t* head, var_t*** assn_list)
{
	_Bool found_bad = 0;
	int var_count = 0;
	const param_t* cur;

	*assn_list = NULL; //In case there aren't any assignments

	for (cur = head; ; cur = cur->next)
	{
		if (!cur) //No assignments
			return head;

		if (cur->type == PARAM_TYPE_OP)
		{
			if (cur->op == '=')
				break;

			found_bad = 1;
		}
		else if (cur->type == PARAM_TYPE_VAR)
			var_count++;
		else
			found_bad = 1;

	}
	//Must be a valid assignment prefix at this point

	if (found_bad)
		Error(ERR_SYNTAX, NULL); //somthing like '1' '='

	if (var_count == 0)
		Error(ERR_SYNTAX, NULL); //'=' with nothing before it

	if (!cur->next) //Ends with a '=' aka nonsense
		Error(ERR_SYNTAX, NULL);

	//Allocate space for the array
	*assn_list = malloc_s((var_count + 1) * sizeof assn_list[0]); //Make room for a null terminator

	const param_t* var = head;
	for (int i = 0; i < var_count; i++, var = var->next)
		//Cast - the parameters never modify the variables, but the assignment list will
		(*assn_list)[i] = (var_t*)var->var; //Copy the var data references into the convenient list. 
	(*assn_list)[var_count] = NULL;

	return cur->next;
}

//returns - < 0 if op1 is lower precedence
//returns - 0 if op1 and op2 have same precedence
//returns - > 0 if op1 is higher precedence
//ERROR - syntax
static int Precedence(char op1, char op2)
{
	int pr1, pr2;

	switch (op1)
	{
	case '+': case '-': pr1 = 0; break;
	case '*': case '/': case '%': pr1 = 1; break;
	case '^': pr1 = 2; break;
	case 'm': pr1 = 3; break;
	default: Error(ERR_SYNTAX, 0);
	}
	switch (op2)
	{
	case '+': case '-': pr2 = 0; break;
	case '*': case '/': case '%': pr2 = 1; break;
	case '^': pr2 = 2; break;
	case 'm': pr2 = 3; break;
	default: Error(ERR_SYNTAX, 0);
	}

	if (pr1 == pr2)
		return 0;
	if (pr1 > pr2)
		return 1;
	return -1;
}

static _Bool IsRightAssociative(char op)
{
	if (op == '^' || op == 'm')
		return 1;
	return 0;
}

#define HEAD_CHECK(head, prev, last) \
									if(!head) head = last; \
									else prev->next = last; \
									prev = last;
				

param_t* ConvertParamList(const param_t* src_head, var_t*** assn_list)
{
	param_t* head = NULL, * prev = NULL;
	int num_ops = 0;
	char* op_stack;
	int op_top = 0;
	const param_t* cur = SetupAssignments(src_head, assn_list);

	if (GetErrno() != ERR_NONE)
		return NULL;

	for (const param_t* c = cur; c; c= c->next)
		if (c->type == PARAM_TYPE_OP)
			num_ops++;

	op_stack = malloc_s(num_ops);

	for (; cur; cur = cur->next)
	{
		param_t *last;

		if (cur->type == PARAM_TYPE_VAR)
		{
			NEW_PARAM(last, cur->var);
			HEAD_CHECK(head, prev, last);
		}
		else if (cur->type == PARAM_TYPE_NUM)
		{
			NEW_PARAM(last, cur->val);
			HEAD_CHECK(head, prev, last);
		}
		else
		{//Operator
			if (IsLParen(cur->op))
			{//add left parentheses
				op_stack[op_top++] = cur->op;
			}
			else if (IsRParen(cur->op))
			{//empty everything except the matching left parenthesis into the queue
				while (1)
				{
					if (op_top == 0)
						Error(ERR_PARENTHESIS, NULL);

					if (IsLParen(op_stack[op_top - 1]))
						break;

					//Note prefix operator
					NEW_PARAM(last, op_stack[--op_top]);
					HEAD_CHECK(head, prev, last);
				}

				op_top--; //Discard left parenthesis
			}
			else
			{//Non-parenthetical operator
				while (op_top > 0)
				{
					char other = op_stack[op_top - 1];
					int precedence;

					if (IsLParen(other))
						break;

					precedence = Precedence(other, cur->op);

					if (GetErrno() != ERR_NONE)
						return NULL;

					//if ((precedence > 0) || (precedence == 0 && !IsRightAssociative(cur->op))) ;
					//else break;
					if ((precedence < 0))
						break;
					if (precedence == 0 && IsRightAssociative(cur->op))
						break;


					NEW_PARAM(last, op_stack[--op_top]);
					HEAD_CHECK(head, prev, last);
				}

				op_stack[op_top++] = cur->op;
			}
		}
	}

	//Empty the remaining operators, if any, into the queue
	for (int i = op_top - 1; i >= 0; i--)
	{
		param_t* last;
		NEW_PARAM(last, op_stack[--op_top]);
		HEAD_CHECK(head, prev, last);
	}

	free(op_stack);

	return head;
}

#undef HEAD_CHECK

#pragma endregion