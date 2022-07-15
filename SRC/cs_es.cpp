#include "common.h"

void MarkSpanishTags(WORDP OL, int i)
{ // we have to manual name concept because primary naming is from the english corresponding bit
		if (finalPosValues[i] & PRONOUN_OBJECT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_OBJECT_SINGULAR) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_singular")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_OBJECT_PLURAL) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_plural")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_OBJECT_I) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_i")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_OBJECT_YOU) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_you")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_INDIRECTOBJECT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_INDIRECTOBJECT_SINGULAR) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_singular")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_INDIRECTOBJECT_PLURAL) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_plural")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_INDIRECTOBJECT_I) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_i")), i, i, CANONICAL);
		if (OL->systemFlags & PRONOUN_INDIRECTOBJECT_YOU) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_you")), i, i, CANONICAL);
		if (finalPosValues[i] & SPANISH_FUTURE || finalPosValues[i] & AUX_VERB_FUTURE)
		{
			MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~spanish_future")), i, i, CANONICAL);
			MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~verb_future")), i, i, CANONICAL);
		}
		if (allOriginalWordBits[i] & SPANISH_HE) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~spanish_he")), i, i, CANONICAL);
		if (allOriginalWordBits[i] & SPANISH_SHE) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~spanish_she")), i, i, CANONICAL);
		if (allOriginalWordBits[i] & SPANISH_SINGULAR) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~spanish_singular")), i, i, CANONICAL);
		if (allOriginalWordBits[i] & SPANISH_PLURAL) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~spanish_plural")), i, i, CANONICAL);
		if (OL->systemFlags & VERB_IMPERATIVE) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~verb_imperative")), i, i, CANONICAL);
}


uint64 GetSpanishLemmaBits(char* wordx)
{
	WORDP D = FindWord(wordx, 0, PRIMARY_CASE_ALLOWED);
	if (!D) return 0;

	WORDP E = RawCanonical(D);
	if (!E || *E->word != '`') return 0; // normal english canonical or simple canonical of foreign (not multiple choice)

	uint64 properties = D->properties;
	char word[MAX_WORD_SIZE];
	char type[MAX_WORD_SIZE];
	strcpy(word, E->word + 1);
	char* lemma = word;
	char* tags = strrchr(word, '`');
	*tags++ = 0; // type decriptors
	// walk list of pos tags and simultaneously walk the list of lemmas
	while (*tags && (tags = ReadCompiledWord(tags, type)))
	{
		char ptag[MAX_WORD_SIZE];
		sprintf(ptag, "_%s", type);
		WORDP X = FindWord(ptag);
		if (X) properties |= X->properties;
		tags = SkipWhitespace(tags);
	}
	return properties;
}


uint64 FindSpanishSingular(char* original, char* word, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	WORDP E = FindWord(word);
	if (E && E->properties & NOUN)
	{
		canonical = E;
		if (!exportdictionary)
		{
			entry = StoreWord(original, AS_IS);
			AddProperty(entry, NOUN | NOUN_PLURAL);
		}

		else entry = canonical;
		sysflags = E->systemFlags;
		return NOUN | NOUN_PLURAL;
	}
	return 0;
}

#ifdef NOT_USED
static bool CheckSpanishLemma(char* lemma, size_t lemmaLen, char* original, char* correction, WORDP& entry, WORDP& canonical, uint64 properties, uint64 sysflags)
{// is this a lemma word? (infinitive form of verb)
	WORDP E = FindWord(lemma);
	if (E && E->properties & VERB)
	{
		if (correction) strcpy(original + lemmaLen, correction); // revise suffix lacking proper accent
		canonical = E;
		if (!exportdictionary)
		{
			entry = StoreWord(original, AS_IS);
			AddProperty(entry, VERB | properties);
			AddSystemFlag(entry, sysflags);
		}
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
		if (!strncmp(copy + len, "\xC3\xA1", 2)) copy[len] = 'a'; // change to unaccented form
		else if (!strncmp(copy + len, "\xC3\xA9", 2))  copy[len] = 'e'; // change to unaccented form
		else if (!strncmp(copy + len, "xC3\xAD", 2)) copy[len] = 'i'; // change to unaccented form
		else if (!strncmp(copy + len, "xC3\xB3", 2)) copy[len] = 'o'; // change to unaccented form
		else if (!strncmp(copy + len, "xC3\xBA", 2)) copy[len] = 'u'; // change to unaccented form
		else continue;
		memmove(copy + len + 1, copy + len + 2, strlen(copy + len + 1)); // remove extra utf8 byte
		++count;
		WORDP F = FindWord(copy); // lemma sans the extra accent (last or next to last syllable)
		if (E && E->properties & VERB)
		{
			canonical = E;
			if (!exportdictionary)
			{
				entry = StoreWord(original, AS_IS);
				AddProperty(entry, VERB | properties);
				AddSystemFlag(entry, sysflags);
			}
			else entry = canonical; // testing use where we dont want to create the word
			return true;
		}
	}
	return false;
}
#endif // NOT_USED

static WORDP PermuteSpanishInfinitive(char* lemma, char* verbType, int stemLen, char* original, const char* suffix, char* rawlemma = NULL);
static WORDP PermuteSpanishInfinitive(char* lemma, char* verbType, int stemLen, char* original, const char* suffix, char* rawlemma)
{
	char word[MAX_WORD_SIZE];
	strcpy(word, lemma);
	char rawword[MAX_WORD_SIZE];
	if (rawlemma) strcpy(rawword, rawlemma);
	else strcpy(rawword, lemma);
	WORDP E;
	if (*verbType) // we know the kind of verb
	{
		strcpy(word + stemLen, verbType); // make lemma from stem and type
		E = FindWord(word);
		if (E && E->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return E; // we would get this
		}
	}
	else // try all 3 verb types
	{
		strcpy(word + stemLen, "er");
		E = FindWord(word);
		if (E && E->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return E; // we would get this
		}
		strcpy(word + stemLen, "ar");
		E = FindWord(word);
		if (E && E->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return E; // we would get this
		}
		strcpy(word + stemLen, "ir");
		E = FindWord(word);
		if (E && E->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return E; // we would get this
		}
	}
	return NULL;
}

static WORDP FindSpanishInfinitive(char* original, const char* suffix, char* verbType, const char* correction, WORDP& entry, WORDP& canonical, uint64 properties, uint64 sysflags)
{
	// Infinitives always end in ar, er or ir added to the stem - verbType names this suffix. 
	// If empty, means try all 3 types
	// Conjugation involves changing to different suffix on stem verb
	// So, given original and a suffix to consider, we know the stem part before that suffix.
	// Of course the stem may have changed spelling along the way, for added complexity.

	size_t originalLen = strlen(original);
	size_t suffixLen = strlen(suffix);
	size_t stemLen = originalLen - suffixLen;
	if (strcmp(original + stemLen, suffix)) 	return NULL; // this conjugation doesnt end with suffix named
	WORDP X = NULL;

	// original ends in designated suffix. See if we can get its infinitive form
	char stem[MAX_WORD_SIZE];
	strncpy(stem, original, stemLen);
	char lemma[MAX_WORD_SIZE];
	stem[stemLen] = 0;
	strcpy(lemma, stem);
	X = PermuteSpanishInfinitive(lemma, verbType, stemLen, original, suffix);

	// didnt have direct match, but sometimes stem changes spelling  

	// if from pronoun, may have to remove added accent
	char lemma1[MAX_WORD_SIZE];
	strcpy(lemma1, stem);
	char c = 'a';
	char* found = strstr(lemma1, "\xC3\xA1");
	if (!found) { found = strstr(lemma1, "\xC3\xA9"); c = 'e'; }
	if (!found) { found = strstr(lemma1, "\xC3\xAD"); c = 'i'; }
	if (!found) { found = strstr(lemma1, "\xC3\xB3"); c = 'o'; }
	if (!found) { found = strstr(lemma1, "\xC3\xBA"); c = 'u'; }
	if (!X && found) // replace accented form with unaccented
	{
		*found = c;
		memmove(found + 1, found + 2, strlen(found + 1)); // know its 2 chars wide
		X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix);
	}

	// irregular verbs - 10 most common
	//	ser – “to be”
	//	haber – auxiliary “to be / to have”
	//	estar – “to be”
	//	tener – “to have”
	//	ir – “to go”
	//	saber – “to know”
	//	dar – “to give”
	//	hacer – “to make”
	//	poder – “to can / to be able to”
	//	decir – “to say”
	// 
	// stem changing verbs alter letters of the stem

	// irregular verb stem is the infinitive used directly in future tense  (estar)
	strcpy(lemma1, stem);
	strcat(lemma1, suffix);
	if (properties & SPANISH_FUTURE)
	{
		X = FindWord(stem);
		if (X && X->properties & VERB && !stricmp(lemma1, original)) {
			;
		}
		else X = NULL;
	}

	// stem changed from e to ie
	strcpy(lemma1, stem);
	char* change = strstr(lemma1, "ie");
	char* again = (change) ? strstr(change + 1, "ie") : NULL;
	if (again) change = again; // prefer 2nd one
	if (!X && change) // ie from e
	{
		memmove(change, change + 1, strlen(change));
		X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	// stem changed from o to ue
	strcpy(lemma1, stem);
	change = strstr(lemma1, "ue");
	again = (change) ? strstr(change + 1, "ue") : NULL;
	if (!X && change) // ue from o
	{
		memmove(change, change + 1, strlen(change));
		*change = 'o';
		X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	// stem changed from e to i
	if (!strcmp(verbType, "ir")) // ir verb i from e 
	{
		strcpy(lemma1, stem);
		char* change = strrchr(lemma1, 'i');
		if (!X && change) // i from e
		{
			*change = 'e';
			X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		}
	}
	else if (!stricmp(verbType, "er"))
	{
		strcpy(lemma1, stem);
		char* change = strstr(lemma1, "ie");
		char* again = (change) ? strstr(change + 1, "ie") : NULL;
		if (again) change = again;
		if (!X && change) // ie from e
		{
			memmove(change, change + 1, strlen(change));
			X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		}

		// stem o changed to ue
		strcpy(lemma1, stem);
		change = strstr(lemma1, "ue");
		again = (change) ? strstr(change + 1, "ue") : NULL;
		if (!X && change) // ue from o
		{
			memmove(change, change + 1, strlen(change));
			*change = 'o';
			X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		}
	}

	// stem changed from c to b (hacer conditional)
	strcpy(lemma1, stem);
	change = strstr(lemma1, "b");
	if (!X && change) // b from c
	{
		memmove(change, change + 1, strlen(change));
		*change = 'c';
		X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	// IRREGULAR VERB SPECIALS 
	// 
	// stem changed from c to r (hacer future)
	strcpy(lemma1, stem);
	change = strstr(lemma1, "r");
	if (!X && change && properties & SPANISH_FUTURE) // r from c
	{
		memmove(change, change + 1, strlen(change));
		*change = 'c';
		X = PermuteSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	if (X)
	{// since we found infinitive for it, assign info to the word
		entry = StoreWord(original, AS_IS);
		AddProperty(entry, properties);
		AddSystemFlag(entry, sysflags);
		canonical = X;
		return X;
	}
	return NULL;
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
	else if (FindSpanishInfinitive(original, "e", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR))
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
	else if (FindSpanishInfinitive(original, "emos", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "imos", "ir", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_I))
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
	else if (FindSpanishInfinitive(original, "áis", "er", NULL, entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
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
	else if (FindSpanishInfinitive(original, "is", "ir", "\xC3\xADs", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU))
	{
		return true;
	}
	// 3rd person plural ellos, ellas, ustedes // ellos/ellas	(habl)an	(com)en	(abr)en
	if (FindSpanishInfinitive(original, "an", "ar", NULL, entry, canonical, properties, PRONOUN_PLURAL))
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
		AddSystemFlag(entry, VERB_IMPERATIVE);
		return true;
	}
	else if (FindSpanishInfinitive(original, "es", "er", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		AddSystemFlag(entry, VERB_IMPERATIVE);
		return true;
	}
	else if (FindSpanishInfinitive(original, "es", "ir", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_YOU))
	{
		AddSystemFlag(entry, VERB_IMPERATIVE);
		return true;
	}

	return false;
}

uint64 PastSpanish(char* original, WORDP& entry, WORDP& canonical)
{
	uint64 properties = PRONOUN_SUBJECT | VERB_PAST;
	// 1st person  preterite  past //		yo				(habl)é			(com)í			(abr)í
	if (FindSpanishInfinitive(original, "\xC3\xBA", "ar", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "e", "ar", " ú", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) // user misspell
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
	else if (FindSpanishInfinitive(original, "iais", "ir", "íais", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // 	USER MISSPELL
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
	uint64 properties = PRONOUN_SUBJECT | AUX_VERB_FUTURE | SPANISH_FUTURE;
	// 1st person //yo(hablar)é   all 3 verb types
	// simple future
	if (FindSpanishInfinitive(original, "é", "", NULL, entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I))
	{
		return true;
	}
	else if (FindSpanishInfinitive(original, "e", "", "ú", entry, canonical, properties, PRONOUN_SINGULAR | PRONOUN_I)) // 	USER MISSPELL
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
	else if (FindSpanishInfinitive(original, "a", "", "á", entry, canonical, properties, PRONOUN_SINGULAR))// 	USER MISSPELL
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
	else if (FindSpanishInfinitive(original, "ais", "", "áis", entry, canonical, properties, PRONOUN_PLURAL | PRONOUN_YOU)) // 	USER MISSPELL
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
	char* ptr = copy-1;
	char c = 0;
	while (*++ptr)
	{
		if (!strnicmp(ptr, "\xC3\xA1 ", 2)) c = 'a';
		else if (!strnicmp(ptr, "\xC3\xA9", 2)) c = 'e';
		else if (!strnicmp(ptr, "\xC3\xAD", 2)) c = 'i';
		else if (!strnicmp(ptr, "\xC3\xB3", 2)) c = 'o';
		else if (!strnicmp(ptr, "\xC3\xBA", 2)) c = 'u';
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
		(!stricmp(word + len - 3, "ía") || !stricmp(word + len - 3, "ia")
			|| !stricmp(word + len - 2, "ua") || !stricmp(word + len - 3, "úa") || !stricmp(word + len - 2, "ez")))
	{
		properties |= SPANISH_SHE;
	}
	else if (len > 2 &&
		(!stricmp(word + len - 2, "or") || !stricmp(word + len - 3, "ío") || !stricmp(word + len - 3, "io")
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
		(!stricmp(word + len - 3, "aje") || !stricmp(word + len - 4, "zón") || !stricmp(word + len - 3, "aro") || !stricmp(word + len - 3, "emo")
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
	else if (len > 4 && (!stricmp(word + len - 5, "cíon") || !stricmp(word + len - 5, "síon")
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
	else if (ending == 'o') properties |= SPANISH_HE;
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
			AddProperty(D, NOUN | SPANISH_PLURAL | gender);
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
	char root = canonical->word[lenc - 1];

	// plurality
	if (IsVowel(root) && ending == 's')
	{
		properties |= SPANISH_PLURAL;
	}
	else if (!IsVowel(root) && ending == 's' && end2 == 'e')
	{
		properties |= SPANISH_PLURAL;
	}
	else
	{
		properties |= SPANISH_SINGULAR;
	}

	// gender
	properties |= ComputeSpanishNounGender(D);

	if (properties) AddProperty(D, properties);
	return D->properties;
}

uint64 ComputeSpanishAdjective(char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
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
		properties |= SPANISH_HE | SPANISH_SHE | SPANISH_PLURAL;
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
	else if (!stricmp(canonical->word + len1 - 2, "ora") || !stricmp(canonical->word + len1 - 2, "or") || !stricmp(canonical->word + len1 - 3, "ón") || !stricmp(canonical->word + len1 - 3, "ín"))
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
	else if (root == 'a')
	{
		properties |= SPANISH_SHE;
		properties |= (ending == 's') ? SPANISH_PLURAL : SPANISH_SINGULAR;
	}
	//1) Masculine singular is the default form and usually ends in - o. This changes to - a for feminine singular.
	else if (ending == 'o' || (!IsVowel(ending) && ending != 's'))
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
	if (properties) AddProperty(D, properties | ADJECTIVE | ADJECTIVE_NORMAL);
	return D->properties;
}

uint64 SpanishPronounIndirectObjects(char* word, int at, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags) // case sensitive, may add word to dictionary, will not augment flags of existing words
{
	// spanish indirect objects: me te le nos os les  (note le and les are different from directobjects, rest same)
	// attach the indirect object pronoun to an infinitive or a present participle.
	// spanish direct objects: me, te, lo, la in the singular, and nos, os, los, las in the plural.
	char base[MAX_WORD_SIZE];
	strcpy(base, word);
	int len = strlen(base);
	if (len < 3) {}
	else if (!stricmp(base + len - 3, "nos") || !stricmp(base + len - 3, "les"))
	{
		base[len - 3] = 0;
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND )) // need to compute participles in some regular verbs
		{
			uint64 flags = PRONOUN_INDIRECTOBJECT_PLURAL | PRONOUN_INDIRECTOBJECT;
			if (word[len - 3] == 'n') flags |= PRONOUN_INDIRECTOBJECT_I;
			// no you its the singular informal
			entry = StoreWord(word, AS_IS);
			AddProperty(entry, D->properties | PRONOUN_OBJECT);
			AddSystemFlag(entry, flags);
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
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS)) // need to compute participles in some regular verbs
		{
			uint64 flags = PRONOUN_INDIRECTOBJECT_SINGULAR | PRONOUN_INDIRECTOBJECT;
			if (word[len - 2] == 'm') flags |= PRONOUN_INDIRECTOBJECT_I;
			else if (word[len - 2] == 't') flags |= PRONOUN_INDIRECTOBJECT_YOU;
			entry = StoreWord(word, AS_IS);
			AddProperty(entry, D->properties | PRONOUN_OBJECT);
			AddSystemFlag(entry, flags);
			canonical = D;
			return entry->properties;
		}
	}
	return 0;
}

uint64 SpanishPronounDirectObjects(char* word, int at, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	char base[MAX_WORD_SIZE];
	strcpy(base, word);
	int len = strlen(base);
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
			entry = StoreWord(word, AS_IS);
			AddProperty(entry, D->properties | PRONOUN_OBJECT);
			AddSystemFlag(entry, flags);
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
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS))
		{
			uint64 flags = PRONOUN_OBJECT_SINGULAR;
			if (word[len - 2] == 'm') flags |= PRONOUN_OBJECT_I;
			else if (word[len - 2] == 't') flags |= PRONOUN_OBJECT_YOU;
			entry = StoreWord(word, AS_IS);
			AddProperty(entry, D->properties | PRONOUN_OBJECT);
			AddSystemFlag(entry, flags);
			return entry->properties;
		}
	}
	return 0;
}

uint64 ComputeSpanishVerb(int at, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags) // case sensitive, may add word to dictionary, will not augment flags of existing words
{
	WORDP D = NULL;
	uint64 properties = 0;
	char word[MAX_WORD_SIZE];
	strcpy(word, original);
	MakeLowerCase(word);

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
			AddSystemFlag(entry, PRONOUN_REFLEXIVE);
			if (E->properties & VERB_INFINITIVE) RemoveProperty(E, VERB_INFINITIVE); // treetagger lied - direct change, not restorable
			AddProperty(E, VERB_PRESENT); // treetagger lied - direct change, not restorable
			if (entry->properties & VERB_INFINITIVE)  RemoveProperty(entry, VERB_INFINITIVE); // treetagger lied - direct change, not restorable
			return E->properties;
		}
	}
	strcpy(base, word);
	len = strlen(base);

	// they overlap, return combo of both.
	//  indirect objects: me te le nos os les  (note le and les are different from directobjects, rest same)
	properties = SpanishPronounIndirectObjects(word, at, original, entry, canonical, sysflags);
	//  pronoun direct objects:  me te lo la nos os los las 
	properties = SpanishPronounDirectObjects(word, at, original, entry, canonical, sysflags);
	if (properties) return properties;

	strcpy(base, word);

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
			entry = StoreWord(word, AS_IS);
			AddProperty(entry, D->properties | PRONOUN_OBJECT);
			AddSystemFlag(entry, flags);
			return entry->properties;
		}
	}
	return 0;
}

uint64 ComputeSpanish(int at, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
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
		uint64 ans = ComputeSpanishAdjective(word, D, entry, canonical, sysflags);
		if (ans && wordCount > 1) return ans;
	}
	if (D && D->properties & NOUN)
	{
		uint64 ans = ComputeSpanishNoun(word, D, entry, canonical, sysflags);
		if (ans && wordCount > 1) return ans;
	}
	else if (!D || !(D->properties & TAG_TEST)) // maybe we can infer unknown is plural noun
	{
		uint64 ans = ComputeSpanishPluralNoun(word, D, entry, canonical, sysflags);
		if (ans && wordCount > 1) return ans;
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

	properties = ComputeSpanishVerb(at, original, entry, canonical, sysflags); // case sensitive, may add word to dictionary, will not augment flags of existing words
	if (properties) return properties;

	char base[MAX_WORD_SIZE];
	size_t len = strlen(original);
	sysflags = 0; // because we transiently overwrite it

	// adverb from adjective
	char* mente = strstr(word, "mente");
	if (mente && !mente[5]) // see if root is adjective
	{
		WORDP Q = FindWord(word, strlen(word) - 5);
		if (Q && Q->properties & ADJECTIVE)
		{
			if (!exportdictionary)
			{
				entry = StoreWord(word, AS_IS);
				AddProperty(entry, ADVERB);
			}
			else entry = canonical = dictionaryBase + 1;
			sysflags = 0;
			return ADJECTIVE;
		}
	}

	// see if plural of a noun
	if (word[len - 1] == 's')  // s after normal vowel ending or after foreign imported word
	{
		strcpy(base, word);
		base[len - 1] = 0; // drop the s
		properties = FindSpanishSingular(original, base, entry, canonical, sysflags);
		if (properties) return properties | NOUN | NOUN_PLURAL;
	}
	if (word[len - 1] == 's' && word[len - 2] == 'e' && !IsVowel(word[len - 3]))  // es after normal consonant ending
	{ // es after consonant
		strcpy(base, word);  
		base[len - 2] = 0; // drop es
		if (base[len - 3] == 'c') base[len - 3] = 'z'; // z changed to c
		// Singular nouns of more than one syllable which end in -en and don’t already have an accent, add one in the plural. los exámenes
		// accent vowel before consonant at end
		//if (base[len - 4] == 'a') strncpy(base + len - 4, "á", 2);
		//else if (base[len - 4] == 'e') strncpy(base + len - 4, "é", 2); //BUGGY
		//else if (base[len - 4] == 'i') strncpy(base + len - 4, "í", 2);
		//else if (base[len - 4] == 'o') strncpy(base + len - 4, "ó", 2);
		//else if (base[len - 4] == 'u') strncpy(base + len - 4, "ú", 2);
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
			if (*base == 'a') { strcpy(base, "\xC3\xA1"); strcat(base, hold); }
			if (*base == 'e') { strcpy(base, "\xC3\xA9"); strcat(base, hold); }
			if (*base == 'i') { strcpy(base, "\xC3\xAD "); strcat(base, hold); }
			if (*base == 'o') { strcpy(base, "\xC3\xB3"); strcat(base, hold); }
			if (*base == 'u') { strcpy(base, "\xC3\xBA"); strcat(base, hold); }
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
