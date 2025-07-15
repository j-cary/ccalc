#include <stdio.h> //getc
#include <string.h> //strcmp

#include "interactive.h"
#include "param.h"
#include "error.h"
#include "var.h"
#include "compute.h"

#define BASE_BUF_SIZE 64

static const char* Input()
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

			for (int i = 0; i < buf_size; i++)
				new_buf[i] = base[i]; //Copy the old buffer into the new one

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

void InteractiveMode()
{
	while (1)
	{
		const char* input = Input();
		param_t* params, * rpn_list;
		var_t** assignment_list;

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
		params = ParseParams(1, &input);
		free(input);

		rpn_list = ConvertParamList(params, &assignment_list);
		CleanupParamList(params);

		double answer = Compute(rpn_list);
		printf("\t\t%lf\n", answer);

		SetVars(assignment_list, answer);
		CleanupParamList(rpn_list);

		free(assignment_list);
	}
}