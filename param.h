#pragma once

typedef struct var_s
{
	char* name;
	double val;
	union
	{
		struct
		{
			unsigned long long init : 1;
			unsigned long long used : 1;
		};
		unsigned long long flags;
	};
} var_t;

typedef struct param_s
{
	enum param_type_e
	{
		PARAM_TYPE_OP,
		PARAM_TYPE_NUM,
		PARAM_TYPE_VAR
	} type;

	union
	{
		double val;
		char op;
		const var_t* var;
	};

	struct param_s* next;
} param_t;

void SetParamVal(param_t* this, const void* val);
void SetParamVar(param_t* this, const void* var);
void SetParamOp(param_t* this, const void* op);

//Returns a dynamically allocated parameter 
//ERROR - input
param_t* ParseParams(int count, const char* args[]);

//ERROR - Parenthesis, syntax
param_t* ConvertParamList(const param_t* head, var_t*** assn_list);
void CleanupParamList(param_t* head);

void PrintParamList(const param_t* head);
