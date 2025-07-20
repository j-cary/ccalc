#include <math.h>

#include "compute.h"
#include "error.h"

double Compute(const param_t* expr)
{
	double* stack;
	int top = 0;

	//Allocate enough space for every single var/val in this stack. Technically overkill, but short of doing a linked list 
	for (const param_t* i = expr; i; i = i->next)
		if (i->type == PARAM_TYPE_VAR || i->type == PARAM_TYPE_NUM)
			top++;

	stack = malloc_s(top * sizeof(double));
	top = 0;

	for (const param_t* i = expr; i; i = i->next)
	{
		if (i->type == PARAM_TYPE_VAR)
		{
			if (!i->var->init)
			{
				free(stack);
				Error(ERR_NOINIT, 0);
			}

			stack[top++] = i->var->val;
		}
		else if (i->type == PARAM_TYPE_NUM)
		{
			stack[top++] = i->val;
		}
		else
		{//operator
			double accumulator, op1, op2;

			if ((i->op == 'm' && top < 1) || (i->op != 'm' && top < 2))
			{
				free(stack);
				Error(ERR_SYNTAX, 0);
			}

			op2 = stack[--top];
			op1 = stack[--top];

			switch (i->op)
			{
			case '+': accumulator = op1 + op2; break;
			case '-': accumulator = op1 - op2; break;
			case '*': accumulator = op1 * op2; break;
			case '/': accumulator = op1 / op2; break;
			case '%': accumulator = (double)((long long)op1 % (long long)op2); break;
			case '^': accumulator = pow(op1, op2); break;
			case 'm': accumulator = -op2; top++; break;
			default: 
				free(stack);
				Error(ERR_INPUT, 0);
				break;
			}

			stack[top++] = accumulator;
		}
	}

	double ret = stack[0];

	free(stack);

	if (top != 1) //something like '2 3' as input
		Error(ERR_SYNTAX, 0);


	return ret;
}