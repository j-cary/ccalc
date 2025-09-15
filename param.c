/***************************************************************************************************
Purpose: Handle parameters
***************************************************************************************************/
#include <stdio.h> //printf
#include "param_shared.h"

/* TODO: guide for adding new cond operators */

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

#define IsLParen(x) (x == '(' || x == '[')
#define IsRParen(x) (x == ')' || x == ']')

/***************************************************************************************************
										Public Variables
***************************************************************************************************/

/* anD, oR, Equal, Less than, Greater than, Not equal */
const char op_and = 'd', op_or = 'r', op_eq = 'e', op_le = 'l', op_ge = 'g', op_ne = 'n';

/***************************************************************************************************
										Private Variables
***************************************************************************************************/

static bool conditional;

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
	bool found_bad = 0;
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

static bool IsRightAssociative(char op)
{
	if (op == '^' || op == 'm')
		return 1;
	return 0;
}

#define HEAD_CHECK(head, prev, last) \
									if(!head) head = last; \
									else prev->next = last; \
									prev = last;
				

param_t* ConvertParamList(const param_t* src_head, var_t*** assn_list, bool cond)
{
	param_t* head = NULL, * prev = NULL;
	int num_ops = 0;
	char* op_stack;
	int op_top = 0;
	const param_t* cur = SetupAssignments(src_head, assn_list);
	conditional = cond;

	if (GetErrno() != ERR_NONE)
		return NULL;

	for (const param_t* c = cur; c; c = c->next)
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