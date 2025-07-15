#include <math.h>

#include "compute.h"
#include "error.h"

#define FIXED_ARRAY 1

double Compute(const param_t* expr)
{
#if FIXED_ARRAY
	double stack[128]; //FIXME
	int top = 0;
#endif

	for (const param_t* i = expr; i; i = i->next)
	{
		if (i->type == PARAM_TYPE_VAR)
		{
			if (!i->var->init)
				Exit(ERR_NOINIT);

			stack[top++] = i->var->val;
		}
		else if (i->type == PARAM_TYPE_NUM)
		{
			stack[top++] = i->val;
		}
		else
		{//operator
			double accumulator, op1, op2;

			if (top < 2)
				Exit(ERR_SYNTAX);

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
			default: Exit(ERROR_INPUT);
			}

			stack[top++] = accumulator;
		}
	}

	if (top != 1) //something like '2 3' as input
		Exit(ERR_SYNTAX);

	return stack[0];
}