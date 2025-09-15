/***************************************************************************************************
Purpose: Handle input from the user and compute accordingly
***************************************************************************************************/
#include <stdio.h> //getc
#include <string.h> //strcmp

#include "interactive.h"
#include "param.h"
#include "error.h"
#include "var.h"
#include "compute.h"

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

#define BASE_BUF_SIZE 64

/***************************************************************************************************
										Private Functions
***************************************************************************************************/

static char* Input()
{
	int		buf_size = BASE_BUF_SIZE;
	char*	base, * last;
	int		in;

	last = base = malloc_s(buf_size);
	*base = '\0'; //In case the user just presses enter

	while (1)
	{
		in = getc(stdin);

		if (in == EOF) //read error
			Exit(ERR_FILE);

		if (in == '\n')
			break;

		if (last - base >= (buf_size - 1))
		{//ran out of buffer space - sub 1 since we write 0 without incrementing the buffer
			int new_buf_size = buf_size + BASE_BUF_SIZE;
			char* new_buf = malloc_s(new_buf_size);

			//Copy the old buffer into the new one
			memcpy(new_buf, base, buf_size);

			free(base);

			//Translate the pointers to the new buffer
			last = new_buf + (last - base);
			base = new_buf;

			buf_size = new_buf_size;
		}

		*(last++) = in;
		*(last) = '\0';
	}

	return base;
}

/***************************************************************************************************
									   Interface Functions
***************************************************************************************************/

void InteractiveMode(void)
{
	while (1)
	{
		char* input = Input();
		param_t* params, * rpn_list;
		var_t** assignment_list;
		enum error_e err;
		bool cond;

		if (!strcmp(input, "exit") || !strcmp(input, "quit"))
		{
			free(input);
			break;
		}
		
		if (!strcmp(input, "clr"))
		{//clear the variable list
			ClearVarList();
			continue;
		}

		//Generate a list of parameters based on the input string
		params = ParseParams(1, (const char**) & input, &cond); //Quiet compiler with cast
		free(input);
		if ((err = ClrErrno()) != ERR_NONE)
		{
			CleanupParamList(params);
			continue;
		}

		rpn_list = ConvertParamList(params, &assignment_list, cond);
		CleanupParamList(params);
		if ((err = ClrErrno()) != ERR_NONE)
		{
			CleanupParamList(rpn_list);
			free(assignment_list);
			continue;
		}


		double answer = Compute(rpn_list);
		if ((err = ClrErrno()) != ERR_NONE)
		{
			CleanupParamList(rpn_list);
			free(assignment_list);
			continue;
		}

		printf("\t\t%lf\n", answer);

		SetVars(assignment_list, answer);
		CleanupParamList(rpn_list);

		free(assignment_list);
	}
}