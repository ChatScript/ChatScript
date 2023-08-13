#include "common.h"

int impliedIf = ALREADY_HANDLED;	// testing context of an if
unsigned int withinLoop = 0;

static void TestIf(char* ptr, FunctionResult& result, char* buffer)
{ //   word is a stream terminated by )
	//   if (%rand == 5 ) example of a comparison
	// if (@14) {} nonempty factset
	//   if (^QuerySubject()) example of a call or if (!QuerySubject())
	//   if ($var) example of existence
	//   if ('_3 == 5)  quoted matchvar
	//   if (1) what an else does
	char* word1 = AllocateStack(NULL, maxBufferSize); // not expecting big value in test condition. 
	char op[MAX_WORD_SIZE];
	int id;
	impliedIf = 1;
	bool header = false;

resume:
	ptr = ReadCompiledWord(ptr, word1);	//   the var or whatever
	bool invert = false;
	if (*word1 == '!') //   inverting, go get the actual
	{
		invert = true;
		// get the real token
		if (word1[1]) memmove(word1, word1 + 1, strlen(word1));
		else ptr = ReadCompiledWord(ptr, word1);
	}
	ptr = ReadCompiledWord(ptr, op);		//   the test if any, or (for call or ) for closure or AND or OR
	bool call = false;

	if (trace & TRACE_OUTPUT && !header && CheckTopicTrace() && *word1 == '1' && word1[1] == 0)
		id = Log(USERLOG, "else( ");
	else if (trace & TRACE_OUTPUT && !header && CheckTopicTrace()) id = Log(USERLOG, "if (");
	header = true;

	if (*op == '(') // function call instead of a variable. but the function call might output a value which if not used, must be considered for failure
	{
		call = true;
		ptr -= strlen(word1) + 3; //   back up so output can see the fn name and space
		ptr = Output(ptr, buffer, result, OUTPUT_ONCE | OUTPUT_KEEPSET); // buffer hold output value // skip past closing paren
		if (result & SUCCESSCODES) result = NOPROBLEM_BIT;	// legal way to terminate the piece with success at any  level

		if (trace & TRACE_OUTPUT && CheckTopicTrace())
		{
			if (*word1 == '^') { ; } // it will have declared itself, no need to trace here
			else if (result & ENDCODES) id = Log(USERLOG, "%c%s ", (invert) ? '!' : ' ', word1);
			else if (*word1 == '1' && word1[1] == 0) id = Log(USERLOG, "else ");
			else id = Log(USERLOG, "%c%s ", (invert) ? '!' : ' ', word1);
		}
		ptr = ReadCompiledWord(ptr, op); // find out what happens next after function call
		if (result == NOPROBLEM_BIT && IsComparison(*op)) // didnt fail and followed by a relationship op, move output as though it was the variable
		{
			strcpy(word1, buffer);
			call = false;	// proceed normally
		}
		else if (result != NOPROBLEM_BIT) { ; } // failed
		else if (!stricmp(buffer, (char*)"0") || !stricmp(buffer, (char*)"false")) result = FAILRULE_BIT;	// treat 0 and false as failure along with actual failures
	}
	*buffer = 0;
	if (call) { ; } // call happened
	else if (IsComparison(*op)) //   a comparison test
	{
		char word1val[MAX_WORD_SIZE];
		char word2val[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(ptr, buffer);
		realCode = ptr;
		result = HandleRelation(word1, op, buffer, true, id, word1val, word2val);
		realCode = 0;
		ptr = ReadCompiledWord(ptr, op);	//   AND, OR, or ) 
	}
	else //   existence of non-failure or any content
	{
		if (*word1 == SYSVAR_PREFIX || *word1 == '_' || *word1 == USERVAR_PREFIX || *word1 == '@' || *word1 == '?' || *word1 == '^')
		{
			char* remap = AllocateStack(NULL, maxBufferSize);
			strcpy(remap, word1); // for tracing
			if (*word1 == INDIRECT_PREFIX && IsDigit(word1[1])) strcpy(word1, FNVAR(word1));  // simple function var, remap it
			const char* found;
			if (word1[0] == LCLVARDATA_PREFIX && word1[1] == LCLVARDATA_PREFIX)
				found = word1 + 2;	// preevaled function variable
			else if (*word1 == SYSVAR_PREFIX) found = SystemVariable(word1, NULL);
			else if (*word1 == '_') found = wildcardCanonicalText[GetWildcardID(word1)];
			else if (*word1 == USERVAR_PREFIX) found = GetUserVariable(word1, false);
			else if (*word1 == '?') found = (tokenFlags & QUESTIONMARK) ? "1" : "";
			else if (*word1 == '^' && word1[1] == USERVAR_PREFIX) // indirect var
			{
				found = GetUserVariable(word1 + 1, false);
				found = GetUserVariable(found, true);
			}
			else if (*word1 == INDIRECT_PREFIX && word1[1] == '^' && IsDigit(word1[2])) found = ""; // indirect function var 
			else if (*word1 == '^' && word1[1] == '_') found = ""; // indirect var
			else if (*word1 == '^' && word1[1] == '\'' && word1[2] == '_') found = ""; // indirect var
			else if (*word1 == '@') found = FACTSET_COUNT(GetSetID(word1)) ? "1" : "";
			else found = word1;

			if (trace & TRACE_OUTPUT && CheckTopicTrace())
			{
				char* label = AllocateBuffer();
				strcpy(label, word1);
				if (*remap == '^') sprintf(label, "%s->%s", remap, word1);
				const char* didfind = found;
				if (didfind == nullGlobal) didfind = NULL;
				else if (didfind == (nullLocal + 2)) didfind = NULL;
				if (!didfind) // true null (not empty string)
				{
					if (invert) id = Log(USERLOG, "!%s`null` ", label);
					else id = Log(USERLOG, "%s`null` ", label);
				}
				else
				{
					if (invert) id = Log(USERLOG, "!%s`%s` ", label, found);
					else id = Log(USERLOG, "%s`%s` ", label, found);
				}
				FreeBuffer();
			}
			ReleaseStack(remap);
			if (!found || !*found) result = FAILRULE_BIT;
			else result = NOPROBLEM_BIT;
		}
		else  //  its a constant of some kind 
		{
			if (trace & TRACE_OUTPUT && CheckTopicTrace())
			{
				if (*word1 == '1' && !word1[1]) {} // dont show the else test
				else if (result & ENDCODES) id = Log(USERLOG, "%c%s ", (invert) ? '!' : ' ', word1);
				else id = Log(USERLOG, "%c%s ", (invert) ? '!' : ' ', word1);
			}
			ptr -= strlen(word1) + 3; //   back up to process the word and space
			ptr = Output(ptr, buffer, result, OUTPUT_ONCE | OUTPUT_KEEPSET) + 2; //   returns on the closer and we skip to accel
		}
	}
	if (invert) result = (result & ENDCODES) ? NOPROBLEM_BIT : FAILRULE_BIT;

	if (*op == 'a') //   AND - compiler validated it
	{
		if (!(result & ENDCODES))
		{
			if (trace & TRACE_OUTPUT && CheckTopicTrace()) id = Log(USERLOG, " AND ");
			goto resume;
			//   If he fails (result is one of ENDCODES), we fail
		}
		else
		{
			if (trace & TRACE_OUTPUT && CheckTopicTrace()) id = Log(USERLOG, " ... ");
		}
	}
	else if (*op == 'o') //  OR
	{
		if (!(result & ENDCODES))
		{
			if (trace & TRACE_OUTPUT && CheckTopicTrace()) id = Log(USERLOG, " ... ");
			result = NOPROBLEM_BIT;
		}
		else
		{
			if (trace & TRACE_OUTPUT && CheckTopicTrace()) id = Log(USERLOG, " OR ");

			goto resume;
		}
	}

	ReleaseStack(word1);
	*buffer = 0;
	impliedIf = ALREADY_HANDLED;
}

char* HandleIf(char* ptr, char* buffer, FunctionResult& result)
{
	// If looks like this: ^^if ( _0 == null ) 00m{ this is it } 00G else ( 1 ) 00q { this is not it } 004 
	// a nested if would be ^^if ( _0 == null ) 00m{ ^^if ( _0 == null ) 00m{ this is it } 00G else ( 1 ) 00q { this is not it } 004 } 00G else ( 1 ) 00q { this is not it } 004
	// after a test condition is code to jump to next test branch when condition fails
	// after { }  chosen branch is offset to jump to end of if
	bool executed = false;
	*buffer = 0;
	CALLFRAME* frame = ChangeDepth(1, "If()", false);
	frame->code = ptr;
	--adjustIndent; // stay showing at prior level

	while (ALWAYS) //   do test conditions until match
	{
		char* endptr;

		if (*ptr == '(') // old format - std if internals
		{
			endptr = strchr(ptr, '{') - ACCELLSIZE; // offset to jump past pattern
		}
		else // new format, can use std if or pattern match
		{
			endptr = ptr + Decode(ptr);
			ptr += ACCELLSIZE; // skip jump to end of pattern and point to pattern
		}

		//   Perform TEST condition
		if (!strnicmp(ptr, (char*)"( pattern ", 10)) // pattern if
		{
			int start = 0;
			int end = 0;
			wildcardIndex = 0;
			int whenmatched = 0;
			bool failed = false;
			char oldmark[MAX_SENTENCE_LENGTH];
			memcpy(oldmark, unmarked, MAX_SENTENCE_LENGTH);
			if (trace & (TRACE_PATTERN | TRACE_MATCH | TRACE_SAMPLE) && CheckTopicTrace()) //   display the entire matching responder and maybe wildcard bindings
			{
				Log(USERLOG, "  IF Pattern... \r\n");
			}
			MARKDATA hitdata;
			hitdata.start = start;
			if (!Match(ptr + 10, 0, hitdata, 1, 0, whenmatched, '(')) failed = true;  // skip paren and blank, returns start as the location for retry if appropriate
			start = hitdata.start;
			end = hitdata.end;
			memcpy(unmarked, oldmark, MAX_SENTENCE_LENGTH);
			ShowMatchResult((failed) ? FAILRULE_BIT : NOPROBLEM_BIT, ptr + 10, NULL);
			// cannot use @retry here
			if (!failed)
			{
				if (trace & (TRACE_PATTERN | TRACE_MATCH | TRACE_SAMPLE) && CheckTopicTrace()) //   display the entire matching responder and maybe wildcard bindings
				{
					Log(USERLOG, "**  Match: ");
					if (wildcardIndex)
					{
						Log(USERLOG, " Wildcards: (");
						for (int i = 0; i < wildcardIndex; ++i)
						{
							if (*wildcardOriginalText[i]) Log(USERLOG, "_%d=%s / %s (%d-%d)   ", i, wildcardOriginalText[i], wildcardCanonicalText[i], wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
							else Log(USERLOG, "_%d=null (%d-%d) ", i, wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
						}
					}
					Log(USERLOG, "\r\n");
				}
			}
			result = (failed) ? FAILRULE_BIT : NOPROBLEM_BIT;
		}
		else
		{
			TestIf(ptr + 2, result, buffer);
			*buffer = 0;
		}
		if (trace & TRACE_OUTPUT && CheckTopicTrace())
		{
			if (result & ENDCODES)  Log(USERLOG, "%s\r\n", "FAIL-if");
			else  Log(USERLOG, ") ");
		}

		ptr = endptr; // now after pattern, pointing to the skip data to go past body.
		*buffer = 0;

		//   perform SUCCESS branch and then end if
		if (!(result & ENDCODES)) //   IF test success - we choose this branch
		{
			executed = true;
			ptr += 5;
			++adjustIndent;
			ptr = Output(ptr, buffer, result); //   skip accelerator-3 and space and { and space - returns on next useful token
			--adjustIndent;
			ptr += Decode(ptr);	//   offset to end of if entirely
			if (*(ptr - 1) == 0)  --ptr;// no space after?
			break;
		}
		else
		{
			result = NOPROBLEM_BIT;		//   test result is not the IF result
		}
		//   On fail, move to next test
		ptr += Decode(ptr);
		if (*(ptr - 1) == 0)
		{
			--ptr;
			break; // the if ended abruptly as a stream like in NOTRACE, and there is no space here
		}
		if (strncmp(ptr, (char*)"else ", 5))  break; //   not an ELSE, the IF is over. 
		ptr += 5; //   skip over ELSE space, aiming at the ( of the next condition condition
	}
	if (executed && trace & TRACE_OUTPUT && CheckTopicTrace())  Log(USERLOG, "End If\r\n");
	ChangeDepth(-1, "If()", true);
	++adjustIndent; // return to if level

	return ptr;
}

char* HandleLoop(char* ptr, char* buffer, FunctionResult& result, bool json)
{
	unsigned int oldIterator = currentIterator;
	currentIterator = 0;
	ChangeDepth(1, "Loop()", false, ptr + 2);
	char word[MAX_WORD_SIZE];
	WORDP var1 = NULL;
	WORDP var2 = NULL;
	int match1 = -1; // json args can be match variables OR $ variables
	int match2 = -1;
	FACT* F = NULL;
	result = NOPROBLEM_BIT;
	char* paren = strchr(ptr, ')') + 2;
	char* endofloop = paren + (size_t)Decode(paren);

	int limit = 0;

	bool newest = false; // oldest members first
	int indexsize = 0;
	FACT** stack = NULL;

	if (!json)
	{
		limit = atoi(GetUserVariable((char*)"$cs_looplimit", false));
		if (limit == 0) limit = 1000;
		char* given = SkipWhitespace(ptr + 1);
		if (IsDigit(*given)) limit = atoi(given); // user explicit legal
		ptr = GetCommandArg(ptr + 2, buffer, result, 0) + 2; //   get the loop counter value and skip closing ) space 
	}
	else
	{
		limit = 1000000;
		char data[MAX_WORD_SIZE];
		*data = 0;
		ptr = GetCommandArg(ptr + 2, data, result, 0); //   get the json object 
		if (trace & TRACE_OUTPUT && CheckTopicTrace())
		{
			--adjustIndent;
			Log(USERLOG, "jsonloop(%s)\r\n", data);
			++adjustIndent;
		}
		if (!*data)
		{
			result = NOPROBLEM_BIT;
			ChangeDepth(-1, "Loop()", true);
			return endofloop; // null value
		}

		// must be JSON
		if (!IsValidJSONName((char*)data))
		{
			result = FAILRULE_BIT;
			ChangeDepth(-1, "Loop()", true);
			return endofloop; // we dont know it
		}

		WORDP jsonstruct = FindWord(data);
		if (!jsonstruct) // no known object
		{
			result = FAILRULE_BIT;
			ChangeDepth(-1, "Loop()", true);
			return endofloop; // we dont know it
		}
		F = GetSubjectNondeadHead(jsonstruct);
		if (!F)// no members
		{
			result = NOPROBLEM_BIT;
			ChangeDepth(-1, "Loop()", true);
			return endofloop; // we dont know it
		}

		// move onto stack so we can walk either way the elements
		char* limited;
		stack = (FACT**)InfiniteStack(limited, "handleloop");
		while (F) // stack object key data
		{
			stack[indexsize++] = F;
			F = GetSubjectNondeadNext(F);
		}
		CompleteBindStack64(indexsize, (char*)stack);
		// dont bother to release it at end. cutback will handle it

		ptr = ReadCompiledWord(ptr, word); // 1st variable
		if (*word == '$')
		{
			if (word[1] != '_')
			{
				result = FAILRULE_BIT;
				ChangeDepth(-1, "Loop()", true);
				return endofloop; // we dont know it
			}
			var1 = StoreWord(word, AS_IS);
		}
		else match1 = atoi(word + 1);
		ptr = ReadCompiledWord(ptr, word); // 2nd variable
		if (*word == '$')
		{
			if (word[1] != '_')
			{
				result = FAILRULE_BIT;
				ChangeDepth(-1, "Loop()", true);
				return endofloop; // we dont know it
			}
			var2 = StoreWord(word, AS_IS);
		}
		else match2 = atoi(word + 1);
		char* at = ReadCompiledWord(ptr, word); // possible ordering modifier
		if (*word != ')')
		{
			if (!stricmp(word, "old")) newest = false;
			ptr = at;
		}
		ptr += 2; // past paren closer
	} // end json zone

	int counter;
	if (*buffer == '@')
	{
		int set = GetSetID(buffer);
		if (set < 0)
		{
			result = FAILRULE_BIT; // illegal id
			ChangeDepth(-1, "Loop()", true);
			return ptr;
		}
		counter = FACTSET_COUNT(set);
	}
	else if (json) counter = 1000000;
	else
	{
		counter = atoi(buffer);
	}
	*buffer = 0;
	if (result & ENDCODES)
	{
		ChangeDepth(-1, "Loop()", true);
		return endofloop;
	}
	ptr += 5;	//   skip jump + space + { + space
	++withinLoop; // used by planner

	bool infinite = false;
	if (counter > limit || counter < 0)
	{
		counter = limit; //   LIMITED
		infinite = true; // loop is bounded by defaults
	}
	--adjustIndent;
	int forward = 0;
	while (counter-- > 0)
	{
		if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "loop(%d)\r\n", counter + 1);
		if (json)
		{
			if (forward == indexsize || indexsize == 0) break; // end of members
			F = (newest) ? stack[forward++] : stack[--indexsize];
			char* arg1 = Meaning2Word(F->verb)->word;
			if (var1)
			{
				var1->w.userValue = arg1;
				if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "var1: %s=>%s ", var1->word, arg1);
			}
			else // match variable, not $ var
			{
				strcpy(wildcardOriginalText[match1], arg1);  //   spot wild cards can be stored
				strcpy(wildcardCanonicalText[match1], arg1);  //   spot wild cards can be stored
				if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "var1: _%d=>%s ", match1, wildcardOriginalText[match1]);
			}
			char* arg2 = Meaning2Word(F->object)->word;
			if (var2)
			{
				var2->w.userValue = arg2;
				if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "    var2: %s=>%s\r\n", var2->word, arg2);
			}
			else
			{
				strcpy(wildcardOriginalText[match2], arg2);  //   spot wild cards can be stored
				strcpy(wildcardCanonicalText[match2], arg2);  //   spot wild cards can be stored
				if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "    var2: _%d=>%s\r\n", match2, wildcardOriginalText[match2]);
			}
		}

		FunctionResult result1;
		Output(ptr, buffer, result1, OUTPUT_LOOP);
		buffer += strlen(buffer);
		if (result1 & ENDCODES || result1 & RETRYCODES)
		{
			if (result1 == NEXTLOOP_BIT) continue;
			result = result1;
			if (result1 == FAILLOOP_BIT) result = FAILRULE_BIT; // propagate failure outside of loop
			else if (result1 == ENDLOOP_BIT) result = NOPROBLEM_BIT; // propagate ok outside of loop
			else if (result1 & (ENDRULE_BIT | FAILRULE_BIT)) result = NOPROBLEM_BIT;
			// rule level terminates only the loop
			break;//   potential failure if didnt add anything to buffer
		}
	}
	ChangeDepth(-1, "Loop{}", true); // allows step out to cover a loop 
	++adjustIndent;
	if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "end of loop\r\n");
	if (counter < 0 && infinite)
		ReportBug("INFO: Loop ran to limit %d\r\n", limit);
	--withinLoop;
	currentIterator = oldIterator;
	return endofloop;
}

FunctionResult MemberRelation(WORDP set, char* val1)
{
	FunctionResult result = FAILRULE_BIT;
	if (!set) return result;
	if (strchr(val1, '_')) // try alternate form with no _
	{
		char* underscore = val1;
		bool changed = false;
		while ((underscore = strchr(underscore, '_')))
		{
			*underscore = ' ';
			changed = true;
		}
		if (changed)
		{
			WORDP D1 = FindWord(val1);
			if (D1)
			{
				NextInferMark();
				if (SetContains(MakeMeaning(set), MakeMeaning(D1))) result = NOPROBLEM_BIT;
			}
		}
	}
	else if (strchr(val1, ' ')) // try alternate form with no space
	{
		char* underscore = val1;
		bool changed = false;
		while ((underscore = strchr(underscore, ' ')))
		{
			*underscore = '_';
			changed = true;
		}
		if (changed)
		{
			WORDP D1 = FindWord(val1);
			if (D1)
			{
				NextInferMark();
				if (SetContains(MakeMeaning(set), MakeMeaning(D1))) result = NOPROBLEM_BIT;
			}
		}
	}
	return result;
}

FunctionResult HandleRelation(char* word1, char* op, char* word2, bool output, int& id, char* word1val, char* word2val)
{ //   word1 and word2 are RAW, ready to be evaluated.
	*word1val = 0;
	*word2val = 0;
	char* val1 = AllocateStack(NULL, maxBufferSize);
	char* val2 = AllocateStack(NULL, maxBufferSize);
	WORDP D;
	WORDP D1;
	WORDP D2;
	int64 v1 = 0;
	int64 v2 = 0;
	FunctionResult result, val1Result;
	FreshOutput(word1, val1, result, OUTPUT_ONCE | OUTPUT_KEEPSET | OUTPUT_NOCOMMANUMBER | OUTPUT_NODEBUG); // 1st arg
	val1Result = result;
	if (trace & TRACE_OUTPUT && output && CheckTopicTrace())
	{
		char* traceop = op;
		if (!traceop) traceop = "x";
		char x[MAX_WORD_SIZE];
		*x =  0;
		if (*op == '&')
		{
#ifdef WIN32
			sprintf(x, (char*)"0x%016I64x", v1);
#else
			sprintf(x, (char*)"0x%016llx", v1);
#endif		
		}
		if (!stricmp(word1, val1))
		{
			if (*word1) Log(USERLOG, "%s %s ", word1, traceop); // no need to show value
			else Log(USERLOG, "null %s ", traceop);
		}
		else Log(USERLOG, "%s`%s` %s ",word1,val1,op);
	}
	
	if (word2 && *word2) FreshOutput(word2, val2, result, OUTPUT_ONCE | OUTPUT_KEEPSET | OUTPUT_NOCOMMANUMBER | OUTPUT_NODEBUG); // 2nd arg
	if (word2 && *word2 && trace & TRACE_OUTPUT && output && CheckTopicTrace())
	{
		char x[MAX_WORD_SIZE];
		*x = 0;
		if (*op == '&') strcpy(x, PrintX64(v1));
		if (word2 && !strcmp(word2, val2))
		{
			if (*val2) id = Log(USERLOG, " %s ", word2); // no need to show value
			else id = Log(USERLOG, " null ");
		}
		else if (*op == '&') id = Log(USERLOG, " %s`%s` ", word2, x);
		else id = Log(USERLOG, " %s`%s` ", word2, val2);
	}
	
	result = FAILRULE_BIT;
	if (!stricmp(val1, (char*)"null")) *val1 = 0; // JSON null values
	if (!stricmp(val2, (char*)"null")) *val2 = 0;
	if (!op)
	{
		if (val1Result & ENDCODES); //   explicitly failed
		else if (*val1) result = NOPROBLEM_BIT;	//   has some value
	}
	else if (*op == '?' || op[1] == '?') // ? and !? 
	{
		if (*word1 == '\'') ++word1; // ignore the quote since we are doing positional
		if (*word1 == '_') // use only precomputed match from memorization
		{
			unsigned int index = GetWildcardID(word1);
			unsigned int begin = WILDCARD_START(wildcardPosition[index]);
			unsigned int finish = WILDCARD_END_ONLY(wildcardPosition[index]);
			D = FindWord(val2); // as is
			if (begin && D)
			{
				MARKDATA hitdata;
				if (GetNextSpot(D, begin - 1, false,0,&hitdata) == begin && (unsigned int)(hitdata.end & REMOVE_SUBJECT) == finish) result = NOPROBLEM_BIT; // otherwise failed and we would have known
			}
			if (*op == '!') result = (result != NOPROBLEM_BIT) ? NOPROBLEM_BIT : FAILRULE_BIT;
		}
		else if (IsValidJSONName(val2, 'a')) // is it in array?
		{
			result = FAILRULE_BIT;
			D = FindWord(val1);
			if (D)
			{
				MEANING M = MakeMeaning(D);
				D = FindWord(val2);
				if (D)
				{
					FACT* F = GetSubjectNondeadHead(D);
					while (F)
					{
						if (F->object == M)
						{
							result = NOPROBLEM_BIT;
							break;
						}
						F = GetSubjectNondeadNext(F);
					}
				}
			}
		}
		else if (!stricmp(word2, "~number")) // number
		{
			if (*word1 == '$') word1 = GetUserVariable(word1);
			return IsPureNumber(word1) ? NOPROBLEM_BIT : FAILRULE_BIT;
		}
		else // laborious.
		{
			D1 = FindWord(val1);
			D2 = FindWord(val2);
			if (D1 && D2)
			{
				NextInferMark();
				if (SetContains(MakeMeaning(D2), MakeMeaning(D1))) result = NOPROBLEM_BIT;
			}
			if (result != NOPROBLEM_BIT) result = MemberRelation(D2, val1);
			if (*op == '!') result = (result != NOPROBLEM_BIT) ? NOPROBLEM_BIT : FAILRULE_BIT;
		}
	}
	else //   boolean tests
	{
		// convert null to numeric operator for < or >  -- but not for equality
		if (!*val1 && (IsSign(*val2) || IsDigit(*val2)) && IsNumber(val2) != NOT_A_NUMBER && (*op == '<' || *op == '>')) strcpy(val1, (char*)"0");
		else if (!*val2 && (IsSign(*val1) || IsDigit(*val1)) && IsNumber(val1) != NOT_A_NUMBER && (*op == '<' || *op == '>')) strcpy(val2, (char*)"0");
		// treat #123 as string but +21545 and -12345 as numbers

		char* currency1;
		char* currency2;
		unsigned char* cur1 = GetCurrency((unsigned char*)val1, currency1);
		unsigned char* cur2 = GetCurrency((unsigned char*)val2, currency2); // use text string comparison though isdigitword calls it a number
		char* end1 = val1 + strlen(val1);
		char* end2 = val2 + strlen(val2);

		if (*val1 == '#' || !IsDigitWord(val1, AMERICAN_NUMBERS, true) || *val2 == '#' || !IsDigitWord(val2, AMERICAN_NUMBERS, true) ||
			strchr(val1, ':') || strchr(val2, ':') ||
			strchr(val1, '%') || strchr(val2, '%') || cur1 || cur2) //   non-numeric string compare - bug, can be confused if digit starts text string
		{
			char* arg1 = val1;
			char* arg2 = val2;
			if (*arg1 == '"' && arg1[1]) // truly a string
			{
				size_t len = strlen(arg1);
				if (arg1[len - 1] == '"') // remove STRING markers
				{
					arg1[len - 1] = 0;
					++arg1;
				}
			}
			if (*arg2 == '"' && arg2[1])
			{
				size_t len = strlen(arg2);
				if (arg2[len - 1] == '"') // remove STRING markers
				{
					arg2[len - 1] = 0;
					++arg2;
				}
			}
			if (*op == '!') result = (stricmp(arg1, arg2)) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '=')
			{
				result = (!stricmp(arg1, arg2)) ? NOPROBLEM_BIT : FAILRULE_BIT;
			}
			else if (*op == '>' && op[1] == '=') result = (stricmp(arg1, arg2) >= 0) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '<' && op[1] == '=') result = (stricmp(arg1, arg2) <= 0) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '>') result = (stricmp(arg1, arg2) > 0) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '<') result = (stricmp(arg1, arg2) < 0) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else result = FAILRULE_BIT;
		}
		//   handle double ops
		else if (IsFloat(val1, end1) || IsFloat(val2, end2)) // at least one arg is float
		{
			char* comma = 0;
			while ((comma = strchr(val1, ',')))  memmove(comma, comma + 1, strlen(comma)); // remove embedded commas
			while ((comma = strchr(val2, ',')))  memmove(comma, comma + 1, strlen(comma)); // remove embedded commas
			double v1f = Convert2Double(val1);
			double v2f = Convert2Double(val2);
			if (*op == '=') result = (v1f == v2f) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '<')
			{
				if (!op[1]) result = (v1f < v2f) ? NOPROBLEM_BIT : FAILRULE_BIT;
				else result = (v1f <= v2f) ? NOPROBLEM_BIT : FAILRULE_BIT;
			}
			else if (*op == '>')
			{
				if (!op[1]) result = (v1f > v2f) ? NOPROBLEM_BIT : FAILRULE_BIT;
				else result = (v1f >= v2f) ? NOPROBLEM_BIT : FAILRULE_BIT;
			}
			else if (*op == '!') result = (v1f != v2f) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else result = FAILRULE_BIT;
		}
		else //   int compare
		{
			char* comma = 0; // pretty number?
			while ((comma = strchr(val1, ',')))  memmove(comma, comma + 1, strlen(comma));
			while ((comma = strchr(val2, ',')))  memmove(comma, comma + 1, strlen(comma));
			ReadInt64(val1, v1);
			ReadInt64(val2, v2);
			if (strchr(val1, ',') && strchr(val2, ',')) // comma numbers
			{
				size_t len1 = strlen(val1);
				size_t len2 = strlen(val2);
				if (len1 != len2) result = FAILRULE_BIT;
				else if (strcmp(val1, val2)) result = FAILRULE_BIT;
				else result = NOPROBLEM_BIT;
				if (*op == '!') result = (result == NOPROBLEM_BIT) ? FAILRULE_BIT : NOPROBLEM_BIT;
				else if (*op != '=') ReportBug((char*)"INFO: Op not implemented for comma numbers %s", op);
			}
			else if (*op == '=') result = (v1 == v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '<')
			{
				if (!op[1]) result = (v1 < v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
				else result = (v1 <= v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
			}
			else if (*op == '>')
			{
				if (!op[1]) result = (v1 > v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
				else result = (v1 >= v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
			}
			else if (*op == '!') result = (v1 != v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else if (*op == '&') result = (v1 & v2) ? NOPROBLEM_BIT : FAILRULE_BIT;
			else result = FAILRULE_BIT;
		}
	}
	ReleaseStack(val1);
	return result;
}
