
#include "common.h"

uint64 posTiming;

typedef struct EndingInfo 
{
	char* word;	
	uint64 properties;
	uint64 flags;
	unsigned int baseflags;
} EndingInfo;
	
EndingInfo noun2[] = 
{
	{ (char*)"th",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ar",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ty",NOUN|NOUN_SINGULAR}, 
	{ (char*)"et",NOUN|NOUN_SINGULAR}, 
	{ (char*)"or",NOUN|NOUN_SINGULAR}, 
	{ (char*)"al",NOUN|NOUN_SINGULAR}, 
	{ (char*)"er",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ee",NOUN|NOUN_SINGULAR}, 
	{ (char*)"id",NOUN|NOUN_SINGULAR}, 
	{ (char*)"cy",NOUN|NOUN_SINGULAR}, 
	{0},
};

EndingInfo noun3[] = 
{
	{ (char*)"ory",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ant",NOUN|NOUN_SINGULAR}, 
	{ (char*)"eer",NOUN|NOUN_SINGULAR}, 
	{ (char*)"log",NOUN|NOUN_SINGULAR}, 
	{ (char*)"oma",NOUN|NOUN_SINGULAR}, 
	{ (char*)"dom",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ard",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ide",NOUN|NOUN_SINGULAR}, 
	{ (char*)"oma",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ity",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ist",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ism",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ing",NOUN|NOUN_SINGULAR}, 
	{ (char*)"gon",NOUN|NOUN_SINGULAR}, 
	{ (char*)"gam",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ese",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ure",NOUN|NOUN_SINGULAR}, 
	{ (char*)"acy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"age",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ade",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ery",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ary",NOUN|NOUN_SINGULAR}, 
	{ (char*)"let",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ess",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ice",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ice",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ine",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ent",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ion",NOUN|NOUN_SINGULAR}, 
	{ (char*)"oid",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ite",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun4[] = 
{
	{ (char*)"tion",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ment",NOUN|NOUN_SINGULAR}, 
	{ (char*)"emia",NOUN|NOUN_SINGULAR}, 
	{ (char*)"opsy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"itis",NOUN|NOUN_SINGULAR}, 
	{ (char*)"opia",NOUN|NOUN_SINGULAR}, 
	{ (char*)"hood",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ness",NOUN|NOUN_SINGULAR}, 
	{ (char*)"logy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ette",NOUN|NOUN_SINGULAR}, 
	{ (char*)"cide",NOUN|NOUN_SINGULAR}, 
	{ (char*)"sion",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ling",NOUN|NOUN_SINGULAR}, 
	{ (char*)"cule",NOUN|NOUN_SINGULAR}, 
	{ (char*)"osis",NOUN|NOUN_SINGULAR}, 
	{ (char*)"esis",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ware",NOUN|NOUN_SINGULAR}, 
	{ (char*)"tude",NOUN|NOUN_SINGULAR}, 
	{ (char*)"cian",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ency",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ence",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ancy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ance",NOUN|NOUN_SINGULAR}, 
	{ (char*)"tome",NOUN|NOUN_SINGULAR}, 
	{ (char*)"tomy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"crat",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ship",NOUN|NOUN_SINGULAR}, 
	{ (char*)"pnea",NOUN|NOUN_SINGULAR}, 
	{ (char*)"path",NOUN|NOUN_SINGULAR}, 
	{ (char*)"gamy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"onym",NOUN|NOUN_SINGULAR}, 
	{ (char*)"icle",NOUN|NOUN_SINGULAR}, 
	{ (char*)"wise",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun5[] = 
{
	{ (char*)"cracy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"scope",NOUN|NOUN_SINGULAR}, 
	{ (char*)"scopy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ocity",NOUN|NOUN_SINGULAR}, 
	{ (char*)"acity",NOUN|NOUN_SINGULAR}, 
	{ (char*)"loger",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ation",NOUN|NOUN_SINGULAR}, 
	{ (char*)"arian",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ology",NOUN|NOUN_SINGULAR}, 
	{ (char*)"algia",NOUN|NOUN_SINGULAR}, 
	{ (char*)"sophy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"cycle",NOUN|NOUN_SINGULAR}, 
	{ (char*)"orium",NOUN|NOUN_SINGULAR}, 
	{ (char*)"arium",NOUN|NOUN_SINGULAR}, 
	{ (char*)"phone",NOUN|NOUN_SINGULAR}, 
	{ (char*)"iasis",NOUN|NOUN_SINGULAR}, 
	{ (char*)"pathy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"phile",NOUN|NOUN_SINGULAR}, 
	{ (char*)"phyte",NOUN|NOUN_SINGULAR}, 
	{ (char*)"otomy",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun6[] = 
{
	{ (char*)"bility",NOUN|NOUN_SINGULAR}, 
	{ (char*)"script",NOUN|NOUN_SINGULAR}, 
	{ (char*)"phobia",NOUN|NOUN_SINGULAR}, 
	{ (char*)"iatric",NOUN|NOUN_SINGULAR}, 
	{ (char*)"logist",NOUN|NOUN_SINGULAR}, 
	{ (char*)"oholic",NOUN|NOUN_SINGULAR}, 
	{ (char*)"aholic",NOUN|NOUN_SINGULAR}, 
	{ (char*)"plegia",NOUN|NOUN_SINGULAR}, 
	{ (char*)"plegic",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ostomy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ectomy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"trophy",NOUN|NOUN_SINGULAR}, 
	{ (char*)"escent",NOUN|NOUN_SINGULAR}, 
	{0},
};
EndingInfo noun7[] = 
{
	{ (char*)"escence",NOUN|NOUN_SINGULAR}, 
	{ (char*)"ization",NOUN|NOUN_SINGULAR}, 
	{0},
};

	
EndingInfo verb5[] = 
{
	{ (char*)"scribe",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};
EndingInfo verb4[] = 
{
	{ (char*)"sect",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};
EndingInfo verb3[] = 
{
	{ (char*)"ise",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{ (char*)"ize",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{ (char*)"ify",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{ (char*)"ate",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};
EndingInfo verb2[] = 
{
	{ (char*)"en",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{ (char*)"er",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{ (char*)"fy",VERB|VERB_INFINITIVE|VERB_PRESENT}, 
	{0},
};

EndingInfo adverb5[] = 
{
	{ (char*)"wards",ADVERB,0}, 
	{0},
};

EndingInfo adverb4[] = 
{
	{ (char*)"wise",ADVERB,0}, 
	{ (char*)"ward",ADVERB,0}, 
	{0},
};
EndingInfo adverb3[] = 
{
	{ (char*)"ily",ADVERB,0}, 
	{ (char*)"bly",ADVERB,0}, 
	{0},
};
EndingInfo adverb2[] = 
{
	{ (char*)"ly",ADVERB,0}, 
	{0},
};


EndingInfo adjective7[] = 
{
	{ (char*)"iferous",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN}, // comprised of
	{0},
};
EndingInfo adjective6[] = 
{
	{ (char*)"escent",ADJECTIVE|ADJECTIVE_NORMAL,0},
	{0},
};
EndingInfo adjective5[] = 
{
	{ (char*)"ative",ADJECTIVE|ADJECTIVE_NORMAL,0}, // tending toward
	{ (char*)"esque",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN},
	{ (char*)"-free",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN}, 
	{ (char*)"gonal",ADJECTIVE|ADJECTIVE_NORMAL,0}, 
	{ (char*)"gonic",ADJECTIVE|ADJECTIVE_NORMAL,0}, // angle
	{ (char*)"proof",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN}, 
	{ (char*)"sophic",ADJECTIVE|ADJECTIVE_NORMAL,0}, // knowledge
	{ (char*)"esque",ADJECTIVE|ADJECTIVE_NORMAL,0}, // in the style of
	{0},
};
EndingInfo adjective4[] = 
{
		{ (char*)"less",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN}, // without
		{ (char*)"etic",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"_out",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"ular",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"uous",ADJECTIVE|ADJECTIVE_NORMAL,0}, // characterized by
		{ (char*)"ical",ADJECTIVE|ADJECTIVE_NORMAL,0}, // pertaining to
		{ (char*)"-off",ADJECTIVE|ADJECTIVE_NORMAL,0}, // capable of being
		{ (char*)"ious",ADJECTIVE|ADJECTIVE_NORMAL,0}, // characterized by
		{ (char*)"able",ADJECTIVE|ADJECTIVE_NORMAL,0,VERB}, // capable of being
		{ (char*)"ible",ADJECTIVE|ADJECTIVE_NORMAL,0,VERB}, // capable of being
		{ (char*)"like",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN}, // resembling
		{ (char*)"some",ADJECTIVE|ADJECTIVE_NORMAL,0}, // characterized by
		{ (char*)"ward",ADJECTIVE|ADJECTIVE_NORMAL,0}, // direction of
		{ (char*)"wise",ADJECTIVE|ADJECTIVE_NORMAL,0}, // direction of
	{0},
};
EndingInfo adjective3[] = 
{
		{ (char*)"ial",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"oid",ADJECTIVE|ADJECTIVE_NORMAL,0}, // shape of
		{ (char*)"ble",ADJECTIVE|ADJECTIVE_NORMAL,0}, // able to
		{ (char*)"ous",ADJECTIVE|ADJECTIVE_NORMAL,0}, // characterized by
		{ (char*)"ive",ADJECTIVE|ADJECTIVE_NORMAL,0}, // having the nature of
		{ (char*)"ate",ADJECTIVE|ADJECTIVE_NORMAL,0}, // quality of
		{ (char*)"ful",ADJECTIVE|ADJECTIVE_NORMAL,0,NOUN}, // quality of notable for
		{ (char*)"ese",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to a place
		{ (char*)"fic",ADJECTIVE|ADJECTIVE_NORMAL,0}, 
		{ (char*)"ant",ADJECTIVE|ADJECTIVE_NORMAL,0}, // inclined to
		{ (char*)"ent",ADJECTIVE|ADJECTIVE_NORMAL,0}, // one who causes
		{ (char*)"ern",ADJECTIVE|ADJECTIVE_NORMAL,0}, // quality of
		{ (char*)"ian",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"ile",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"_to",ADJECTIVE|ADJECTIVE_PARTICIPLE,0}, // to
		{ (char*)"_of",ADJECTIVE|ADJECTIVE_PARTICIPLE,0}, // of
		{ (char*)"ing",ADJECTIVE|ADJECTIVE_PARTICIPLE,0},  // verb present participle as adjective  // BUG adjectiveFormat = ADJECTIVE_PARTICIPLE;
		{ (char*)"ied",ADJECTIVE|ADJECTIVE_PARTICIPLE,0}, // verb past participle as adjective
		{ (char*)"ine",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
		{ (char*)"ual",ADJECTIVE|ADJECTIVE_NORMAL,0}, // gradual
	{0},
};
EndingInfo adjective2[] = 
{
		{ (char*)"ic",ADJECTIVE|ADJECTIVE_NORMAL,0},  // pertaining to
		{ (char*)"ar",ADJECTIVE|ADJECTIVE_NORMAL,0},  // relating to
		{ (char*)"ac",ADJECTIVE|ADJECTIVE_NORMAL,0},  // pertaining to
		{ (char*)"al",ADJECTIVE|ADJECTIVE_NORMAL,0},  // pertaining to
		{ (char*)"en",ADJECTIVE|ADJECTIVE_NORMAL,0}, 
		{ (char*)"an",ADJECTIVE|ADJECTIVE_NORMAL,0}, // relating to
	{0}
};
EndingInfo adjective1[] = 
{
		{ (char*)"y",ADJECTIVE|ADJECTIVE_NORMAL,0}, 
	{0},
};

static int64 ProcessNumber(int at, char* original, WORDP& revise, WORDP &entry, WORDP &canonical, uint64& sysflags, uint64 &cansysflags, bool firstTry, bool nogenerate, int start, int kind) // case sensitive, may add word to dictionary, will not augment flags of existing words
{
	size_t len = strlen(original);

	char* slash = strchr(original, '/');
	if (!slash) slash = strchr(original, '-');

	if (IsDate(original))
	{
		uint64 properties = NOUN | NOUN_SINGULAR;
		canonical = entry = StoreWord(original, properties, TIMEWORD);
		cansysflags = sysflags = entry->systemFlags | TIMEWORD;
		return properties;
	}
	if (kind == NOT_A_NUMBER) return 0; // not a date after all

	// hyphenated word which is number on one side:   Cray-3  3-second
	char* hyphen = strchr(original, '-');
	if (hyphen && !hyphen[1]) hyphen = NULL; // not real
	int64 properties = 0;
	if ((IsDigit(*original) || IsDigit(original[1]) || *original == '\'') && (kind || hyphen))
	{
		// DATE IN 2 DIGIT OR 4 DIGIT NOTATION
		char word[MAX_WORD_SIZE];
		*word = 0;
		// 4digit year 1990 and year range 1950s and 1950's
		if (IsDigit(*original) && IsDigit(original[1]) && IsDigit(original[2]) && IsDigit(original[3]) &&
			(!original[4] || (original[4] == 's' && !original[5]) || (original[4] == '\'' && original[5] == 's' && !original[6]))) sprintf(word, (char*)"%d", atoi(original));
		//  2digit year and range '40 and '40s
		if (*original == '\'' && IsDigit(original[1]) && IsDigit(original[2]) &&
			(!original[3] || (original[3] == 's' && !original[4]) || (original[3] == '\'' && original[4] == 's' && !original[5]))) sprintf(word, (char*)"19%d", atoi(original + 1));
		if (*word)
		{
			properties = NOUN | NOUN_NUMBER | ADJECTIVE | ADJECTIVE_NUMBER;
			entry = StoreWord(original, properties, TIMEWORD);
			canonical = StoreWord(word, properties, TIMEWORD);
			sysflags = entry->systemFlags | TIMEWORD;
			cansysflags = canonical->systemFlags | TIMEWORD;
			return properties;
		}

		// handle time like 4:30
		if (len > 3 && len < 6 && IsDigit(original[len - 1]) && (original[1] == ':' || original[2] == ':')) // 18:32
		{
			properties = NOUN | NOUN_NUMBER | ADJECTIVE | ADJECTIVE_NUMBER;
			entry = canonical = StoreWord(original, properties); // 18:32
			sysflags = entry->systemFlags | TIMEWORD;
			cansysflags = canonical->systemFlags | TIMEWORD;
			return properties;
		}

		// handle number range data like 120:129 
		char* at = original;
		int colon = 0;
		while (*++at && (IsDigit(*at) || *at == ':'))
		{
			if (*at == ':') ++colon;
		}
		if (!*at && colon == 1) // was completely digits and a single colon
		{
			properties = NOUN | NOUN_NUMBER | ADJECTIVE | ADJECTIVE_NUMBER;
			entry = canonical = StoreWord(original, properties);
			return properties;
		}

		// handle date range like 1920-22 or 1920-1955  or any number range
		if (hyphen && hyphen != original)
		{
			char* at = original - 1;
			while (*++at)
			{
				if (IsDigit(*at)) continue;
				if (*at == '-' && at == hyphen) continue;
				break;
			}
			if (!*at)
			{
				properties = NOUN | NOUN_NUMBER | ADJECTIVE | ADJECTIVE_NUMBER;
				entry = canonical = StoreWord(original, properties);
				if ((hyphen - original) == 4)
				{
					sysflags = entry->systemFlags | TIMEWORD;
					cansysflags = canonical->systemFlags | TIMEWORD;
				}
				return properties;
			}
		}

		// mark numeric fractions
		char* fraction = strchr(original, '/');
		if (fraction)
		{
			long basenumber = 0;
			// if we have piece before fraction
			if (hyphen) // we have prepart composite number like 3-1/2
			{
				*hyphen = 0;
				basenumber = atoi(original); // part before the - 
				*hyphen = '-';
				original = hyphen + 1; // piece before fraction
			}
			char number[MAX_WORD_SIZE];
			strcpy(number, original); // the number before the -
			if (IsNumber(number, numberStyle) != NOT_A_NUMBER  && IsNumber(fraction + 1, numberStyle))
			{
				int x = atoi(number);
				int y = atoi(fraction + 1);
				double val = (double)((double)x / (double)y);
				val += basenumber;
				WriteFloat(number, val);
				properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER;
				if (!entry) entry = StoreWord(original, properties);
				canonical = FindWord(number, 0, PRIMARY_CASE_ALLOWED);
				if (canonical) properties |= canonical->properties;
				else canonical = StoreWord(number, properties);
				sysflags = entry->systemFlags;
				cansysflags = entry->systemFlags;
				return properties;
			}
		}
	}

	// cannot be a text number if upper case not at start
	// unless the word only exists as upper case, e.g. Million in German
	// penn numbers as words do not go to change their entry value -- DO NOT TREAT "once" as a number, though canonical can be 1
	if (kind != ROMAN_NUMBER) {
		WORDP X = FindWord(original, 0, UPPERCASE_LOOKUP);
		if (!X || FindWord(original, 0, LOWERCASE_LOOKUP) || IS_NEW_WORD(X))
		{
			MakeLowerCase(original);
		}
	}
	entry = StoreWord(original);
	char number[MAX_WORD_SIZE];
	char* value;
	uint64 baseflags = (entry) ? entry->properties : 0;
	if (kind == ROMAN_NUMBER) baseflags = 0; // ignore other meanings
	char* br = hyphen;
	if (!br) br = strchr(original, '_');

	if (kind == PLACETYPE_NUMBER)
	{
		entry = StoreWord(original, properties);
		if (at > 1 && start != at && IsNumber(wordStarts[at - 1], numberStyle) != NOT_A_NUMBER && !strstr(original, (char*)"second")) // fraction not place
		{
			double val = 1.0 / Convert2Integer(original, numberStyle);
			WriteFloat(number,val);
		}
		else sprintf(number, (char*)"%d", (int)Convert2Integer(original, numberStyle));
		sysflags |= ORDINAL;
		properties |= ADVERB | ADJECTIVE | ADJECTIVE_NORMAL | ADJECTIVE_NUMBER | NOUN | NOUN_NUMBER | (baseflags & TAG_TEST); // place numbers all all potential adverbs:  "*first, he wept"  but not in front of an adjective or noun, only as verb effect
	}
	else if (kind == FRACTION_NUMBER && strchr(original, '%'))
	{
		int64 val1 = atoi(original);
		double val = (double)(val1 / 100.0);
		WriteFloat(number,  val);
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER;
		entry = StoreWord(original, properties);
		canonical = StoreWord(number, properties);
		properties |= canonical->properties;
		sysflags = entry->systemFlags;
		cansysflags = canonical->systemFlags;
		return properties;
	}
	else if (kind == FRACTION_NUMBER && br) // word fraction
	{
		char c = *br;
		*br = 0;
		int64 val1 = Convert2Integer(original, numberStyle);
		int64 val2 = Convert2Integer(br + 1, numberStyle);
		double val;
		if (IsNumber(original, numberStyle) == FRACTION_NUMBER) // half-dozen
		{
			val = (double)(1.0 / (double)val1);
			val *= (double)val2; // one-half
		}
		else val = (double)((double)val1 / (double)val2); // one-half
		WriteFloat(number, val);
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER;
		*br = c;
		entry = StoreWord(original, properties);
		canonical = StoreWord(number, properties);
		properties |= canonical->properties;
		sysflags = entry->systemFlags;
		cansysflags = canonical->systemFlags;
		return properties;
	}
	else if (kind == CURRENCY_NUMBER) // money
	{
		char copy[MAX_WORD_SIZE];
		strcpy(copy, original);
		unsigned char* currency = GetCurrency((unsigned char*)copy, value);
		if (currency > (unsigned char*)value) *currency = 0; // remove trailing currency
		int64 n = Convert2Integer(value, numberStyle);
		double fn = Convert2Float(value, numberStyle);
		if ((double)n == fn)
		{
#ifdef WIN32
			sprintf(number, (char*)"%I64d", n);
#else
			sprintf(number, (char*)"%lld", n);
#endif
		}
		else if (strchr(value, numberPeriod) || strchr(value,'e') || strchr(value,'E')) WriteFloat(number, fn);
		else
		{
#ifdef WIN32
			sprintf(number, (char*)"%I64d", n);
#else
			sprintf(number, (char*)"%lld", n);
#endif
		}
		properties = NOUN | NOUN_NUMBER;
		AddProperty(entry, CURRENCY);
	}
	else if (kind == FRACTION_NUMBER)
	{
		int64 val = Convert2Integer(original, numberStyle);
		double val1 = (double)1.0 / (double)val;
		sprintf(number, (char*)"%f", val1);
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER | (baseflags & (PREDETERMINER | DETERMINER));
	}
	else // ordinary int, double and percent
	{
		len = strlen(original);
		bool percent = original[len - 1] == '%';
		if (percent) original[len - 1] = 0;
		char* exponent = strchr(original, 'e');
		if (!exponent) exponent = strchr(original, 'E');
		if (exponent && !IsDigit(exponent[-1]) && exponent[-1] != '.') exponent = NULL; // no digit or period before
		if (exponent && !IsDigit(exponent[1]) && exponent[1] != '+' && exponent[1] != '-') exponent = NULL; // no digit or period before

		if (strchr(original, numberPeriod) || exponent) // floating
		{
			double val = Convert2Float(original, numberStyle);
			if (percent) val /= 100;
			WriteFloat(number,  val);
		}
		else if (percent)
		{
			int64 val = Convert2Integer(original, numberStyle);
			double val1 = ((double)val) / 100.0;
			WriteFloat(number,val1);
		}
		else
		{
			int64 val = Convert2Integer(original, numberStyle);
			if (val == NOT_A_NUMBER && IsNumber(original, numberStyle, false))
			{
				strcpy(number, original);
			}
			else
			{
#ifdef WIN32
				sprintf(number, (char*)"%I64d", val);
#else
				sprintf(number, (char*)"%lld", val);
#endif
			}
		}
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER | (baseflags & (PREDETERMINER | DETERMINER));
		if (percent) original[len - 1] = '%';
	}
	canonical = StoreWord(number, properties, sysflags);
	cansysflags |= sysflags;

	// other data already existing on the number

	if (entry->properties & PART_OF_SPEECH)
	{
		uint64 val = entry->properties; // numbers we "know" in some form should be as we know them. like "once" is adverb and adjective, not cardinal noun
		if (entry->properties & NOUN && !(entry->properties & NOUN_BITS)) // we dont know its typing other than as noun... figure it out
		{
			if (entry->internalBits & UPPERCASE_HASH) val |= NOUN_PROPER_SINGULAR;
			else val |= NOUN_SINGULAR;
		}
		if (val & ADJECTIVE_NORMAL) // change over to known number
		{
			properties ^= ADJECTIVE_NORMAL;
			properties |= ADJECTIVE_NUMBER | ADJECTIVE;
		}
		if (val & NOUN_SINGULAR) // change over to known number
		{
			properties ^= NOUN_SINGULAR;
			properties |= NOUN_NUMBER | NOUN;
			if (tokenControl & TOKEN_AS_IS && !canonical) canonical = entry;
		}
		if (val & ADVERB)
		{
			properties |= entry->properties & (ADVERB);
			if (tokenControl & TOKEN_AS_IS && !canonical) canonical = entry;
		}
		if (val & PREPOSITION)
		{
			properties |= PREPOSITION; // like "once"
			if (tokenControl & TOKEN_AS_IS && !canonical) canonical = entry;
		}
		if (val & PRONOUN_BITS)  // in Penntags this is CD always but "no one is" is NN or PRP
		{
			properties |= entry->properties & PRONOUN_BITS;
			//if (at > 1 && !stricmp(wordStarts[at-1],(char*)"no"))
			//{
			//properties |= val & PRONOUN_BITS; // like "one"
			if (tokenControl & TOKEN_AS_IS && !canonical) canonical = entry;
			//}
		}
		if (val & VERB)
		{
			properties |= entry->properties & (VERB_BITS | VERB); // like "once"
			if (tokenControl & TOKEN_AS_IS && !canonical) canonical = FindWord(GetInfinitive(original, false), 0, LOWERCASE_LOOKUP);
		}
		if (val & POSSESSIVE && tokenControl & TOKEN_AS_IS && !stricmp(original, (char*)"'s") && at > start) // internal expand of "it 's" and "What 's" and capitalization failures that contractions.txt wouldn't have handled 
		{
			properties |= AUX_BE | POSSESSIVE | VERB_PRESENT_3PS | VERB;
			entry = FindWord((char*)"'s", 0, PRIMARY_CASE_ALLOWED);
		}
	}
	entry = StoreWord(original, properties, sysflags);
	sysflags |= entry->systemFlags;
	cansysflags |= canonical->systemFlags;
	return properties;
}

bool KnownUsefulWord(WORDP entry)
{
	bool known = (entry) ? ((entry->properties & PART_OF_SPEECH) != 0) : false;
	if (!known && entry && entry->systemFlags & PATTERN_WORD) known = true; // script knows it to match
	if (!known && entry) // do we know it as concept or topic keyword?
	{
		FACT* F = GetSubjectNondeadHead(entry);
		while (F)
		{
			if (F->verb == Mmember)
			{
				known = true;
				break;
			}
			F = GetSubjectNondeadNext(F);
		}
	}
	return known;
}


uint64 GetPosData( int at, char* original,WORDP& revise, WORDP &entry,WORDP &canonical,uint64& sysflags,uint64 &cansysflags,bool firstTry,bool nogenerate, int start) // case sensitive, may add word to dictionary, will not augment flags of existing words
{ // this is not allowed to write properties/systemflags/internalbits if the word is preexisting
	uint64 properties = 0;
	sysflags = cansysflags = 0;
	canonical = 0;
	if (start == 0) start = 1;
	if (revise) revise = NULL;
	if (*original == 0) // null string
	{
		entry = canonical = StoreWord("null word");
		sysflags = 0;
		cansysflags = 0;
		revise = entry;
		return 0;
	}

	if (oobExists)
	{
		WORDP D = FindWord(original);
		if (D) 
		{
			entry = canonical = D;
			sysflags = D->systemFlags;
			return D->properties;
		}
		else
		{
			entry = canonical  = StoreWord(original);
			return 0;
		}
	}

	bool csEnglish = (externalTagger && stricmp(language, "english")) ? false : true;
	bool isGerman = (!stricmp(language, "german")) ? true : false;

	// number processing has happened
	int kind = IsNumber(original, numberStyle);
	if (firstTry && (kind != NOT_A_NUMBER || IsDate(original )))
		properties = ProcessNumber(at, original, revise, entry, canonical, sysflags, cansysflags, firstTry, nogenerate, start,kind); // case sensitive, may add word to dictionary, will not augment flags of existing wordskind);
	if (canonical && (IsDigit(*canonical->word) || IsNonDigitNumberStarter(*canonical->word))) return properties;
	entry = FindWord(original, 0, PRIMARY_CASE_ALLOWED);

	if (!csEnglish)
	{
		if (!entry) entry = FindWord(original, 0, SECONDARY_CASE_ALLOWED); // Try harder to find the foreign word, e.g. German wochentag -> Wochentag
		if (entry && !properties) canonical = GetCanonical(entry);
	}
	if (*original == '$' && !original[1]) { ; } // $
	else if (*original == '~' || (*original == USERVAR_PREFIX && !IsDigit(original[1])) || *original == '^' || (*original == SYSVAR_PREFIX && original[1]))
	{
		char copy[MAX_WORD_SIZE];
		MakeLowerCopy(copy,original);
		entry = canonical = StoreWord(copy);
		sysflags = 0;
		return 0;
	}

	if (at >= 2 && wordStarts[at-1] && *wordStarts[at-1] == '"') 
	{
		int x = at-1;
		while (--x >= start) if (*wordStarts[x] == '"') break;
		if (x > 0 && wordStarts[x] && *wordStarts[x] != '"') start = at; // there is no quote before us so we are starting quote (or ending quote on new sentence)
	}
	if (at > 0 && wordStarts[start] && (*wordStarts[start] == '"' || *wordStarts[start] == '(')) ++start; // skip over any quotes or paren starter -- consider next thing a starter
	if (at == 0) at = 1; //but leave <0 alone, means dont look at neighbors
	if (at > 0 && !wordStarts[at-1]) wordStarts[at-1] = AllocateHeap((char*)""); // protection
	if (at > 0 && !wordStarts[at+1]) wordStarts[at+1] = AllocateHeap((char*)"");	// protection

	if (tokenControl & ONLY_LOWERCASE && IsUpperCase(*original) && ((csEnglish && *original != 'I') || original[1])) MakeLowerCase(original);

	if (entry && entry->systemFlags & CONDITIONAL_IDIOM) 
	{
		char* script = entry->w.conditionalIdiom;
		if (script[1] != '=') return entry->properties; // has conditions, is not absolute and may be overruled
	}
	if (entry && !KnownUsefulWord(entry)) entry = 0; // not a word we actually know (might be phrase starter like Wait)

	///////////// PUNCTUATION

	if (*original == '-' && original[1] == '-' && !original[2])  // -- mdash equivalent WSJ
	{
		entry = canonical = StoreWord(original,PUNCTUATION);
		return PUNCTUATION;
	}
	if (*original == '.')
	{
		if (!original[1] || !strcmp(original,(char*)"..."))  // periods we NORMALLY kill off  .   and ...
		{
			entry = canonical = StoreWord(original,PUNCTUATION);
			return PUNCTUATION;
		}
	}

	///////////// URL WEBLINKS
	if (IsUrl(original,0)) 
	{
		properties = NOUN|NOUN_SINGULAR;
		char word[MAX_WORD_SIZE];
		MakeLowerCopy(word,original);
		entry = canonical = StoreWord(word,properties,WEB_URL); 
		cansysflags = sysflags = WEB_URL;
		return properties;
	}


	///////// WORD REWRITES particularly from pennbank tokenization

	if (csEnglish && *original == '@' && !original[1])
	{
		strcpy(original,(char*)"at");
		entry = canonical = FindWord(original,0,PRIMARY_CASE_ALLOWED);
		original = AllocateHeap(entry->word); 
		if (revise) revise = entry;
	}

	WORDP ZZ = FindWord(original,0,LOWERCASE_LOOKUP);
	if (!ZZ) {;}
	else if (ZZ->properties & (PRONOUN_SUBJECT|PRONOUN_OBJECT))
	{
		entry =  ZZ;
		original = AllocateHeap(entry->word);
		if (revise) revise = entry;
	}
	else if (start != at && tokenControl & STRICT_CASING) {;} // believe all upper case items not at sentence start when using strict casing
	else if (ZZ->properties & (DETERMINER|PREPOSITION|PRONOUN_POSSESSIVE|PRONOUN_BITS|AUX_VERB) && IsNumber(original, numberStyle) == NOT_A_NUMBER ) // prep and determiner are ALWAYS considered lowercase for parsing (which happens later than proper name extraction)
	{
		entry =  ZZ;
		original = AllocateHeap(entry->word);
		if (revise) revise = entry;
		if (ZZ->properties & (MORE_FORM|MOST_FORM)) canonical = NULL;	// we dont know yet
	}
	else if (at > 0 && ZZ->properties & (DETERMINER_BITS|PREPOSITION|CONJUNCTION|AUX_VERB) && strcmp(original,(char*)"May") && (*wordStarts[at-1] == '-' || *wordStarts[at-1] == ':' || *wordStarts[at-1] == '"' || at == startSentence || !(STRICT_CASING  & tokenControl))) // not the month
	{
		entry =  ZZ; //force lower case on all determiners and such
		original = AllocateHeap(entry->word);
		if (revise) revise = entry;
	}
	else if (at == start && ZZ->properties & VERB_INFINITIVE && !entry) // upper case start has no meaning but could be imperative verb, be that
	{
		entry = ZZ;
		original = AllocateHeap(entry->word);
		if (revise) revise = entry;
	}
	
	if (csEnglish && !stricmp(original,(char*)"yes") )
	{
		entry =  canonical = FindWord(original,0,LOWERCASE_LOOKUP); //force lower case pronoun, dont want "yes" to be Y's
		original = AllocateHeap(entry->word);
		if (revise) revise = entry;
	}
	else if (csEnglish && at > 0 && !stricmp(original,(char*)"'s") && (!stricmp(wordStarts[at-1],(char*)"there") || !stricmp(wordStarts[at-1],(char*)"it") || !stricmp(wordStarts[at-1],(char*)"who") || !stricmp(wordStarts[at-1],(char*)"what")  || !stricmp(wordStarts[at-1],(char*)"that") )) //there 's and it's  who's what's
	{
		entry = FindWord((char*)"is",0,LOWERCASE_LOOKUP);
		original = AllocateHeap(entry->word);
		if (revise) revise = entry;
	}
	size_t len = strlen(original);

	if (csEnglish && IsUpperCase(*original) && firstTry && kind != ROMAN_NUMBER) // someone capitalized things we think of as ordinary.
	{
		if (tokenControl & STRICT_CASING && at != start)  // we MUST believe him
		{
			if (!entry || !(entry->properties)) // things like Va.  as a substitute have no known data
			{
				if (original[len-1] == 's')
				{
					properties = NOUN|NOUN_PROPER_PLURAL; // a guess
				}
				else properties = NOUN|NOUN_PROPER_SINGULAR;
				entry = StoreWord(original,properties);
			}
			else properties = entry->properties;
			char* noun = GetSingularNoun(original,false,true);
			if (noun) canonical = FindWord(noun,0,UPPERCASE_LOOKUP);
			if (!canonical) canonical = entry;
			return properties;
		}

		WORDP check = FindWord(original,0,LOWERCASE_LOOKUP);
		if (check && check->properties & (PREPOSITION|DETERMINER_BITS|CONJUNCTION|PRONOUN_BITS|POSSESSIVE_BITS)) 
		{
			entry =  FindWord(original,0,LOWERCASE_LOOKUP); //force lower case pronoun, dont want "His" as plural of HI nor thi's
			original = AllocateHeap(entry->word);
			if (revise) revise = entry;
		}
	}

	// compute length NOW after possible changes to original
	len = strlen(original);

	// ILLEGAL STUFF that our tokenization wouldn't provide
	if (csEnglish && len > 2 && original[len-2] == '\'')  // "it's  and other illegal words"
	{
		canonical = entry = StoreWord(original,0);
		cansysflags = sysflags = entry->systemFlags; // probably nothing here
		return 0;
	}
	
	// hyphenated word which is number on one side:   Cray-3  3-second
	char* hyphen = strchr(original,'-');
	if (hyphen && !hyphen[1]) hyphen = NULL; // not real

	// use forced canonical?
	if (!canonical && entry)
	{
		WORDP canon = GetCanonical(entry);
		if (canon) canonical = canon;
	}
	
	if (entry && entry->properties & (PART_OF_SPEECH|TAG_TEST|PUNCTUATION)) // we know this usefully already
	{
		properties |= entry->properties;
		sysflags |= entry->systemFlags;
		if (csEnglish && properties & VERB_PAST)
		{
			char* participle = GetPastParticiple(GetInfinitive(original,true));
			if (participle && !strcmp(participle,original)) properties |= VERB_PAST_PARTICIPLE; // wordnet exceptions doesnt bother to list both
		}
		if (csEnglish && properties & VERB_PAST_PARTICIPLE)
		{
			char* participle = GetPastParticiple(GetInfinitive(original,true));
			if (participle && !strcmp(participle,original)) properties |= NOUN_ADJECTIVE;
		}
		WORDP canon = GetCanonical(entry);
		if (canon) canonical = canon;
		if (canonical) cansysflags = canonical->systemFlags;

		// possessive pronoun-determiner like my is always a determiner, not a pronoun. 
		if (entry->properties & (COMMA | PUNCTUATION | PAREN | QUOTE | POSSESSIVE | PUNCTUATION)) return properties;
	}
	bool known = KnownUsefulWord(entry);
	bool preknown = known;

	/////// WHETHER OR NOT WE KNOW THE WORD, IT MIGHT BE ALSO SOME OTHER WORD IN ALTERED FORM (like plural noun or comparative adjective)
	char lower[MAX_WORD_SIZE];

	if (csEnglish && !(properties & VERB_BITS) && (at == start || !IsUpperCase(*original))) // could it be a verb we dont know directly (even if we know the word)
	{
		MakeLowerCopy(lower,original);
		char* verb =  GetInfinitive(lower,true); 
		if (verb)  // inifinitive will be different from original or we would already have found the word
		{
			known = true;
			properties |= VERB | verbFormat;
			if (verbFormat & (VERB_PAST|VERB_PAST_PARTICIPLE)) // possible shared form with participle
			{
				char* pastparticiple = GetPastParticiple(verb);
				if (pastparticiple && !strcmp(pastparticiple,lower)) properties |= VERB_PAST_PARTICIPLE | NOUN_ADJECTIVE;
			}
			entry = StoreWord(lower,properties);
			canonical =  FindWord(verb,0,PRIMARY_CASE_ALLOWED); // we prefer verb as canonical form
		}
	}
	
	if (csEnglish && !(properties & (NOUN_BITS|PRONOUN_BITS))) // could it be plural noun we dont know directly -- eg dogs or the plural of a singular we know differently-- "arms is both singular and plural" - avoid pronouns like "his" or "hers"
	{
		MakeLowerCopy(lower,original);
		WORDP X = FindWord(original,0,LOWERCASE_LOOKUP);
		if (X && strcmp(original,X->word)) // upper case noun we know in lower case -- "Proceeds"
		{
			properties |= X->properties & NOUN_BITS;
			if (!entry) entry = canonical = X;
		}
		if (original[len-1] == 's')
		{
			char* noun = GetSingularNoun(original,true,true);
			if (noun && strcmp(noun,original)) 
			{
				WORDP X = StoreWord(original,NOUN);
				if (!entry) entry = X;
				uint64 which = (X->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_PLURAL : NOUN_PLURAL;
				AddProperty(X,which);
				properties |= NOUN|which;
				if (!canonical) canonical = FindWord(noun,0,PRIMARY_CASE_ALLOWED); // 2ndary preference for canonical is noun

				// can it be a number?
				unsigned int kind =  IsNumber(noun, numberStyle);
				char number[MAX_WORD_SIZE];
				if (kind != NOT_A_NUMBER && kind != PLACETYPE_NUMBER) // do not decode seconds to second Place, but tenths can go to tenth
				{
					int64 val = Convert2Integer(noun, numberStyle);
					if (val < 1000000000 && val >  -1000000000)
					{
						int smallval = (int) val;
						sprintf(number,(char*)"%d",smallval);
					}
					else
					{
		#ifdef WIN32
						sprintf(number,(char*)"%I64d",val);	
		#else
						sprintf(number,(char*)"%lld",val);	
		#endif
					}
					properties = NOUN|NOUN_NUMBER;
					canonical = StoreWord(number,properties,sysflags);
				}
			}
		}
	}

	if (csEnglish && !(properties & ADJECTIVE_BITS) && (at == start || !IsUpperCase(*original)) && len > 3) // could it be comparative adjective we werent recognizing even if we know the word
	{
		MakeLowerCopy(lower,original);
		if (lower[len-1] == 'r' && lower[len-2] == 'e' && !canonical) // if canonical its like "slaver" and should never be reduced
		{
			char* adjective = GetAdjectiveBase(lower,true);
			if (adjective && strcmp(adjective,lower)) 
			{
				WORDP D = StoreWord(lower,ADJECTIVE|ADJECTIVE_NORMAL|adjectiveFormat);
				if (!entry) entry = D;
				properties |= ADJECTIVE|ADJECTIVE_NORMAL;
				if (!canonical) canonical = FindWord(adjective,0,PRIMARY_CASE_ALLOWED); 
				properties |= adjectiveFormat;
			}
		}
		else if (lower[len-1] == 't' && lower[len-2] == 's'  && lower[len-3] == 'e' && !canonical)
		{
			char* adjective = GetAdjectiveBase(lower,true);
			if (adjective && strcmp(adjective,lower)) 
			{
				WORDP D = StoreWord(lower,ADJECTIVE|ADJECTIVE_NORMAL|adjectiveFormat);
				if (!entry) entry = D;
				properties |= ADJECTIVE|ADJECTIVE_NORMAL;
				if (!canonical) canonical = FindWord(adjective,0,PRIMARY_CASE_ALLOWED); 
				properties |= adjectiveFormat;
			}
		}
	}

	if (csEnglish && !(properties & ADVERB) && !(properties & (NOUN|VERB)) && (at == start || !IsUpperCase(*original)) && len > 3) // could it be comparative adverb even if we know the word
	{
		MakeLowerCopy(lower,original);
		if (lower[len-1] == 'r' && lower[len-2] == 'e'  && !canonical)
		{
			char* adverb = GetAdverbBase(lower,true);
			if (adverb && strcmp(adverb,lower)) 
			{
				WORDP D = StoreWord(original,ADVERB|adverbFormat);
				if (!entry) entry = D;
				properties |= ADVERB;
				if (!canonical) canonical = FindWord(adverb,0,PRIMARY_CASE_ALLOWED); 
				properties |= adverbFormat;
			}
		}
		else if (lower[len-1] == 't' && lower[len-2] == 's'  && lower[len-3] == 'e'  && !canonical)
		{
			char* adverb = GetAdverbBase(lower,true);
			if (adverb && strcmp(adverb,lower)) 
			{
				WORDP D = StoreWord(lower,ADVERB|adverbFormat);
				if (!entry) entry = D;
				properties |= ADVERB;
				if (!canonical) canonical = FindWord(adverb,0,PRIMARY_CASE_ALLOWED); 
				properties |= adverbFormat;
			}
		}
	}

	// DETERMINE CANONICAL OF A KNOWN WORD
	if (csEnglish && !canonical && !IS_NEW_WORD(entry)) // we dont know the word and didn't interpolate it from noun or verb advanced forms (cannot get canonical of word created this volley)
	{
		if (properties & (VERB | NOUN_GERUND))
		{
			char* inf = GetInfinitive(original, true);
			if (inf) canonical = FindWord(inf, 0, PRIMARY_CASE_ALLOWED); // verb or known gerund (ing) or noun plural (s) which might be a verb instead
		}
		if (!canonical && properties & (MORE_FORM | MOST_FORM)) // get base form
		{
			char* adj = GetAdjectiveBase(original, true);
			if (adj) canonical = FindWord(adj,0,PRIMARY_CASE_ALLOWED);
			if (!canonical) canonical = FindWord(GetAdverbBase(original,true),0,PRIMARY_CASE_ALLOWED);
		}
		if (!canonical && properties & NOUN) // EVEN if we do know it... flies is a singular and needs canonical for fly BUG
		{
			char* singular = GetSingularNoun(original,true,true);
			// even if it is a noun, if it ends in s and the root is also a noun, make it plural as well (e.g., rooms)
			if (original[len-1] == 's' && singular && stricmp(singular,original)) 
			{
				known = true;
				properties |= NOUN_PLURAL;
			}
			if (!canonical) canonical = FindWord(singular,0,PRIMARY_CASE_ALLOWED);
		}

		if (!canonical)
		{
			char* adj = GetAdjectiveBase(original, true);
			if (adj) canonical = FindWord(adj, 0, PRIMARY_CASE_ALLOWED);
		}

		if (properties & (ADJECTIVE_NORMAL|ADVERB)) // some kind of known adjective or adverb
		{
			char* adjective;
			char* adverb;
			if (hyphen  && hyphen != original) // looking to add "more" and "most" properties to a hyphenated adjective or adverb
			{
				*hyphen = 0;
				char word[MAX_WORD_SIZE];
				WORDP X = FindWord(original,0,LOWERCASE_LOOKUP);
				if (X && X->properties & ADJECTIVE_NORMAL) // front part is known adjective
				{
					adjective = GetAdjectiveBase(original,true); 
					if (adjective && strcmp(adjective,original)) // base is not the same
					{
						if (!canonical) 
						{
							sprintf(word,(char*)"%s-%s",adjective,hyphen+1);
							canonical = StoreWord(word,ADJECTIVE_NORMAL|ADJECTIVE);
						}
						properties |= adjectiveFormat;
					}
				}
				if (X && X->properties & ADVERB) // front part is known adverb
				{
					adverb = GetAdverbBase(original,true); 
					if (adverb && strcmp(adverb,original)) // base is not the same
					{
						if (!canonical)
						{
							sprintf(word,(char*)"%s-%s",adverb,hyphen+1);
							canonical = StoreWord(word,ADVERB);
						}
						properties |= adverbFormat;
					}
				}
				*hyphen = '-';
			}
		}
		else if (!canonical && properties & (NOUN_SINGULAR | NOUN_PROPER_SINGULAR)) canonical = entry;
	}
	if (csEnglish && preknown && !canonical) // may have been seen earlier in sentence so have to check what base SHOULD be
	{
		char* base = NULL;
		if (entry->properties & (MORE_FORM|MOST_FORM))
		{
			if (entry->properties & ADJECTIVE) base = GetAdjectiveBase(entry->word,true);
			else  base = GetAdverbBase(entry->word,true);
		}
		else if (entry->properties & VERB) base = GetInfinitive(entry->word,true);
		else if (entry->properties & NOUN) base = GetSingularNoun(entry->word,false,true);
		canonical = (base) ? FindWord(base) : entry; 
	}
	
	if (csEnglish && hyphen  && hyphen != original) // check for possible adj meaning "six-pack" is known as noun but can also be adj. as is "6-month"
	{
		*hyphen = 0;
		WORDP X = FindWord(original,0,LOWERCASE_LOOKUP);
		if (!X) X = StoreWord(original);
		WORDP Y = FindWord(hyphen+1,0,LOWERCASE_LOOKUP);
		*hyphen = '-';
		// adjective made from counted singular noun:  6-month
		if (X && Y && IsNumber(X->word,numberStyle,false) != NOT_A_NUMBER)
		{
			if (Y->properties & NOUN && !stricmp(Y->word,GetSingularNoun(Y->word, false, true))) // number with singular noun cant be anything but adjective
			{
				properties |= ADJECTIVE_NORMAL;
				canonical = entry = StoreWord(original,ADJECTIVE_NORMAL);
				preknown = true;	// lie. we know it is only an adjective
			}
		}

		if (X && X->properties & NOUN_BITS && !stricmp(hyphen+1,(char*)"like")) // front part is noun, "needle-like" becomes an adjective
		{
			properties |= ADJECTIVE_NORMAL;
			canonical = entry = StoreWord(original,ADJECTIVE_NORMAL);
			preknown = true;	// lie. we know it is only an adjective
		}
		else if (Y && Y->properties & ADJECTIVE_BITS && !(Y->properties & (NOUN_BITS|VERB_BITS))) // year-earlier
		{
			properties |= (Y->properties & (ADJECTIVE_NORMAL | ADVERB)) | adjectiveFormat;
			canonical = entry = StoreWord(original,properties);
			preknown = true;	// lie. we know it is only an adjective
		}
	}
	char tmpword[MAX_WORD_SIZE];

	// A WORD WE NEVER KNEW - figure it out
	if (csEnglish && !preknown) // if we didnt know the original word, then even if we've found noun/verb forms of it, we need to test other options
	{
			// process by know parts of speech or potential parts of speech
		if (!(properties & NOUN)) // could it be a noun but not already known as a known (eg noun_gerund from verb)
		{
			char* noun = GetSingularNoun(original,true,true); 
			if (noun) 
			{
				known = true;
				properties |= NOUN | nounFormat;
				entry = StoreWord(original,0);
				if (!canonical || !stricmp(canonical->word,original)) canonical = FindWord(noun,0,PRIMARY_CASE_ALLOWED);
			}
		}
		if (!(properties & ADJECTIVE) && !IsUpperCase(*original)) 
		{
			char* adjective = GetAdjectiveBase(original,true); 
			if (len > 4 && !adjective && hyphen  && hyphen != original && !strcmp(original+len-3,(char*)"ing")) // some kind of verb ing formation which can be adjective particple
			{
				*hyphen = 0;
				char word[MAX_WORD_SIZE];
				WORDP X = FindWord(original,0,LOWERCASE_LOOKUP);
				*hyphen = '-';
				if (X && X->properties & ADJECTIVE_NORMAL)
				{
					adjective = GetAdjectiveBase(original,true); 
					if (adjective && strcmp(adjective,original)) // base is not the same
					{
						sprintf(word,(char*)"%s-%s",adjective,hyphen+1);
						canonical = StoreWord(word,ADJECTIVE_NORMAL|ADJECTIVE);
					}
				}
			}
			if (!adjective && hyphen  && hyphen != original) // third-highest
			{
				adjective = GetAdjectiveBase(hyphen+1,true);
				if (adjective && strcmp(hyphen+1,adjective)) // base is not the same
				{
					sprintf(tmpword,(char*)"%s-%s",original,adjective);
					canonical = StoreWord(tmpword,ADJECTIVE_NORMAL|ADJECTIVE);
					properties |= adjectiveFormat;
				}
			}
			if (adjective && hyphen  && hyphen != original) 
			{
				known = true;
				properties |= ADJECTIVE|ADJECTIVE_NORMAL;
				entry = StoreWord(original,adjectiveFormat);
				properties |= adjectiveFormat;
				if (!canonical) canonical = entry;
			}
		}
		if (!(properties & ADVERB) && !IsUpperCase(*original)) 
		{
			char* adverb = GetAdverbBase(original,true); 
			if (!canonical && adverb) canonical = FindWord(adverb,0,PRIMARY_CASE_ALLOWED);
			if (!adverb && hyphen  && hyphen != original) 
			{
				adverb = GetAdverbBase(hyphen+1,true);
				if (adverb && strcmp(hyphen+1,adverb)) // base is not the same
				{
					sprintf(tmpword,(char*)"%s-%s",original,adverb);
					canonical = StoreWord(tmpword,ADJECTIVE_NORMAL|ADJECTIVE);
					properties |= adverbFormat;
				}
			}
			if (adverb && hyphen  && hyphen != original)  
			{
				known = true;
				properties |= ADVERB;
				entry = StoreWord(original,adverbFormat);
				properties |= adverbFormat;
				if (!canonical) canonical = FindWord(adverb,0,PRIMARY_CASE_ALLOWED);
			}
		}
		
		if (hyphen  && hyphen != original && !IsUpperCase(*original)) // unknown hypenated words even when going to base
		{
			WORDP X;
			// co-author   could be noun with front stuff in addition
			char* noun = GetSingularNoun(original,true,true);
			if (!noun) // since we actually know the base, we are not trying to interpolate
			{
				noun = GetSingularNoun(hyphen+1,true,true);
				if (noun)
				{
					properties |= NOUN | nounFormat;
					X = FindWord(noun,0,PRIMARY_CASE_ALLOWED);
					if (X)  // inherit properties of known word
					{
						properties |= X->properties & (NOUN|NOUN_PROPERTIES);
						sysflags |= X->systemFlags & (NOUN_NODETERMINER);
					}

					entry = StoreWord(original,properties,sysflags);
					strcpy(tmpword,original);
					strcpy(tmpword+(hyphen+1-original),noun);
					if (!canonical || !stricmp(canonical->word,original)) canonical = StoreWord(tmpword,NOUN|NOUN_SINGULAR,sysflags);
				}
			}
			char* verb = GetInfinitive(hyphen+1,true);
			if (verb)
			{
				X = FindWord(verb,0,PRIMARY_CASE_ALLOWED);
				if (X)  
				{
					properties |=  VERB|verbFormat; // note "self-governed" can be noun or verb and which makes a difference to canonical.
					if (!canonical) 
					{
						strcpy(tmpword,original);
						char* h = tmpword + (hyphen-original);
						strcpy(h+1,verb);
						canonical = StoreWord(tmpword,VERB|VERB_INFINITIVE);
					}
				}
			}

			if (!properties) // since we recognize no component of the hypen, try as number stuff
			{
				*hyphen = 0;
				if (IsNumber(original, numberStyle) != NOT_A_NUMBER )
				{
					int64 n;
					n = Convert2Integer((IsNumber(original, numberStyle) != NOT_A_NUMBER || IsDigit(*original)) ? original : (hyphen+1), numberStyle);
					#ifdef WIN32
					sprintf(tmpword,(char*)"%I64d",n); 
#else
					sprintf(tmpword,(char*)"%lld",n); 
#endif
					*hyphen = '-';
					properties = NOUN|NOUN_NUMBER|ADJECTIVE|ADJECTIVE_NUMBER;
					entry = StoreWord(original,properties,TIMEWORD|MODEL_NUMBER);
					canonical = StoreWord(tmpword,properties,TIMEWORD|MODEL_NUMBER);
					sysflags |= MODEL_NUMBER | TIMEWORD;
					cansysflags |= MODEL_NUMBER|TIMEWORD;
					return properties;
				}
				*hyphen = '-';
			}
		}
	}

	// fill in supplemental flags - all German nouns are uppercase, so cannot use it to determine if proper or not
	if (!isGerman && properties & NOUN && !(properties & NOUN_BITS))
	{
		if (entry && entry->internalBits & UPPERCASE_HASH) properties |= NOUN_PROPER_SINGULAR;
		else properties |= NOUN_SINGULAR;
	}
	if (canonical && entry) entry->systemFlags |= canonical->systemFlags & AGE_LEARNED; // copy age data across
	else if (entry && IS_NEW_WORD(entry) && !canonical) canonical = DunknownWord;

	if (properties){;}
	else if (tokenControl & ONLY_LOWERCASE) {;}
	else if (at > 0 && tokenControl & STRICT_CASING && at != start && *wordStarts[at-1] != ':'){;} // can start a sentence after a colon (like newspaper headings
	else if (isGerman) {;}
	else 
	{
#ifndef NOPOSPARSER
		// we will NOT allow capitalization shift on 1st word if it appears to be part of proper noun, unless its a simple finite word potentially
		WORDP X = (at == start) ? FindWord(original,0,UPPERCASE_LOOKUP) : NULL;
		if (X) // we have a known uppercase word
		{
			if (at == endSentence || at  < 0) {;}
			else if (!IsUpperCase(*wordStarts[at+1]) || !wordStarts[at+1][1] || !wordStarts[at+1][2]) X = NULL; // next word is not big enough upper case
			else
			{
				WORDP Y = FindWord(wordStarts[at+1],0,UPPERCASE_LOOKUP);
				if (Y && Y->properties & (NOUN_TITLE_OF_ADDRESS|NOUN_FIRSTNAME)) X = NULL;	// first name wont have stuff before it
				else if (X->properties & ADVERB && !(X->properties & ADJECTIVE_BITS)) X = NULL; // adverb wont likely head a name
				else if (X->properties & (DETERMINER_BITS|VERB_INFINITIVE|PRONOUN_BITS|CONJUNCTION)) X = NULL; // nor will these
			}
		}

		if (X) {;}	// probable proper name started
		else if (firstTry && *original /*&& IsAlphaUTF8(*original) */) // auto try for opposite case if we dont recognize the word, and not concept or other non-words
		{
			char alternate[MAX_WORD_SIZE];
			if (IsUpperCase(*original)) MakeLowerCopy(alternate,original);
			else
			{
				strcpy(alternate,original);
				alternate[0] = toUppercaseData[alternate[0]];
			}
			WORDP D1,D2;
			WORDP xrevise;
			uint64 flags1 = GetPosData(at,alternate,xrevise,D1,D2,sysflags,cansysflags,false,nogenerate,start);
			if (revise) 
			{
				wordStarts[at] = xrevise->word;
				original = xrevise->word;
			}
			if (flags1) 
			{
				original = D1->word;
				if (revise) wordStarts[at] = xrevise->word;
				entry = D1;
				canonical = D2;
				return flags1;
			}
		}
#else
		if ( IsUpperCase(*original)) // dont recognize this, see if we know  lower case if this was upper case
		{
			WORDP D = FindWord(original,0,LOWERCASE_LOOKUP);
			WORDP revise;
			if (D) return GetPosData(at,D->word,revise,entry,canonical,sysflags,cansysflags,false,nogenerate);
		}
#endif
	}

	// we still dont know this word, go generic if its lower and upper not known
	char word[MAX_WORD_SIZE];
	strcpy(word,original);
	word[len-1] = 0; // word w/o trailing s if any
	if (nogenerate){;}
	else if (!csEnglish) {;}
	else if (!firstTry) {;} // this is second attempt
	else if (!properties && firstTry) 
	{
		size_t len = strlen(original);
		if (IsUpperCase(*original) || IsUpperCase(original[1]) || (len > 2 && IsUpperCase(original[2])) ) // default it noun upper case
		{
			properties |= (original[len-1] == 's') ? NOUN_PROPER_PLURAL : NOUN_PROPER_SINGULAR;
			entry = StoreWord(original,0);
		}
		else // was lower case
		{
			properties |= (original[len-1] == 's') ? (NOUN_PLURAL|NOUN_SINGULAR) : NOUN_SINGULAR;
			entry = StoreWord(original,0);
		}
		properties |= NOUN;

		// default everything else
		bool mixed = (entry->internalBits & UPPERCASE_HASH) ? true : false;
		for (unsigned int i = 0; i < len; ++i) 
		{
			if (!IsAlphaUTF8(original[i]) && original[i] != '-' && original[i] != '_') 
			{
				mixed = true; // has non alpha in it
				break;
			}
		}
		if (len ==1 ){;}
		else if (!mixed || entry->internalBits & UTF8) // could be ANYTHING
		{
			properties |= VERB | VERB_PRESENT | VERB_INFINITIVE |  ADJECTIVE | ADJECTIVE_NORMAL | ADVERB | ADVERB;
			if (properties & VERB)
			{
				if (!strcmp(original+len-2,(char*)"ed")) properties |= VERB_PAST | VERB_PAST_PARTICIPLE;
				if (original[len-1] == 's') properties |= VERB_PRESENT_3PS;
			}
			uint64 expectedBase = 0;
			if (ProbableAdjective(original,len,expectedBase)) 
			{
				AddSystemFlag(entry,ADJECTIVE|expectedBase); // probable decode
				sysflags |= ADJECTIVE|expectedBase;
			}
			expectedBase = 0;
			if (ProbableAdverb(original,len,expectedBase)) 
			{
				AddSystemFlag(entry,ADVERB|expectedBase);		 // probable decode
				sysflags |= ADVERB|expectedBase;
			}
			if (ProbableNoun(original,len)) 
			{
				AddSystemFlag(entry,NOUN);			// probable decode
				sysflags |= NOUN;
			}
			uint64 vflags = ProbableVerb(original,len);
			if (vflags ) 
			{
				AddSystemFlag(entry,VERB);			// probable decode
				sysflags |= VERB;
				properties &= -1 ^ VERB_BITS;			// remove default tenses
				properties |= vflags;					// go with projected tenses
			}
		}

		// treat all hypenated words as adjectives "bone-headed"
		if (!canonical) canonical = DunknownWord;
	}
	if (!entry) 
	{
		entry = StoreWord(original,properties);	 // nothing found (not allowed alternative)
		if (!canonical) canonical = DunknownWord;
	}
	if (!canonical) canonical = entry;
	if (IsModelNumber(original))  properties |= MODEL_NUMBER;
 
	AddProperty(entry,properties);
	// interpolate singular normal nouns to adjective_noun EXCEPT qword nouns like "why"
	//if (properties & (NOUN_SINGULAR|NOUN_PLURAL) && !(entry->properties & QWORD) && !strchr(entry->word,'_')) flags |= ADJECTIVE_NOUN; // proper names also when followed by ' and 's  merge to be adjective nouns  
	//if (properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL) && *wordStarts[at+1] == '\'' && (wordStarts[at+1][1] == 0 || wordStarts[at+1][1] == 's')) flags |= ADJECTIVE_NOUN; 
	//if (properties & NOUN_PROPER_SINGULAR && !(tokenControl & TOKEN_AS_IS)) flags |= ADJECTIVE_NOUN; // could be before "*US president"

	if (properties & VERB_INFINITIVE) properties |= NOUN_INFINITIVE;
	if (properties & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && !(properties & AUX_VERB)) // potential aux verbs will never be adjective participle
	{
		properties |= ADJECTIVE_PARTICIPLE|ADJECTIVE;
	}
	if (properties & VERB_PRESENT_PARTICIPLE) properties |= NOUN_GERUND|NOUN;
	if (*entry->word == '~' || *entry->word == '^' || *entry->word == USERVAR_PREFIX) canonical = entry;	// not unknown, is self
	cansysflags |= canonical->systemFlags;
	sysflags |= entry->systemFlags;
	return properties;
}

void SetSentenceTense(int start, int end)
{
	uint64 aux[25];
	unsigned int auxIndex = 0;
	uint64 mainverbTense = 0;
	unsigned int mainVerb = 0;
	unsigned int mainSubject = 0;
	memset(aux,0,25 * sizeof(uint64));
	unsigned int defaultTense = 0;
	bool subjectFound = false;
	if ((trace & TRACE_POS || prepareMode == POS_MODE) && CheckTopicTrace()) 
	{
		if (  tmpPrepareMode == POS_MODE || prepareMode == POSVERIFY_MODE  || prepareMode == POSTIME_MODE ) Log(STDTRACELOG,(char*)"Not doing a parse.\r\n");
	}

	// assign sentence type
	if (!verbStack[MAINLEVEL] || !(roles[verbStack[MAINLEVEL]] &  MAINVERB)) // FOUND no verb, not a sentence
	{
		if ((trace & TRACE_POS || prepareMode == POS_MODE) && CheckTopicTrace()) Log(STDTRACELOG,(char*)"Not a sentence\r\n");
		if (tokenFlags & (QUESTIONMARK|EXCLAMATIONMARK)) {;}
		else if (posValues[startSentence] & AUX_VERB) tokenFlags |= QUESTIONMARK;// its a question because AUX starts
		else if (allOriginalWordBits[startSentence]  & QWORD)
		{
			if (!stricmp(wordStarts[startSentence],(char*)"how") && endSentence != 1 && !(posValues[startSentence+1] & (AUX_VERB | VERB_BITS)));  // not "how very american you are"
			else tokenFlags |= QUESTIONMARK; 
		}
		else if (allOriginalWordBits[startSentence] & PREPOSITION && allOriginalWordBits[startSentence+1] & QWORD) tokenFlags |= QUESTIONMARK;
	}
	else if (!(tokenFlags & (QUESTIONMARK|EXCLAMATIONMARK))) // see if question or statement if we dont know
	{
		int i;
		for (i = startSentence; i <= endSentence; ++i) 
		{
			if (roles[i] & MAINSUBJECT) 
			{
				subjectFound = true;
				break;
			}
		}
		if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] > verbStack[MAINLEVEL] && (allOriginalWordBits[startSentence] & QWORD || (allOriginalWordBits[startSentence] & PREPOSITION && allOriginalWordBits[startSentence+1] & QWORD)))  tokenFlags |= QUESTIONMARK; // qword + flipped order must be question (vs emphasis?)
	
		bool foundVerb = false;
		for (i = startSentence; i <= endSentence; ++i) 
		{
			if (ignoreWord[i]) continue;
			if (roles[i] & MAINVERB) foundVerb = true;
			if (roles[i] & MAINSUBJECT) 
			{
				if (i == startSentence && originalLower[startSentence] && originalLower[startSentence]->properties & QWORD) tokenFlags |= QUESTIONMARK;
				break;
			}
			if (phrases[i] || clauses[i] || verbals[i]) continue;
			if ( bitCounts[i] != 1) break;	// all bets about structure are now off
			if (posValues[i] & AUX_VERB)
			{
				size_t len = strlen(wordStarts[i]);
				bool normalAux = true;
				if (len > 3 && !stricmp(wordStarts[i]+len-3,"ing")) normalAux = false; // not Getting or gets in present tense
				if (!normalAux){;}
				else if (i == startSentence) // its a question because AUX or VERB comes before MAINSUBJECT
				{
					tokenFlags |= QUESTIONMARK;
					break;
				}
				else if (!subjectFound && !foundVerb) // its a question because AUX or VERB comes before MAINSUBJECT unless we have a command before
				{
					// EXCEPT for negative adverb starter: (char*)"never can I go home"
					if (parseFlags[i-1] & NEGATIVE_ADVERB_STARTER ) {;}
					else
					{
						tokenFlags |= QUESTIONMARK;
						break;
					}
				}
			}
			if (roles[i] & CONJUNCT_SENTENCE && i > 3) break;	// do only one of the sentences
		}
	}

	// command?
	int i;
	for (i = start; i <= end; ++i)
    {
		if (ignoreWord[i]) continue;
		if (roles[i] & (MAINOBJECT|MAINSUBJECT|MAINVERB)) break; 
	}
	if (!stricmp(wordStarts[start],(char*)"why"))
	{
		tokenFlags |= QUESTIONMARK; 
		if (posValues[start+1] & VERB_INFINITIVE || posValues[start+2] & VERB_INFINITIVE) tokenFlags |= IMPLIED_YOU;
	}
	else if (!stricmp(wordStarts[start],(char*)"how") && !stricmp(wordStarts[start+1],(char*)"many")) tokenFlags |= QUESTIONMARK; 
	else if (posValues[start] & AUX_VERB && (!(posValues[start] & AUX_DO) || !(allOriginalWordBits[start] & AUX_VERB_PRESENT))){;} // not "didn't leave today." ommited subject
	else if (roles[start] & MAINVERB && !stricmp(wordStarts[start],(char*)"assume")) tokenFlags |= COMMANDMARK|IMPLIED_YOU; // treat as sentence command
	else if (roles[start] & MAINVERB && (!stricmp(wordStarts[start+1],(char*)"you") || subjectStack[MAINLEVEL])) tokenFlags |= QUESTIONMARK; 
	else if (roles[start] & MAINVERB && posValues[start] & VERB_INFINITIVE) tokenFlags |= COMMANDMARK|IMPLIED_YOU; 

	// determine sentence tense when not past from verb using aux (may pick wrong aux)
	for (int i = start; i <= end; ++i)
    {
		if (ignoreWord[i]) continue;
		if (roles[i] & MAINSUBJECT) mainSubject = i;

		bool notclauseverbal = true;
#ifndef DISCARDPARSER
		if (roles[i] & MAINVERB) 
		{
			mainverbTense = posValues[i] & VERB_BITS;
			mainVerb = i;
			if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PAST_PARTICIPLE|VERB_PAST) && !(tokenFlags & PRESENT)) tokenFlags |= PAST; // "what are you interested in"
			else if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS)) defaultTense = PRESENT;
		}
		// if not parsing but are postagging
		if (!mainverbTense && !roles[i] && posValues[i] & VERB_BITS) 
		{
			mainverbTense = posValues[i] & VERB_BITS;
			mainVerb = i;
			if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PAST_PARTICIPLE|VERB_PAST)) tokenFlags |= PAST;
			else if (!(tokenFlags & FUTURE) && posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS)) defaultTense = PRESENT;
		}
		if (clauses[i] || verbals[i]) notclauseverbal = false;

#endif

		if (posValues[i] & AUX_VERB && notclauseverbal)
		{
			aux[auxIndex] = originalLower[i] ? (originalLower[i]->properties & (AUX_VERB | VERB_BITS)) : 0;	// pattern of aux

			// question? 
			if (i == start && !(tokenControl & NO_INFER_QUESTION) && subjectFound) tokenFlags |= QUESTIONMARK;
			if (defaultTense){;}
			else if (aux[auxIndex] & AUX_BE)
			{
				if (aux[auxIndex]  & (VERB_PRESENT|VERB_PRESENT_3PS)) tokenFlags |= PRESENT;
				else if (aux[auxIndex]  & (VERB_PAST | VERB_PAST_PARTICIPLE)) tokenFlags |= PAST;
			}
			else if (aux[auxIndex]  & AUX_VERB_FUTURE ) 
			{
				tokenFlags &= -1 ^ (PAST|PRESENT); // I am going to swim will be future
				tokenFlags |= FUTURE;
			}
			else if (aux[auxIndex]  & AUX_VERB_PAST) tokenFlags |= PAST;
			auxIndex++;
			if (auxIndex > 20) break;
		}
	}
	if (!stricmp(wordStarts[start],(char*)"what") && posValues[start] & DETERMINER_BITS) tokenFlags |= QUESTIONMARK; 
	if (!auxIndex && canonicalLower[start] && !stricmp(canonicalLower[start]->word,(char*)"be") && !(tokenControl & NO_INFER_QUESTION)) tokenFlags |= QUESTIONMARK;  // are you a bank teller
	// How very american is NOT a question
	if (canonicalLower[start] && !stricmp(canonicalLower[start]->word,(char*)"how") && (tokenFlags & PERIODMARK || !mainVerb)){;} // just believe them
	else if ( canonicalLower[start] && canonicalLower[start]->properties & QWORD && canonicalLower[start+1] && canonicalLower[start+1]->properties & AUX_VERB  && !(tokenControl & NO_INFER_QUESTION))   tokenFlags |= QUESTIONMARK;  // what are you doing?  what will you do?
	else if ( posValues[start] & PREPOSITION && canonicalLower[start+1] && canonicalLower[start+1]->properties & QWORD && canonicalLower[start+2] && canonicalLower[start+2]->properties & AUX_VERB  && !(tokenControl & NO_INFER_QUESTION))   tokenFlags |= QUESTIONMARK;  // what are you doing?  what will you do?

#ifdef INFORMATION
	Active tenses:  have + past participle makes things perfect  --		 be + present particicple makes things continuous
		1. present
			a. simple
			b. perfect - I have/has + past participle
			c. continuous - I am + present participle
			d. perfect continuous - I have been + present participle
		2.  past
			a. simple
			b. perfect - I had + past participle
			c. continuous - I was + present participle
			d. perfect continuous - I had been + present participle
		3. future
			a: simple - I will + infinitive or  I am going_to + infinitive
			b. perfect - I will have + past participle 
			c. continuous - I will be + present participle
			d. perfect continuous - I will have been + present participle
	Passive tenses:  be + past participle makes things passive
		1. present
			a. simple - I am  + past participle
			b. perfect - I have/has + been + past participle
			c. continuous - I am + being + past participle
			d. perfect continuous - I have been + being + past participle
		2.  past
			a. simple - I was + past participle
			b. perfect - I had + been + past participle
			c. continuous - I was +  being + past participle
			d. perfect continuous - I had been + being + past participle
		3. future
			a: simple - I will + be + past participle  or  I am going_to + be + past participle
			b. perfect - I will have + been + past participle 
			c. continuous - I will be + past participle
			d. perfect continuous - I will have been + being + past participle
#endif

	if (auxIndex)
	{
		// special future "I am going to swim"
		if (*aux & (VERB_PRESENT|VERB_PRESENT_3PS) && *aux & AUX_BE && auxIndex == 1 && mainverbTense == VERB_PRESENT_PARTICIPLE && !stricmp(wordStarts[mainVerb],(char*)"going") && posValues[mainVerb+1] == TO_INFINITIVE)  tokenFlags |= FUTURE;
		else if (aux[auxIndex-1] & AUX_HAVE && mainverbTense & VERB_PAST_PARTICIPLE) 
		{
			if (*aux & (VERB_PRESENT|VERB_PRESENT_3PS)) 
			{
				tokenFlags |= PRESENT_PERFECT|PAST; 
				if (tokenFlags & PRESENT)  tokenFlags ^= (PRESENT|PERFECT);
			}
			else tokenFlags |= PERFECT; 
		}
		else if (aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PRESENT_PARTICIPLE) tokenFlags |= CONTINUOUS; 

		// compute passive
		if ((aux[auxIndex-1] & AUX_BE || (canonicalLower[auxIndex-1] && !stricmp(canonicalLower[auxIndex-1]->word,(char*)"get"))) && mainverbTense & VERB_PAST_PARTICIPLE) // "he is lost" "he got lost"
		{
			tokenFlags |= PASSIVE;
			if (aux[auxIndex-1] & VERB_PRESENT_PARTICIPLE)  tokenFlags |= CONTINUOUS;	// being xxx
		}
		
		if (*aux & AUX_HAVE && aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PRESENT_PARTICIPLE) 
		{
				tokenFlags |= PERFECT;	// I have/had been xxx
		}
		if (aux[1] & AUX_HAVE && aux[auxIndex-1] & AUX_BE && mainverbTense & VERB_PRESENT_PARTICIPLE) 
		{
				tokenFlags |= PERFECT;	// I will have/had been xxx
		}
		if (*aux & AUX_VERB_FUTURE)  
		{
			tokenFlags |= FUTURE;
			if (mainverbTense & VERB_PRESENT_PARTICIPLE && aux[auxIndex-1] & AUX_BE && aux[auxIndex-1] & VERB_INFINITIVE) tokenFlags |= CONTINUOUS;	// be painting
		}
		else if (*aux & (AUX_VERB_PAST | VERB_PAST))  tokenFlags |= PAST; 
		if (tokenFlags & PERFECT && !(tokenFlags & FUTURE)) 
		{
				tokenFlags |= PAST; // WE CAN NOT LET PRESENT PERFECT STAND, we need it to be in the past! "we have had sex"
		}
		else if (!(tokenFlags & (PAST|FUTURE))) tokenFlags |= PRESENT; 
	}
	else if (defaultTense) tokenFlags |=  defaultTense;
	else if (!(tokenFlags & (PRESENT|PAST|FUTURE))) tokenFlags |= PRESENT; 

	if (!(tokenFlags & IMPLIED_YOU) && (!mainSubject | !mainVerb) ) tokenFlags  |= NOT_SENTENCE;
}

static char* MakePastTense(char* original,WORDP D,bool participle)
{
	if (*original == '-') ++original; // never start on a hyphen
 	static char buffer[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	strcpy(word,original);
	if (*original == '-' || *original == '_') return NULL;	// protection

    //   check for multiword behavoir. Always change the 1st word only
    char* at =  strchr(word,'_'); 
	if (!at) at = strchr(word,'-');
    if (at && at[1])
    {
		int cnt = BurstWord(word,HYPHENS);
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		unsigned int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		int len = 0;
		for (int i = 0; i < cnt; ++i) //   get its components
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			len += lens[i];
			separators[i] = word[len++];
		}
		for (int i = 0; i < cnt; ++i)
		{
			if (D && (D->systemFlags & (VERB_CONJUGATE1|VERB_CONJUGATE2|VERB_CONJUGATE3)))
			{
				if (D->systemFlags & VERB_CONJUGATE1 && i != 0) continue;
				if (D->systemFlags & VERB_CONJUGATE2 && i != 1) continue;
				if (D->systemFlags & VERB_CONJUGATE3 && i != 2) continue;
			}
			// even if not verb, we will interpolate: (char*)"slob_around" doesnt have slob as a verb - faff_about doesnt even have "faff" as a word
			WORDP X = FindWord(words[i]);
			if (!X || !(X->properties & VERB)) continue;
			char* inf;
			inf = (participle) ? (char*) GetPastParticiple(words[i]) : (char*) GetPastTense(words[i]);
			if (!inf) continue;
			*trial = 0;
			char* at1 = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at1,inf);
					at1 += strlen(inf);
					*at1++ = separators[j];
				}
				else
				{
					strcpy(at1,words[j]);
					at1 += lens[j];
					*at1++ = separators[j];
				}
			}
			strcpy(buffer,trial);
			return buffer;
		}
	}

    strcpy(buffer,word);
    size_t len = strlen(buffer);
	D = FindWord(word,len);
    if (buffer[len-1] == 'e') strcat(buffer,(char*)"d");
	// double consonant often, but not for "fixed"
    else if (!IsVowel(buffer[len-1]) && buffer[len-1] != 'y'  && buffer[len-1] != 'x' && buffer[len-1] != 'w' && buffer[len-1] != 'h' && IsVowel(buffer[len-2]) && len > 2 && !IsVowel(buffer[len-3])) 
    {
        char lets[2];
        lets[0] = buffer[len-1];
        lets[1] = 0;
		if (D && D->systemFlags & SPELLING_EXCEPTION) {;} // eg DONT double end in "develop" to developed
        else strcat(buffer,lets); 
        strcat(buffer,(char*)"ed");
    }
    else if (buffer[len-1] == 'y' && !IsVowel(buffer[len-2])) 
    {
        buffer[len-1] = 'i';
        strcat(buffer,(char*)"ed");
    }   
    else if (buffer[len-1] == 'c' ) strcat(buffer,(char*)"ked");
    else strcat(buffer,(char*)"ed");
    return buffer; 
}

char* GetPastTense(char* original)
{
 	if (!original || !*original || *original == '~') return NULL;
	WORDP D = FindWord(original,0,LOWERCASE_LOOKUP);
    if (D && D->properties & VERB && GetTense(D)) //   die is both regular and irregular?
    {
        int n = 9;
        while (--n && D)
        {
            if (D->properties & VERB_PAST) return D->word; //   might be an alternate present tense moved to main
            D = GetTense(D); //   known irregular
        }
    }
	return MakePastTense(original,D,false);
}

char* GetPresent(char* word)
{
	if (!word || !*word || *word == '~') return NULL;
	if (*word == '-') ++word; // never start on a hyphen
    WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
    if (D && D->properties & VERB && GetTense(D) ) 
    {
        int n = 9;
        while (--n && D)
        {
            if (D->properties & VERB_PRESENT) return D->word; //   might be an alternate present tense moved to main
            D = GetTense(D); //   known irregular
        }
   }
   return word; // same as base infinitive otherwise
}

char* GetPastParticiple(char* word)
{
	if (!word || !*word || *word == '~') return NULL;
	if (*word == '-') ++word; // never start on a hyphen
    WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
    if (D && D->properties & VERB && GetTense(D) ) 
    {
        int n = 9;
        while (--n && D)
        {
            if (D->properties & VERB_PAST_PARTICIPLE) return D->word; //   might be an alternate present tense moved to main
            D = GetTense(D); //   known irregular
        }
   }
   return MakePastTense(word,D,true);
}

char* GetPresentParticiple(char* word)
{
	if (!word || !*word || *word == '~') return NULL;
	if (*word == '-') ++word;	// never start on a hyphen  
    static char buffer[MAX_WORD_SIZE];
    WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
    if (D && D->properties & VERB && GetTense(D) ) 
    {//   born (past) -> bore (a past and a present) -> bear 
        int n = 9;
        while (--n && D) //   we have to cycle all the conjugations past and present of to be
        {
            if (D->properties &  VERB_PRESENT_PARTICIPLE) return D->word; 
            D = GetTense(D); //   known irregular
        }
    }
    
    strcpy(buffer,word);
    size_t len = strlen(buffer);
    char* inf = GetInfinitive(word,false);
    if (!inf) return 0;
    if (buffer[len-1] == 'g' && buffer[len-2] == 'n' && buffer[len-3] == 'i' && stricmp(inf,word)) return word;   //   ISNT participle though it has ing ending (unless its base is "ing", like "swing"

    //   check for multiword behavoir. Always change the 1st word only
    char* at =  strchr(word,'_'); 
	if (!at) at = strchr(word,'-');
    if (at)
    {
		int cnt = BurstWord(word,HYPHENS);
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		int len = 0;
		for (int i = 0; i < cnt; ++i) //   get its components
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			len += lens[i];
			separators[i] = word[len++];
		}
		for (int i = 0; i < cnt; ++i)
		{
			if (D && (D->systemFlags & (VERB_CONJUGATE1|VERB_CONJUGATE2|VERB_CONJUGATE3)))
			{
				if (D->systemFlags & VERB_CONJUGATE1 && i != 0) continue;
				if (D->systemFlags & VERB_CONJUGATE2 && i != 1) continue;
				if (D->systemFlags & VERB_CONJUGATE3 && i != 2) continue;
			}
			WORDP E = FindWord(words[i],0,LOWERCASE_LOOKUP);
			if (!E || !(E->properties & VERB)) continue;
			char* inf = GetPresentParticiple(words[i]); //   is this word an infinitive?
			if (!inf) continue;
			*trial = 0;
			char* at = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at,inf);
					at += strlen(inf);
					*at++ = separators[j];
				}
				else
				{
					strcpy(at,words[j]);
					at += lens[j];
					*at++ = separators[j];
				}
			}
			strcpy(buffer,trial);
			return buffer;
		}
	}


    strcpy(buffer,inf); //   get the real infinitive

    if (!stricmp(buffer,(char*)"be"));
    else if (buffer[len-1] == 'h' || buffer[len-1] == 'w' ||  buffer[len-1] == 'x' ||  buffer[len-1] == 'y'); //   no doubling w,x,y h, 
    else if (buffer[len-2] == 'i' && buffer[len-1] == 'e') //   ie goes to ying
    {
        buffer[len-2] = 'y';
        buffer[len-1] = 0;
    }
    else if (buffer[len-1] == 'e' && !IsVowel(buffer[len-2]) ) //   drop ending Ce  unless -nge (to keep the j sound) 
    {
        if (buffer[len-2] == 'g' && buffer[len-3] == 'n');
        else buffer[len-1] = 0; 
    }
    else if (buffer[len-1] == 'c' ) //   add k after final c
    {
        buffer[len-1] = 'k'; 
        buffer[len] = 0;
    }
   //   double consonant 
    else if (!IsVowel(buffer[len-1]) && IsVowel(buffer[len-2]) && (!IsVowel(buffer[len-3]) || (buffer[len-3] == 'u' && buffer[len-4] == 'q'))) //   qu sounds like consonant w
    {
        char* base = GetInfinitive(word,false);
        WORDP D = FindWord(base,0,LOWERCASE_LOOKUP);
        if (D && D->properties & VERB && GetTense(D) ) 
        {
            int n = 9;
            while (--n && D)
            {
                if (D->properties & VERB_PAST) 
                {
                    unsigned int len = D->length;
                    if (D->word[len-1] != 'd' || D->word[len-2] != 'e' || len < 5) break; 
                    if (IsVowel(D->word[len-3])) break; //   we ONLY want to see if a final consonant is doubled. other things would just confuse us
                    strcpy(buffer,D->word);
                    buffer[len-2] = 0;      //   drop ed
                    strcat(buffer,(char*)"ing");   //   add ing
                    return buffer; 
                }
                D = GetTense(D); //   known irregular
            }
            if (!n) ReportBug((char*)"verb loop") //   complain ONLY if we didnt find a past tense
        }

        char lets[2];
        lets[0] = buffer[len-1];
        lets[1] = 0;
		if (D && D->systemFlags & SPELLING_EXCEPTION) {;} // dont double
        else strcat(buffer,lets);    //   double consonant
    }
    strcat(buffer,(char*)"ing");
    return buffer; 
}

uint64 ProbableVerb(char* original, unsigned int len)
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;
	
	char* hyphen = strchr(original,'-');
	if (hyphen)
	{
		WORDP X = FindWord(hyphen+1,0,PRIMARY_CASE_ALLOWED);
		if (X && X->properties & VERB) return X->properties & (VERB_BITS|VERB);
		char* verb = GetInfinitive(hyphen+1,false);
		if (verb) return VERB | verbFormat;
		uint64 flags = ProbableVerb(hyphen+1,len - (hyphen-original+1));
		if (flags) return flags;
	}

	char* item;
	char* test;
	int i;
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = verb5[++i].word)) if (!stricmp(test,item)) return verb5[i].properties;
	}	
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = verb4[++i].word)) if (!stricmp(test,item)) return verb4[i].properties;
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = verb3[++i].word)) if (!stricmp(test,item)) return verb3[i].properties;
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = verb2[++i].word)) if (!stricmp(test,item)) return verb2[i].properties;
	}
	return 0;
}

static char* InferVerb(char* original, unsigned int len)
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;
	uint64 flags = ProbableVerb(original,len);
	if (flags) return StoreWord(word,flags)->word;
	return NULL;
}

char* GetAdjectiveMore(char* word)
{
	if (!word || !*word || *word == '~') return NULL;
	size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len,LOWERCASE_LOOKUP);
	if (!D || !(D->properties & ADJECTIVE)) return NULL; 

    if (D->properties & MORE_FORM)  return D->word; 

    if (GetComparison(D) ) 
    {
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & MORE_FORM) return D->word; 
            D = GetComparison(D); //   known irregular
        }
    }
	return NULL;
}

char* GetAdjectiveMost(char* word)
{
 	if (!word || !*word || *word == '~') return NULL;
	size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len,LOWERCASE_LOOKUP);
	if (!D || !(D->properties & ADJECTIVE)) return NULL; 

    if (D->properties & MOST_FORM)  return D->word; 

    if (GetComparison(D) ) 
    {
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & MOST_FORM) return D->word; 
            D = GetComparison(D); //   known irregular
        }
    }
	return NULL;
}

char* GetAdverbMore(char* word)
{
	if (!word || !*word || *word == '~') return NULL;
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len,LOWERCASE_LOOKUP);
	if (!D || !(D->properties & ADVERB)) return NULL; 

    if ( D->properties & MORE_FORM)  return D->word; 

    if (GetComparison(D) ) 
    {
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & MORE_FORM) return D->word; 
            D = GetComparison(D); //   known irregular
        }
    }
	return NULL;
}

char* GetAdverbMost(char* word)
{
 	if (!word || !*word || *word == '~') return NULL;
    WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	if (!D || !(D->properties & ADVERB)) return NULL; 

    if (D && D->properties & MOST_FORM)  return D->word; 

    if (GetComparison(D)) 
    {
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & MOST_FORM) return D->word; 
            D = GetComparison(D); //   known irregular
        }
    }
	return NULL;
}

char* GetThirdPerson(char* word)
{
 	if (!word || !*word || *word == '~') return NULL;
    size_t len = strlen(word);
    WORDP D = FindWord(word,len,LOWERCASE_LOOKUP);
	if (!D || !(D->properties & VERB)) return NULL; 

    if (D->properties & VERB_PRESENT_3PS)  return D->word; 

    if (GetTense(D) ) 
    {//   born (past) -> bore (a past and a present) -> bear 
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & VERB_PRESENT_3PS) return D->word; 
            D = GetTense(D); //   known irregular
        }
    }

	static char result[MAX_WORD_SIZE];
	strcpy(result,word);
	if (word[len-1] == 'y')
	{
		if (!IsVowel(word[len-2])) strcpy(result+len-1,(char*)"ies");
		else  strcpy(result+len,(char*)"s");
	}
	else if (word[len-1] == 'h' && (word[len-2] == 's' || word[len-2] == 'c'))  strcpy(result+len,(char*)"es");
	else if (word[len-1] == 'o' || word[len-1] == 'x' )  strcpy(result+len,(char*)"es");
	else if (word[len-1] == 's' && word[len-2] == 's' )  strcpy(result+len,(char*)"es");
	else strcat(result,(char*)"s");
	return result;
}

char* GetInfinitive(char* word, bool nonew)
{
	if (!word || !*word || *word == '~') return NULL;
	if (strchr(word,'/')) return NULL;  // not possible verb
 	uint64 controls = tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : LOWERCASE_LOOKUP;
	verbFormat = 0;	//   secondary answer- std infinitive or unknown
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len,controls);
    if (D && D->properties & VERB_INFINITIVE) 
	{
		verbFormat = VERB_INFINITIVE;  // fall  (fell) conflict
		return D->word; //    infinitive value
	}
	WORDP E =  (D) ? GetCanonical(D) : NULL; // "were" has direct canonical
	if (E && E->properties & VERB) 
	{
		if (D->properties & (VERB_PRESENT|VERB_PRESENT_3PS)) verbFormat |= VERB_PRESENT;
		if (D->properties & VERB_PRESENT_PARTICIPLE) verbFormat |= VERB_PRESENT_PARTICIPLE;
		if (D->properties & VERB_PAST) verbFormat |= VERB_PAST;
		if (D->properties & VERB_PAST_PARTICIPLE) verbFormat |= VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE|NOUN_ADJECTIVE;
		// we didnt know the found word as a verb, but its canonical is, so infer what we can (verb will be regular)
		if (!(D->properties & VERB_BITS))
		{
			if (word[len-1] == 's') verbFormat |= VERB_PRESENT_3PS;	// even if not stored on word because word like "eats" is a known noun instead
			if (word[len-1] == 'g') verbFormat |= VERB_PRESENT_PARTICIPLE;	// even if not stored on word because word like "eats" is a known noun instead
			if (word[len-2] == 'e' && word[len-1] == 'd') verbFormat |= VERB_PAST_PARTICIPLE;	// even if not stored on word because word like "eats" is a known noun instead
		}
		return E->word;
	}
		
    if (D && D->properties & VERB && GetTense(D) ) 
    {//   born (past) -> bore (a past and a present) -> bear 
		if (D->properties & VERB_PRESENT_PARTICIPLE) verbFormat = VERB_PRESENT_PARTICIPLE;
		else if (D->properties & (VERB_PAST|VERB_PAST_PARTICIPLE)) 
		{
			verbFormat = 0;
			if (D->properties & VERB_PAST) verbFormat |= VERB_PAST;
			if (D->properties & VERB_PAST_PARTICIPLE) verbFormat |= VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE;
		}
		else if (D->properties & (VERB_PRESENT|VERB_PRESENT_3PS)) verbFormat = VERB_PRESENT;
		unsigned int n = 10;
        while (--n && D) //   scan all the conjugations 
        {
            if (D->properties & VERB_INFINITIVE) 
                return D->word; 
            D = GetTense(D); //   known irregular
        }
    }

	char last = word[len-1];  
    char prior = (len > 2) ? word[len-2] : 0;  //   Xs
    char prior1 = (len > 3) ? word[len-3] : 0; //   Xes (but not nes)
    char prior2 = (len > 4) ? word[len-4] : 0; //   Xhes
	char prior3 = (len > 5) ? word[len-5] : 0; //   Xhes

    //   check for multiword behavior. 
	int cnt = BurstWord(word,HYPHENS);
    if (cnt > 1)
    {
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		unsigned int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		unsigned int len = 0;
		for (int i = 0; i < cnt; ++i) //   get its components  dash-it
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			len += lens[i];
			if ((i+1) == cnt) separators[i] = 0; // no final separator
			else separators[i] = word[len++];
		}
		for (int i = 0; i < cnt; ++i)
		{
			char* inf = GetInfinitive(words[i],false); //   is this word an infinitive?
			if (!inf || !*inf) continue;
			*trial = 0;
			char* at = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at,inf);
					at += strlen(inf);
					*at++ = separators[j];
				}
				else
				{
					strcpy(at,words[j]);
					at += lens[j];
					*at++ = separators[j];
				}
			}
			*at = 0;
			WORDP E = FindWord(trial,0,controls);
			if (E && E->properties & VERB_INFINITIVE) return E->word;
		}

       return NULL;  //   not a verb
    }

    //   not known verb, try to get present tense from it
    if (last == 'd' && prior == 'e' && len > 3)   //   ed ending?
    {
		verbFormat = VERB_PAST|VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE;

		// if vowel-vowel-consonant e d, prefer that
		if (len > 4 && !IsVowel(prior1) && IsVowel(prior2) && IsVowel(prior3))
		{
			D = FindWord(word,len-2,controls);    //   drop ed
 			if (D && D->properties & VERB_INFINITIVE) return D->word;
		}
		if (word[len-3] == 'k')
		{
			D = FindWord(word,len-3,controls);	//   drop ked, on scare
			if (D && D->properties & VERB_INFINITIVE) return D->word;
		}
		D = FindWord(word,len-1,controls);	//   drop d, on scare
		if (D && D->properties & VERB_INFINITIVE) return D->word;
        D = FindWord(word,len-2,controls);    //   drop ed
        if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        D = FindWord(word,len-1,controls);    //   drop d
        if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        if (prior1 == prior2  )   //   repeated consonant at end
        {
            D = FindWord(word,len-3,controls);    //   drop Xed
            if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        }
        if (prior1 == 'i') //   ied came from y
        {
            word[len-3] = 'y'; //   change i to y
            D = FindWord(word,len-2,controls);    //   y, drop ed
            word[len-3] = 'i';
            if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        }

		if (!xbuildDictionary && !nonew && !fullDictionary)
		{
			char wd[MAX_WORD_SIZE];
			strcpy(wd,word);
			if (len > 4 && !IsVowel(prior1) && IsVowel(prior2) && IsVowel(prior3))
			{
				wd[len-2] = 0;
			}
			else if (prior1 == prior2) // double last and add ed
			{
				wd[len-3] = 0;
			}
			else if (!IsVowel(prior2) && prior1 == 'i') // ied => y copied->copy
			{
				strcpy(wd+len-3,(char*)"y");
			}
			else if (!IsVowel(prior1) && IsVowel(prior2)) // Noted -> note 
			{
				wd[len-1] = 0;	// just chop off the s, leaving the e
			}
			else wd[len-2] = 0; // chop ed off
			return StoreWord(wd,VERB|VERB_PAST|VERB_PAST_PARTICIPLE|ADJECTIVE_PARTICIPLE|ADJECTIVE)->word;
		}
     }
   
    //   could this be a participle verb we dont know about?
    if (prior1 == 'i' && prior == 'n' && last == 'g' && len > 4)//   maybe verb participle
    {
        char word1[MAX_WORD_SIZE];
		verbFormat = VERB_PRESENT_PARTICIPLE;
 
        //   try removing doubled consonant
        if (len > 4 &&  word[len-4] == word[len-5])
        {
            D = FindWord(word,len-4,controls);    //   drop Xing spot consonant repeated
            if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        }

        //   y at end, maybe came from ie
        if (word[len-4] == 'y')
        {
            strcpy(word1,word);
            word1[len-4] = 'i';
            word1[len-3] = 'e';
            word1[len-2] = 0;
            D = FindWord(word1,len-2,controls);    //   drop ing but add ie
            if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        }

        //   two consonants at end, see if raw word is good priority over e added form
        if (len > 4 && !IsVowel(word[len-4]) && !IsVowel(word[len-5])) 
        {
            D = FindWord(word,len-3,controls);    //   drop ing
            if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        }

        //   otherwise try stem with e after it assuming it got dropped
        strcpy(word1,word);
        word1[len-3] = 'e';
        word1[len-2] = 0;
        D = FindWord(word1,len-2,controls);    //   drop ing and put back 'e'
        if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it

        //   simple ing added to word
        D = FindWord(word,len-3,controls);    //   drop ing
        if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it

		if (!xbuildDictionary && !nonew)
		{
			char wd[MAX_WORD_SIZE];
			strcpy(wd,word);
			if (prior3 == prior2) wd[len-4] = 0; // double last and add ing like swimming => swim
			else wd[len-3] = 0; // chop ing off
			return StoreWord(wd,VERB|VERB_PRESENT_PARTICIPLE|NOUN_GERUND|NOUN)->word;
		}
	}
    //   ies from y  3rd person present
    if (prior1 == 'i' && prior == 'e' && last == 's' && len > 4 && !IsVowel(word[len-4]))//   maybe verb participle
    {
 		verbFormat = VERB_PRESENT_3PS;
        char word1[MAX_WORD_SIZE];
        strcpy(word1,word);
        word1[len-3] = 'y';
        word1[len-2] = 0;
        D = FindWord(word1,len-2,controls);    //   drop ing, add e
        if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
	}

     //   unknown singular verb  3rd person present
    if (last == 's' && len > 3 && prior != 'u' && prior != '\'') // but should not be "us" ending (adjectives)
    {
 		verbFormat = VERB_PRESENT_3PS;
		bool es = false;
		if (prior == 'e' && word[len-3] == 'h' && (word[len-4] == 's' || word[len-4] == 'c'))  es = true;
		else if (prior == 'e' && (word[len-3] == 'o' || word[len-3] == 'x' ))  es = true;
		else if (prior == 'e' && word[len-3] == 's' && word[len-4] == 's' )  es = true;
		if (es)
        {
            D = FindWord(word,len-2,controls);    //   drop es
            if (D && D->properties & VERB_INFINITIVE) return D->word; //   found it
        }

        D = FindWord(word,len-1,controls);    //   drop s
        if (D && D->properties & VERB && D->properties & VERB_INFINITIVE) return D->word; //   found it
		if (D && D->properties & NOUN) return NULL; //   dont move bees to be

		D = FindWord(word,len-1,UPPERCASE_LOOKUP); // if word exists in upper case, this is a plural and NOT a verb with s
		if (D) return NULL;

		if (!xbuildDictionary && !nonew && prior != 's') // not consciousness
		{
			char wd[MAX_WORD_SIZE];
			strcpy(wd,word);
			if ( prior == 'e' && prior1 == 'i') // was toadies  from toady
			{
				strcpy(wd+len-3,(char*)"y");
			}
			else wd[len-1] = 0; // chop off tail s
			return StoreWord(wd,VERB_PRESENT_3PS|VERB|NOUN|NOUN_SINGULAR)->word;
		}
   }

    if (IsHelper(word)) 
	{
		verbFormat = 0;
		return word;
	}
	if ( nonew || xbuildDictionary ) return NULL;
	verbFormat = VERB_INFINITIVE;
	return InferVerb(word,len);
}

char* GetPluralNoun(WORDP noun)
{
	if (!noun) return NULL;
    if (noun->properties & NOUN_PLURAL) return noun->word; 
    WORDP plural = GetPlural(noun);
	if (noun->properties & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR)) 
    {
        if (plural) return plural->word;
        static char word[MAX_WORD_SIZE];
		unsigned int len = noun->length;
		char end = noun->word[len-1];
		char before = (len > 1) ? (noun->word[len-2]) : 0;
		if (end == 's') sprintf(word,(char*)"%ses",noun->word); // glass -> glasses
		else if (end == 'h' && (before == 'c' || before == 's')) sprintf(word,(char*)"%ses",noun->word); // witch -> witches
		else if ( end == 'o' && !IsVowel(before)) sprintf(word,(char*)"%ses",noun->word); // hero -> heroes>
		else if ( end == 'y' && !IsVowel(before)) // cherry -> cherries
		{
			if (noun->internalBits & UPPERCASE_HASH) sprintf(word,(char*)"%ss",noun->word); // Germany->Germanys
			else
			{
				strncpy(word,noun->word,len-1);
				strcpy(word+len-1,(char*)"ies"); 
			}
		}
		else sprintf(word,(char*)"%ss",noun->word);
        return word;
    }
    return noun->word;
}

static char* InferNoun(char* original,unsigned int len) // from suffix might it be singular noun? If so, enter into dictionary
{
	if (len == 0) len = strlen(original);
	char word[MAX_WORD_SIZE];
	strncpy(word,original,len);
	word[len] = 0;
	uint64 flags = ProbableNoun(original,len);

	// ings (plural of a gerund like paintings)
	if (len > 6 && !stricmp(word+len-4,(char*)"ings"))
	{
		StoreWord(word,NOUN|NOUN_PLURAL);
		word[len-1] = 0;
		return StoreWord(word,NOUN|NOUN_SINGULAR)->word; // return the singular form
	}
		
	// ves (plural form)
	if (len > 4 && !strcmp(word+len-3,(char*)"ves") && IsVowel(word[len-4])) // knife
	{
		//Plurals of words that end in -f or -fe usually change the f sound to a v sound and add s or -es.
		word[len-3] = 'f';
		char* singular = GetSingularNoun(word,false,false);
		if (singular && !stricmp(singular,word)) 
		{
			nounFormat = NOUN_PLURAL;
			return StoreWord(singular,NOUN|NOUN_SINGULAR)->word;
		}
		word[len-2] = 'e';
		singular = GetSingularNoun(word,false,true);
		if (singular && !stricmp(singular,word)) 
		{
			nounFormat = NOUN_PLURAL;
			return StoreWord(singular,NOUN|NOUN_SINGULAR)->word;
		}
	}

	if (flags) return StoreWord(word,flags)->word;

	if (strchr(word,'_')) return NULL;		// dont apply suffix to multiple word stuff

	WORDP X = FindWord(word,0,LOWERCASE_LOOKUP);
	if (IsUpperCase(*word) && X) return NULL; // we dont believe it

	if (IsUpperCase(*word)) return StoreWord(word,NOUN|NOUN_PROPER_SINGULAR)->word;

	// if word is an abbreviation it is a noun (proper if uppcase)
	if (strchr(word,'.')) return StoreWord(word,NOUN|NOUN_SINGULAR)->word;

	// hypenated word check word at end
	char* hypen = strchr(word+1,'-');
	if ( hypen && len > 2)
	{
		char* stem = GetSingularNoun(word+1,true,false);
		if (stem && !stricmp(stem,word+1)) return StoreWord(word,NOUN|NOUN_SINGULAR)->word;
		if (stem)
		{
			strcpy(word+1,stem);
			return StoreWord(word,NOUN|NOUN_SINGULAR)->word;
		}
	}
	return NULL;
}

uint64 ProbableNoun(char* original,unsigned int len) // from suffix might it be singular noun? 
{
	if (len == 0) len = strlen(original);
	char word[MAX_WORD_SIZE];
	strncpy(word,original,len);
	word[len] = 0;

	char* hyphen = strchr(original,'-');
	if (hyphen)
	{
		WORDP X = FindWord(hyphen+1,0,PRIMARY_CASE_ALLOWED);
		if (X && X->properties & NOUN) return X->properties & (NOUN_BITS|NOUN);
		uint64 flags = ProbableNoun(hyphen+1,len - (hyphen-original+1));
		if (flags) return flags;
	}
	
	char* item;
	char* test;
	int i;
	if (len >= 10) // word of 3 + suffix of 7
	{
		test = word+len-7;
		i = -1;
		while ((item = noun7[++i].word)) if (!stricmp(test,item)) return noun7[i].properties;
	}	
	if (len >= 9) // word of 3 + suffix of 6
	{
		test = word+len-6;
		i = -1;
		while ((item = noun6[++i].word)) if (!stricmp(test,item)) return noun6[i].properties;
	}	
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = noun5[++i].word)) if (!stricmp(test,item)) return noun5[i].properties;
	}	
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = noun4[++i].word)) if (!stricmp(test,item)) return noun4[i].properties;
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = noun3[++i].word)) if (!stricmp(test,item)) return noun3[i].properties;
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = noun2[++i].word)) if (!stricmp(test,item)) return noun2[i].properties;
	}

	// ings (plural of a gerund like paintings)
	if (len > 6 && !stricmp(word+len-4,(char*)"ings")) return NOUN|NOUN_SINGULAR;
		
	// ves (plural form)
	if (len > 4 && !strcmp(word+len-3,(char*)"ves") && IsVowel(word[len-4])) return NOUN|NOUN_PLURAL; // knife

	return 0;
}

char* GetSingularNoun(char* word, bool initial, bool nonew)
{ 
 	if (!word || !*word || *word == '~') return NULL;
	uint64 controls = PRIMARY_CASE_ALLOWED;
	nounFormat = 0;
    size_t len = strlen(word);
    WORDP D = FindWord(word,0,controls);
	nounFormat = NOUN_SINGULAR;
	if (D && D->properties & NOUN_PROPER_SINGULAR) //   is already singular
	{
		nounFormat = NOUN_PROPER_SINGULAR;
		return D->word;
	}

    //   we know the noun and its plural, use singular
    if (D && D->properties & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR|NOUN_NUMBER)) //   is already singular
    {
		nounFormat = D->properties & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR|NOUN_NUMBER);
		if (word[len-1] == 's') // even if singular, if simpler exists, use that. Eg.,  military "arms"  vs "arm" the part
		{
			if (nonew)
			{
				WORDP E = FindWord(word,len-1,controls);
				if  (E && E->properties & NOUN) return E->word;
			}
			char* sing = InferNoun(word,len);
			if (sing) return sing;
		}

        //   HOWEVER, some singulars are plurals--- words has its own meaning as well
        unsigned int len = D->length;
        if (len > 4 && D->word[len-1] == 's')
        {
            WORDP F = FindWord(D->word,len-1,controls);
            if (F && F->properties & NOUN)  return F->word;  
        }
        return D->word; 
    }
	WORDP plural = (D) ? GetPlural(D) : NULL;
    if (D  && D->properties & NOUN_PLURAL && plural) 
	{
		nounFormat = NOUN_PLURAL;
		return plural->word; //   get singular form from plural noun
	}

	if (D && D->properties & NOUN && !(D->properties & NOUN_PLURAL) && !(D->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL))) return D->word; //   unmarked as plural, it must be singular unless its a name
    if (!D && IsNumber(word, numberStyle) != NOT_A_NUMBER)  return word;
	if (D && D->properties & AUX_VERB) return NULL; // avoid "is" or "was" as plural noun

	// check known from plural s or es
	if (len > 2 && word[len-1] == 's' && word[len-2] != 's')
	{
		static char mod[MAX_WORD_SIZE]; // we return this sometimes
		char hold[MAX_WORD_SIZE];
		strcpy(hold,word); // overlap?
		strcpy(mod,hold);
		mod[len-1] = 0;
		char* singular = GetSingularNoun(mod,false,true);
		nounFormat = (IsUpperCase(*word)) ? NOUN_PROPER_PLURAL : NOUN_PLURAL; // would fail on iPhones BUG
		uint64 format = nounFormat; // would fail on iPhones BUG
		if (singular) return singular; // take off s is correct
		if (len > 3 && word[len-2] == 'e' && (word[len-3] == 'x' || word[len-3] == 's' || word[len-3] == 'h' || IsVowel(word[len-3]) )) // es is rarely addable (not from james) ,  tornado  and fish minus fox  allow it
		{
			mod[len-2] = 0;
			singular = GetSingularNoun(mod,false,true);
			nounFormat = format;
			if (singular) return singular; // take off es is correct
			//With words that end in a consonant and a y, you'll need to change the y to an i and add es
			if (len > 4 && mod[len-3] == 'i' && !IsVowel(mod[len-4]))
			{
				mod[len-3] = 'y';
				singular = GetSingularNoun(mod,false,true);
				nounFormat = format;
				if (singular) return singular; // take off ies is correct, change to y
			}

			//Plurals of words that end in -f or -fe usually change the f sound to a v sound and add s or -es.
			if (len > 4 && mod[len-3] == 'v')
			{
				mod[len-3] = 'f';
				singular = GetSingularNoun(mod,false,true);
				nounFormat = format;
				if (singular) return singular; // take off ves is correct, change to f
				mod[len-2] = 'e';
				singular = GetSingularNoun(mod,false,true);
				nounFormat = format;
				if (singular) return singular; // take off ves is correct, change to fe
			}
		}
	}
	if ( nonew || xbuildDictionary ) return NULL;

	nounFormat = (IsUpperCase(*word)) ? NOUN_PROPER_SINGULAR : NOUN_SINGULAR;
	return InferNoun(word,len);
}


static char* InferAdverb(char* original, unsigned int len) // might it be adverb based on suffix? if so, enter into dictionary
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;
	uint64 expectedBase = 0;
	uint64 flags = ProbableAdverb(original,len,expectedBase);
	
	//est
	if (len > 4 && !strcmp(word+len-3,(char*)"est"))
	{
		adjectiveFormat = MOST_FORM;
		WORDP E = StoreWord(word,ADVERB|MOST_FORM);
		WORDP base = E;
		if (word[len-3] == word[len-4])  // doubled consonant
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-2] = 0;
			base =  StoreWord(word,ADVERB,0);
			//SetComparison(E,MakeMeaning(base));
			//SetComparison(base,MakeMeaning(E));
		}
		return base->word;
	}
	//er
	if (len > 4 && !strcmp(word+len-2,(char*)"er")  )
	{
		adjectiveFormat = MORE_FORM;
		WORDP E = StoreWord(word,ADVERB|MORE_FORM);
		WORDP base = E;
		if (word[len-3] == word[len-4]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-2] = 0;
			base =  StoreWord(word,ADVERB,0);
			//SetComparison(E,MakeMeaning(base));
			//SetComparison(base,MakeMeaning(E));
		}
		return base->word;
	}
	if (flags) 
	{
		WORDP E = StoreWord(word,flags,PROBABLE_ADVERB|expectedBase);
		return E->word;
	}

	return NULL;
}

uint64 ProbableAdverb(char* original, unsigned int len,uint64& expectedBase) // might it be adverb based on suffix? 
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;
	
	char* hyphen = strchr(original,'-');
	if (hyphen)
	{
		WORDP X = FindWord(hyphen+1,0,PRIMARY_CASE_ALLOWED);
		if (X && X->properties & ADVERB) return X->properties & (ADVERB);
		uint64 flags = ProbableVerb(hyphen+1,len - (hyphen-original+1));
		if (flags) return flags;
	}


	char* item;
	char* test;
	int i;
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = adverb5[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-4);
				if (X && X->properties & adverb5[i].baseflags) expectedBase = ADVERB;
				return adverb5[i].properties;
			}
		}
	}
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = adverb4[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-4);
				if (X && X->properties & adverb4[i].baseflags) expectedBase = ADVERB;
				return adverb4[i].properties;
			}
		}
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = adverb3[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-3);
				if (X && X->properties & adverb3[i].baseflags) expectedBase = ADVERB;
				return adverb3[i].properties;
			}
		}
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = adverb2[++i].word))
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-2);
				if (X && X->properties & adverb2[i].baseflags) expectedBase = ADVERB;
				return adverb2[i].properties;
			}
		}
	}

	//est
	if (len > 4 && !strcmp(word+len-3,(char*)"est")) 
	{
		WORDP X = FindWord(word,len-3);
		if (X && X->properties & ADVERB) expectedBase = ADVERB;
		return ADVERB;
	}
	//er
	if (len > 4 && !strcmp(word+len-2,(char*)"er")  ) 
	{
		WORDP X = FindWord(word,len-2);
		if (X && X->properties & ADVERB) expectedBase = ADVERB;
		return ADVERB;
	}
	return 0;
}

char* GetAdverbBase(char* word, bool nonew)
{
 	if (!word || !*word || *word == '~') return NULL;
	uint64 controls = tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : LOWERCASE_LOOKUP;
 	adverbFormat = 0;
	if (IsUpperCase(*word)) return NULL; // not as proper
    size_t len = strlen(word);
    if (len == 0) return NULL;
    char lastc = word[len-1];  
    char priorc = (len > 2) ? word[len-2] : 0; 
    char prior2c = (len > 3) ? word[len-3] : 0; 
    WORDP D = FindWord(word,0,controls);
	adverbFormat = 0;
    if (D && D->properties &  QWORD) return D->word; //   we know it as is
	if (D && D->properties & ADVERB && !(D->properties & (MORE_FORM|MOST_FORM) )) return D->word; //   we know it as is

	if (D && D->properties & ADVERB)
    {
        int n = 5;
		WORDP original = D;
        while (--n  && D)
        {
            D = GetComparison(D);
            if (D && !(D->properties & (MORE_FORM|MOST_FORM))) 
			{
				if (original->properties & MORE_FORM) adjectiveFormat = MOST_FORM;
				else if (original->properties & MOST_FORM) adjectiveFormat = MOST_FORM;
				return D->word;
			}
        }
    }
 
    if (len > 4 && priorc == 'l' && lastc == 'y')
    {
		char form[MAX_WORD_SIZE];
        D = FindWord(word,len-2,controls); // rapidly
        if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
		if (prior2c == 'i')
		{
			D = FindWord(word,len-3,controls); // lustily
			if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
			// if y changed to i, change it back
			strcpy(form,word);
			form[len-3] = 'y';
			form[len-2] = 0;
			D = FindWord(word,len-2,controls); // happily  from happy
			if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
		}
		// try terrible -> terribly
		strcpy(form,word);
		form[len-1] = 'e';
 		D = FindWord(word,len-2,controls); // happily  from happy
		if (D && D->properties & (VERB|ADJECTIVE)) return D->word;
    }
	if (len >= 5 && priorc == 'e' && lastc == 'r')
    {
        D = FindWord(word,len-2,controls);
        if (D && D->properties & ADVERB) 
		{
			adverbFormat = MORE_FORM;
			return D->word;
		}
    }
	if (len >= 5 && priorc == 'e' && lastc == 'r')
	{
		D = FindWord(word, len - 1, controls);
		if (D && D->properties & ADVERB)
		{
			adverbFormat = MORE_FORM;
			return D->word;
		}
	}

	if (len > 5 && prior2c == 'e' && priorc == 's' && lastc == 't')
    {
        D = FindWord(word,len-3,controls);
        if (D && D->properties & ADVERB) 
		{
			adverbFormat = MOST_FORM;
			return D->word;
		}
    }
	if (len > 5 && prior2c == 'e' && priorc == 's' && lastc == 't')
	{
		D = FindWord(word, len - 2, controls);
		if (D && D->properties & ADVERB)
		{
			adverbFormat = MOST_FORM;
			return D->word;
		}
	}

	if ( nonew || xbuildDictionary) return NULL;
	
	return InferAdverb(word,len);
}


static char* InferAdjective(char* original, unsigned int len) // might it be adjective based on suffix? If so, enter into dictionary
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;

	// est -  comparative
	if (len > 4 &&  !strcmp(word+len-3,(char*)"est")  )
	{
		adjectiveFormat = MOST_FORM;
		WORDP E = StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL|MOST_FORM);
		WORDP X = FindWord(word,len-2);
		if (X && X->properties & ADJECTIVE_NORMAL) AddSystemFlag(E,PROBABLE_ADJECTIVE);
		WORDP base = E;
		if (word[len-4] == word[len-5]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-3] = 0;
			base =  StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL,0);
			//SetComparison(E,MakeMeaning(base));
			//SetComparison(base,MakeMeaning(E));
		}
		
		return base->word;
	}

	// er -  comparative
	if (len >= 4 &&  !strcmp(word+len-2,(char*)"er")  )
	{
		adjectiveFormat = MORE_FORM;
		WORDP E = StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL|MORE_FORM);
		WORDP X = FindWord(word,len-2);
		if (X && X->properties & ADJECTIVE_NORMAL) AddSystemFlag(E,PROBABLE_ADJECTIVE);
		WORDP base = E;
		if (word[len-3] == word[len-4]) 
		{
			char word1[MAX_WORD_SIZE];
			strcpy(word1,word);
			word1[len-2] = 0;
			base =  StoreWord(word,ADJECTIVE|ADJECTIVE_NORMAL,0);
			//SetComparison(E,MakeMeaning(base));
			//SetComparison(base,MakeMeaning(E));
		}
		return base->word;
	}
	uint64 expectedBase = 0;
	uint64 flags = ProbableAdjective(original,len,expectedBase);
	if (flags) 
	{
		WORDP E = StoreWord(word,flags);
		if (expectedBase) AddSystemFlag(E,expectedBase);
		return E->word;
	}

	return 0;
}

uint64 ProbableAdjective(char* original, unsigned int len,uint64 &expectedBase) // probable adjective based on suffix?
{
	char word[MAX_WORD_SIZE];
	if (len == 0) len = strlen(original);
	strncpy(word,original,len);
	word[len] = 0;
		
	char* hyphen = strchr(original,'-');
	if (hyphen)
	{
		WORDP X = FindWord(hyphen+1,0,PRIMARY_CASE_ALLOWED);
		if (X && X->properties & ADJECTIVE) return X->properties & (ADJECTIVE_BITS|ADJECTIVE);
		uint64 flags = ProbableVerb(hyphen+1,len - (hyphen-original+1));
		if (flags) return flags;

		if (!strcmp(hyphen,(char*)"-looking" )) return ADJECTIVE|ADJECTIVE_NORMAL; // good-looking gross-looking
		if (!strcmp(hyphen,(char*)"-old" )) return ADJECTIVE|ADJECTIVE_NORMAL; // centuries-old
	}

	int i;
	char* test;
	char* item;
	if (len >= 10) // word of 3 + suffix of 7
	{
		test = word+len-7;
		i = -1;
		while ((item = adjective7[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-7);
				if (X && X->properties & adjective7[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective7[i].properties;
			}
		}
	}
	if (len >= 9) // word of 3 + suffix of 6
	{
		test = word+len-6;
		i = -1;
		while ((item = adjective6[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-6);
				if (X && X->properties & adjective6[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective6[i].properties;
			}
		}
	}
	if (len >= 8) // word of 3 + suffix of 5
	{
		test = word+len-5;
		i = -1;
		while ((item = adjective5[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-5);
				if (X && X->properties & adjective5[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective5[i].properties;
			}
		}
	}
	if (len >= 7) // word of 3 + suffix of 4
	{
		test = word+len-4;
		i = -1;
		while ((item = adjective4[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-4);
				if (X && X->properties & adjective4[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective4[i].properties;
			}
		}
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		test = word+len-3;
		i = -1;
		while ((item = adjective3[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-3);
				if (X && X->properties & adjective3[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective3[i].properties;
			}
		}
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		test = word+len-2;
		i = -1;
		while ((item = adjective2[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-2);
				if (X && X->properties & adjective2[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective2[i].properties;
			}
		}
	}
	if (len >= 4) // word of 3 + suffix of 1
	{
		test = word+len-1;
		i = -1;
		while ((item = adjective1[++i].word)) 
		{
			if (!stricmp(test,item)) 
			{
				WORDP X = FindWord(word,len-1);
				if (X && X->properties & adjective1[i].baseflags) expectedBase = PROBABLE_ADJECTIVE;
				return adjective1[i].properties;
			}
		}
	}

	// est -  comparative -- hard est
	if (len >= 4 &&  !strcmp(word+len-3,(char*)"est")  ) 
	{
		WORDP X = FindWord(word,len-3);
		if (X && X->properties & ADJECTIVE_NORMAL) expectedBase = PROBABLE_ADJECTIVE;
		return ADJECTIVE|ADJECTIVE_NORMAL|MOST_FORM;
	}
	// est -  comparative -- strange st
	if (len >= 4 && !strcmp(word + len - 3, (char*)"est"))
	{
		WORDP X = FindWord(word, len - 2); // starnge
		if (X && X->properties & ADJECTIVE_NORMAL) expectedBase = PROBABLE_ADJECTIVE;
		return ADJECTIVE | ADJECTIVE_NORMAL | MOST_FORM;
	}

	// er -  comparative -- harder
	if (len >= 4 &&  !strcmp(word+len-2,(char*)"er")  ) 
	{
		WORDP X = FindWord(word,len-2);
		if (X && X->properties & ADJECTIVE_NORMAL) expectedBase = ADJECTIVE;
		return ADJECTIVE|ADJECTIVE_NORMAL|MORE_FORM;
	}
	// er -  comparative -- stranger
	if (len >= 4 && !strcmp(word + len - 2, (char*)"er"))
	{
		WORDP X = FindWord(word, len - 1);
		if (X && X->properties & ADJECTIVE_NORMAL) expectedBase = ADJECTIVE;
		return ADJECTIVE | ADJECTIVE_NORMAL | MORE_FORM;
	}

	return 0;
}


char* GetAdjectiveBase(char* word, bool nonew)
{
 	if (!word || !*word || *word == '~') return NULL;
	uint64 controls = tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : LOWERCASE_LOOKUP;
	adjectiveFormat = 0;
    size_t len = strlen(word);
    WORDP D = FindWord(word,0,controls);
	char lastc = word[len-1];  
    char priorc = (len > 2) ? word[len-2] : 0;  //   Xs
    char priorc1 = (len > 3) ? word[len-3] : 0; //   Xes
    char priorc2 = (len > 4) ? word[len-4] : 0; //   Xhes
    char priorc3 = (len > 5) ? word[len-5] : 0; //   Xgest
 
    if (D && D->properties & ADJECTIVE && !(D->properties & (MORE_FORM|MOST_FORM)))
	{
		adjectiveFormat = 0;
		return D->word; //   already base
	}
    if (D && D->properties & ADJECTIVE)
    {
        int n = 5;
		WORDP original = D;
        while (--n  && D)
        {
            D = GetComparison(D);
            if (D && !(D->properties & (MORE_FORM|MOST_FORM))) // this is the base
			{
				if (original->properties & MORE_FORM) adjectiveFormat = MORE_FORM;
				else if (original->properties & MOST_FORM) adjectiveFormat = MOST_FORM;
				return D->word;
			}
        }
    }
 
    //   see if composite
    char composite[MAX_WORD_SIZE];
    strcpy(composite,word);
    char* hyphen = strchr(composite+1,'-');
    if (hyphen && (hyphen-4) > composite)
    {
        hyphen -= 4;
		char* althyphen = (hyphen - composite) + word;
        if (hyphen[2] == 'e' && hyphen[3] == 'r') //   lower-density?
        {
            strcpy(hyphen+2,althyphen+4); //   remove er
            char* answer = GetAdjectiveBase(composite,false);
            if (answer) return answer;
        }
        if (hyphen[1] == 'e' && hyphen[2] == 's' && hyphen[3] == 't' ) //   lowest-density?
        {
            strcpy(hyphen+1,althyphen+4); //   remove est
            char* answer = GetAdjectiveBase(composite,false);
            if (answer) return answer;
        }
    }

    if (len > 4 && priorc == 'e' && lastc == 'r') //   more
    {
		 adjectiveFormat = MORE_FORM;
         D = FindWord(word,len-2,controls);
         if (D && D->properties & ADJECTIVE) return D->word; //   drop er
         D = FindWord(word,len-1,controls);
         if (D && D->properties & ADJECTIVE) return D->word; //   drop e (already ended in e)

         if (priorc1 == priorc2  )  
         {
            D = FindWord(word,len-3,controls);
            if (D && D->properties & ADJECTIVE) return D->word; //   drop Xer
         }
         if (priorc1 == 'i') //   changed y to ier?
         {
            word[len-3] = 'y';
            D = FindWord(word,len-2,controls);
            word[len-3] = 'i';
            if (D && D->properties & ADJECTIVE) return D->word; //   drop Xer
          }
	}  
	else if (len > 5 && priorc1 == 'e' &&  priorc == 's' && lastc == 't') //   most
    {
		adjectiveFormat = MOST_FORM;
        D = FindWord(word,len-3,controls);//   drop est
        if (D && D->properties & ADJECTIVE) return D->word; 
        D = FindWord(word,len-2,controls);//   drop st (already ended in e)
        if (D && D->properties & ADJECTIVE) return D->word; 
        if (priorc2 == priorc3  )   
        {
             D = FindWord(word,len-4,controls);//   drop Xest
             if (D && D->properties & ADJECTIVE) return D->word; 
        }
        if (priorc2 == 'i') //   changed y to iest
        {
             word[len-4] = 'y';
             D = FindWord(word,len-3,controls); //   drop est
             word[len-4] = 'i';
             if (D && D->properties & ADJECTIVE) return D->word; 
        }   
    }
	if ( nonew || xbuildDictionary) return NULL;
	
	return InferAdjective(word,len);
}
