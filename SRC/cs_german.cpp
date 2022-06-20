#include "common.h"

static WORDP GetGermanPrimaryTail(char* word, size_t length = 0);
static WORDP GetGermanPrimaryTail(char* word, size_t length)
{
	WORDP D = NULL;
	int len = strlen(word) - 2; // minimum joined piece
	while (--len >= 0) // minimum size leftover 2 (dont do tiny words)
	{
		D = FindWord(word + len, 0, UPPERCASE_LOOKUP); // 2nd word  if noun will be upper
		if (D && !(D->properties & (NOUN | VERB | ADJECTIVE | ADVERB))) D = NULL;
		if (!D) D = FindWord(word + len, 0, LOWERCASE_LOOKUP); // 2nd word other
		if (D && !(D->properties & (NOUN | VERB | ADJECTIVE | ADVERB))) D = NULL;
		if (D)
		{
			while (--len >= 0) // keep going
			{
				WORDP F = FindWord(word + len, 0, UPPERCASE_LOOKUP); // 2nd word  if noun will be upper
				if (!F) F = FindWord(word + len, 0, LOWERCASE_LOOKUP); // 2nd word other
				if (F && !(F->properties & (NOUN | VERB | ADJECTIVE | ADVERB))) F = NULL;
				if (len == 0 && length) break; // dont swallow entire known word
				if (F) D = F; // found longer, upgrade to it
			}
		}
	}
	return D;
}


int German2Spliter(char* word, int i)
{ // https://www.dartmouth.edu/~deutsch/Grammatik/Wortbildung/Komposita.html
	char* tokenlist[20];
	int n = 20;
	size_t len = strlen(word);
	WORDP D = GetGermanPrimaryTail(word, len);
	if (!D) return 0;
	tokenlist[--n] = D->word;
	size_t len1 = 0;
	char c = 0;
	char connector;
	int connectorLength = 0;
	while (1) // use up entire word as pieces
	{
		size_t oldlen = len;
		size_t oldlen1 = len1;

		// hide new tail word
		len1 = strlen(D->word) + connectorLength; // found piece and connector
		len = strlen(word); // full avail includes connector
		word[oldlen - oldlen1] = c; // restore old marker
		if (len == len1) break; // reached front
		c = word[len - len1]; // split off found word
		word[len - len1] = 0;
		int newlen = len - len1; // length of available
		connectorLength = 0;

		D = GetGermanPrimaryTail(word);
		if (D)
		{ // hundemude--- hund and hunde both possible (plural)  -- NO connector used
			tokenlist[--n] = D->word;
			continue;
		}
		//["-e-"]: "hundemüde" (dog - tired); "die Mausefalle" (mousetrap); "der Pferdestall" (horse stable); "das Schweinefleisch (pork); "das Wartezimmer" (waiting room).
		//["-n" or "-en"]: "das Backpfeifengesicht" (a face that makes you want to slap it); "das Bauernbrot (farmhouse bread); "das Freudenhaus" (brothel); "die Gedankenfreiheit" (freedom of thought); "der Kettenraucher" (chain-smoker); "der Tintenkleks" (inkblot).
		//["-er"]: "der Bilderrahmen" (picture frame); "der Geisterfahrer" (wrong - way driver).
		//["-s-" or "-es-"]: "das Arbeitstier" (eager beaver; workaholic); "der Freundeskreis" (circle of friends); "geistesgestört" (deranged); "der Glückspilz" (lucky fellow); "der Gottesdienst" (church service); "die Jahreszeit" (season[of the year]); "das Landesgeld" (domestic currency); "der Liebeskummer" (lover's grief, heart-sickness); "die Staatspolizei" (state police); "das Tageslicht" (daylight); "der Verbesserungsvorschlag" (suggestion for improvement); "das Verhütungsmittel" (contraceptive); "das Verkehrsamt" (tourist office).
		//["-ens"]: "die Herzensgüte" (goodness of heart); "das Friedensabkommen" (peace agreement); "das Schmerzensgeld" (compensation for painand suffering).
		if (word[newlen - 3] == 'e' && word[newlen - 2] == 'n' && word[newlen - 1] == 's') // 3-character connector
		{
			connector = word[newlen - 3];
			word[newlen - 3] = 0;
			D = GetGermanPrimaryTail(word);
			word[newlen - 3] = connector;
			if (D)
			{
				connectorLength = 3;
				tokenlist[--n] = D->word;
				continue;
			}
		}
		if (word[newlen - 2] == 'e' && (word[newlen - 1] == 'n' || word[newlen - 1] == 'r' || word[newlen - 1] == 's')) // 2-character connector
		{
			word[newlen - 2] = 0;
			D = GetGermanPrimaryTail(word);
			word[newlen - 2] = 'e';
			if (D)
			{
				connectorLength = 2;
				tokenlist[--n] = D->word;
				continue;
			}
		}
		if (word[newlen - 1] == 'e' || word[newlen - 1] == 'n' || word[newlen - 1] == 's' || word[newlen - 1] == '-') // 1-character connector
		{
			connector = word[newlen - 1];
			word[newlen - 1] = 0;
			D = GetGermanPrimaryTail(word);
			word[newlen - 1] = connector;
			if (D)
			{
				connectorLength = 1;
				tokenlist[--n] = D->word;
				continue;
			}
		}
		word[len - len1] = c;
		return 0; // failed to find next word
	}
	return ReplaceWords("German Compound", i, 1, 20 - n, tokenlist + n - 1) ? (20 - n) : 0;
}
