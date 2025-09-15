/***************************************************************************************************
Purpose: Entry point for the program. Dispatches control to interactive mode if necessary
***************************************************************************************************/
#include <stdio.h>
#include "param.h"
#include "error.h"
#include "var.h"
#include "compute.h"
#include "interactive.h"

int main(int argc, const char* argv[])
{
	param_t* param_list, * rpn_list;
	var_t** assignment_list; //dynamic array of ptrs to vars


	InitExit();
	InitVarList(argv[0]);

	printf("Loaded vars: ");
	PrintVarList();

	if (argc == 1)
	{
		InteractiveMode();
	}
	else
	{
		enum error_e err;

		param_list = ParseParams(argc - 1, argv + 1);
		if ((err = GetErrno()) != ERR_NONE)
		{
			CleanupParamList(param_list);
			return err;
		}

		rpn_list = ConvertParamList(param_list, &assignment_list);
		CleanupParamList(param_list);
		if ((err = GetErrno() != ERR_NONE))
		{
			free(assignment_list); //this is guaranteed to be set to NULL or allocated by ConverParam
			CleanupParamList(rpn_list);
			return err;
		}

		double answer = Compute(rpn_list);
		if ((err = GetErrno() != ERR_NONE))
		{
			CleanupParamList(rpn_list);
			free(assignment_list);
			return err;
		}

		printf("\t\t%lf\n", answer);

		SetVars(assignment_list, answer);
		CleanupParamList(rpn_list);

		free(assignment_list);
	}

	CleanupVars();
	return 0;
}