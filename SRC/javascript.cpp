#ifndef DISCARDJAVASCRIPT
#include "duktape/duktape.h"
//#include "duktape/duktape.cpp"

duk_context *ctxPermanent = NULL;
duk_context *ctxTransient = NULL;
duk_context *ctx;

#endif
#include "common1.h"
#ifndef WIN32
#define stricmp strcasecmp 
#define strnicmp strncasecmp 
#endif
void ChangeSpecial(char* buffer);

// there are 2 contexts:  a permanently resident one for the system and a transient one per volley.

// ONE instance per server... so all routines are shared until gone. Need to distinguish "inits" from calls.
// OR runtime per user, always init and call.

FunctionResult RunJavaScript(char* definition, char* buffer, unsigned int args)
{ // Javascript permanent {call void name type type} eval ...code...
	char* defstart = definition;
	bool inited = (*definition++ == '.');
	*buffer = 0; 
#ifndef DISCARDJAVASCRIPT
	char context[MAX_WORD_SIZE];
	definition = ReadCompiledWord(definition,context);
	if (!stricmp(context,"permanent"))
	{
		if (!ctxPermanent)
		{
			ctxPermanent = duk_create_heap_default();
			if (!ctxPermanent) return FAILRULE_BIT;	// unable to init it
		}
		ctx = ctxPermanent;
	}
	else
	{
		if (!ctxTransient)
		{
			ctxTransient = duk_create_heap_default();
			if (!ctxTransient) return FAILRULE_BIT;	// unable to init it
		}
		ctx = ctxTransient;
	}
	char word[MAX_WORD_SIZE];
	definition = ReadCompiledWord(definition,word); // is it a call?    call test string int 
	char name[MAX_WORD_SIZE];
	*name = 0;
	char returnType[MAX_WORD_SIZE];
	char* callbase = NULL;
	char* code = definition;
	
	if (!stricmp(word,"call")) // skip over the description for now
	{
		definition = ReadCompiledWord(definition,returnType); 
		definition = ReadCompiledWord(definition,name); 
		ReadCompiledWord(definition,word); // arg type there?
		callbase = definition; // start of arguments list
		code = definition;
		while (!stricmp(word,"string") || !stricmp(word,"int")  || !stricmp(word,"float")) 
		{
			code = definition; // start of arguments list
			definition = ReadCompiledWord(definition,word);
		}
	}

	// compile requirements - execute the code definition if not inited
	bool compile = false;
	if (!stricmp(word,"compile")) 
	{
		code = ReadCompiledWord(code,word);
		compile = true; // compile vs eval
		if (!*code) return FAILRULE_BIT; // require some code
	}
	else if (!stricmp(word,"eval")) 
	{
		if (!*code) return FAILRULE_BIT; // require some code
	}
	else if (!*name) return FAILRULE_BIT;	// need to define someting, be it a compile or a call
	
	char* terminator = strchr(code,'`');
	*terminator = 0; // hide this from javascript
	
	if (*code && !inited) // code was supplied, handle it if not yet executed
	{
		*defstart = '.';
		char file[SMALL_WORD_SIZE];
		ReadCompiledWord(code,file); 
		if (!stricmp(file,"file")) // read files
		{
			while ((code = ReadCompiledWord(code,file)) != NULL && !stricmp(file,"file"))
			{
				code = ReadCompiledWord(code,file);  // name
				char* name = file;
				if (file[0] == '"')
				{
					size_t len = strlen(file);
					file[len-1] = 0;
					++name;
				}
				if (!compile) duk_eval_file(ctx, name);
				else duk_compile_file(ctx, 0, name);
				duk_pop(ctx);
			}
		}
		else
		{
			if (!compile) duk_eval_string(ctx, (const char *)code); // execute the definition
			else duk_compile_string(ctx, 0, (const char *)code); // compile the definition
			duk_pop(ctx);  /* pop result/error */
		}
	}

	// now do a call, if one should be done
	if (*name)
	{
		duk_push_global_object(ctx);
		int found = (int) duk_get_prop_string(ctx, -1 /*index*/, name); // find the function 
		if (!found)
		{
			duk_pop(ctx); // discard context
			*terminator = '`';
			return FAILRULE_BIT;
		}
		unsigned int index = 0;
		FunctionResult result = NOPROBLEM_BIT;
		while (index < args)
		{
			char type[MAX_WORD_SIZE];
			*buffer = 0;
			if ((result = JavascriptArgEval(index,buffer)) != NOPROBLEM_BIT)
			{
				for (unsigned int i = 0; i < index; ++i) duk_pop(ctx); // discard saved args
				duk_pop(ctx); // discard context
				*terminator = '`';
				return FAILRULE_BIT;
			}
			callbase = ReadCompiledWord(callbase,type);
			if (!stricmp(type,"string"))
			{
				duk_push_string(ctx, buffer);
			}
			else if (!stricmp(type,"int" )) 
			{
				if (!(*buffer >= '0' && *buffer <= '9') && *buffer != '-' && *buffer != '+')
				{
					result = FAILRULE_BIT;
					break;
				}
				duk_push_int(ctx, atoi(buffer));
			}
			else if (!stricmp(type,"float" )) 
			{
				if (!(*buffer >= '0' && *buffer <= '9') && *buffer != '-' && *buffer != '+' && *buffer != '.')
				{
					result = FAILRULE_BIT;
					break;
				}
				duk_push_number(ctx, (double)atof(buffer));
			}
			else break;
			++index;
		}
		*buffer = 0;
		if (result != NOPROBLEM_BIT) // abandon the call
		{				
			for (unsigned int i = 0; i < index; ++i) duk_pop(ctx); // discard saved args
			duk_pop(ctx); // discard context
			*terminator = '`';
			return FAILRULE_BIT;
		}

		if (duk_pcall(ctx, args) != 0) // call failed
		{
			printf("Javascript Error: %s\r\n", duk_safe_to_string(ctx, -1));
			duk_pop(ctx);
			*terminator = '`';
			return FAILRULE_BIT;
		} 
		else 
		{
			if (!stricmp(returnType,"string")) 	
			{
				strcpy(buffer,duk_safe_to_string(ctx, -1)); // assumes there is a return string!
				if (strchr(buffer,'\n') || strchr(buffer,'\r') || strchr(buffer,'\t')) ChangeSpecial(buffer); // do not allow special characters in string
			}
			else if (!stricmp(returnType,"int")) strcpy(buffer,duk_safe_to_string(ctx, -1)); // assumes there is a return string!
			else if (!stricmp(returnType,"float")) strcpy(buffer,duk_safe_to_string(ctx, -1)); // assumes there is a return string!
			duk_pop(ctx);
			*terminator = '`';
			return NOPROBLEM_BIT;
		}
	}
	*terminator = '`';
	return NOPROBLEM_BIT;
#else
	return FAILRULE_BIT; // if javascript not compiled into engine
#endif
}

void DeletePermanentJavaScript()
{
#ifndef DISCARDJAVASCRIPT
	duk_destroy_heap(ctxPermanent);
	ctxPermanent = NULL;
#endif
}

void DeleteTransientJavaScript()
{
#ifndef DISCARDJAVASCRIPT
	duk_destroy_heap(ctxTransient);
	ctxTransient = NULL;
#endif
}
