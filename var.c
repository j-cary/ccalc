#include <string.h>
#include <stdio.h> //FILE
#include <ctype.h> //isspace

#include "error.h"
#include "var.h"

#define VARS_MAX 128
#define LINE_MAX_WIDTH 512

static var_t varlist[VARS_MAX];
static var_t* const ans = &varlist[0];
static unsigned short varcount = 1;

static FILE* varfile = NULL;
static const char* path;


static void OpenVarFile(const char* _path, const char* mode)
{
	const char filename[] = "ccalc_vars.txt";
	char drive[_MAX_DRIVE], dir[_MAX_DIR];
	char full[_MAX_DRIVE + _MAX_DIR + sizeof filename + 2];

	_splitpath_s(_path, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
	sprintf_s(full, _MAX_DRIVE + _MAX_DIR + sizeof filename + 2, "%s%s%s", drive, dir, filename);

	fopen_s(&varfile, full, mode);
}

static void ParseVarFile(FILE* file)
{
	char buf[LINE_MAX_WIDTH];

	fseek(varfile, 0, SEEK_SET);

	double val;

	while (!feof(file))
	{
		var_t* var;

		if (fscanf_s(varfile, "%s", buf, (unsigned)sizeof buf) != 1)
			break;

		if (fscanf_s(varfile, "%lf", &val) != 1)
			break;

		var = GetVar(buf, buf + strlen(buf));
		var->val = val;
		var->init = 1;
	}
}

//Open up the saved var file
void InitVarList(const char* _path)
{
	//setup the reserved var
	ans->init = 0;
	ans->name = malloc_s(4 * sizeof(char));
	strcpy_s(ans->name, 4, "ans");


	path = _path;
	OpenVarFile(path, "a+");
	if (!varfile)
	{
		printf("Error: Failed to open variable file\n");
		return;
	}

	ParseVarFile(varfile);
	fclose(varfile);
}

void ClearVarList()
{
	ans->init = 0;

	for (unsigned short i = 1; i < varcount; i++)
	{
		varlist[i].init = 0;
		free(varlist[i].name);
	}

	printf("Cleared variable list\n");
	varcount = 1;
}

void PrintVarList()
{
	if (varcount < 2)
		return;

	printf("%s", varlist[1].name);

	for (unsigned short i = 2; i < varcount; i++)
		printf(", %s", varlist[i].name);
	printf("\n");
}

//Save to the var file
void CleanupVars()
{
	OpenVarFile(path, "w");

	if (varfile)
	{
		for (unsigned short i = 0; i < varcount; i++)
		{
			if(varlist[i].init)
				fprintf_s(varfile, "%s %lf\n", varlist[i].name, varlist[i].val);
		}

		//re-write to the var file
		fclose(varfile);
	}
	else
		printf("Couldn't open variable file for saving\n");

	for (unsigned short i = 0; i < varcount; i++)
		free(varlist[i].name);
}

var_t* GetVar(const char* start, const char* end)
{
	var_t* cur;
	size_t strlen = end - start;

	if (varcount >= VARS_MAX)
		Exit(ERR_MEM);

	for (unsigned short i = 0; i < varcount; i++)
		if(!strncmp(varlist[i].name, start, strlen))
			return &varlist[i]; //this var exists already

	cur = &varlist[varcount];
	varcount++;

	cur->flags = 0;
	cur->name = malloc_s(strlen + 1);
	strncpy_s(cur->name, strlen + 1, start, strlen);

	return cur;
}

void SetVars(var_t** var_list, double value)
{
	ans->val = value;
	ans->init = 1;

	if (!var_list)
		return;

	//Iterating i directly will just move it through the static varlist
	for (var_t* i = *var_list; i; i = *(++var_list))
	{
		i->val = value;
		i->init = 1;
	}
}