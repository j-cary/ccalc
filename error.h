#pragma once
#include <stdlib.h> //size_t

enum error_e
{
	ERR_UNLISTED = -1, //Just print the usage message
	ERR_NONE, //normal exit
	ERR_GENERIC,
	ERR_MEM,
	ERR_PROGRAM,	//error in program logic

	ERR_INPUT,		//No abort
	ERR_NOINIT,		//No abort
	ERR_PARENTHESIS,//No abort
	ERR_SYNTAX,		//No abort
	ERR_FILE,		//No abort

	//Must stay at the end
	ERROR_MAX_PRINTABLE //actually have to subtract one from this to get the max printable
};

void InitExit();
__declspec(noreturn) void _Error(enum error_e code);

// Gets the errno.
enum error_e GetErrno();

// Gets the errno and clears it. Only intended to be used in the main module.
enum error_e ClrErrno();

//Print a warning message and set the errno
//For functions with a void return type, simply append a comma after the error code.
//This MUST BE USED for codes that do not abort the program
#define Error(e, ret) do { \
						_Error(e); \
						return ret; } while(0)

//Exit the program.
#define Exit(e) _Error(e)


void* malloc_s(size_t size);