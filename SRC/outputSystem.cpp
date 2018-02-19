#include "common.h"
unsigned int maxOutputUsed = 0;
unsigned int currentOutputLimit = MAX_BUFFER_SIZE;	// max size of current output base
char* currentOutputBase = NULL;		// current base of buffer which must not overflow
char* currentRuleOutputBase = NULL;	// the partial buffer within outputbase started for current rule, whose output can be canceled.
#define MAX_OUTPUT_NEST 50
static char* oldOutputBase[MAX_OUTPUT_NEST];
static char* oldOutputRuleBase[MAX_OUTPUT_NEST];
static unsigned int oldOutputLimit[MAX_OUTPUT_NEST];
int oldOutputIndex = 0;
unsigned int outputNest = 0;
static char* ProcessChoice(char* ptr,char* buffer,FunctionResult &result,int controls) ;
static char* Output_Function(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once);
static char* Output_Dollar(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once,bool nojson);
#ifdef JUNK
Special strings:

When you put ^"xxxx" as a string in a normal output field, it is a format string. By definition you didnt need it compileable.
It will be treated...

When you use it as an argument to a function (top level), it is compiled.

When you put ^"xxxx" as a string in a table, it will be compiled and be either a pattern compile or an output compile.   
Internally that becomes "^xxx" which is fully executable.


#endif

bool SafeCopy(char* output, char* word, bool space)
{		
	size_t len = strlen(word);
	if (((output - currentOutputBase) + len ) > (currentOutputLimit - 200)) 
	{
		ReportBug((char*)"output buffer too big for copy %s\r\n",output) // buffer overflow
		return false;
	}
	if (space) {*output++ = ' '; *output = 0;}
	strcpy(output,word);
	return true;
}

void ResetOutput()
{
	outputNest = 0;
}

void PushOutputBuffers()
{
	oldOutputBase[oldOutputIndex] = currentOutputBase;
	oldOutputRuleBase[oldOutputIndex] = currentRuleOutputBase;
	oldOutputLimit[oldOutputIndex] = currentOutputLimit;
	++oldOutputIndex;
	if (oldOutputIndex == MAX_OUTPUT_NEST) 
	{
		ReportBug("PushOutputBuffer limit reached");
		--oldOutputIndex; // just fail it
	}
}

void PopOutputBuffers()
{
	--oldOutputIndex;
	if (oldOutputIndex < 0) ++oldOutputIndex;
	currentOutputBase = oldOutputBase[oldOutputIndex];
	currentRuleOutputBase = oldOutputRuleBase[oldOutputIndex];
	currentOutputLimit = oldOutputLimit[oldOutputIndex];
}

void AllocateOutputBuffer()
{
	PushOutputBuffers();
	currentRuleOutputBase = currentOutputBase = AllocateBuffer(); // cant use stack- others may allocate on it from output and we cant free them
	currentOutputLimit = maxBufferSize;
	*currentOutputBase = 0;
}

void FreeOutputBuffer()
{
	FreeBuffer(); // presumed the current buffer allocated via AllocateOutputBuffer
	PopOutputBuffers();
}

static bool IsAssignmentOperator(char* word)
{
	if ((*word == '<' || *word == '>') && word[1] == *word && word[2] == '=') return true;	 // shift operators
	if (*word == '|' && word[1] == '^' && word[2] == '=') return true;
	return ((*word == '=' && word[1] != '=' && word[1] != '>') || (*word && *word != '!' && *word != '\\' && *word != '=' && word[1] == '='   )); // x = y, x *= y
}

char* GetCommandArg(char* ptr, char* buffer,FunctionResult& result,unsigned int control) 
{
	int oldImpliedSet = impliedSet; // so @0object will decode
	if (!(control & ASSIGNMENT)) impliedSet = ALREADY_HANDLED;
	if (control == 0) control |= OUTPUT_KEEPSET | OUTPUT_ONCE | OUTPUT_NOCOMMANUMBER;
	else control |= OUTPUT_ONCE | OUTPUT_NOCOMMANUMBER;

	unsigned int size = MAX_BUFFER_SIZE - (buffer - currentOutputBase); // how much used
	if (size > MAX_BUFFER_SIZE) size = MAX_WORD_SIZE * 4; // arbitrary assumption
	ptr = FreshOutput(ptr, buffer, result, control, size);
	if (!(control & ASSIGNMENT))  impliedSet = oldImpliedSet; // assignment of @0 = ^querytopics needs to be allowed to change to alreadyhandled
	return ptr;
}

char* ReadShortCommandArg(char* ptr, char* buffer,FunctionResult& result,unsigned int control) // always word size or less
{
	int oldImpliedSet = impliedSet; // so @0object will decode
	if (!(control & ASSIGNMENT)) impliedSet = ALREADY_HANDLED;
	if (control == 0) control |= OUTPUT_KEEPSET | OUTPUT_NOTREALBUFFER | OUTPUT_ONCE | OUTPUT_NOCOMMANUMBER;
	else control |= OUTPUT_ONCE | OUTPUT_NOCOMMANUMBER| OUTPUT_NOTREALBUFFER ;
	char* answer = FreshOutput(ptr,buffer,result,control,MAX_WORD_SIZE);
	if (!(control & ASSIGNMENT)) impliedSet = oldImpliedSet; // assignment of @0 = ^querytopics needs to be allowed to change to alreadyhandled
	return answer;
}

static char* FixFormatOutput(char* output,unsigned int controls) // revises output in place
{
	size_t len = strlen(output);
	if (*output == '"' && output[len-1] == '"' && controls & OUTPUT_NOQUOTES) // strip quotes
	{
		output[--len] = 0;	// remove trailing
		memmove(output,output+1,len--);
	}
	if (controls & OUTPUT_NOUNDERSCORE)
	{
		char* at = output;
		while (( at = strchr(at,'_'))) *at = ' ';
	}
	return output + len;
}

bool LegalVarChar(char at)
{
	return  (IsAlphaUTF8OrDigit(at) || at == '_' || at == '-' );
}

static char* ReadUserVariable(char* input, char* var)
{		
	char* at = input++; // skip $ and below either $ or _ if one exists or first legal char
	while (LegalVarChar(*++input) || *input == '.' || *input == '[' || *input == ']' || (*input == '$' && *(input-1) == '.'))
	{
		if (*input == '.'  || *input == '[' || *input == ']')
		{
			if (!LegalVarChar(input[1]) && input[1] != '$' && input[1] != '[') break; // not a var dot, just an ordinary one 
		}
	} 
	strncpy(var,at,input-at);
	var[input-at] = 0;
	return input;
}

static char* ReadMatchVariable(char* input, char* var)
{		
	char* at = input; 
	while (IsDigit(*++input)){;} 
	strncpy(var,at,input-at);
	var[input-at] = 0;
	return input;
}

void ReformatString(char starter, char* input,char*& output, FunctionResult& result,unsigned int controls, char* space) // take ^"xxx" format string and perform substitutions on variables within it
{
	*output = 0;
	controls |= OUTPUT_NOCOMMANUMBER; // never reformat a number from here
	size_t len = strlen(input);
	if (!len)
		return;
	--len;
	char c = input[len];
	char* original = input;
	input[len] = 0;	// remove closing "
	if (*input == ':') // has been compiled by script compiler. safe to execute fully. actual string is "^:xxxxx" 
	{
		++input;
 		Output(input,output,result,controls); // directly execute the content but no leading space
		input[len] = c;
		return;
	}	
	bool str = false;
	char* start = output;
	*output = 0;
	char mainValue[3];
	mainValue[1] = 0;
	char var[200]; // no variable name should be this big
	char prior = 0;
	while (input && *input)
	{
		if (*input == '"' && prior != '\\') str = !str; // toggle string status
		prior = *input;
		if (prior == '^' && input[1] == USERVAR_PREFIX && (IsAlphaUTF8(input[2]) ||  input[2] == '$')) // ^ user variable
		{
			var[0] = '^';
			input = ReadUserVariable(input+1,var+1); // end up after the var
			Output(var,output,result,controls);
			output = FixFormatOutput(output,controls); 
		}
		else if (prior == '^' && input[1] == '_' && IsDigit(input[2])) // ^ canonical match variable
		{
			var[0] = '^';
			input = ReadMatchVariable(input+1,var+1); // end up after the var
			Output(var,output,result,controls);
			output = FixFormatOutput(output,controls); 
		}
		else if (prior == USERVAR_PREFIX && (IsAlphaUTF8(input[1]) ||  input[1] == TRANSIENTVAR_PREFIX ||  input[1] == LOCALVAR_PREFIX)) // user variable
		{
			input = ReadUserVariable(input,var); // end up after the var
			Output(var,output,result,controls);
			output = FixFormatOutput(output,controls); 
		}
		else if (prior == SYSVAR_PREFIX && IsAlphaUTF8(input[1]))
		{
			input = ReadCompiledWord(input,var,false,true);
			char* at = var;
			while (*++at && IsAlphaUTF8(*at)){;}
			input -= strlen(at);
			*at = 0;
			char* value = SystemVariable(var,NULL);
			if (*value) 
			{
				strcpy(output,value);
				output = FixFormatOutput(output,controls);
			}
			else if (!FindWord(var)) 
			{
				strcpy(output,var);
				output = FixFormatOutput(output,controls); // not a system variable
			}
		}
		else if (prior == '_' && IsDigit(input[1]) && *(input-1) != '@') // canonical match variable
		{
			input = ReadMatchVariable(input,var); // end up after the var
			Output(var,output,result,controls);
			output = FixFormatOutput(output,controls); 
		}
		else if (prior == '\'' && input[1] == '_' && IsDigit(input[2])) // quoted match variable
		{
			var[0] = '\'';
			input = ReadMatchVariable(input+1,var+1); // end up after the var
			Output(var,output,result,controls);
			output = FixFormatOutput(output,controls); 
		}
		else if (prior == '^' && input[1] == '\'' && input[2] == '_' && IsDigit(input[3])) // ^ quoted match variable
		{
			var[0] = '^';
			var[1] = '\'';
			input = ReadMatchVariable(input+2,var+2); // end up after the var
			Output(var,output,result,controls);
			output = FixFormatOutput(output,controls); 
		}
		else if (prior == '@' && IsDigit(input[1])) // factset
		{
			input = ReadCompiledWord(input,var,false,true);
			
			// go get value of reference
			GetCommandArg(var,output,result,0);
			output = FixFormatOutput(output,controls);
			if (result & ENDCODES) 
			{
				output = start + 1;  // null return
				break;
			}
		}
		else if (prior == '^' && IsDigit(input[1])) // function variable
		{
			char* base = input; 
			while (*++input && IsDigit(*input)){;} // find end of function variable name 
			char* tmp = FNVAR(base+1);
			// if tmp turns out to be $var or _var %var, need to recurse to get it
			if (*tmp == LCLVARDATA_PREFIX && tmp[1] == LCLVARDATA_PREFIX)
			{
				strcpy(output,tmp+2);
				output = FixFormatOutput(output,controls); 	// is already evaled
			}
			else if (*tmp == USERVAR_PREFIX && !IsDigit(tmp[1])) // user variable (could be json object ref)
			{
				strcpy(output,GetUserVariable(tmp));
				output = FixFormatOutput(output,controls); 
			}
			else if (*tmp == '_' && IsDigit(tmp[1])) // canonical match variable
			{
				char* wildbase = tmp++;
				if (IsDigit(*tmp)) ++tmp; // 2nd digit
				strcpy(output,GetwildcardText(GetWildcardID(wildbase),true));
				output = FixFormatOutput(output,controls); 
			}
			else if (*tmp == '\'' && tmp[1] == '_' && IsDigit(tmp[2])) // quoted match variable
			{
				char* wildbase = ++tmp;
				++tmp;
				if (IsDigit(*tmp)) ++tmp; // 2nd digit
				strcpy(output,GetwildcardText(GetWildcardID(wildbase),false));
				output = FixFormatOutput(output,controls);
			}
			else if (*tmp == SYSVAR_PREFIX && IsAlphaUTF8(tmp[1])) // system variable
			{
				char* value = SystemVariable(tmp,NULL);
				if (*value) 
				{
					strcpy(output,value);
					output = FixFormatOutput(output,controls);
				}
				else if (!FindWord(tmp)) 
				{
					strcpy(output,tmp);
					output = FixFormatOutput( output,controls); // not a system variable
				}
			}	
			else if (*tmp == FUNCTIONSTRING && (tmp[1] == '"' || tmp[1] == '\''))
			{
				ReformatString(tmp[1],tmp+2,output,result,controls);
				output += strlen(output);
			}
			else if (!stricmp(tmp,(char*)"null")) {;} // value is to be ignored
			else 
			{
				strcpy(output,tmp);
				output = FixFormatOutput(output,controls); 
			}
		}
		else if (prior == '^' && (IsAlphaUTF8(input[1]) ))
		{
			char* at = var;
			*at++ = *input++;
			while (IsLegalNameCharacter(*input) ) *at++ = *input++;
			*at = 0;
			if (output != start && *(output-1) == ' ') --output; // no space before
			input = Output_Function(var, input, NULL, output, controls,result,false);
			output += strlen(output);
			if (result & ENDCODES)  return;
		}
		else if (prior == '\\') // protected special character
		{
			++input;
			if (starter == '"' && *input == 'n') 
			{
				strcpy(output,"\\n");
				output += 2;
			}
			else if (starter == '"' && *input == 't') 
			{
				strcpy(output,"\\t");
				output += 2;
			}
			else if (starter == '"' && *input == 'r') 
			{
				strcpy(output,"\\r");
				output += 2;
			}
			else if (starter == '"') *output++ = *input; // just pass along the protected char in ^"xxx" strings
			else // is ^'xxxx' string - other than our special ' we need, leave all other escapes alone as legal json
			{
				if (str) *output++ = *input;  // copy inside a string
				else if (*input == '\'' ) *output++ = *input;  // cs required the \, not in final output
				else {*output++ = '\\'; *output++ = *input;}  // json can escape anything, particularly doublequote
			}
			++input; // skip over the specialed character
		}
		else // ordinary character
		{
			*mainValue = *input++;
			strcpy(output,mainValue);
			output = FixFormatOutput(output,controls);
		}
		*output = 0;
	}
	original[len] = c;
	*output = 0; // when failures, return the null string
	if (trace & TRACE_OUTPUT) Log(STDTRACELOG,(char*)" %s",start);
}

void StdNumber(char* word,char*& buffer,int controls) // text numbers may have sign and decimal
{
	size_t len = strlen(word);
	char* ptr = word;
    if ( IsAlphaUTF8(*ptr) || !IsDigitWord(word, AMERICAN_NUMBERS) || strchr(word,':')) // either its not a number or its a time - leave unchanged
    {
        strcpy(buffer,word);  
		if (controls & OUTPUT_NOUNDERSCORE)
		{
			char* at = buffer;
			while (( at = strchr(at,'_'))) *at = ' ';
		}

        return;
    }

	// capture any percentage symbol
	char* end = word + len;
	bool percent = false;
	if (*(end - 1) == '%')
	{
		*--end = 0;
		percent = true;
	}

	int useNumberStyle = numberStyle;
	if (controls & OUTPUT_NOCOMMANUMBER || len < 5) useNumberStyle = NOSTYLE_NUMBERS;

	if (IsFloat(word,end))
	{
		if (!fullfloat) // insure is not full
		{
			char c = word[len];
			word[len] = 0;
			WriteFloat(buffer, atof(word), useNumberStyle);
			word[len] = c;
		}
		else // write out what we have, don't redo the formatting
		{
			FormatFloat(word, buffer, useNumberStyle);
		}
	}
	else
	{
		// add commas between number triples, unless not needed
		// except india uses doubles until final triple
		WriteInteger(word, buffer, useNumberStyle);
	}
	if (percent) strcat(buffer, "%");
}

char* StdIntOutput(int n)
{
	char buffer[50];
	static char answer[50];
	*answer = 0;
#ifdef WIN32
	sprintf(buffer,(char*)"%I64d",(long long int) n); 
#else
	sprintf(buffer,(char*)"%lld",(long long int) n); 
#endif
	char* ptr = answer;
	StdNumber(buffer,ptr,0);
	return answer;
}

static char* ProcessChoice(char* ptr,char* buffer,FunctionResult &result,int controls) //   given block of  [xx] [yy] [zz]  randomly pick one
{
	char* choice[CHOICE_LIMIT];
	char** choiceset = choice;
	int count = 0;
    char* endptr = 0;

	//   gather choices
	while (*ptr && *ptr == '[') // get next choice for block   may not nest choice blocks...
	{
		//   find closing ]
		endptr = ptr-1;
		while (ALWAYS) 
		{
			endptr = strchr(endptr+1,']'); // find a closing ] 
			if (!endptr) // failed
			{
				respondLevel = 0;
				return 0;
			}
			if (*(endptr-1) != '\\') break; // ignore literal \[
		}
        // choice can be simple: [ xxx ]  or conditional [ $var1 xxx] or conditional [!$var1 xx] but avoid assignment [ $var1 = 10 xxx ] 
		char word[MAX_WORD_SIZE];
		char* base = ptr + 1; // start of 1st token within choice
		char* simpleOutput = ReadCompiledWord(base,word);
		char* var = word;
		bool notted = false;
		if (*word == '!')
		{
			notted = true;
			++var;
		}
		if (*var == USERVAR_PREFIX && (IsAlphaUTF8(var[1]) || var[1] == TRANSIENTVAR_PREFIX   ||  var[1] == LOCALVAR_PREFIX)) // user variable given
		{
			ReadCompiledWord(simpleOutput,tmpWord);
			if (*tmpWord == '=' || tmpWord[1] == '=') choiceset[count++] = base; //  some kind of assignment, it's all simple output
			else if (!notted && !*GetUserVariable(var)) {;}	// user variable test fails
			else if (notted && *GetUserVariable(var)) {;}	// user variable test fails
			else choiceset[count++] = simpleOutput;
		}
		else choiceset[count++] = base;

		ptr = SkipWhitespace(endptr + 1);   // past end of choice
	}

	//   pick a choice randomly - a choice starts past the [  but has visible the closing ]
	respondLevel = 0;
	while (count > 0)
	{
		int r = random(count);
		if (outputchoice >= 0 && outputchoice < count) r = outputchoice; // forced by user
		char* ptr = SkipWhitespace(choiceset[r]);
		if (*ptr == ']') break; // choice does nothing by intention
		char level = 0;
		if (IsAlphaUTF8(*ptr) && ptr[1] == ':' && ptr[2] == ' ') 
		{
			level = *ptr;
			ptr += 3; // skip special rejoinder
		}
		Output(ptr,buffer,result,controls);
		if (result & ENDCODES) break; // declared done
		
		// is choice a repeat of something already said... if so try again
		if (*buffer && HasAlreadySaid(buffer)) 
		{
			if (trace & TRACE_OUTPUT) Log(STDTRACELOG,(char*)"Choice %s already said\r\n",buffer);
			*buffer = 0;
			choiceset[r] = choiceset[--count];
		}
		else 
		{
			if (level) respondLevel = level;
			break; // if choice didnt fail, it has completed, even if it generates no output
		}
	}
	return (endptr[1]) ? (endptr+2) : (endptr+1); //   skip to end of rand past the ] and space
}

char* FreshOutput(char* ptr,char* buffer,FunctionResult &result,int controls,unsigned int limit)
{ // used to get isolated values not normally part of real output stream
	++outputNest;
	if (limit != maxBufferSize) AllocateOutputBuffer(); // where he wants to put it is SMALL and we're not ready for that. allocate a big bufer can copy later
	else 
	{
		PushOutputBuffers();
		currentRuleOutputBase = currentOutputBase = buffer; // is a normal buffer
	}
	ptr = Output(ptr,currentOutputBase,result,controls); // has no content before it
	if (limit != maxBufferSize) // someone's small local buffer
	{
		size_t olen = strlen(currentRuleOutputBase);
		if (olen >= limit) 
		{
			strncpy(buffer,currentRuleOutputBase,limit-1);
			buffer[limit-1] = 0;
			ReportBug((char*)"FreshOutput of %d exceeded caller limit of %d. Truncated: %s\r\n",olen,limit,currentRuleOutputBase);
		}
		else strcpy(buffer,currentRuleOutputBase);
		FreeOutputBuffer();
	}
	else PopOutputBuffers();
	--outputNest;
	return ptr;
}

#ifdef INFORMATION
There are two kinds of output streams. The ONCE only stream expects to read an item and return.
If a fail code is hit when processing an item, then if the stream is ONCE only, it will be done
and return a ptr to the rest. If a general stream hits an error, it has to flush all the remaining
tokens and return a ptr to the end.
#endif

static char* Output_Percent(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once)
{			
	// Handles system variables:  %date
	// Handles any other % item - %
	if (IsAlphaUTF8(word[1])) // must be a system variable
    {
		if (!once && IsAssignmentOperator(ptr)) return PerformAssignment(word,ptr,buffer,result); //   =  or *= kind of construction
		strcpy(word,SystemVariable(word,NULL));
	}
	if (*word) strcpy(buffer,word); 
	return ptr;
}

static char* Output_Backslash(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result)
{
	// handles newline:  \n
	// handles backslashed strings: \"testing"  means dump the rest of the token out
	// handles backslashed standalone double quote - \"  - means treat as even/odd pair and on 2nd one (closing) do not space before it
	// handles any other backslashed item:  \help  means just put out the item without the backslash
	if (word[1] == 'r' && !word[2]) 
	{
		strcpy(buffer,(char*)"\\r");
		return ptr;		
	}
	if (word[1] == 'n')  //   \n
	{
		if (space) --buffer;	// remove space before newline
#ifdef WIN32
		strcpy(buffer,(char*)"\\r\\n");
#else
		strcpy(buffer,(char*)"\\n");
#endif
		ptr -= strlen(word);
		if (*ptr == 'n') --ptr;
		ptr += 2;
	}
	else if (word[1] == 't') // tab
	{
		strcpy(buffer,(char*)"\\t");
		ptr -= strlen(word);
		if (*ptr == 't') --ptr;
		ptr += 2;
	}
    else strcpy(buffer,word+1);  //   some other random backslashed content, including \" 
	return ptr;
}

static char* ResetOutputPtr(char* start,char* buffer)
{
	char* at = strrchr(start,'`');	// where it ended
	if (!at) return start;
	size_t len = strlen(at+1); 
	memmove(start,at+1,len+1);	// shift new data back to start
	if (currentOutputBase != start && start[len-1] == ' ') --len;	// output ended with a space, remove it
	start[len] = 0;
	return start + len; // resume back at original buffer location
}

static char* Output_Function(char* word, char* ptr,  char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once)
{
	char* start = buffer;
	if (IsDigit(word[1]))  //   function variable
	{	
		if (!once && IsAssignmentOperator(ptr)) ptr = PerformAssignment(word,ptr,buffer,result); 
		else
		{
			char* value = FNVAR(word+1);
			size_t len = strlen(value);
			size_t size = (buffer - currentOutputBase);
			if ((size + len) >= (currentOutputLimit-50) ) 
			{
				result = FAILRULE_BIT;
				return ptr;
			}
			if (*value == LCLVARDATA_PREFIX && value[1] == LCLVARDATA_PREFIX) 
				strcpy(buffer,value+2); // already evaluated. do not reeval
			else
			{
				strcpy(buffer,value);
				if (*buffer)
				{
					*word = ENDUNIT;	// marker for retry
					word[1] = '^';	// additional marker for function variables
				}
			}
		}
	}
	else if (word[1] == '"' || word[1] == '\'') ReformatString(word[1],word+2,buffer,result,0,space); // functional string, uncompiled.  DO NOT USE function calls within it
	else  if (word[1] == USERVAR_PREFIX || word[1] == '_' || word[1] == '\'' || (word[1] == '^' && IsDigit(word[2]))) // ^$$1 = null or ^_1 = null or ^'_1 = null or ^^which = null is indirect user assignment or retrieval
	{
		if (!once && IsAssignmentOperator(ptr)) // we are lefthand side indirect
		{
			if (word[1] != '^' || !IsDigit(word[2])) // anything but ^^2
			{
				Output(word+1,buffer,result,controls); // no leading space  - we now have the variable value from the indirection
				strcpy(word,buffer);
			}
			*buffer = 0;
			ptr = PerformAssignment(word,ptr,buffer,result,true); //   =  or *= kind of construction -- dont do json indirect assignment
		}
		else // we are right side (expression) indirect
		{
			Output(word+1,buffer,result,controls); // no leading space  - we now have the variable value from the indirection
			if (word[1] == USERVAR_PREFIX) // direct retry to avoid json issues
			{
				strcpy(word,GetUserVariable(word+1));
				Output_Dollar(word, "", space,buffer,controls,result,false,false); // allow json processing
			}
			else *word = ENDUNIT;	// marker for retry
		}
	}
	else if (!strcmp(word,(char*)"^if")) ptr = HandleIf(ptr,buffer,result);  
	else if (!strcmp(word,(char*)"^loop")) ptr = HandleLoop(ptr,buffer,result); 
	else if (word[1] == '^') // if and loop
	{
		if (!once && IsAssignmentOperator(ptr))  
			ptr = PerformAssignment(word,ptr,buffer,result); //   =  or *= kind of construction
		else if (!word[2]) strcpy(buffer,word); // "^^" exponent operator
		else result = FAILRULE_BIT;
	}
	else // functions or ordinary words
	{
		if (*ptr != '(' || !word[1]) 
		{
			strcpy(buffer,word); // a non function
		}
		else // ordinary function
		{
			if (*currentRuleOutputBase && (!strcmp(word,(char*)"^gambit") || !strcmp(word,(char*)"^respond") || !strcmp(word,(char*)"^reuse") || !strcmp(word,(char*)"^retry") || !strcmp(word,(char*)"^refine")  || !strcmp(word,(char*)"^print")   )) // leaving current rule
			{
				char* end = currentRuleOutputBase;
				if (*currentRuleOutputBase == '`') end = strrchr(currentRuleOutputBase,'`') + 1; // is there really something new there?
				if (*end && !AddResponse(currentRuleOutputBase,responseControl)) result = FAILRULE_BIT;
				if (*start == '`') buffer = ResetOutputPtr(start,buffer);
			}

			ptr =  DoFunction(word,ptr,buffer,result); 

			if (space && *space != ' ' && result != ENDCALL_BIT) // we need to add a space, but not if requesting a call return ^return
			{
				memmove(buffer+1,buffer,strlen(buffer) + 1);
				*buffer = ' ';
			}
		}
	}
	return ptr;
}

static char* Output_AttachedPunctuation(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result)
{
	strcpy(buffer,word); 
	return ptr;
}

static char* Output_Text(char* word,char* ptr,  char* space,char*& buffer, unsigned int controls,FunctionResult& result)
{
	// handles text or script
	if (*ptr != '(' || controls & OUTPUT_FACTREAD || IsDigit(*word)) StdNumber(word,buffer,controls); //   SIMPLE word  - paren if any is a nested fact read, or number before ( which cant be ^number
	else  //   function call missing ^
	{
		memmove(word+1,word,strlen(word)+1);
		*word = '^';  // supply ^
		ptr = Output_Function(word, ptr,space,buffer, controls,result,false);
		if (result == UNDEFINED_FUNCTION) // wasnt a function after all.
		{
			result = NOPROBLEM_BIT;
			StdNumber(word+1,buffer,controls);			
		}
	}
	return ptr;
}

static char* Output_AtSign(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once)
{
	// handles factset assignement: @3 = @2
	// handles factset field: @3object
	if (!once && IsAssignmentOperator(ptr)) ptr = PerformAssignment(word,ptr,buffer,result);
	else if (impliedSet != ALREADY_HANDLED) strcpy(buffer,word);
    else if (IsDigit(word[1]) && IsAlphaUTF8(*GetSetType(word)) && !(controls & OUTPUT_KEEPQUERYSET)) //   fact set reference
    {
		int store = GetSetID(word);
		if (store == ILLEGAL_FACTSET) return ptr;
		unsigned int count = FACTSET_COUNT(store);
		if (!count || count >= MAX_FIND) return ptr;
		FACT* F = factSet[store][1]; // use but don't use up most recent fact
		MEANING T;
		uint64 flags;
		char type = *GetSetType(word);
		if (type  == 's' ) 
		{
			T = F->subject;
			flags = F->flags & FACTSUBJECT;
		}
		else if (type== 'v')
		{
			T = F->verb;
			flags = F->flags & FACTVERB;
		}
		else if (type  == 'f' ) 
		{
			T = Fact2Index(F);
			flags =  FACTSUBJECT;
		}
		else if (type == 'a' && impliedWild != ALREADY_HANDLED)
		{
			ARGUMENT(1) = AllocateStack(word);
			result = FLR(buffer,(char*)"l");
			return ptr;
		}
		else 
		{
			T = F->object;
			flags = F->flags & FACTOBJECT;
		}
		char* answer;
		char buf[100];
		if (flags) 
		{
			sprintf(buf,(char*)"%d",T);
			answer = buf;
		}
		else  answer = Meaning2Word(T)->word;
 		strcpy(buffer,answer);
	}
	else  strcpy(buffer,word);
    return ptr;
 }

static char* Output_Bracket(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result)
{
	// handles normal token: [ice 
	// handles choice: [ this is data ]
	if (word[1] || (controls & OUTPUT_NOTREALBUFFER && !(controls & OUTPUT_FNDEFINITION)) || !*ptr) StdNumber(word,buffer,controls); // normal token
	else 
	{
		while (*ptr != '[') --ptr;
		ptr = ProcessChoice(ptr,buffer,result,controls);   
	}
	return ptr;
}

static char* Output_Quote(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result)
{
	// handles possessive: 's
	// handles original wildcard: '_2
	// handles quoted variable: '$hello
	if (word[1] == 's' && !word[2])	strcpy(buffer,(char*)"'s");	// possessive
	else if (word[1] == '_')			// original wildcard
	{
		int index = GetWildcardID(word+1); //   which one
		char* at = wildcardOriginalText[index];
		StdNumber(at,buffer,controls);
		if (controls & OUTPUT_NOQUOTES && *buffer == '"') // remove quotes from variable data
		{
			size_t len = strlen(buffer);
			if (buffer[len-1] == '"')
			{
				buffer[len-1] = 0;
				memmove(buffer,buffer+1,len);
			}
		}
	}
	else if (word[1] == USERVAR_PREFIX || word[1] == '@' || word[1] == SYSVAR_PREFIX)  //    variable or factset or system variable quoted, means dont reeval its content
	{
		strcpy(buffer,word+1);
	}
	else if (word[1] == '^' && IsDigit(word[2]))  //   function variable quoted, means dont reeval its content
	{
		size_t len = strlen(FNVAR(word+2));
		size_t size = (buffer - currentOutputBase);
		if ((size + len) >= (currentOutputLimit-50) ) 
		{
			result = FAILRULE_BIT;
			return ptr;
		}

		strcpy(buffer,FNVAR(word+2));
	}
	else StdNumber(word,buffer,controls);
	return ptr;
}


static char* Output_String(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result)
{
	// handles function string: (char*)"^ .... "  which means go eval the contents of the string
	// handles simple string: (char*)"this is a string"  which means just put it out (with or without quotes depending on controls)
	// handles compiled strings:  "^:xxx" which means formatting has already been performed (function arguments)
	size_t len;
	if (controls & OUTPUT_UNTOUCHEDSTRING && word[1] != FUNCTIONSTRING) strcpy(buffer,word); //functions take untouched strings typically
	else if (word[1] == FUNCTIONSTRING) // treat as format string   
	{
		if (!dictionaryLocked) strcpy(buffer,word); // untouched - function strings passed as arguments to functions are untouched until used up. - need untouched so can create facts from tables with them in it
		else ReformatString(*word,word+2,buffer,result,controls);
	}
	else if (controls & OUTPUT_NOQUOTES)
	{
		strcpy(buffer,word+1);
		len = strlen(buffer);
		if  (buffer[len-1] == '"') buffer[len-1] = 0; // remove end quote
	}
	else strcpy(buffer,word); // untouched - function strings passed as arguments to functions are untouched until used up. - need untouched so can create facts from tables with them in it
	return ptr;
}
	
static char* Output_Underscore(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once)
{
	// handles wildcard assigment: _10 = hello
	// handles wildcard: _19
	// handles simple _ or _xxxx 
	if (!once && IsAssignmentOperator(ptr)) ptr = PerformAssignment(word,ptr,buffer,result); 
	else if (IsDigit(word[1])) // wildcard 
	{
		int id = GetWildcardID(word);
		if (id >= 0)
		{
			StdNumber(wildcardCanonicalText[id],buffer,controls);
			if (controls & OUTPUT_NOQUOTES && *buffer == '"') // remove quotes from variable data
			{
				size_t len = strlen(buffer);
				if (buffer[len-1] == '"')
				{
					buffer[len-1] = 0;
					memmove(buffer,buffer+1,len);
				}
			}

			if (*buffer == '^' && IsAlphaUTF8(buffer[1]) && *SkipWhitespace(ptr) == '(') *word = ENDUNIT; // if fn call substituted, force reeval
		}
	}
	else strcpy(buffer,word); // stand-alone _ or some non wildcard
	return ptr;
}

static char* Output_Dollar(char* word, char* ptr, char* space,char*& buffer, unsigned int controls,FunctionResult& result,bool once,bool nojson)
{
	// handles user variable assignment: $myvar = 4
	// handles user variables:  $myvar
	// handles US money: $1000.00
	if ((word[1] == '$' || word[1] == '_') && !word[2]) StdNumber(word,buffer,controls); // simple $_ or $$
	else if (word[1] && !IsDigit(word[1])) // variable
    {
		if (controls & OUTPUT_EVALCODE && !(controls & OUTPUT_KEEPVAR)) 
		{
			char* answer = GetUserVariable(word,nojson);
			if (*answer == USERVAR_PREFIX && ( answer[1] == LOCALVAR_PREFIX || answer[1] == TRANSIENTVAR_PREFIX || IsAlphaUTF8(answer[1]))) strcpy(word,answer); // force nested indirect on var of a var value
		}

		if (!once && IsAssignmentOperator(ptr)) 
			ptr = PerformAssignment(word,ptr,buffer,result); 
		else
		{
			char* value = GetUserVariable(word,nojson);
			StdNumber(value,buffer,controls);
			char* at = SkipWhitespace(buffer);
			if (controls & OUTPUT_NOQUOTES && *at == '"') // remove quotes from variable data
			{
				size_t len = strlen(at);
				if (at[len-1] == '"')
				{
					at[len-1] = 0;
					memmove(at,at+1,len);
				}
			}
			if (*at == '"' && at[1] == FUNCTIONSTRING) *word = ENDUNIT; // reeval the function string
		}
	}	
    else StdNumber(word,buffer,controls); // money or simple $
	return ptr;
}

char* Output(char* ptr,char* buffer,FunctionResult &result,int controls)
{ // moves any special stuff to front of buffer
	//   an output stream consists of words, special words, [] random zones, commands, C-style script. It autoformats whitespace.
	*buffer = 0; 
	char* start = buffer;
    result = NOPROBLEM_BIT;
	if (*ptr == ENDUNIT && ptr[1] == ENDUNIT) ptr += 2;		// skip over a transient $_ marker
	if (!*ptr) return NULL;	
	bool once = false;
	if (controls & OUTPUT_ONCE) // do one token
	{
		once = true;
		controls ^= OUTPUT_ONCE;
	}
	if (buffer < currentOutputBase || (size_t)(buffer - currentOutputBase) >= (size_t)(currentOutputLimit-200)) // output is wrong or in danger already?
	{
		result = FAILRULE_BIT;
		return ptr;
	}

    char* word = AllocateBuffer("output"); // allocation from stack doesnt work for unknown reason
    ++outputlevel; // where we are

    bool quoted = false;
	char* startQuoted = NULL;
	// nested depth assignments from our heap space
	int paren = 0;
    while (ptr)
    {
		if (!*start) buffer = start;	// we may have built up a lot of buffer, then flushed it along the way. If so, drop back to start to continue.
		char* space = NULL;
		*buffer = 0;
		ptr = SkipWhitespace(ptr); // functions no longer skip blank after end, in case being used by reformat and preserving the space
		
        outputCode[outputlevel] = ptr; // where we are

#ifndef DISCARDTESTING
        if (debugAction)
        {
            (*debugAction)(ptr);
            *buffer = 0;
            ptr = outputCode[outputlevel]; // can replace where we are
        }
#endif
        if (!*ptr || (*ptr == ENDUNIT && ptr[1] != ENDUNIT)) break; // out of data

		char* priorPtr = ptr;
        ptr = ReadCompiledWord(ptr,word,false,true);  // stop when $var %var _var @nvar end normally- insure no ) ] } lingers on word in case it wasnt compiled
		if (*word == '$' && *ptr == '[' && ptr[1] == ']') // merge $word[]
		{
			strcat(word, "[]");
			ptr += 2;
		}
		if (*word == '/' && ptr[0] == '(') continue;	// JA PATCH for mistaken /(OS)

		char* startptr = ptr;
		ptr = SkipWhitespace(ptr);		// find next token to tes for assignment and  the like

		if (!*word  && !paren  ) break; // end of data or choice or body
		if (!(controls & OUTPUT_RAW) && (*word == ')' || *word == ']' || *word == '}')  && !paren  ) break; // end of data or choice or body

		if (*word == '[' && word[1]) // break apart uncompiled choice
		{
			ptr = SkipWhitespace(priorPtr) + 1;
			word[1] = 0;
		}
		else if (*word == FUNCTIONSTRING && (word[1] == '"' || word[1] == '\'' )  ) {;} // function strings can all sorts of stuff in them
		else if ((*word == '"') && word[1] == FUNCTIONSTRING) {;}   // function strings can all sorts of stuff in them
		else if (*word != '$') // separate closing brackets from token in case not run thru script compilation
		{
			bool quote = false;
			char* at = word-1;
			while (*++at )
			{
				if (*at == '\\') 
				{
					++at; // skip over next character
					continue;
				}
				if (*at == '"') quote = !quote;
				if (quote) continue; // ignore inside a quote

				if (*at == ']' || *at == ')' || *at == '}') break; // was not backslash quoted item
			}
			if (*at && at != word) // tail-- remove if not literalized
			{
				ptr = priorPtr + (at - word);
				*at = 0;
			}
		}

		//   determine whether to space before item or not. Don't space at start but normally space after existing output tokens with exceptions.
		if (start != buffer) 
		{
			bool allow = true;
			char c = *(buffer-1);
            if (quoted && buffer == startQuoted) // first thingy after \"
            {
                allow = false;
                startQuoted = NULL;
            }
            else if (!stricmp(language, "french") &&  (*word == ':' || *word == ';' || *word == '!' || *word == '?')) {; }
			else if (quoted && *word == '\\' && word[1] == '"') allow = false; // ending quoted
			// dont space after $   or [ or ( or " or / e   USERVAR_PREFIX
			else if (c == '(' || c == '[' || c == '{'  || c == '$'  || c == '/' || c == '`' || c == '\n') allow = false; //erased text is `
			else if (c == '#') { ; } // expecting a number but c# is not
			else if (c == 'n' && *(buffer-2) == '\\') allow = false; // slow form of \n
			else if (*word == '"' && word[1] == '^') allow = false; // format string handles its own spacing so
			else if (*word == '\\' && word[1] == ')') allow = false; // dont space before )
			else if (*word == '\\' && word[1] == '"' && (controls & OUTPUT_DQUOTE_FLIP) ) allow = false;	// closing dq
			else if ((*word == '.' && !word[1]) || (*word == '?' && !word[1]) || (*word == '!' && !word[1])|| (*word == ',' && !word[1])|| (*word == ':' && !word[1]) || (*word == ';' && !word[1])) allow = false;
			else if (*word == '\'' && (!word[1] || word[1] == 's' )) allow = false;
			if (allow) // add space separator
			{
				space = buffer;
				*buffer++ = ' '; 
				*buffer = 0;
			}
		}
		size_t len = strlen(word);
		if (len > 2 && word[len-2] == '\\') // escaped tail, separate it.
		{
			ptr = startptr - 2;
			if (*ptr != '\\') --ptr;
			word[len-2] = 0;
		}
	retry:
		switch (*word)
        {
		// groupings
		case ')':  case ']': case '}':  // ordinary output closers, never space before them
			if (space) 
			{
				--buffer;
				space = NULL;
			}
			*buffer++ = *word;
			*buffer = 0;
 			--paren;
			break;
		case '(': case '{':
			if (controls & OUTPUT_RAW) strcpy(buffer,word);
			else if (word[1]) StdNumber(word,buffer,controls); // its a full token like (ice)_ is_what
			else 
			{
				++paren;
				*buffer++ = *word;
				*buffer = 0;
			}
            break;
 		case '[':  //   random choice. process the choice area, then resume in this loop after the choice is made
			if (controls & OUTPUT_RAW) strcpy(buffer,word);
			else ptr = Output_Bracket(word, ptr, space, buffer, controls,result);
			break;

		// variables of various flavors
        case USERVAR_PREFIX: //   user variable or money
 			ptr = Output_Dollar(word, ptr, space, buffer, controls,result,once,false);
			break;
 		case '_': //   wildcard or standalone _ OR just an ordinary token
			ptr = Output_Underscore(word, ptr, space, buffer, controls,result,once);
			break;
        case SYSVAR_PREFIX:  //   system variable
	 		ptr = Output_Percent(word, ptr, space, buffer, (quoted) ? (controls | OUTPUT_DQUOTE_FLIP)  : controls,result,once);
			break;
		case '@': //   a fact set reference like @9 @1subject @2object or something else
			ptr = Output_AtSign(word, ptr,  space, buffer, controls,result,once);
			break;

		// sets, functions, strings
		case '~':	//concept set 
			strcpy(buffer,word);
            break;
 		case '^': //   function call or function variable or FORMAT string (by definition uncompiled) or ^$var
			ptr = Output_Function(word, ptr, space, buffer, controls,result,once);
			break;
        case '"': // string of some kind
			ptr = Output_String(word, ptr, space, buffer, controls,result);
			break;

		// prefixes:  quote, backslash
		case '\\':  //   backslash needed for new line  () [ ]   
  			ptr = Output_Backslash(word, ptr, space, buffer, (quoted) ? (controls | OUTPUT_DQUOTE_FLIP)  : controls,result);
			if (word[1] == '"') quoted = !quoted; // no space on 1st thing following quoted
            break;
		case '\'': //   quoted item
			ptr = Output_Quote(word, ptr,  space, buffer, controls,result);
			break;

		// punctuation which should not be spaced   .  ,  :  ;  !  ?  
		case '.': case '?': case '!': case ',':  case ';':  //   various punctuation wont want a space before
			ptr = Output_AttachedPunctuation( word, ptr,space, buffer, (buffer >= start) ? (controls | OUTPUT_ISOLATED_PERIOD)  : controls,result);
			break;
		case ':': // a debug command?
		{
#ifndef DISCARDTESTING
			unsigned int oldtopicid = currentTopicID;
			char* oldrule = currentRule;
			int oldruleid = currentRuleID;
			int oldruletopic = currentRuleTopic;
			char* c = ptr - strlen(word);
			while (*c != ':') {--c;}
			TestMode answer = Command(c,NULL,true);
			if (answer == RESTART) result = RESTART_BIT;	// end it all now
			currentTopicID = oldtopicid;
			currentRule = oldrule;
			currentRuleID = oldruleid;
			currentRuleTopic = oldruletopic;
			if (quitting == true) 
			{
				result = FAILINPUT_BIT;
				FreeBuffer("output1");
                --outputlevel;
                return NULL;
			}
			if (FAILCOMMAND != answer) 
			{
				ptr = NULL;
				break; // just abort flow after this since we did a command
			}
#endif
			// ordinary :
			// Handles spacing after a number:  2 .  
			// Handles not spacing before common punctuation:   . ? !  ,  :  ; 
			if (*word == '.' && controls & OUTPUT_ISOLATED_PERIOD) // if period after a number, always space after it (to be clear its not part of the number)
			{
				if (start != buffer && IsDigit(*(buffer-1))) *buffer++ = ' ';
			}
			ptr = Output_AttachedPunctuation(word,ptr,  space, buffer, (buffer >= start) ? (controls | OUTPUT_ISOLATED_PERIOD)  : controls,result);
			break;
		}
        default: //  text or C-Script
			ptr = Output_Text(word, ptr, space, buffer, controls,result);
        }
		
		if (*start == '`') buffer = ResetOutputPtr(start,buffer); // buffer got flushed
		else if (space && !space[1]) // generated nothing but the space, remove the space
		{
			*--buffer = 0;
			space = NULL;
		}

		// word has been told to retry having now substituted into buffer what is intended. used with indirections
		if (*word == ENDUNIT && word[1] != ENDUNIT)  // rule end but not $_ var value
		{
			if (!*buffer) result = FAILRULE_BIT;
			else if (once && (word[1] != USERVAR_PREFIX && word[1] != '_' && word[1] != '\'' && word[1] != '^')){;}	// have our answer (unless it was a function variable substitution)
			else
			{
				strcpy(word,SkipWhitespace(buffer));
				*buffer = 0;
				goto retry; 
			}
		}

		//   update location and check for overflow
		buffer += strlen(buffer);
		if (quoted && !startQuoted) startQuoted = buffer;
		unsigned int size = (buffer - currentOutputBase);
		if (currentOutputBase == ourMainOutputBuffer && size > maxOutputUsed) maxOutputUsed = size;
		if (size >= currentOutputLimit) 
		{
			char hold[100];
			*hold = 0;
			if (currentRule) strncpy(hold,currentRule,80);
			hold[50] = 0;
			char hold1[100];
			strncpy(hold1,currentOutputBase,80);
			hold1[50] = 0;
			ReportBug((char*)"Output overflowed %d > %d on rule %s output: %s\r\n", size, currentOutputLimit,hold,hold1);
		}
        if (size >= (currentOutputLimit-200) && !(result  & FAILCODES)) 
		{
			result = FAILRULE_BIT;
		}

		if (result & (RETRYRULE_BIT|RETRYTOPRULE_BIT|ENDCODES))
		{
			if (result & FAILCODES && !(controls & OUTPUT_LOOP)) *start = 0; //  kill output
			if (!once && ptr) ptr = BalanceParen(ptr,true,false); // swallow excess input BUG - does anyone actually care
			break;
		}
		if (once) break;    
	}
	FreeBuffer("output2");
    --outputlevel;
    return ptr;
}