#include "error.h"
#include <stdio.h>

static enum error_e code = ERROR_NONE;

const char* error_msgs[ERROR_MAX_PRINTABLE - 1] =
{
	"Unknown error",
	"Error: Bad input character",
	"Error: No free memory",
	"Error: Attempted to use an un-initialized variable",
	"Error: Mismatched parentheses",
	"Error: Programmer mistake",
	"Error: Syntax",
	"Error: Failed to read from file",
};

const char* usage_msg = "Usage: ";

void ExitFunc(void)
{
	if (code == ERROR_NONE)
		return; //Nothing to print

	if (code >= ERROR_MAX_PRINTABLE)
		printf("BAD ERROR CODE %i\n", code);
	else if (code > 0)
		printf("%s\n", error_msgs[code - 1]);

	printf("%s\n", usage_msg);
}

void InitExit()
{
	atexit(&ExitFunc);
}

__declspec(noreturn)
void Exit(enum error_e _code)
{
	code = _code;
	exit(code);
}

void* malloc_s(size_t size)
{
	void* buf = malloc(size);

	if (!buf)
		Exit(ERR_MEM);

	return buf;
}