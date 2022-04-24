
#include "common.h"

uint64 posTiming;
bool parseLimited = false;

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
		{ (char*)"ly",ADJECTIVE | ADJECTIVE_NORMAL,0},  // pertaining to
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

WORDP wordForms[50];
int nWordForms;
WORDP negateWordForms[50];
int nNegateWordForms;

static void ShowWord(char* word, bool negate = false);
static void ShowWord(char* word, bool negate)
{
	WORDP D = FindWord(word);
	if (!D || D->inferMark == inferMark || !(D->properties & BASIC_POS)) return;
	D->inferMark = inferMark;
	if (negate) 	negateWordForms[nNegateWordForms++] = D;
	else wordForms[nWordForms++] = D;
	if (!negate)
	{
		// negation prefixes
		char other[MAX_WORD_SIZE];
		sprintf(other, "dis%s", word);
		ShowWord(other,true);
		sprintf(other, "un%s", word);
		ShowWord(other, true);
		sprintf(other, "non%s", word);
		ShowWord(other, true);
		sprintf(other, "in%s", word);
		ShowWord(other, true);
		sprintf(other, "im%s", word);
		ShowWord(other, true);
		sprintf(other, "ir%s", word);
		ShowWord(other, true);
		sprintf(other, "il%s", word);
		ShowWord(other, true);
		sprintf(other, "a%s", word);
		ShowWord(other, true);
		// gains
		sprintf(other, "hyper%s", word);
		ShowWord(other, true);
		sprintf(other, "extra%s", word);
		ShowWord(other, true);
	}
}

void ShowForm(char* word)
{
	char other[MAX_WORD_SIZE];
	int i = -1;
	char* item;
	while ((item = adjective2[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = adjective3[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = adjective4[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = adjective5[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}  
	i = -1;
	while ((item = adjective6[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}  
	i = -1;
	while ((item = adjective7[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}  
	i = -1;
	while ((item = adverb2[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = adverb3[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = adverb4[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = adverb5[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = verb2[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = verb3[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = verb4[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = verb5[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = noun2[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = noun3[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = noun4[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = noun5[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = noun6[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	i = -1;
	while ((item = noun7[++i].word))
	{
		sprintf(other, "%s%s", word, item);
		ShowWord(other);
	}
	
}

static int64 ProcessNumber(int atloc, char* original, WORDP& revise, WORDP &entry, WORDP &canonical, uint64& sysflags, uint64 &cansysflags, bool firstTry, bool nogenerate, int start, int kind) // case sensitive, may add word to dictionary, will not augment flags of existing words
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
	if ((IsDigit(*original) || IsDigit(original[1]) || *original == '\'') && (kind || hyphen || strchr(original,':')))
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
			sysflags = entry->systemFlags | TIMEWORD| NOUN_NODETERMINER;
			cansysflags = canonical->systemFlags | TIMEWORD;
			return properties;
		}

		// handle time like 4:30
		if (len > 3 && len < 6 && IsDigit(original[len - 1]) && (original[1] == ':' || original[2] == ':')) // 18:32
		{
			properties = NOUN | NOUN_NUMBER | ADJECTIVE | ADJECTIVE_NUMBER;
			entry = canonical = StoreWord(original, properties); // 18:32
			sysflags = entry->systemFlags | TIMEWORD| NOUN_NODETERMINER;
			cansysflags = canonical->systemFlags | TIMEWORD| NOUN_NODETERMINER;
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
			entry = canonical = StoreWord(original, properties, NOUN_NODETERMINER);
			return properties;
		}

		// handle date range like 1920-22 or 1920-1955  or any number range
		if (hyphen && hyphen != original)
		{
			at = original - 1;
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
					sysflags = entry->systemFlags | TIMEWORD| NOUN_NODETERMINER;
					cansysflags = canonical->systemFlags | TIMEWORD| NOUN_NODETERMINER;
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
				if (!entry) entry = StoreWord(original, properties, NOUN_NODETERMINER);
				canonical = FindWord(number, 0, PRIMARY_CASE_ALLOWED);
				if (canonical) properties |= canonical->properties;
				else canonical = StoreWord(number, properties);
				sysflags = entry->systemFlags| NOUN_NODETERMINER;
				cansysflags = entry->systemFlags| NOUN_NODETERMINER;
				return properties;
			}
		}
	}

	// cannot be a text number if upper case not at start
	// unless the word only exists as upper case, e.g. Million in German
	// penn numbers as words do not go to change their entry value -- DO NOT TREAT "once" as a number, though canonical can be 1
	if (kind != ROMAN_NUMBER) 
	{
		WORDP X = FindWord(original, 0, UPPERCASE_LOOKUP);
		if (!X || FindWord(original, 0, LOWERCASE_LOOKUP) || IS_NEW_WORD(X))
		{
			MakeLowerCase(original);
		}
	}
	entry = StoreWord(original);
	char number[MAX_WORD_SIZE];
	uint64 baseflags = (entry) ? entry->properties : 0;
	if (kind == ROMAN_NUMBER) baseflags = 0; // ignore other meanings
	char* br = hyphen;
	if (!br) br = strchr(original, '_');

	if (kind == PLACETYPE_NUMBER)
	{
		entry = StoreWord(original, properties);
		if (atloc > 1 && start != atloc && IsNumber(wordStarts[atloc - 1], numberStyle) != NOT_A_NUMBER && !strstr(original, (char*)"second")) // fraction not place
		{
			double val = 1.0 / Convert2Integer(original, numberStyle);
			WriteFloat(number,val);
		}
		else sprintf(number, (char*)"%d", (int)Convert2Integer(original, numberStyle));
		sysflags |= ORDINAL| NOUN_NODETERMINER;
		properties |= ADVERB | ADJECTIVE | ADJECTIVE_NORMAL | ADJECTIVE_NUMBER | NOUN | NOUN_NUMBER | (baseflags & TAG_TEST); // place numbers all all potential adverbs:  "*first, he wept"  but not in front of an adjective or noun, only as verb effect
	}
	else if (kind == FRACTION_NUMBER && strchr(original, '%'))
	{
		int64 val1 = atoi(original);
		double val = (double)(val1 / 100.0);
		WriteFloat(number,  val);
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER;
		entry = StoreWord(original, properties, NOUN_NODETERMINER);
		canonical = StoreWord(number, properties);
		properties |= canonical->properties;
		sysflags = entry->systemFlags| NOUN_NODETERMINER;
		cansysflags = canonical->systemFlags| NOUN_NODETERMINER;
		return properties;
	}
	else if (kind == FRACTION_NUMBER && br) // word fraction
	{
		char c = *br;
		*br = 0;
		int64 val1 = Convert2Integer(original, numberStyle);
		int64 val2 = Convert2Integer(br + 1, numberStyle);
		if (val1 == NOT_A_NUMBER || val2 == NOT_A_NUMBER) {
			*br = c;
			return 0; // not a number after all
		}
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
		entry = StoreWord(original, properties, NOUN_NODETERMINER);
		canonical = StoreWord(number, properties, NOUN_NODETERMINER);
		properties |= canonical->properties;
		sysflags = entry->systemFlags| NOUN_NODETERMINER;
		cansysflags = canonical->systemFlags| NOUN_NODETERMINER;
		return properties;
	}
	else if (kind == CURRENCY_NUMBER) // money
	{
		char copy[MAX_WORD_SIZE];
        char* value;
        strcpy(copy, original);
        value = NULL;
		unsigned char* currency = GetCurrency((unsigned char*)copy, value);
        if (value && currency && currency > (unsigned char*)value) *currency = 0; // remove trailing currency
        
        if (!value) value = "0"; // eg 100$% faulty number

        int64 n = Convert2Integer(value, numberStyle);
        if (n == NOT_A_NUMBER) return 0; // not a number after all
		double fn = Convert2Double(value, numberStyle);
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
		sysflags |= NOUN_NODETERMINER;
		AddProperty(entry, CURRENCY);
	}
	else if (kind == FRACTION_NUMBER)
	{
		int64 val = Convert2Integer(original, numberStyle);
		if (val == NOT_A_NUMBER) return 0; // not a number after all
		double val1 = (double)1.0 / (double)val;
		sprintf(number, (char*)"%f", val1);
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER | (baseflags & (PREDETERMINER | DETERMINER));
		sysflags |= NOUN_NODETERMINER;
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
		properties = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER | (baseflags & (PREDETERMINER | DETERMINER));
		sysflags |= NOUN_NODETERMINER;
		if (strchr(original, numberPeriod) || exponent) // floating
		{
			double val = Convert2Double(original, numberStyle);
			if (percent) val /= 100;
			WriteFloat(number,  val);
		}
		else if (percent)
		{
			int64 val = Convert2Integer(original, numberStyle);
			if (val == NOT_A_NUMBER) return 0; // not a number after all
			double val1 = ((double)val) / 100.0;
			WriteFloat(number,val1);
		}
		else
		{
			int64 val = Convert2Integer(original, numberStyle);
			if (val == NOT_A_NUMBER)
			{
				if (IsNumber(original, numberStyle, false)) strcpy(number, original);
				else
				{
					strcpy(number, original); // we have no other use 
				}
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
		if (percent) original[len - 1] = '%';
	}
	canonical = StoreWord(number, properties, sysflags);
	cansysflags |= sysflags;

	// other data already existing on the number

	if (entry != NULL && (entry->properties & PART_OF_SPEECH))
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
		if (val & POSSESSIVE && tokenControl & TOKEN_AS_IS && !stricmp(original, (char*)"'s") && atloc > start) // internal expand of "it 's" and "What 's" and capitalization failures that contractions.txt wouldn't have handled 
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
			if (ValidMemberFact(F))
			{
				known = true;
				break;
			}
			F = GetSubjectNondeadNext(F);
		}
	}
	return known;
}

WORDP FindGermanPlural(WORDP singular)
{
	char word[MAX_WORD_SIZE];
	strcpy(word, singular->word);
	strcat(word, "e"); // eg Tag->Tage
	WORDP D = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (D && GetCanonical(D) == singular) return D;

	strcpy(word, singular->word);
	strcat(word, "en"); // eg Frau->Frauen
	D = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (D && GetCanonical(D) == singular) return D;

	strcpy(word, singular->word);
	strcat(word, "n"); // eg Junge->Jungen
	D = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (D && GetCanonical(D) == singular) return D;

	strcpy(word, singular->word);
	strcat(word, "r"); // 
	D = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (D && GetCanonical(D) == singular) return D;

	strcpy(word, singular->word);
	strcat(word, "er"); // eg Geist->Geister
	D = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (D && GetCanonical(D) == singular) return D;

	strcpy(word, singular->word);
	strcat(word, "s"); // eg Foto->Fotos
	D = FindWord(word, 0, UPPERCASE_LOOKUP);
	if (D && GetCanonical(D) == singular) return D;

	//Sometimes adding an - e and converting a vocal to an Umlaut
	//Ball->Bälle
	//Korb->Körbe
	//Kuh->Kühe

	//	Sometimes plural is the same
	//	Computer->Computer
	//	Löffel->Löffel

	//	Sometimes the same but converting a vocal to an Umlaut
	//	Apfel->Äpfel
	//	Vogel->Vögel
	//	Garten->Gärten

	//		Adding an - er and converting a vocal to an Umlaut
	//	Buch->Bücher
	//	Mann->Männer

	return NULL;
}

uint64 FindSpanishSingular(char* original, char* word, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	WORDP E = FindWord(word);
	if (E && E->properties & NOUN)
	{
		canonical = E;
		if (!exportdictionary) entry = StoreWord(original, AS_IS | NOUN | NOUN_PLURAL);
		else entry = canonical;
		sysflags = E->systemFlags;
		return NOUN | NOUN_PLURAL;
	}
	return 0;
}

static bool CheckSpanishLemma(char* lemma, size_t lemmaLen,char* original, char* correction, WORDP& entry, WORDP& canonical, uint64 properties, uint64 sysflags)
{
	WORDP E = FindWord(lemma);
	if (E && E->properties & VERB)
	{
		if (correction) strcpy(original + lemmaLen, correction); // revise suffix lacking proper accent
		canonical = E;
		if (!exportdictionary) entry = StoreWord(original, AS_IS | VERB | properties, sysflags);
		else entry = canonical; // testing use where we dont want to create the word
		return true;
	}
	// tamper by removing uprising accent which might not be in base due to object pronoun
	char copy[MAX_WORD_SIZE];
	strcpy(copy, lemma);
	int count = 0;
	size_t len = strlen(copy);
	while (--len && count < 2)
	{
			if (!strncmp(copy+len, "á", 2)) copy[len] = 'a'; // change to unaccented form
			else if (!strncmp(copy + len, "é", 2))  copy[len] = 'e'; // change to unaccented form
			else if (!strncmp(copy + len, "í", 2)) copy[len] = 'i'; // change to unaccented form
			else if (!strncmp(copy + len, "ó", 2)) copy[len] = 'o'; // change to unaccented form
			else if (!strncmp(copy + len, "ú", 2)) copy[len] = 'u'; // change to unaccented form
			else continue;
			memmove(copy + len + 1, copy + len + 2, strlen(copy + len + 1)); // remove extra utf8 byte
			++count;
			WORDP F = FindWord(copy); // lemma sans the extra accent (last or next to last syllable)
			if (E && E->properties & VERB)
			{
				canonical = E;
				if (!exportdictionary) entry = StoreWord(original, AS_IS | VERB | properties, sysflags);
				else entry = canonical; // testing use where we dont want to create the word
				return true;
			}
	}
	return false;
}

static bool FindSpanishInfinitive(char* original, char* suffix, char* verbType, char* correction, WORDP& entry, WORDP& canonical, uint64 properties, uint64 sysflags)
{
	size_t originalLen = strlen(original);
	size_t suffixLen = strlen(suffix);
	size_t lemmaLen = originalLen - suffixLen;
	if (strcmp(original + lemmaLen, suffix)) 	return false;

	char lemma[MAX_WORD_SIZE];
	strcpy(lemma, original);
	strcpy(lemma + lemmaLen, verbType); // make lemma from stem and type

	if (CheckSpanishLemma(lemma, lemmaLen, original, correction, entry, canonical, properties, sysflags)) return true;
	
	// if from pronoun, may have to remove added accent
	char lemma1[MAX_WORD_SIZE];
	strcpy(lemma1, lemma);
	char c = 'a';
	char* found = strstr(lemma1, "á");
	if (!found) {found = strstr(lemma1, "é"); c = 'e';}
	if (!found) { found = strstr(lemma1, "í"); c = 'i'; }
	if (!found) { found = strstr(lemma1, "ó"); c = 'o'; }
	if (!found) { found = strstr(lemma1, "ú"); c = 'u'; }
	if (found)
	{
		*found = c;
		memmove(found + 1, found + 2, strlen(found+1)); // know its 2 chars wide
		if (CheckSpanishLemma(lemma1, lemmaLen, original, correction, entry, canonical, properties, sysflags)) return true;
	}

	// irregular verbs
	char copy[MAX_WORD_SIZE];
	strcpy(copy, lemma);
	char* change = strstr(copy, "ie");
	char* again = (change) ? strstr(change + 1, "ie") : NULL;
	if (again) change = again;
	if (change) // ie from e
	{
		memmove(change, change + 1, strlen(change));
		if (CheckSpanishLemma(copy, lemmaLen-1, original, correction, entry, canonical, properties, sysflags)) return true;
	}
	
	strcpy(copy, lemma);
	change = strstr(copy, "ue");
	again = (change) ? strstr(change + 1, "ue") : NULL;
	if (change) // ue from o
	{
		memmove(change, change + 1, strlen(change));
		*change = 'o';
		if (CheckSpanishLemma(copy, lemmaLen - 1, original, correction, entry, canonical, properties, sysflags)) return true;
	}
	if (!strcmp(verbType,"ir")) // ir verb i from e 
	{
		strcpy(copy, lemma);
		char* change = strrchr(copy, 'i');
		if (change) // i from e
		{
			*change = 'e';
			if (CheckSpanishLemma(copy, lemmaLen , original, correction, entry, canonical, properties, sysflags)) return true;
		}
	}

	if (!stricmp(verbType, "er"))
	{
		strcpy(copy, lemma);
		char* change = strstr(copy, "ie");
		char* again = (change) ? strstr(change + 1, "ie") : NULL;
		if (again) change = again;
		if (change) // ie from e
		{
			memmove(change, change + 1, strlen(change));
			if (CheckSpanishLemma(copy, lemmaLen - 1, original, correction, entry, canonical, properties, sysflags)) return true;
		}

		strcpy(copy, lemma);
		change = strstr(copy, "ue");
		again = (change) ? strstr(change + 1, "ue") : NULL;
		if (change) // ue from o
		{
			memmove(change, change + 1, strlen(change));
			*change = 'o';
			if (CheckSpanishLemma(copy, lemmaLen - 1, original, correction, entry, canonical, properties, sysflags)) return true;
		}
	}

	return false;
}

bool PresentSpanish(char* original, WORDP& entry, WORDP& canonical)
{
	uint64 properties = PRONOUN_SUBJECT | VERB_PRESENT;
	// 1st person  //		yo			(habl)o			(com)o		(abr)o
	if (FindSpanishInfinitive(original, "o", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) // yo
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "o", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) /// él, ella, usted	habl + a	habla
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "o", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}

	// 2nd person //		tú				(habl)as		(com)es	(abr)es
	if (FindSpanishInfinitive(original, "as", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU)) // tu
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "es", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "es", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	// 3rd person //		él	/ ella		(habl)a			(com)e		(abr)e
	if (FindSpanishInfinitive(original, "a", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR)) /// él, ella, usted	habl + a	habla
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "e", "er", NULL, entry, canonical, properties,  PRONOUN_SINGULAR))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "e", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	// 1st person plural //		nosotros	(habl)amos	(com)emos	(abr)imos
	if (FindSpanishInfinitive(original, "amos", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) /// él, ella, usted	habl + a	habla
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "emos", "er", NULL, entry, canonical, properties,  PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "imos", "ir", NULL, entry, canonical, properties,  PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}

	// 2nd person plural   //		vosotros	(habl)áis		(com)éis	(abr)ís
	if (FindSpanishInfinitive(original, "áis", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) /// él, ella, usted	habl + a	habla
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ais", "ar", "áis", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) //USer MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "éis", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "eis", "er", "éis", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // user misspell
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ís", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "is", "ir", "ís", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	// 3rd person plural ellos, ellas, ustedes // ellos/ellas	(habl)an	(com)en	(abr)en
	if (FindSpanishInfinitive(original, "an", "ar", NULL, entry, canonical, properties,PRONOUN_PLURAL))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "en", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "en", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}

	// imperative tu form?  parar(to stop) = paras(you stop) → ¡Para!(Stop!)
	char copy[MAX_WORD_SIZE];
	strcpy(copy, original);
	strcat(copy, "s"); // put back removed s from present tu tense
	// tu form
	if (FindSpanishInfinitive(original, "as", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU)) // tu
	{
		entry->systemFlags |= VERB_IMPERATIVE;
		return true;
	}
	else if (FindSpanishInfinitive(original, "es", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		entry->systemFlags |= VERB_IMPERATIVE;
		return true;
	}
	else if (FindSpanishInfinitive(original, "es", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		entry->systemFlags |= VERB_IMPERATIVE;
		return true;
	}

	return false;
}

uint64 PastSpanish(char* original, WORDP& entry, WORDP& canonical)
{
	uint64 properties = PRONOUN_SUBJECT | VERB_PAST;
	// 1st person  preterite  past //		yo				(habl)é			(com)í			(abr)í
	if (FindSpanishInfinitive(original, "é", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "e", "ar", "é", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) // user misspell
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "i", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "i", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	// 2nd person 	 past preterite  	tú	(habl)aste	(com)iste	(abr)iste
	if (FindSpanishInfinitive(original, "aste", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		 return true;
	}
	else if (FindSpanishInfinitive(original, "iste", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iste", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	// 3rd person  //		él / ella			(habl)ó			(com)ió		(abr)ió
	if (FindSpanishInfinitive(original, "ó", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "o", "ar", "ó", entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ió", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "io", "er", "ió", entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ió", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "io", "ir", "ió", entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		return true;
	}
	// 1st person plural 		nosotros	(habl)amos	(com)imos	(abr)imos
	if (FindSpanishInfinitive(original, "amos", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "imos", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "imos", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // USER MISSPELL
	{
		return true;
	}
	// 2nd person plural //		vosotros		(habl)asteis	(com)isteis	(abr)isteis
	if (FindSpanishInfinitive(original, "asteis", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // USER MISSPELL
	{
		return true;
	}
	 else  if (FindSpanishInfinitive(original, "isteis", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // USER MISSPELL
	 {
		 return true;
	 }
	 else  if (FindSpanishInfinitive(original, "isteis", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // USER MISSPELL
	 {
			return true;
	}
	// 3rd person plural //		ellos/ellas		(habl)aron	(com)ieron	(abr)ieron
	if (FindSpanishInfinitive(original, "aron", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL)) // USER MISSPELL
	{
		return true;
	}
	else 	if (FindSpanishInfinitive(original, "ieron", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL)) // USER MISSPELL
	{
		return true;
	}
	else 	if (FindSpanishInfinitive(original, "ieron", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL)) // USER MISSPELL
	{
		return true;
	}

	// imperfect past 
	// 1st person  //	yo				(habl)aba		(com)ía
	if (FindSpanishInfinitive(original, "aba", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	else 	if (FindSpanishInfinitive(original, "ia", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	else 	if (FindSpanishInfinitive(original, "ia", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		 return true;
	}
	// 2nd person //	tú					(habl)abas	(com)ías
	if (FindSpanishInfinitive(original, "abas", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ías", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ias", "er", "ías", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ías", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ias", "ir", "ías", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU)) // user misspell
	{
		return true;
	}
	// 3rd person //	él / ella			(habl)aba		(com)ía
	if (FindSpanishInfinitive(original, "aba", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ía", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ia", "er", "ía", entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		 return true;
	 }
	else if (FindSpanishInfinitive(original, "ía", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ia", "ir", "ía", entry, canonical, properties, PRONOUN_SINGULAR)) // USER MISSPELL
	{
		return true;
	}
	// 1st person plural //	nosotros		(habl)ábamos	(com)íamos
	if (FindSpanishInfinitive(original, "ábamos", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "abamos", "ar", "ábamos", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "íamos", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iamos", "er", "íamos", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // USER MISSPELL
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "íamos", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iamos", "ir", "íamos", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // 	USER MISSPELL
	{
		return true;
	}
	// 2nd person plural //	vosotros		(habl)abais	(com)íais
	if (FindSpanishInfinitive(original, "abais", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "íais", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iais", "er", "íais", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // user misspell
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "íais", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iais", "ir", "íais",  entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // 	USER MISSPELL
	{
		return true;
	}
	// 3rd person plural //	ellos/ellas		(habl)aban	(com)ían
	if (FindSpanishInfinitive(original, "aban", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ían", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ean", "er", "ían", entry, canonical, properties, PRONOUN_PLURAL)) // user misspell
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ían", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ian", "ir", "ían", entry, canonical, properties, PRONOUN_PLURAL)) // user misspell
	{
		return true;
	}

	return false;
}

bool FutureSpanish(char* original, WORDP& entry, WORDP& canonical)
{
	uint64 properties = PRONOUN_SUBJECT | AUX_VERB_FUTURE;
	// 1st person //yo(hablar)é   all 3 verb types
	if (FindSpanishInfinitive(original, "é", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		 return true;
	}
	else if (FindSpanishInfinitive(original, "e", "", "é", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) // 	USER MISSPELL
	{
		return true;
	}
	// 2nd person //	tú(hablar)ás  all 3 verb types
	if (FindSpanishInfinitive(original, "ás", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "as", "", "ás", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU)) // 	USER MISSPELL
	{
		return true;
	}
	// 3rd person //él / ella(hablar)á  all 3  verb types
	 if (FindSpanishInfinitive(original, "á", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	 else if (FindSpanishInfinitive(original, "a", "", "á", entry, canonical, properties, PRONOUN_SINGULAR)) // 	USER MISSPELL
	{
		return true;
	}
	// 1st person plural  //nosotros(hablar)emos  all 3 verb types
	 if (FindSpanishInfinitive(original, "emos", "", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	// 2nd person plural  //vosotros(hablar)áis  all 3 verb types
	 if (FindSpanishInfinitive(original, "áis", "", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	 else if (FindSpanishInfinitive(original, "ais", "", "áis",entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // 	USER MISSPELL
	{
		return true;
	}
	// 3rd person plural //ellos / ellas(hablar)án  all 3 verb types
	 if (FindSpanishInfinitive(original, "án", "", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}
	 else if (FindSpanishInfinitive(original, "an", "", "án", entry, canonical, properties, PRONOUN_PLURAL))
	{
		 return true;
	}
	// conditional future 
	// 1st person // yo(hablar)ía  all 3 types
	 if (FindSpanishInfinitive(original, "ía", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	 else if (FindSpanishInfinitive(original, "ia", "", "ía", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) // USER MISSPELL
	{
		return true;
	}
	// 2nd person // tú(hablar)ías			 all 3 types
	 if (FindSpanishInfinitive(original, "ías", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ias", "", "ías", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU)) // user MISSPELL
	{
		return true;
	}
	// 3rd person // él / ella(hablar)ía   all 3 types
	 if (FindSpanishInfinitive(original, "ía", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ia", "", "ía", entry, canonical, properties, PRONOUN_SINGULAR)) // user misspell
	{
		return true;
	}
	// 1st person plural // nosotros(hablar)íamos  all 3 types
	 if (FindSpanishInfinitive(original, "íamos", "", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iamos", "", "íamos", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I)) // user misspell
	{
		 return true;
	}
	// 2nd person plural // vosotros(hablar)íais  all 3 types
	 if (FindSpanishInfinitive(original, "íais", "", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		 return true;
	}
	else if (FindSpanishInfinitive(original, "iais", "", "íais", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // user misspell
	{
		return true;
	}
	// 3rd person plural  //ellos / ellas(hablar)ían  all 3 types
	 if (FindSpanishInfinitive(original, "ían", "", NULL, entry, canonical, properties, PRONOUN_PLURAL))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ian", "a", "ían", entry, canonical, properties, PRONOUN_PLURAL)) // user misspell
	{
		return true;
	}

	return false;
}

static bool GerundSpanish(char* word, char* original, WORDP& entry, WORDP& canonical)
{
	uint64 properties = NOUN | NOUN_GERUND;
	if (FindSpanishInfinitive(original, "ando", "ar", NULL, entry, canonical, properties, 0))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iando", "er", NULL, entry, canonical, properties, 0))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "iando", "ir", NULL, entry, canonical, properties, 0))
	{
		return true;
	}
	return false;
}

bool PastParticipleSpanish(char* word, char* original, WORDP& entry, WORDP& canonical)
{
	uint64 properties = VERB | VERB_PAST_PARTICIPLE | ADJECTIVE;
	if (FindSpanishInfinitive(original, "ado", "ar", NULL, entry, canonical, properties, 0))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ido", "er", NULL, entry, canonical, properties, 0))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "ido", "ir", NULL, entry, canonical, properties, 0))
	{
		return true;
	}
	return false;
}

WORDP FindSpanishWord(char* base, char* full)
{ // presume it had accent added, try to remove it
	WORDP D = FindWord(full); // dictionary knows it?
	if (D)
	{
		D = GetCanonical(D, VERB_INFINITIVE);
		if (D && D->properties & VERB_INFINITIVE) return D;
	}

	D = FindWord(base);
	if (D) return D;
	char copy[MAX_WORD_SIZE];
	strcpy(copy, base);
	char* ptr = copy;
	char c = 0;
	while (*++ptr)
	{
		if (!strnicmp(ptr, "á", 2)) c = 'a'; 
		else if (!strnicmp(ptr, "é", 2)) c = 'e';
		else if (!strnicmp(ptr, "í", 2)) c = 'i';
		else if (!strnicmp(ptr, "ó", 2)) c = 'o';
		else if (!strnicmp(ptr, "ú", 2)) c = 'u';
		if (c)
		{
			*ptr = c;
			memmove(ptr + 1, ptr + 2, strlen(ptr + 1));
			strcpy(base, copy);
			return FindWord(copy);
		}
	}
	return NULL;
}

uint64 ComputeSpanishNounGender(WORDP D)
{
	char* word = D->word;
	size_t len = strlen(word);
	char ending = word[len - 1];
	uint64 properties = D->properties & (SPANISH_HE | SPANISH_SHE);
	if (properties & (SPANISH_HE | SPANISH_SHE)) {}
	else if (!stricmp(word, "sofá") || !stricmp(word, "mapa") || !stricmp(word, "dia") || !stricmp(word, "día")
		|| !stricmp(word, "cura") || !stricmp(word, "planeta")
		|| !stricmp(word, "corazón") || !stricmp(word, "buzón")
		)  properties |= SPANISH_HE; // exception to next rule
	else if (!stricmp(word, "mano") || !stricmp(word, "radio") || !stricmp(word, "foto") || !stricmp(word, "moto")
		|| !stricmp(word, "flor") || !stricmp(word, "leche") || !stricmp(word, "carne"))  properties |= SPANISH_SHE; // exception to next rule
	else  if (!stricmp(word, "agua")) properties |= SPANISH_HE; // exception to next rule - masculine article but feminine adjective
	else if (len > 2 &&
		(!stricmp(word + len - 2, "ía") || !stricmp(word + len - 2, "ia")
			|| !stricmp(word + len - 2, "ua") || !stricmp(word + len - 2, "úa") || !stricmp(word + len - 2, "ez")))
	{
		properties |= SPANISH_SHE;
	}
	else if (len > 2 &&
		(!stricmp(word + len - 2, "or") || !stricmp(word + len - 2, "ío") || !stricmp(word + len - 2, "io")
			|| !stricmp(word + len - 2, "ma") || !stricmp(word + len - 2, "an")))
	{
		properties |= SPANISH_HE;
	}
	else if (len > 3 &&
		(!stricmp(word + len - 3, "cha") || !stricmp(word + len - 3, "bra") || !stricmp(word + len - 3, "ara")
			|| !stricmp(word + len - 3, "dad") || !stricmp(word + len - 3, "tad") || !stricmp(word + len - 3, "tud")
			))
	{
		properties |= SPANISH_SHE;
	}
	else if (len > 3 &&
		(!stricmp(word + len - 3, "aje") || !stricmp(word + len - 3, "zón") || !stricmp(word + len - 3, "aro") || !stricmp(word + len - 3, "emo")
			))
	{
		properties |= SPANISH_HE;
	}
	else if (len > 3 &&
		!stricmp(word + len - 3, "nte"))
	{
		properties |= SPANISH_HE | SPANISH_SHE;
	}
	else if (len > 4 &&
		(!stricmp(word + len - 4, "ecto") || !stricmp(word + len - 4, "ismo")
			))
	{
		properties |= SPANISH_HE;
	}
	else if (len > 4 && !stricmp(word + len - 4, "ista"))
	{
		properties |= SPANISH_HE | SPANISH_SHE;
	}
	else if (len > 4 && (!stricmp(word + len - 4, "cíon") || !stricmp(word + len - 4, "síon")
		|| !stricmp(word + len - 4, "triz")))
	{
		properties |= SPANISH_SHE;
	}
	else if (len > 5 && (!stricmp(word + len - 5, "umbre")))
	{
		properties |= SPANISH_SHE;
	}
	//https://vamospanish.com/discover/spanish-grammar-nouns-and-gender/
	else if (ending == 'r') properties |= SPANISH_HE;
	else if (ending == 'z') properties |= SPANISH_SHE;
	else properties |= SPANISH_SHE | SPANISH_HE;
	return properties;
}

uint64 ComputeSpanishPluralNoun(char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{ // unknown in dictionary
	if (!D) D = StoreWord(word); // generate it as a word
	entry = D;
	sysflags = D->systemFlags;
	uint64 properties = 0;
	size_t len = strlen(word);
	char ending = word[len - 1];
	char end2 = word[len - 2];

	if (!canonical) // plurals of nouns may not be in dictionary
	{
		WORDP E = NULL;
		if (ending == 's' && end2 == 'e') E = FindWord(word, len - 2);
		else if (ending == 's') E = FindWord(word, len - 1);
		// dictionary has noun_singular and noun_plural. here
		// we infer to actual known SPANISH_SINGULAR OR PLURAL
		if (E && (E->properties & NOUN_SINGULAR))
		{
			canonical = E;
			uint64 gender = ComputeSpanishNounGender(E);
			sysflags = E->systemFlags; // best we can do
			D->properties |= NOUN | SPANISH_PLURAL | gender; // permanent change to dict
			
			return D->properties;
		}
		else return 0;
	}
	else return 0;
}

uint64 ComputeSpanishNoun(char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	entry = D;
	if (!canonical)
	{
		uint64 ans = ComputeSpanishPluralNoun(word, D, entry, canonical, sysflags);
		if (!canonical) 	canonical = D;
	}
	sysflags = D->systemFlags;
	uint64 properties = 0;
	size_t len = strlen(word);
	char ending = word[len - 1];
	char end2 = word[len - 2];

	size_t lenc = strlen(canonical->word);
	char root = canonical->word[lenc-1];

	// plurality
	if (IsVowel(root) && ending == 's')
	{
		properties |= SPANISH_PLURAL;
	}
	else if (!IsVowel(root) && ending == 's' && end2 == 'e')
	{
		properties |= SPANISH_PLURAL ;
	}
	else
	{
		properties |= SPANISH_SINGULAR;
	}

	// gender
	properties |= ComputeSpanishNounGender(D);
	
	if (properties) D->properties |= properties; // permanent change to dict
	return D->properties;
}

uint64 ComputeSpanishAdjective(char* word,WORDP D, WORDP & entry, WORDP & canonical, uint64 & sysflags)
{
	//		1) Masculine singular is the default formand usually ends in - o. This changes to - a for feminine singular.For plural, add - s.
//			rojo(red)
//			rojo	 	rojos
//			roja	 	rojas
//	2) 	HUH? When the masculine adjective ends in - a or -e, there is no difference between the masculine and feminine forms. The plural is still created by adding - s.
//			realista(realistic)
//			realista	 	realistas
//			realista	 	realistas
//			triste(sad)
//			triste	 	tristes
//			triste	 	tristes
//3) When the adjective ends in any consonant except - n, -r, or -z, there is no difference between the masculineand feminine forms, and the plural is created by adding - es.
//		fácil(easy)
//  HUH conflict with es not plural
//			fácil	 	fáciles
//			fácil	 	fáciles
// 4) When the adjective ends in z, there is no difference between the masculineand feminine forms, and the plural is created by changing the z to a c and adding - es. (Why ? )
//5) For adjectives that end in - n or -r, * the feminine is created by adding - a, the masculine plural by adding - es and the feminine plural by adding - as.
//hablador(talkative)
//hablador	 	habladores
//habladora	 	habladoras
	if (!canonical) canonical = D;
	entry = D;
	sysflags = D->systemFlags;
	uint64 properties = 0;

	// dict already correctly marks gender and plurality
	if (D->properties & (SPANISH_HE | SPANISH_SHE)) return D->properties;

	size_t len = strlen(word);
	char ending = word[len - 1];
	char end2 = word[len - 2];
	char end3 = word[len - 3];
	size_t len1 = strlen(canonical->word);
	char root = canonical->word[len1 - 1];

	// some adjectives do not change at all
	if (!stricmp(word, "rosa") || !stricmp(word, "naranja") || !stricmp(word, "cada") || !stricmp(word, "violeta"))
	{
		properties |= SPANISH_HE | SPANISH_SHE | SPANISH_SINGULAR | SPANISH_PLURAL;
	}
	// but legal plurals for above are also
	else if (!stricmp(word, "rosas") || !stricmp(word, "naranjas") || !stricmp(word, "cadas") || !stricmp(word, "violetas"))
	{
		properties |= SPANISH_HE | SPANISH_SHE  | SPANISH_PLURAL;
	}
	// ending ista are both
	else if (!stricmp(canonical->word + len1 - 4, "ista"))
	{
		properties |= SPANISH_SHE | SPANISH_HE;
		properties |= SPANISH_SINGULAR;
	}
	// ending istas are both
	else if (!stricmp(canonical->word + len1 - 5, "istas"))
	{
		properties |= SPANISH_SHE | SPANISH_HE;
		properties |= SPANISH_PLURAL;
	}

	// Adjectives that end in -or, -ora, -ón, or -ín do have feminine forms. Simply add a or -as to the masculine singular form and delete the written accent if necessary.
	else if (!stricmp(canonical->word + len1 - 2, "ora") || !stricmp(canonical->word + len1 - 2, "or") || !stricmp(canonical->word + len1 - 2, "ón") || !stricmp(canonical->word + len1 - 2, "ín"))
	{
		if (ending == 'a') // female singular
		{
			properties |= SPANISH_SHE;
			properties |= SPANISH_SINGULAR;
		}
		else if (ending == 's') // plural
		{
			properties |= (end2 == 'a') ? SPANISH_SHE : SPANISH_HE;
			properties |= SPANISH_PLURAL;
		}
		else // default male
		{
			properties |= SPANISH_HE;
			properties |= SPANISH_SINGULAR;
		}
	}
	else if (!stricmp(canonical->word + len1 - 3, "óna") || !stricmp(canonical->word + len1 - 3, "ína"))
	{
		properties |= SPANISH_SHE;
		properties |= SPANISH_SINGULAR;
	}

	// https://www.thespanishlearningclub.com/pages/blog?p=the-gender-of-nouns-in-spanish
	//2) When the masculine adjective ends in - a or -e, there is no difference between the masculine and feminine forms.
	else if (root == 'e') // grande
	{
		properties |= SPANISH_SHE | SPANISH_HE;
		properties |= (ending == 's') ? SPANISH_PLURAL : SPANISH_SINGULAR;
	}
	else if ( root == 'a')
	{
		properties |= SPANISH_SHE;
		properties |= (ending == 's') ? SPANISH_PLURAL : SPANISH_SINGULAR;
	}
	//1) Masculine singular is the default form and usually ends in - o. This changes to - a for feminine singular.
	else if (ending == 'o' || !IsVowel(ending))
	{//https://happylanguages.co.uk/lesson/grammar-tip-masculine-and-feminine-in-spanish/
		properties |= SPANISH_HE;
		properties |= SPANISH_SINGULAR;
	}
	
	//	1) For plural, add - s. Masculine singular is the default form and usually ends in - o. This changes to - a for feminine singular. 
	else if (ending == 's' && (end2 == 'o' || end2 == 'a'))
	{
		properties |= (end2 == 'o') ? SPANISH_HE : SPANISH_SHE;
		properties |= SPANISH_PLURAL;
	}

	//3) When the adjective ends in any consonant except - n, -r, or -z, there is no difference between the masculine and feminine forms.
	else if (ending != 's' && !IsVowel(ending) && (ending != 'n' && ending != 'r' && ending != 'z')) // singular
	{
		properties |= SPANISH_HE | SPANISH_SHE;
		properties |= SPANISH_SINGULAR;
	}
	//3) the plural is created by adding - es when the adjective ends in any consonant except - n, -r, or -z, there is no difference between the masculine and feminine forms.
	else if (ending == 's' && end2 == 'e' && !IsVowel(end3) && (end3 != 'n' && end3 != 'r' && end3 != 'z'))
	{
		// if original was Z, was changed to C
		properties |= SPANISH_HE | SPANISH_SHE;
		properties |= SPANISH_PLURAL;
	}
	//5) the masculine plural by adding - es and the feminine plural by adding - as For adjectives that end in - n or -r, * the feminine is created by adding - a, .
	else if (ending == 's' && end2 == 'a' && (end3 == 'n' || end3 == 'r'))
	{
		properties |= SPANISH_SHE;
		properties |= SPANISH_PLURAL;
	}
	//5) the masculine plural by adding - es and the feminine plural by adding - as.
	else if (ending == 's' && end2 == 'e' && (end3 == 'n' || end3 == 'r'))
	{
		properties |= SPANISH_HE;
		properties |= SPANISH_PLURAL;
	}
	//5) For adjectives that end in - n or -r, * the feminine is created by adding - a, 
	else if (ending == 'n' || ending == 'r')
	{
		properties |= SPANISH_HE;
		properties |= SPANISH_PLURAL;
	}
	//5) For adjectives that end in - n or -r, * the feminine is created by adding - a, 
	else if (ending == 'a' && (end2 == 'r' || end2 == 'n'))
	{
		properties |= SPANISH_SHE | SPANISH_PLURAL;;
	}
	if (properties) D->properties |= properties | ADJECTIVE | ADJECTIVE_NORMAL; // permanent change to dict
	return D->properties;
}

uint64 ComputeSpanish(int at, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags,bool adjust) // case sensitive, may add word to dictionary, will not augment flags of existing words
{
	// can we find this as a conjugation of something?
	char word[MAX_WORD_SIZE];
	strcpy(word, original);
	MakeLowerCase(word);
	WORDP D = FindWord(word);
	uint64 properties = (D) ? D->properties : 0;
	canonical = GetCanonical(D, properties);

	if (D && D->properties & ADJECTIVE)
	{
		uint64 ans = ComputeSpanishAdjective(word, D,  entry, canonical, sysflags);
		if (ans) return ans;
	}
	if (D && D->properties & NOUN)
	{
		uint64 ans = ComputeSpanishNoun(word, D, entry, canonical, sysflags);
		if (ans) return ans;
	}
	else if (!D || !(D->properties & TAG_TEST)) // maybe we can infer unknown is plural noun
	{
		uint64 ans = ComputeSpanishPluralNoun(word, D, entry, canonical, sysflags);
		if (ans) return ans;
	}

	// we need to check for pronouns because dict doesnt have them
	// dont use verb if conflict conjunction or preposition (para is one as it is also a command parar form)
	// WE DONT exclude adjective here because some verb forms are adjective as well 
	// verb infinitive we dont do here, because we might have decided it is a pronoun conjugation of an infinitive buscarlo
	if (properties & (DETERMINER | ADVERB | CONJUNCTION | PREPOSITION) ||
		(properties & PRONOUN_SUBJECT && !(properties & VERB)))// pure pronoun
	{
		if (!canonical) canonical = D;
		entry = D;
		sysflags = D->systemFlags;
		return properties;
	}

	D = NULL;
	properties = 0;

	char base[MAX_WORD_SIZE];
	size_t len = strlen(original);

	// these verb tests assume NO embedded object pronouns, those happen later
	// but within these it does try embedded subject pronouns

	// try reflexive forms of any regular verb
	if (len > 2 && word[len - 2] == 's' && word[len - 1] == 'e')
	{
		strcpy(base, word);
		base[len - 2] = 0;
		WORDP E = FindWord(base);
		if (E && E->properties & VERB)
		{
			canonical = E;
			if (!exportdictionary) entry = StoreWord(word, AS_IS | VERB_PRESENT);
			else entry = canonical;
			sysflags |= PRONOUN_REFLEXIVE;
			entry->systemFlags |= PRONOUN_REFLEXIVE;
			if (E->properties & VERB_INFINITIVE) E->properties ^= VERB_INFINITIVE; // treetagger lied - direct change, not restorable
			E->properties |= VERB_PRESENT; // treetagger lied - direct change, not restorable
			if (entry->properties & VERB_INFINITIVE)  entry->properties ^= VERB_INFINITIVE; // treetagger lied - direct change, not restorable
			return E->properties;
		}
	}
	strcpy(base, word);
	len = strlen(base);

	// spanish indirect objects: me te le nos os les  (note le and les are different from directobjects, rest same)
	// attach the indirect object pronoun to an infinitive or a present participle.
	if (len < 3) {}
	else if (!stricmp(base + len - 3, "nos") || !stricmp(base + len - 3, "les"))
	{
		base[len - 3] = 0;
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties && (VERB_INFINITIVE | NOUN_GERUND) &&
			D->systemFlags & VERB_INDIRECTOBJECT) // need to compute participles in some regular verbs
		{
			uint64 flags = PRONOUN_INDIRECTOBJECT_PLURAL | PRONOUN_INDIRECTOBJECT;
			if (word[len - 3] == 'n') flags |= PRONOUN_INDIRECTOBJECT_I;
			// no you its the singular informal
			entry = StoreWord(word, AS_IS | D->properties, flags);
			canonical = D;
			return entry->properties;
		}
	}
	// nos must be tested before os
	else if (!stricmp(base + len - 2, "me") || !stricmp(base + len - 2, "te") || !stricmp(base + len - 2, "le") ||
		!stricmp(base + len - 2, "os") || !stricmp(base + len - 2, "se"))
	{
		base[len - 2] = 0;
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties && (VERB_INFINITIVE | NOUN_GERUND) &&
			D->systemFlags & VERB_INDIRECTOBJECT) // need to compute participles in some regular verbs
		{
			uint64 flags = PRONOUN_INDIRECTOBJECT_SINGULAR | PRONOUN_INDIRECTOBJECT;
			if (word[len - 2] == 'm') flags |= PRONOUN_INDIRECTOBJECT_I;
			else if (word[len - 2] == 't') flags |= PRONOUN_INDIRECTOBJECT_YOU;
			entry = StoreWord(word, AS_IS | D->properties, flags);
			canonical = D;
			return entry->properties;
		}
	}

	strcpy(base, word);

	//  pronoun direct objects built into verb me te lo la nos os los las 
	if (len < 3) {}
	else if (!stricmp(base + len - 3, "nos") || !stricmp(base + len - 3, "los") || !stricmp(base + len - 3, "las"))
	{
		base[len - 3] = 0;
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND))
		{
			uint64 flags = PRONOUN_OBJECT_PLURAL;
			if (word[len - 3] == 'n') flags |= PRONOUN_OBJECT_I;
			// no you its the singular informal
			entry = StoreWord(word, AS_IS | D->properties | PRONOUN_OBJECT, flags);
			return entry->properties;
		}
	}
	// nos must be tested before os
	else if (len > 4 && (!stricmp(base + len - 4, "amos") || !stricmp(base + len - 4, "emos") || !stricmp(base + len - 4, "imos"))) {} // defer to regular conjugation, this will repeat later cominamos protected
	else if (!stricmp(base + len - 2, "me") || !stricmp(base + len - 2, "te") || !stricmp(base + len - 2, "lo") ||
		!stricmp(base + len - 2, "la") || !stricmp(base + len - 2, "os"))
	{
		base[len - 2] = 0;
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND))
		{
			uint64 flags = PRONOUN_OBJECT_SINGULAR;
			if (word[len - 2] == 'm') flags |= PRONOUN_OBJECT_I;
			else if (word[len - 2] == 't') flags |= PRONOUN_OBJECT_YOU;
			entry = StoreWord(word, AS_IS | D->properties | PRONOUN_OBJECT, flags);
			return entry->properties;
		}
	}

	// present tense
	if (PresentSpanish(word, entry, canonical))
	{
		sysflags = entry->systemFlags;
		return entry->properties;
	}

	// past  tense 
	if (PastSpanish(word, entry, canonical))
	{
		sysflags = entry->systemFlags;
		return entry->properties;
	}

	// future   tense 
	if (FutureSpanish(word, entry, canonical))
	{
		sysflags = entry->systemFlags;
		return entry->properties;
	}
	// past participle   tense 
	if (PastParticipleSpanish(word, original, entry, canonical))
	{
		sysflags = entry->systemFlags;
		return entry->properties;
	}
	// gerund
	if (GerundSpanish(word, word, entry, canonical))
	{
		sysflags = entry->systemFlags;
		return entry->properties;
	}

	// deferred os ending
	if (len > 4 && (!stricmp(base + len - 4, "amos") || !stricmp(base + len - 4, "emos") || !stricmp(base + len - 4, "imos"))) //deferred from above
	{
		base[len - 2] = 0; // os ending
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND))
		{
			uint64 flags = PRONOUN_OBJECT_SINGULAR;
			if (word[len - 2] == 'm') flags |= PRONOUN_OBJECT_I;
			else if (word[len - 2] == 't') flags |= PRONOUN_OBJECT_YOU;
			entry = StoreWord(word, AS_IS | D->properties | PRONOUN_OBJECT, flags);
			return entry->properties;
		}
	}

	sysflags = 0; // because we transiently overwrite it

	// adverb from adjective
	char* mente = strstr(word, "mente");
	if (mente && !mente[5]) // see if root is adjective
	{
		WORDP Q = FindWord(word, strlen(word) - 5);
		if (Q && Q->properties & ADJECTIVE)
		{
			if (!exportdictionary) entry = entry = StoreWord(word, AS_IS | ADVERB);
			else entry = canonical = dictionaryBase + 1;
			sysflags = 0;
			return ADJECTIVE;
		}
	}

	// see if plural of a noun
	if (word[len - 1] == 's')  // s after normal vowel ending or after foreign imported word
	{
		strcpy(base, word);
		base[len - 1] = 0;
		properties = FindSpanishSingular(original, base, entry, canonical, sysflags);
		if (properties) return properties | NOUN | NOUN_PLURAL;
	}
	if (word[len - 1] == 's' && word[len - 2] == 'e' && !IsVowel(word[len - 3]))  // es after normal consonant ending
	{
		strcpy(base, word);
		base[len - 2] = 0;
		if (base[len - 3] == 'a') strncpy(base + len - 2, "á", 1);
		else if (base[len - 3] == 'e') strncpy(base + len - 2, "é", 1);
		else if (base[len - 3] == 'i') strncpy(base + len - 3, "í", 1);
		else if (base[len - 3] == 'o') strncpy(base + len - 3, "ó", 1);
		else if (base[len - 3] == 'u') strncpy(base + len - 3, "ú", 1);
		// Singular nouns of more than one syllable which end in -en and don’t already have an accent, add one in the plural. los exámenes
		properties = FindSpanishSingular(original, base, entry, canonical, sysflags);
		if (properties) return properties;
	}
	if (word[len - 1] == 's' && word[len - 2] == 'e')
	{
		strcpy(base, word);
		base[len - 2] = 0;
		base[len - 3] = 'z';
		properties = FindSpanishSingular(original, base, entry, canonical, sysflags);
		if (properties) return properties | NOUN | NOUN_PLURAL;
	}
	// Singular nouns of more than one syllable which end in -en and don’t already have an accent, add one in the plural. los exámenes
	if (word[len - 1] == 's' && word[len - 2] == 'e')
	{
		strcpy(base, word);
		base[len - 2] = 0;
		char* x = base;
		bool accented = false;
		char* firstvowel = NULL;
		char* padd = base;
		while (*++padd)
		{
			if (*padd > 0x7f)  // should it only be vowels?
			{
				accented = true;
				break;
			}
			if (IsVowel(*base) && !firstvowel) firstvowel = padd;
		}
		if (!accented)
		{
			char hold[MAX_WORD_SIZE];
			strcpy(hold, base + 1);
			if (*base == 'a') { strcpy(base, "á"); strcat(base, hold); }
			if (*base == 'e') { strcpy(base, "é"); strcat(base, hold); }
			if (*base == 'i') { strcpy(base, "í"); strcat(base, hold); }
			if (*base == 'o') { strcpy(base, "ó"); strcat(base, hold); }
			if (*base == 'u') { strcpy(base, "ú"); strcat(base, hold); }
		}

		properties = FindSpanishSingular(original, base, entry, canonical, sysflags);
		if (properties) return properties | NOUN | NOUN_PLURAL;
	}

	if (word[len - 1] == 's' && word[len - 2] == 'e')  // es after changing accented vowel
	{
		strcpy(base, word);
		base[len - 2] = 0;
		properties = FindSpanishSingular(original, base, entry, canonical, sysflags);
		if (properties) return properties | NOUN | NOUN_PLURAL;
	}

	// accept what dict has for a verb (desparation, lacks lemma) or adjective or conjugates 
	D = FindWord(word);
	if (D && D->properties & (VERB | ADJECTIVE | NOUN))
	{
		uint64 priority = 0;
		if (D->properties & VERB) priority = VERB;
		else if (D->properties & NOUN) priority = NOUN;
		else if (D->properties & ADJECTIVE) priority = ADJECTIVE;
		entry = D;
		canonical = GetCanonical(D, priority);
		if (!canonical) canonical = entry;
		sysflags = D->systemFlags;
		return D->properties;
	}

	return 0;
}

uint64 GetPosData( int at, char* original,WORDP& revise, WORDP &entry,WORDP &canonical,uint64& sysflags,uint64 &cansysflags,bool firstTry,bool nogenerate, int start) // case sensitive, may add word to dictionary, will not augment flags of existing words
{ // this is not allowed to write properties/systemflags/internalbits if the word is preexisting
	uint64 properties = 0;
	sysflags = cansysflags = 0;
	canonical = 0;  
	if (at < 1) { ; } // not from sentence
	else if (canonicalLower[at]) canonical = canonicalLower[at];  // note canonicalLower may already be set by external postagging
	else if (canonicalUpper[at]) canonical = canonicalUpper[at];  // note canonicalUpper may already be set by external postagging
	entry = 0;
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

	bool csEnglish = !stricmp(language, "english");
	bool isGerman = !stricmp(language, "german");
	bool isSpanish = !stricmp(language, "spanish");

	// number processing has happened
	int numberkind = IsNumber(original, numberStyle);
	if (firstTry && (numberkind != NOT_A_NUMBER || IsDate(original )))
		properties = ProcessNumber(at, original, revise, entry, canonical, sysflags, cansysflags, firstTry, nogenerate, start, numberkind); // case sensitive, may add word to dictionary, will not augment flags of existing wordskind);
    size_t xlen = strlen(original);
	if (canonical && (IsDigit(*canonical->word) || IsNonDigitNumberStarter(*canonical->word)))
    {
        // it is possible that have canonical from external tagger but word because of inflections
        // numbers.txt only has base words, not all forms
        if (!entry) entry = FindWord(original, xlen, STANDARD_LOOKUP);
        return properties;
    }
	entry = FindWord(original, xlen, PRIMARY_CASE_ALLOWED);
	if (entry)
	{
		if (entry->systemFlags & PATTERN_WORD || !entry->word[1]) {} // english letters have no pos tag at present
		else if (!entry->properties || !(entry->properties & TAG_TEST)) entry = NULL; // word has no meaning for us
		else if (IS_NEW_WORD(entry) ) entry = NULL; // word has no meaning for us
	}
	if (!entry)
	{
		entry = FindWord(original, xlen, SECONDARY_CASE_ALLOWED);
		if (entry)
		{
			if (entry->systemFlags & PATTERN_WORD) {}
			else if (!entry->properties || IS_NEW_WORD(entry) || !(entry->properties & TAG_TEST)) entry = NULL; // word has no meaning for us
		}
	}
	//

	if (entry && xlen > 1 && original[xlen - 1] == 's' && entry->internalBits & UPPERCASE_HASH) // aim to being plural of singular noun
	{
		WORDP S = FindWord(original, xlen - 1, LOWERCASE_LOOKUP);
		if (S) entry = NULL; // let it go thru normal
	}
	if (entry) original = entry->word;
	// if uppercase, see if lowercase singular exists
	if (csEnglish && entry && entry->internalBits & UPPERCASE_HASH && !((entry->properties | properties) & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_HUMAN | PRONOUN_SUBJECT | PRONOUN_OBJECT)) && original[xlen - 1] == 's') // possible plural
	{
		WORDP singular = FindWord(original, xlen - 1, LOWERCASE_LOOKUP);
		if (singular) entry = singular;
		else if (original[xlen - 2] == 'e') singular = FindWord(original, xlen - 2, LOWERCASE_LOOKUP);
		if (singular) entry = singular;
	}

	if (!csEnglish || isGerman)
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

	if (entry && entry->internalBits & CONDITIONAL_IDIOM) 
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

    // x between or after number like "5 x" or "5 x 5"
    if (at > 1 && (*original == 'x' || *original == 'X') && !original[1])
    {
        if (IsDigit(*wordStarts[at - 1]))
        {
            entry = canonical = StoreWord(original, PUNCTUATION);
            return PUNCTUATION;
        }
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
		if (IsMail(original))
		{
			// for mail domain is lower and local-part is case sensitive so preserve it to canonical ( RFC5321 )
			char* wordStart = original;
			char* domainStart = strchr(original, '@');
			int endUserName = domainStart - wordStart;
			PartialLowerCopy(word,original,0,endUserName);
		}
		else
		{
			// for url domain is lower and the path/query/hash is case sensitive so preserve it to canonical ( RFC3986 6.2.2.1 (and reinforced in proposal RFC7230 2.7.3) )
			char* firstPeriod = strchr(original, '.');
			char* domainEnd = strchr(firstPeriod, '/');
			int startOfTail;
			if(!domainEnd) startOfTail = strlen(original) - 1;
			else startOfTail = domainEnd - original;
			PartialLowerCopy(word,original,startOfTail,strlen(original) - 1);
		}
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

	if (csEnglish && IsUpperCase(*original) && firstTry && numberkind != ROMAN_NUMBER) // someone capitalized things we think of as ordinary.
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
		if (canon && !canonical) canonical = canon;
		if (canonical) cansysflags = canonical->systemFlags;

		// german postag data marks all nouns without separating singular from plural
		if (isGerman && (properties & (NOUN_SINGULAR | NOUN_PLURAL))
			== (NOUN_SINGULAR | NOUN_PLURAL))
		{
			if (entry != canonical && canonical) // if we know canonical is different from us, we are plural
			{
				cansysflags ^= NOUN_PLURAL; // canonical is singular
				properties ^= NOUN_SINGULAR; // main word noun is plural
			}
			else
			{
				WORDP D = FindGermanPlural(entry);
				if (D) properties ^= NOUN_PLURAL;
			}
		}

		// possessive pronoun-determiner like my is always a determiner, not a pronoun. 
		if (entry->properties & (COMMA | PUNCTUATION | PAREN | QUOTE | POSSESSIVE | PUNCTUATION)) return properties;
	}
	bool known = KnownUsefulWord(entry);
	bool preknown = known;

	if (isSpanish)
	{
		if (!entry) entry = FindWord(original, 0, SECONDARY_CASE_ALLOWED); // Try harder to find the foreign word, e.g. German wochentag -> Wochentag
		if (entry && !properties) canonical = GetCanonical(entry);
#ifdef NEVER_RUN
		if (false && entry) // need dynamic pronoun data
		{
			sysflags = entry->systemFlags;
			return entry->properties;
		}
#endif 
		return ComputeSpanish(at, original, entry, canonical, sysflags,true);
	}

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
		if (X && strcmp(original,X->word) && (X->properties || X->systemFlags & PATTERN_WORD)) // upper case noun we know in lower case -- "Proceeds"
		{
			properties |= X->properties & NOUN_BITS;
			if (!entry) entry = canonical = X;
		}
		char* noun = GetSingularNoun(original, true, true);
		if (noun) // we know it's root as a noun
		{
			X = StoreWord(original, NOUN);
			if (!entry)
			{
				canonical = StoreWord(noun, NOUN | NOUN_SINGULAR);
				entry = X;
			}
			uint64 which;
			if (original[len-1] == 's') which = (X->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_PLURAL : NOUN_PLURAL;
			else which = (X->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_SINGULAR : NOUN_SINGULAR;
			AddProperty(X, which);
			properties |= NOUN | which;
		}
		else if (original[len-1] == 's')
		{
			if (noun && strcmp(noun,original)) 
			{
				X = StoreWord(original,NOUN);
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
			char* singnoun = GetSingularNoun(Y->word, false, true);
			if (Y->properties & NOUN && singnoun && !stricmp(Y->word, singnoun)) // number with singular noun cant be anything but adjective
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
				if (X && X->properties & ADJECTIVE_NORMAL)
				{
					adjective = GetAdjectiveBase(original,true); 
					if (adjective && strcmp(adjective,original)) // base is not the same
					{
						sprintf(word,(char*)"%s-%s",adjective,hyphen+1);
						canonical = StoreWord(word,ADJECTIVE_NORMAL|ADJECTIVE);
					}
				}
				*hyphen = '-';
			}
			if (!adjective && hyphen  && hyphen != original) // third-highest
			{
				adjective = GetAdjectiveBase(hyphen+1,true);
				if (adjective && strcmp(hyphen+1,adjective)) // base is not the same
				{
					*hyphen = 0;
					sprintf(tmpword,(char*)"%s-%s",original,adjective);
					canonical = StoreWord(tmpword,ADJECTIVE_NORMAL|ADJECTIVE);
					properties |= adjectiveFormat;
					*hyphen = '-';
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
					*hyphen = 0;
					sprintf(tmpword,(char*)"%s-%s",original,adverb);
					canonical = StoreWord(tmpword,ADJECTIVE_NORMAL|ADJECTIVE);
					properties |= adverbFormat;
					*hyphen = '-';
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
	if (canonical && entry) AddSystemFlag(entry,canonical->systemFlags & AGE_LEARNED); // copy age data across
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
            bool notGenerateAlt = nogenerate;
			if (IsUpperCase(*original)) MakeLowerCopy(alternate,original);
			else
			{
				strcpy(alternate,original);
				alternate[0] = toUppercaseData[alternate[0]];
                notGenerateAlt = true;  // don't want to create a speculative uppercase form if it doesn't already exist
			}
			WORDP D1,D2;
			WORDP xrevise;
			uint64 flags1 = GetPosData(at,alternate,xrevise,D1,D2,sysflags,cansysflags,false,notGenerateAlt,start);
			if (xrevise)
			{
				wordStarts[at] = xrevise->word;
				original = xrevise->word;
			}
			if (flags1) 
			{
				original = D1->word;
				if (xrevise) wordStarts[at] = xrevise->word;
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

	// german comparative and superlative adjectives
	if (isGerman && entry && canonical && entry->properties & ADJECTIVE &&
		!(entry->properties & (MORE_FORM | MOST_FORM)))
	{
		size_t entrylen = strlen(entry->word);
		// for superlative, add sten // billig -> billigsten
		if (len > 5 && entry->word[entrylen - 4] == 's'  && entry->word[entrylen - 3] == 's' && entry->word[entrylen - 2] == 'e'
		&& entry->word[entrylen - 1] == 'n')
		{
			entry->properties |= MOST_FORM;
			properties |= MOST_FORM;
		}	
		else
		{
			entry->properties |= MORE_FORM;
			properties |= MORE_FORM;
		}
	}
	// german comparative and superlative adverbs
	if (isGerman && entry && canonical && entry->properties & ADVERB &&
		!(entry->properties & (MORE_FORM | MOST_FORM)))
	{
		size_t entrylen = strlen(entry->word);
		// for superlative, add sten 
		if (entrylen > 5 && entry->word[entrylen - 4] == 's'  && entry->word[entrylen - 3] == 's' && entry->word[entrylen - 2] == 'e'
		&& entry->word[entrylen - 1] == 'n')
		{
			entry->properties |= MOST_FORM;
			properties |= MOST_FORM;
		}
		else
		{
			entry->properties |= MORE_FORM;
			properties |= MORE_FORM;
		}
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
		size_t ylen = strlen(original);
		if (IsUpperCase(*original) || IsUpperCase(original[1]) || (ylen > 2 && IsUpperCase(original[2])) ) // default it noun upper case
		{
			properties |= (original[ylen-1] == 's') ? NOUN_PROPER_PLURAL : NOUN_PROPER_SINGULAR;
			entry = StoreWord(original,0);
		}
		else // was lower case
		{
			properties |= (original[ylen-1] == 's') ? (NOUN_PLURAL|NOUN_SINGULAR) : NOUN_SINGULAR;
			entry = StoreWord(original,0);
		}
		properties |= NOUN;

		// default everything else
		bool mixed = (entry->internalBits & UPPERCASE_HASH) ? true : false;
		for (unsigned int i = 0; i < ylen; ++i) 
		{
			if (!IsAlphaUTF8(original[i]) && original[i] != '-' && original[i] != '_') 
			{
				mixed = true; // has non alpha in it
				break;
			}
		}
		if (ylen ==1 ){;}
		else if (!mixed || entry->internalBits & UTF8) // could be ANYTHING
		{
			properties |= VERB | VERB_PRESENT | VERB_INFINITIVE |  ADJECTIVE | ADJECTIVE_NORMAL | ADVERB | ADVERB;
			if (properties & VERB)
			{
				if (!strcmp(original+ylen-2,(char*)"ed")) properties |= VERB_PAST | VERB_PAST_PARTICIPLE;
				if (original[ylen-1] == 's') properties |= VERB_PRESENT_3PS;
			}
			uint64 expectedBase = 0;
			if (ProbableAdjective(original,ylen,expectedBase)) 
			{
				AddSystemFlag(entry,ADJECTIVE|expectedBase); // probable decode
				sysflags |= ADJECTIVE|expectedBase;
			}
			expectedBase = 0;
			if (ProbableAdverb(original,ylen,expectedBase)) 
			{
				AddSystemFlag(entry,ADVERB|expectedBase);		 // probable decode
				sysflags |= ADVERB|expectedBase;
			}
			if (ProbableNoun(original,ylen)) 
			{
				AddSystemFlag(entry,NOUN);			// probable decode
				sysflags |= NOUN;
			}
			uint64 vflags = ProbableVerb(original,ylen);
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
    if (IsModelNumber(original))  properties |= MODEL_NUMBER;
	if (!entry)
	{
        if (!properties && !firstTry && nogenerate) {;}  // don't store word unless have something
		else entry = StoreWord(original,properties);	 // nothing found (not allowed alternative)
		if (!canonical) canonical = DunknownWord;
	}
	if (!canonical) canonical = entry;
 
	if (at >= 0 && !nogenerate) AddProperty(entry, properties);
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
	if (entry && (*entry->word == '~' || *entry->word == '^' || *entry->word == USERVAR_PREFIX)) canonical = entry;	// not unknown, is self
	cansysflags |= canonical->systemFlags;
	if (entry) sysflags |= entry->systemFlags;

	// adjective noun?
	if (!compiling && properties & NOUN_BITS)
	{
		if (*wordStarts[at + 1] == '\'') properties |= ADJECTIVE_NOUN;
	}
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
		if (  tmpPrepareMode == POS_MODE || prepareMode == POSVERIFY_MODE  || prepareMode == POSTIME_MODE ) Log(USERLOG,"Not doing a parse.\r\n");
	}

	// assign sentence type
	if (!verbStack[MAINLEVEL] || !(roles[verbStack[MAINLEVEL]] &  MAINVERB)) // FOUND no verb, not a sentence
	{
		if ((trace & TRACE_POS || prepareMode == POS_MODE) && CheckTopicTrace()) Log(USERLOG,"Not a sentence\r\n");
		if (tokenFlags & (QUESTIONMARK|EXCLAMATIONMARK)) {;}
		else if (posValues[startSentence] & AUX_VERB
			&& !(posValues[startSentence + 1] & TO_INFINITIVE))
		{	// would have, could have, etc are not aux subject. Rather implied "I" for example before them
			if (posValues[startSentence + 1] & AUX_VERB || roles[startSentence + 1] & MAINOBJECT) 
				tokenFlags |= IMPLIED_SUBJECT; // includes "hit dog" "does nothing"
			else tokenFlags |= QUESTIONMARK; 
		}
		else if (allOriginalWordBits[startSentence]  & QWORD)
		{
            int i = startSentence;
            if (posValues[i + 1] & (ADVERB | ADJECTIVE)) ++i;
            if (i < wordCount && !stricmp(wordStarts[i + 1], "much")) ++i;
            // how
            if (posValues[i+1] & (VERB_BITS | AUX_VERB))
                tokenFlags |= QUESTIONMARK;
		}
		else if (allOriginalWordBits[startSentence] & PREPOSITION && allOriginalWordBits[startSentence+1] & QWORD) 
            tokenFlags |= QUESTIONMARK;
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
		if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] > verbStack[MAINLEVEL] && (allOriginalWordBits[startSentence] & QWORD || (allOriginalWordBits[startSentence] & PREPOSITION && allOriginalWordBits[startSentence+1] & QWORD)))  
            tokenFlags |= QUESTIONMARK; // qword + flipped order must be question (vs emphasis?)
	
		bool foundVerb = false;
        bool foundsubject2 = false;
        bool foundverb2 = false;
		for (i = startSentence; i <= endSentence; ++i) 
		{
			if (ignoreWord[i]) continue;
            if (roles[i] & SUBJECT2) foundsubject2 = true;
            if (roles[i] & VERB2) foundverb2 = true;
            if (roles[i] & MAINVERB) foundVerb = true;
			if (roles[i] & MAINSUBJECT) 
			{
				if (i == startSentence && originalLower[startSentence] && originalLower[startSentence]->properties & QWORD && (posValues[startSentence+1] & (VERB_BITS | AUX_VERB_TENSES))) 
                    tokenFlags |= QUESTIONMARK;
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
                if (foundsubject2 && foundverb2) { break; } // a clause, dont trust we understand
				else if (i == startSentence) // its a question because AUX or VERB comes before MAINSUBJECT
				{
					// would have, could have, etc are not aux subject. Rather implied "I" for example before them
					if (posValues[i + 1] & AUX_VERB || roles[i + 1] & MAINOBJECT)
						tokenFlags |= IMPLIED_SUBJECT; // includes "hit dog" "does nothing"
					else tokenFlags |= QUESTIONMARK;
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
	else if (!stricmp(wordStarts[start],(char*)"how") && !stricmp(wordStarts[start+1],(char*)"many")) 
        tokenFlags |= QUESTIONMARK; 
	else if (posValues[start] & AUX_VERB && (!(posValues[start] & AUX_DO) || !(allOriginalWordBits[start] & AUX_VERB_PRESENT))){;} // not "didn't leave today." ommited subject
	else if (roles[start] & MAINVERB && !stricmp(wordStarts[start],(char*)"assume")) tokenFlags |= COMMANDMARK|IMPLIED_YOU; // treat as sentence command
    // dont want "ate a cherry" to be a question so commented out rule
    //	else if (roles[start] & MAINVERB && (!stricmp(wordStarts[start+1],(char*)"you") || subjectStack[MAINLEVEL])) tokenFlags |= QUESTIONMARK;
	else if (roles[start] & MAINVERB && posValues[start] & VERB_INFINITIVE) tokenFlags |= COMMANDMARK|IMPLIED_YOU; 
	else if (posValues[start] & VERB_PAST) tokenFlags |= IMPLIED_SUBJECT;

    if (start > 1 && posValues[start] & VERB_PAST_PARTICIPLE)
    {
         if (aux[start - 1] == AUX_BE || aux[start - 2] == AUX_BE)
            roles[start] |= PASSIVE_VERB;
         else if (!stricmp(wordCanonical[start - 1], "get") || (start > 2 &&  !stricmp(wordCanonical[start - 2], "get") ))
             roles[start] |= PASSIVE_VERB;
         if (roles[start] & MAINVERB && roles[start] & PASSIVE_VERB)
            tokenFlags |= PASSIVE;
    }

	// determine sentence tense when not past from verb using aux (may pick wrong aux)
	for (i = start; i <= end; ++i)
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
			if (i == start && !(tokenControl & NO_INFER_QUESTION) && subjectFound) 
				tokenFlags |= QUESTIONMARK;
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
	if (!stricmp(wordStarts[start],(char*)"what") && posValues[start] & DETERMINER_BITS) 
		tokenFlags |= QUESTIONMARK; 
	if (!auxIndex && canonicalLower[start] && !stricmp(canonicalLower[start]->word,(char*)"be") && !(tokenControl & NO_INFER_QUESTION)) 
		tokenFlags |= QUESTIONMARK;  // are you a bank teller
	// How very american is NOT a question
	if (canonicalLower[start] && !stricmp(canonicalLower[start]->word,(char*)"how") && (tokenFlags & PERIODMARK || !mainVerb)){;} // just believe them
	else if ( canonicalLower[start] && canonicalLower[start]->properties & QWORD && canonicalLower[start+1] && canonicalLower[start+1]->properties & AUX_VERB  && !(tokenControl & NO_INFER_QUESTION))  
		tokenFlags |= QUESTIONMARK;  // what are you doing?  what will you do?
	else if ( posValues[start] & PREPOSITION && canonicalLower[start+1] && canonicalLower[start+1]->properties & QWORD && canonicalLower[start+2] && canonicalLower[start+2]->properties & AUX_VERB  && !(tokenControl & NO_INFER_QUESTION))   
		tokenFlags |= QUESTIONMARK;  // what are you doing?  what will you do?

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

		if ((aux[auxIndex-1] & AUX_BE || (canonicalLower[auxIndex-1] && !stricmp(canonicalLower[auxIndex-1]->word,(char*)"get"))) && mainverbTense & VERB_PAST_PARTICIPLE) // "he is lost" "he got lost"
		{
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

	if (!(tokenFlags & (IMPLIED_YOU | IMPLIED_SUBJECT)) && (!mainSubject | !mainVerb) ) tokenFlags  |= NOT_SENTENCE;
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
		int lenx = 0;
		for (int i = 0; i < cnt; ++i) //   get its components
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			lenx += lens[i];
			separators[i] = word[lenx++];
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
			char* infin = GetPresentParticiple(words[i]); //   is this word an infinitive?
			if (!infin) continue;
			*trial = 0;
			at = trial;
			for (int j = 0; j < cnt; ++j) //   rebuild word
			{
				if (j == i)
				{
					strcpy(at, infin);
					at += strlen(infin);
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
        WORDP G = FindWord(base,0,LOWERCASE_LOOKUP);
        if (G && G->properties & VERB && GetTense(G) ) 
        {
            int n = 9;
            while (--n && G)
            {
                if (G->properties & VERB_PAST) 
                {
                    unsigned int lenx = G->length;
                    if (G->word[lenx-1] != 'd' || G->word[lenx-2] != 'e' || lenx < 5) break; 
                    if (IsVowel(G->word[lenx-3])) break; //   we ONLY want to see if a final consonant is doubled. other things would just confuse us
                    strcpy(buffer,G->word);
                    buffer[lenx-2] = 0;      //   drop ed
                    strcat(buffer,(char*)"ing");   //   add ing
                    return buffer; 
                }
                G = GetTense(G); //   known irregular
            }
            if (!n) ReportBug((char*)"INFO: verb loop") //   complain ONLY if we didnt find a past tense
        }

        char lets[2];
        lets[0] = buffer[len-1];
        lets[1] = 0;
		if (G && G->systemFlags & SPELLING_EXCEPTION) {;} // dont double
        else strcat(buffer,lets);    //   double consonant
    }
    strcat(buffer,(char*)"ing");
    return buffer; 
}

WORDP SuffixAdjust(char* word, int lenword, char* suffix, int lensuffix,uint64 bits)
{
	char copy[MAX_WORD_SIZE];
	WORDP D;
	if (lensuffix  >= lenword) return NULL;
	if (stricmp(word + lenword - lensuffix, suffix)) return NULL; // not possible does not end
	strcpy(copy, word);

	if (!stricmp(suffix, "ing") && copy[lenword - lensuffix - 1] == 'k') // possible k replacing c or added after c
	{
        int lenprefix = lenword - lensuffix;
        D = FindWord(copy, lenprefix, LOWERCASE_LOOKUP); // banking
        if (D && D->properties & bits) return D;
        D = FindWord(copy, lenprefix, UPPERCASE_LOOKUP);
        if (D && D->properties & bits) return D;

        --lenprefix;
        D = FindWord(copy, lenprefix, LOWERCASE_LOOKUP); // bivouacking
		if (D && D->properties & bits) return D;
		D = FindWord(copy, lenprefix, UPPERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
	}
	if (!stricmp(suffix, "ed") && copy[lenword - lensuffix ] == 'e') // merely need to add d to word ending in e
	{
		D = FindWord(copy, lenword - lensuffix + 1, LOWERCASE_LOOKUP); // knifed
		if (D && D->properties & bits) return D;
		D = FindWord(copy, lenword - lensuffix + 1, UPPERCASE_LOOKUP); // Rollerbladed
		if (D && D->properties & bits) return D;
	}
	
	if (copy[lenword -lensuffix - 1] == copy[lenword - lensuffix- 2]) // doubled consonant?
	{
			D = FindWord(copy, lenword- lensuffix - 1, LOWERCASE_LOOKUP);
			if (D && D->properties & bits) return D;
			D = FindWord(copy, lenword - lensuffix - 1, UPPERCASE_LOOKUP);
			if (D && D->properties & bits) return D;
	}
	if (IsVowel(*suffix)) // drop silent e if suffix starts vowel
	{
		copy[lenword - lensuffix] = 'e';
		D = FindWord(copy, lenword - lensuffix + 1, LOWERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		D = FindWord(copy, lenword - lensuffix + 1, UPPERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		strcpy(copy, word);
	}

	if (copy[lenword - lensuffix - 1] == 'i') // trailing y changed to i for consonant suffix  babies from baby+s or rally to rallied
	{
		copy[lenword - lensuffix - 1] = 'y';
		D = FindWord(copy, lenword - lensuffix, LOWERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		D = FindWord(copy, lenword - lensuffix , UPPERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		strcpy(copy, word);
	}

	if (copy[lenword - lensuffix - 1] == 'y' ) // trailing ie changed to y - tyed from tie adding ed
	{
		copy[lenword - lensuffix -1 ] = 'i';
		copy[lenword - lensuffix] = 'e';	
		D = FindWord(copy, lenword - lensuffix + 1, LOWERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		D = FindWord(copy, lenword - lensuffix - 1, UPPERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		strcpy(copy, word);
	}

	if (!stricmp(suffix, "al") && copy[lenword-2] == 'a' && copy[lenword - 1] == 'l') // leave out the final’s’ before ‘al’, 		politics / political;
	{
		copy[lenword - lensuffix - 1] = 's';
		D = FindWord(copy, lenword - lensuffix + 1, LOWERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		D = FindWord(copy, lenword - lensuffix - 1, UPPERCASE_LOOKUP);
		if (D && D->properties & bits) return D;
		strcpy(copy, word);
	}

	if (copy[lenword - lensuffix - 1] == 'k' ) // possible k replacing c or added after c
	{
		D = FindWord(copy, lenword - lensuffix - 1, LOWERCASE_LOOKUP); // bivouacked
		size_t l = (D) ? strlen(D->word) : 0;
		if (D && D->properties & bits && D->word[l-1] == 'c') return D; // k added AFTER c
		
		copy[lenword - lensuffix - 1] = 'c';
		D = FindWord(copy, lenword - lensuffix, LOWERCASE_LOOKUP); 
		if (D && D->properties & bits) return D;
		strcpy(copy, word);
	}

	// pined_away -> pine first (above) not pin (below)
	D = FindWord(copy, lenword - lensuffix, LOWERCASE_LOOKUP); // direct attach, no change
	if (D && D->properties & bits) return D;
	D = FindWord(copy, lenword - lensuffix, UPPERCASE_LOOKUP); // direct attach, no change
	if (D && D->properties & bits) return D;

	char* hyphen;
	if (bits & VERB_BITS && (hyphen = strchr(copy, '-'))) // flow-through but not land-office_business
	{
		WORDP prep = FindWord(hyphen + 1);
		if (prep && prep->properties & PREPOSITION)
		{
			*hyphen = 0;
			WORDP X = SuffixAdjust(copy, hyphen - copy, suffix, lensuffix, bits);
			*hyphen = '-';
			if (X)
			{
				char newword[MAX_WORD_SIZE];
				sprintf(newword, "%s%s", X->word, hyphen);
				return StoreWord(newword, AS_IS | X->properties,X->systemFlags);
			}
		}
	}
	return NULL;
}

static bool ValidateVerb(char* word, int len, char* item, int itemlen, uint64 bits)
{
	return SuffixAdjust(word, len, item, itemlen,VERB_BITS);
}

static WORDP PrefixWord(char* word, int len)
{
	if (!strnicmp(word, "anti", 4)) word += 4;
	else if (!strnicmp(word, "de", 2)) word += 2;
	else if (!strnicmp(word, "dis", 3)) word += 3;
	else if (!strnicmp(word, "ex", 2)) word += 2;
	else if (!strnicmp(word, "il", 2)) word += 2;
	else if (!strnicmp(word, "im", 2)) word += 2;
	else if (!strnicmp(word, "in", 2)) word += 2;
	else if (!strnicmp(word, "mis", 3)) word += 3;
	else if (!strnicmp(word, "non", 3)) word += 3;
	else if (!strnicmp(word, "over", 4)) word += 4;
	else if (!strnicmp(word, "pre", 3)) word += 3;
	else if (!strnicmp(word, "pro", 3)) word += 3;
	else if (!strnicmp(word, "re", 2)) word += 2;
	else if (!strnicmp(word, "sub", 3)) word += 3;
	else if (!strnicmp(word, "tri", 3)) word += 3;
	else if (!strnicmp(word, "un", 2)) word += 2;
	else if (!strnicmp(word, "with", 4)) word += 4;
	else return NULL;
	WORDP D = FindWord(word);
	if (D && D->properties & (NOUN_BITS | VERB_BITS | ADJECTIVE_BITS | ADVERB))
		return D;
	return NULL;
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
	int i;
	if (len >= 8) // word of 3 + suffix of 5
	{
		i = -1;
		while ((item = verb5[++i].word))
		{
			if (ValidateVerb(word, len,item, 5,0))
				return verb5[i].properties;
		}
	}	
	if (len >= 7) // word of 3 + suffix of 4
	{
		i = -1;
		while ((item = verb4[++i].word))
		{
			if (ValidateVerb(word, len,item,  4,0))
				return verb4[i].properties;
		}
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		i = -1;
		while ((item = verb3[++i].word))
		{
			if (ValidateVerb(word,  len,item, 3,0))
				return verb3[i].properties;
		}
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		i = -1;
		while ((item = verb2[++i].word))
		{
			if (ValidateVerb(word, len, item,  2,0))
				return verb2[i].properties;
		}
	}

	WORDP X = PrefixWord(word, len);
	if (X) return X->properties & (VERB_BITS | NOUN_BITS | ADJECTIVE_BITS | ADVERB);
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

#pragma warning(disable: 4068)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static WORDP GetInfinitiveCasing(char* word, int len)
{
	WORDP D = FindWord(word, len, LOWERCASE_LOOKUP);    
	if (D && D->properties & VERB_INFINITIVE) return D;
	D = FindWord(word, len, UPPERCASE_LOOKUP);
	if (D && D->properties & VERB_INFINITIVE) return D; // rare verbs that are upper case like "Charleston"
	return NULL;
}
#pragma GCC diagnostic pop

char* GetInfinitive(char* word, bool nonew)
{
	if (!word || !*word || *word == '~') return NULL;
	if (strchr(word,'/')) return NULL;  // not possible verb
 	uint64 controls = tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : LOWERCASE_LOOKUP;
	verbFormat = 0;	//   secondary answer- std infinitive or unknown
    size_t len = strlen(word);
    if (len == 0) return NULL;
    WORDP D = FindWord(word,len, LOWERCASE_LOOKUP);
	if (!D || !(D->properties & VERB)) D = FindWord(word, len, UPPERCASE_LOOKUP); // like to Charleston

	if (D && (!IS_NEW_WORD(D)) && D->properties & VERB_INFINITIVE)
	{
		verbFormat = VERB_INFINITIVE| VERB_PRESENT;  // fall  (fell) conflict -- note that find->found  but found is also an infinitive!
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

    //   check for multiword behavior. 
	int cnt = BurstWord(word,HYPHENS);
    if (cnt > 1)
    {
		char trial[MAX_WORD_SIZE];
        char words[10][MAX_WORD_SIZE];
		unsigned int lens[10];
		char separators[10];
		if (cnt > 9) return NULL;
		unsigned int lenx = 0;
		for (int i = 0; i < cnt; ++i) //   get its components  dash-it
		{
			strcpy(words[i],GetBurstWord(i));
			lens[i] = strlen(words[i]);
			lenx += lens[i];
			if ((i+1) == cnt) separators[i] = 0; // no final separator
			else separators[i] = word[lenx++];
		}
		if (cnt < 4) for (int i = 0; i < cnt; ++i) // only handle 3 word units (cozy_up_to), since we might create a 3 word unit
		{
			char* inf = GetInfinitive(words[i],nonew); //   is this word an infinitive?
			if (!inf || !*inf) continue;

			// if 1st word is verb and noun, we want later word to be preposition (`bomb`-disposal not good)
			if (i == 0)
			{
				bool prep = false;
				WORDP Y = FindWord(inf);
				if (Y && Y->properties & NOUN) 
				{
					for (int j = 1; j < cnt; ++j)
					{
						WORDP X = FindWord(words[j]);
						if (X && X->properties & PREPOSITION) prep = true;
					}
				}
				if (!prep) continue; // try rest of word
			}

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
			WORDP G = FindWord(trial);
			if (G) return G->word;
		}
    }

	// verb conjugations are to add d, s, or ing for regular verbs, with some suffixification
	WORDP Z = NULL;
	Z = SuffixAdjust(word, len, "ing", 3, VERB_INFINITIVE); // actual word like sing wont end up here.
	if (Z)
	{
		verbFormat = VERB_PRESENT_PARTICIPLE | ADJECTIVE_PARTICIPLE|NOUN_GERUND;
		return Z->word;
	}
	Z = SuffixAdjust(word, len, "ed", 2, VERB_INFINITIVE); // ed
	if (Z )
	{
		verbFormat = VERB_PAST_PARTICIPLE | VERB_PAST| ADJECTIVE_PARTICIPLE;
		return Z->word;
	}
	Z = SuffixAdjust(word, len, "s", 1, VERB_INFINITIVE); // s
	if (Z)
	{
		verbFormat = VERB_PRESENT_3PS;
		return Z->word;
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

char* GetPluralNoun(char* noun,char* plu)
{
	if (!noun) return NULL;
	WORDP D = FindWord(noun);
    if (D && D->properties & (NOUN_PLURAL | NOUN_PROPER_PLURAL)) return D->word; 
    WORDP plural = GetPlural(D);
     if (plural) return plural->word;

	 char* nounbase = GetSingularNoun(noun, false, true);
	 if (nounbase && stricmp(noun,nounbase)) return noun; // already is plural

	 char* underscore = strrchr(noun, '_');
	 if (underscore)
	 {
		 char extra[MAX_WORD_SIZE];
		 char* plur = GetPluralNoun(underscore + 1, extra);
		 if (plur)
		 {
			 strcpy(plu, noun);
			 strcpy(plu + (underscore - noun + 1), plur);
			 return plu;
		 }
	 }

		unsigned int len = strlen(noun);
		char end = noun[len-1];
		char before = (len > 1) ? (noun[len-2]) : 0;
		if (end == 's') sprintf(plu,(char*)"%ses",noun); // glass -> glasses
		else if (end == 'h' && (before == 'c' || before == 's')) sprintf(plu,(char*)"%ses",noun); // witch -> witches
		else if ( end == 'o' && !IsVowel(before)) sprintf(plu,(char*)"%ses",noun); // hero -> heroes>
		else if ( end == 'y' && !IsVowel(before)) // cherry -> cherries
		{
			if (D && D->internalBits & UPPERCASE_HASH) sprintf(plu,(char*)"%ss",noun); // Germany->Germanys
			else
			{
				strncpy(plu,noun,len-1);
				strcpy(plu +len-1,(char*)"ies");
			}
		}
		else sprintf(plu,(char*)"%ss",noun);
        return plu;
}

static char* InferNoun(char* original,unsigned int len) // from suffix might it be singular noun? If so, enter into dictionary
{
	if (len == 0) len = strlen(original);
	static char word[MAX_WORD_SIZE];
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

	char* underscore = strrchr(word, '_'); // locate last words
	if (underscore)
	{
		WORDP X = FindWord(underscore+1, 0, LOWERCASE_LOOKUP);
		if (X && X->properties & NOUN_BITS) return word;

		return NULL;		// dont apply suffix to multiple word stuff
	}

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

static bool ValidateNoun(char* word, int len, char* item, int itemlen,uint64 bits)
{
	return SuffixAdjust(word, len, item, itemlen,bits);
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
	int i;
	if (len >= 10) // word of 3 + suffix of 7
	{
		i = -1;
		while ((item = noun7[++i].word))
		{
			if (ValidateNoun(word, len, item, 7,0))
				return noun7[i].properties;
		}
	}	
	if (len >= 9) // word of 3 + suffix of 6
	{
		i = -1;
		while ((item = noun6[++i].word))
		{
			if (ValidateNoun(word, len, item, 6,0))
				return noun6[i].properties;
		}
	}	
	if (len >= 8) // word of 3 + suffix of 5
	{
		i = -1;
		while ((item = noun5[++i].word)) 
		{
			if (ValidateNoun(word, len, item,  5,0))
				return noun5[i].properties;
		}
	}	
	if (len >= 7) // word of 3 + suffix of 4
	{
		i = -1;
		while ((item = noun4[++i].word)) 
		{
			if (ValidateNoun(word, len,item, 4,0))
				return noun4[i].properties;
		}
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		i = -1;
		while ((item = noun3[++i].word)) 
		{
			if (ValidateNoun(word,len, item, 3,VERB_BITS))  // insist root is found since suffix is short
				return noun3[i].properties;
		}
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		i = -1;
		while ((item = noun2[++i].word)) 
		{
			if (ValidateNoun(word, len,item,2,VERB_BITS)) // insist root is found since suffix is short
				return noun2[i].properties;
		}
	}

	// ings (plural of a gerund like paintings)
	if (len > 6 && !stricmp(word+len-4,(char*)"ings")) return NOUN|NOUN_SINGULAR;
		
	// ves (plural form)
	if (len > 4 && !strcmp(word+len-3,(char*)"ves") && IsVowel(word[len-4])) return NOUN|NOUN_PLURAL; // knife
	
	WORDP X = PrefixWord(word, len);
	if (X) return X->properties & (VERB_BITS | NOUN_BITS | ADJECTIVE_BITS | ADVERB);

	return 0;
}

char* GetSingularNoun(char* word, bool initial, bool nonew)
{ 
 	if (!word || !*word || *word == '~') return NULL;
	uint64 controls = PRIMARY_CASE_ALLOWED;
	nounFormat = 0;
    size_t len = strlen(word);
    WORDP D = FindWord(word,0,controls);
	if (IS_NEW_WORD(D) || !(D->properties & NOUN_BITS)) D = NULL;
	nounFormat = NOUN_SINGULAR;
	if (D && D->properties & NOUN_PROPER_SINGULAR) //   is already singular
	{
		nounFormat = NOUN_PROPER_SINGULAR;
		return D->word;
	}

	char* underscore = strrchr(word, '_');
	if (!D && underscore)
	{
		WORDP X = FindWord(underscore + 1);
		if (X)
		{
			char* sing = GetSingularNoun(X->word, initial, nonew);
			if (sing)
			{
				static char buffer[MAX_WORD_SIZE];
				strcpy(buffer, word);
				strcpy(buffer + (underscore - word + 1), sing);
				return buffer;
			}
			if (X->properties & (NOUN_PLURAL | NOUN_PROPER_PLURAL))
				return NULL; // this is plural, we couldnt get a singular
		}

		D = NULL;
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
        unsigned int lenx = D->length;
        if (lenx > 4 && D->word[lenx-1] == 's')
        {
            WORDP F = FindWord(D->word,lenx-1,controls);
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

	// check for multiword behavior.   eg bomb-disposal  
	int cnt = BurstWord(word, HYPHENS);
	if (cnt > 1 && cnt < 4) // noun will be at end, with modifiers before
	{
			char* noun = GetSingularNoun(GetBurstWord(cnt - 1), false, nonew); //   is this trailing word a noun?
			if (noun) return word;
	}

	if (xbuildDictionary && D && D->properties & NOUN_BITS) return D->word;
	if ( nonew ) return NULL;

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
	if (IS_NEW_WORD(D)) D = NULL;
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

	// prefix un or in
	if (word[1] == 'n' && (word[0] == 'i' || word[0] == 'u')) // unsinkable from sinkable, incoherent from coherent 
	{
		WORDP D = FindWord(word + 2, len - 2, LOWERCASE_LOOKUP);
		if (D && D->properties & ADJECTIVE_BITS) return ADJECTIVE_NORMAL;
	}

	int i;
	char* item;
	if (len >= 10) // word of 3 + suffix of 7
	{
		i = -1;
		while ((item = adjective7[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 7, NOUN_BITS|VERB_BITS))
				return adjective7[i].properties;
		}
	}
	if (len >= 9) // word of 3 + suffix of 6
	{
		i = -1;
		while ((item = adjective6[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 6, NOUN_BITS | VERB_BITS))
				return adjective6[i].properties;
		}
	}
	if (len >= 8) // word of 3 + suffix of 5
	{
		i = -1;
		while ((item = adjective5[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 5, NOUN_BITS | VERB_BITS))
				return adjective5[i].properties;
		}
	}
	if (len >= 7) // word of 3 + suffix of 4
	{
		i = -1;
		while ((item = adjective4[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 4, NOUN_BITS | VERB_BITS))
				return adjective4[i].properties;
		}
	}
	if (len >= 6) // word of 3 + suffix of 3
	{
		i = -1;
		while ((item = adjective3[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 3, NOUN_BITS | VERB_BITS))
				return adjective3[i].properties;
		}
	}
	if (len >= 5) // word of 3 + suffix of 2
	{
		i = -1;
		while ((item = adjective2[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 2, NOUN_BITS | VERB_BITS))
				return adjective2[i].properties;
		}
	}
	if (len >= 4) // word of 3 + suffix of 1
	{
		i = -1;
		while ((item = adjective1[++i].word)) 
		{
			if (ValidateNoun(word, len, item, 2, NOUN_BITS | VERB_BITS))
				return adjective1[i].properties;
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
	WORDP X = PrefixWord(word, len);
	if (X) return X->properties & (VERB_BITS | NOUN_BITS | ADJECTIVE_BITS | ADVERB);

	return 0;
}

char* GetAdjectiveBase(char* word, bool nonew)
{
 	if (!word || !*word || *word == '~') return NULL;
	uint64 controls = tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : LOWERCASE_LOOKUP;
	adjectiveFormat = 0;
    size_t len = strlen(word);
    WORDP D = FindWord(word,0,controls);
	if (IS_NEW_WORD(D)  ) D = NULL;
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
