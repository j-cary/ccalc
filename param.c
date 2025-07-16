#include <stdio.h> //printf
#include <ctype.h> //isdigit
#include <stdlib.h> //atof

#include "param.h"
#include "var.h"
#include "error.h"

#define PRINTINFO 0

const char* const operators = "+-*/%^()[]=";
const char* const meaningful_operators = "+-*/%^()[]"; //no equals

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

#define NEW_PARAM(param, set)	param = malloc_s(sizeof(param_t)); \
								param->next = NULL; \
								_Generic((set), \
									double : SetParamVal(param, &set),	\
									const var_t* : SetParamVar(param, &set),\
									char : SetParamOp(param, &set))

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

static param_t* EvalOp(const char** iter)
{
	param_t* param;

	NEW_PARAM(param, **iter);

	(*iter)++;
	return param;
}

static param_t* ParseParam(const char* param, param_t** last)
{
	param_t* head = NULL, * prev = NULL; //set prev to NULL to shut up compiler

	for (const char* c = param; *c; /*c++*/)
	{
		if (IsOp(*c))
			*last = EvalOp(&c);
		else if (IsVarChar(*c))
			*last = EvalVar(&c);
		else if (isdigit(*c))
			*last = EvalNum(&c);
		else if (isspace(*c))
			c++;
		else
			Exit(ERR_INPUT);

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

	for (int i = 0; i < count; i++)
	{
		const char* arg = args[i];
		param_t* localhead, *last = NULL;

		localhead = ParseParam(arg, &last);

		if (!head) //first iteration
			head = localhead;
		else
			prev->next = localhead;
		prev = last;
	}

#if _DEBUG && PRINTINFO
	PrintParamList(head);
#endif
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
static const param_t* SetupAssignments(const param_t* head, var_t*** assn_list)
{
	//_Bool found_op = 0, found_num = 0;
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
		Exit(ERR_SYNTAX); //somthing like '1' '='

	if (var_count == 0)
		Exit(ERR_SYNTAX); //'=' with nothing before it

	if (!cur->next) //Ends with a '=' aka nonsense
		Exit(ERR_SYNTAX);

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
//returns - >0 if op1 is higher precedence
static int Precedence(char op1, char op2)
{
	if (!IsRealOp(op2))
		Exit(ERR_SYNTAX);

	switch (op1)
	{
	case '+':
	case '-':
		if (op2 == '+' || op2 == '-')
			return 0;
		return -1;
	case '*':
	case '/':
	case '%':
		if (op2 == '+' || op2 == '-')
			return 1;
		if (op2 != '^')
			return 0;
		return -1;
	case '^':
		if (op2 == '^')
			return 0;
	}

	Exit(ERR_PROGRAM);
	return 0; 
}

static _Bool IsRightAssociative(char op)
{
	if (op == '^')
		return 1;
	return 0;
}

#define HEAD_CHECK(head, prev, last) \
									if(!head) head = last; \
									else prev->next = last; \
									prev = last;
				

#define STATIC_ARRAY 0

//Convert the list to RPN. Generate a list of vars to assign to 
param_t* ConvertParamList(const param_t* src_head, var_t*** assn_list)
{
	param_t* head = NULL, * prev = NULL;
#if !STATIC_ARRAY
	int num_ops = 0;
	char* op_stack;
#else
	//double out_queue[128];
	char op_stack[128];
#endif
	* assn_list = NULL;
	int op_top = 0;
	const param_t* cur = SetupAssignments(src_head, assn_list);

#if !STATIC_ARRAY
	for (const param_t* c = cur; c; c= c->next)
		if (c->type == PARAM_TYPE_OP)
			num_ops++;

	op_stack = malloc_s(num_ops);
#endif

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
						Exit(ERR_PARENTHESIS);

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

#if _DEBUG && PRINTINFO
	printf("======\n");
	PrintParamList(head);
#endif

#if !STATIC_ARRAY
	free(op_stack);
#endif

	return head;
}

#undef HEAD_CHECK

#pragma endregion