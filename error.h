#pragma once
#include <stdlib.h> //size_t

enum error_e
{
	ERROR_UNLISTED = -1, //Just print the usage message
	ERROR_NONE, //normal exit
	ERROR_GENERIC,
	ERROR_INPUT,
	ERR_MEM,
	ERR_NOINIT,
	ERR_PARENTHESIS,
	ERR_PROGRAM, //error in program logic
	ERR_SYNTAX,
	ERR_FILE,

	//Must stay at the end
	ERROR_MAX_PRINTABLE //actually have to subtract one from this to get the max printable
};

void InitExit();
void Exit(int);

void* malloc_s(size_t size);