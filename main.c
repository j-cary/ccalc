#include <stdio.h>
#include "param.h"
#include "error.h"
#include "var.h"
#include "compute.h"
#include "interactive.h"

#define USE_FAKEARGS 0

int main(int argc, const char* argv[])
{
	char* fake_argv[] =
	{
		//".exe", "x=", "1", "+3.57 + 44 + var + -d", "-2"
		//".exe", "x y z=", "1-(2+3)*4"
		".exe", "420", "+69",
	};
	int used_argc;
	char** used_argv;
	param_t* param_list, * rpn_list;
	var_t** assignment_list; //dynamic array of ptrs to vars

#if USE_FAKEARGS
	used_argc = sizeof fake_argv / sizeof fake_argv[0];
	used_argv = fake_argv;
#else
	used_argv = argv;
	used_argc = argc;
#endif


	InitExit();
	InitVarList(argv[0]);

	printf("Loaded vars: ");
	PrintVarList();

	if (used_argc == 1)
	{
		InteractiveMode();
	}
	else
	{
		param_list = ParseParams(used_argc - 1, (const char**)used_argv + 1); //Quiet compiler with cast

		rpn_list = ConvertParamList(param_list, &assignment_list);
		CleanupParamList(param_list);

		double answer = Compute(rpn_list);
		printf("\t\t%lf\n", answer);

		SetVars(assignment_list, answer);
		CleanupParamList(rpn_list);

		free(assignment_list);
	}

	CleanupVars();
	return 0;
}