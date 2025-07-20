#include "error.h"
#include <stdio.h>

static enum error_e code = ERR_NONE;

const char* error_msgs[ERROR_MAX_PRINTABLE - 1] =
{
	"Unknown error",
	"Error: No free memory",
	"Error: Programmer mistake",

	"Error: Bad input character", //No exit
	"Error: Attempted to use an un-initialized variable", //No exit
	"Error: Mismatched parentheses", //No exit
	"Error: Syntax", //No exit
	"Error: Failed to read from file", //No exit
};

const char* usage_msg = "Usage: ";

#define ERROR_FIRST_NOEXIT ERR_INPUT

void ExitFunc(void)
{
	if (code == ERR_NONE)
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
void _Error(enum error_e _code)
{
	code = _code;

	if (code >= ERROR_FIRST_NOEXIT)
	{
		printf("%s\n", error_msgs[code - 1]);
	}
	else
	{
		exit(code);
	}
}

enum error_e GetErrno()
{
	return code;
}

enum error_e ClrErrno()
{
	enum error_e err = code;
	code = ERR_NONE;
	return err;
}

void* malloc_s(size_t size)
{
	void* buf = malloc(size);

	if (!buf)
		Exit(ERR_MEM);

	return buf;
}