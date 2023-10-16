#include "common.h"
#include <unordered_map>
#include <string>
//
//  All pos data comes from original spanish dictionary. If not there, we dont know about it
// 
// NOTE: the ComputeSpanish1... code tests using correctly accented forms of Spanish words.
// It is assumed that if the user accents a character, it is correct. If this becomes a problem, we will use
// spellcheck to permute that as well as incorrectly unaccented characters.

// computespanish uses all of the original dictionary (not seeing UNIVERSAL merged words) and it not
// allowed to change properties or systemflags of words, except at at the computespanish outer layer.

// For this (and other treetagger-based dictionaries), the only reliable data is that lemmas are there, marked
// by their part of speech. While it contains common conjugations, we cannot assume all are there. Nor
// is there data on additional attributes like gender and plurality (in fact often it marks a singular noun as both
// singular and plural). So the task of ComputeSpanish1 is two-fold. First, to determine for spellcheck
//  if a word in user input  that is NOT recognized, can be determined to be a conjugation of a known lemma. 
// Second, for GetPosData, to return the set of all possible parts of speech and ancillary data like lemma, gender,
// plurality, and embedded object referencing for PosTagging.
// Currently this is not in the dictionary as static data because we would have to compute it using this function anyway
// to write the revised dictionary. And if this code is wrong and we already used it to rewrite the dictionary flags 
// of the source,  then recovering from faulty markings may be difficult. 

// primary entry point for conjugation processing
uint64 ComputeSpanish(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags);
uint64 ComputeSpanish1(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags);

typedef struct IrregularSpanishInfo
{
	const char* lemma;
	const char* original;		//  conjugated form
	const char* stem;				// suffixes go against this
	const uint64 properties;
	const uint64 sysFlags;
	string unaccented;
} IrregularSpanishInfo;

static IrregularSpanishInfo irregularSpanishVerbs[] =
{// any stem that is a blank means we dont know, and it may not matter

	// we deliver SIMPLE PAST (PRETERITE) and IMPERFECT past
	// we deliver spanish FUTURE and conditional tenses as FUTURE

	// preterite past
	{"ser","fue","fu",VERB | VERB_PAST,0, ""}, // él fue, ellos fueron
	{"ser","fueron","fu", VERB | VERB_PAST,0, ""},
	{"estar","estuve","estuv", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_I, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estuviste","estuv", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_YOU, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estuviseis","estuv", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"tener","tuve","ten", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_I, ""}, // - yo tuve, él tuvo
	{"tener","tuviste","ten", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_YOU, ""}, // - yo tuve, él tuvo
	{"tener","tuvo","ten", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT, ""}, // - yo tuve, él tuvo
	{"tener","tuvimos","ten", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_I, ""}, // - yo tuve, él tuvo
	{"tener","tuvisteis","ten", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""}, // - yo tuve, él tuvo
	{"tener","tuvieron","ten", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_HE_SHE_IT, ""}, // - yo tuve, él tuvo
	{"poder","pude","pud", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"poder","pudiste","pud", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"poder","pudo","pud", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT, ""},
	{"poder","pudimos","pud", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_I, ""},
	{"poder","pudisteis","pud", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"poder","pudieron","pud", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU | PRONOUN_HE_SHE_IT, ""},
	{"hacer","hice","hic", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_I, ""}, // most subjects yo hice, 
	{"hacer","hiciste","hic", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_YOU, ""}, // most subjects yo hice, 
	{"hacer","hicimos","hic", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_I, ""}, // most subjects yo hice, 
	{"hacer","hicisteis","hic", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""}, // most subjects yo hice, 
	{"hacer","hicieron","hic", VERB | VERB_PAST,PRONOUN_PLURAL, ""}, // most subjects yo hice, 
	{"poner","puse","pus", VERB | VERB_PAST,0, ""}, // yo puse, él puso
	{"poner","puso","pus", VERB | VERB_PAST,0, ""}, // yo puse, él puso
	{"decir","dije","dij", VERB | VERB_PAST,0, ""}, //  yo dije, él dijo
	{"decir","dijiste","dij", VERB | VERB_PAST,0, ""},
	{"decir","dijo","	dij", VERB | VERB_PAST,0, ""},
	{"decir","dijimos","	dij", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_I, ""},
	{"decir","dijisties","	dij", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"decir","dijeron","	dij", VERB | VERB_PAST,PRONOUN_PLURAL, ""},
	{"ver","vi","vi", VERB | VERB_PAST,0, ""},
	{"ver","vio","vi", VERB | VERB_PAST,0, ""},
	{"querer","quise","quis", VERB | VERB_PAST,0, ""}, // yo quise, él quiso
	{"querer","quiso","quis", VERB | VERB_PAST,0, ""},
	// in present tense, various irregular verbs vary in their 1stperson singular form and conjugate normally otherwise
	{"caber","quepo","cab", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"caer","caigo","ca", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"coger","cojo","cog", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"comer","come","com", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | PRONOUN_HE_SHE_IT | VERB_IMPERATIVE, ""},
	{"conocer","conozco","conoc", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"contar","cuento","cont", VERB | VERB_PRESENT, PRONOUN_I | PRONOUN_SINGULAR, ""},
	{"dar","doy"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"decir","digo","dec", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"decir","dices","dec", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"decir","dice","dec", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"decir","dicen","dec", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_PLURAL, ""},
	{"estar","estoy","estuv", VERB | VERB_PRESENT,PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"estar",u8"está","estuv", VERB | VERB_PRESENT,PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT, ""},
	{"estar",u8"están","estuv", VERB | VERB_PRESENT,PRONOUN_PLURAL | PRONOUN_HE_SHE_IT, ""},
	{"hacer","hago"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"mostrar","muestro","muestr", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"poner","pongo"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"salir","salgo"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"sentir","siento","sent", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"sentir","sientes","sent", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"sentir","siente","sent", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT | PRONOUN_YOU | VERB_IMPERATIVE | PRONOUN_INDIRECTOBJECT_SINGULAR | PRONOUN_INDIRECTOBJECT_YOU, ""},
	{"sentir","sienta","sent", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{"sentir","sientas","sent", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{"sentir","sintamos","sent", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I | VERB_IMPERATIVE, ""},
	{"sentir","sienten","sent", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_HE_SHE_IT, ""},
	{"sentir","sientan","sent", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"tener","tengo","ten", VERB | VERB_PRESENT,PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"tener","tienes","ten", VERB | VERB_PRESENT,PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"tener","tiene","ten", VERB | VERB_PRESENT,PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT, ""},
	{"tener","tienen","ten", VERB | VERB_PRESENT,PRONOUN_PLURAL | PRONOUN_HE_SHE_IT, ""},
	{"tomar","tome","tom", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | PRONOUN_HE_SHE_IT | VERB_IMPERATIVE, ""},
	{"traducir","traduzco"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"traer","traigo"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"valer","valgo"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"ver","veo"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	// imperfect past
	{"estar","estaba","estuv", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_I, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estabas","estuv", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_YOU, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estuvimos","estuv", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_I, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estuvistereis","estuv", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estuvieron","estuv", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_HE_SHE_IT, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"estar","estuvo","estuv", VERB | VERB_PAST, PRONOUN_SINGULAR | PRONOUN_HE_SHE_IT, ""}, //yo estuve, tu estuviste, él estuvo, nosotros estuvimos, vosotros estuvisteis, ellos estuvieron
	{"ir","iba","ir",	  VERB | VERB_PAST, PRONOUN_I | PRONOUN_SINGULAR, ""},
	{"ir","ibas","ir",   VERB | VERB_PAST, PRONOUN_YOU | PRONOUN_SINGULAR, ""}, // ir (to go)
	{"ir","ibamos","ir",	 VERB | VERB_PAST, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"ir","ibais","ir",	VERB | VERB_PAST, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"ir","iban","ir",   VERB | VERB_PAST, PRONOUN_PLURAL, ""},
	{"ser","era","s", VERB | VERB_PAST, PRONOUN_I | PRONOUN_SINGULAR, ""},
	{"ser","eras","s", VERB | VERB_PAST, PRONOUN_YOU | PRONOUN_SINGULAR, ""}, // ser (to be)
	{"ser","eramos","s", VERB | VERB_PAST, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"ser","erais","s", VERB | VERB_PAST, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"ser","eran","s", VERB | VERB_PAST, PRONOUN_PLURAL, ""},
	{"ver",u8"veía","v", VERB | VERB_PAST, PRONOUN_I | PRONOUN_SINGULAR, ""},
	{"ver",u8"veías","v", VERB | VERB_PAST, PRONOUN_YOU | PRONOUN_SINGULAR, ""}, // ver (to see)
	{"ver",u8"veíamos","v", VERB | VERB_PAST, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"ver",u8"veían","v", VERB | VERB_PAST, PRONOUN_PLURAL, ""},

	// and some are irregular in all forms present
	{"estar","estamos","est", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"estar",u8"estáis","est", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"estar",u8"estás","est", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"haber","ha"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR , ""},
	{"haber","han"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL, ""},
	{"haber","has"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"haber","hay"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR , ""},
	{"haber","he"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"haber","hemos"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"haber",u8"habéis"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"hacer",u8"hacía","hic", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_I, ""}, // most subjects yo hice, 
	{"hacer",u8"hacíaimos","hic", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_I, ""}, // most subjects yo hice, 
	{"hacer",u8"hacíain","hic", VERB | VERB_PAST,PRONOUN_PLURAL, ""}, // most subjects yo hice,
	{"hacer",u8"hacíais","hic", VERB | VERB_PAST,PRONOUN_PLURAL | PRONOUN_YOU, ""}, // most subjects yo hice, 
	{"hacer",u8"hacías","hic", VERB | VERB_PAST,PRONOUN_SINGULAR | PRONOUN_YOU, ""}, // most subjects yo hice, 
	{"ir","va"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR, ""},
	{"ir","vais"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"ir","vamos"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"ir","van"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL , ""},
	{"ir","vas"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"ir","voy"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"ser","eres"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"ser","es"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR, ""},
	{"ser","sois"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"ser","somos"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"ser","son"," ", VERB | VERB_PRESENT, PRONOUN_PLURAL, ""},
	{"ser","soy"," ", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_I, ""},
	// irregular simple future
	{"decir",u8"diré","dec", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"decir",u8"dirás","dec", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"decir",u8"dirá","dec", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR, ""},
	{"decir",u8"diremos","dec", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"decir",u8"diréis","dec", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"decir",u8"dirán","dec", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL, ""},
	{"estar",u8"estaré","est", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"estar",u8"estarás","est", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"estar",u8"estará","est", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR, ""},
	{"estar","estaremos","est", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"estar",u8"estaréis","est", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"estar",u8"estarán","est", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL, ""},
	{"hacer",u8"haré","hac", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"hacer",u8"harás","hac", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"hacer",u8"hará","hac", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR, ""},
	{"hacer","haremos","hac", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"hacer",u8"haréis","hac", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"hacer",u8"harán","eshact", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL, ""},
	// irregular conditional future
	{"decir",u8"diría","dec", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"decir",u8"dirías","dec", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"decir",u8"diríamos","dec", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"decir",u8"diríais","dec", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"decir",u8"dirían","dec", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL, ""},
	{"estar",u8"estaría","est", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{"estar",u8"estarías","est", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{"estar",u8"estaríamos","est", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_I, ""},
	{"estar",u8"estaríais","est", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{"estar",u8"estarían","est", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL, ""},
	{ "hacer",u8"haría","hac", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_I, ""},
	{ "hacer",u8"harías","hac", VERB | AUX_VERB_FUTURE, PRONOUN_SINGULAR | PRONOUN_YOU, ""},
	{ "hacer",u8"hacíamos","hac", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_I, ""},
	{ "hacer",u8"haríais","hac", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL | PRONOUN_YOU, ""},
	{ "hacer",u8"harían","eshact", VERB | AUX_VERB_FUTURE, PRONOUN_PLURAL, ""},
	// irregular past participle
	{ "abrir",u8"abierto","abr", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "escribir","escrito","escrib", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "cubrir",u8"cubierto","abr", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "decir",u8"dicho","dec", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "hacer",u8"hecho","hac", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "leer",u8"leído","leí", VERB | VERB_PAST_PARTICIPLE, 0, ""},
	{ "morir",u8"muerto","mor", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "poner",u8"puesto","pon", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "romper",u8"roto","romp", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "satisfacer",u8"satisfecho","satisfac", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	{ "ver",u8"visto","v", VERB | VERB_PAST_PARTICIPLE | ADJECTIVE, 0, ""},
	// irregular present participle (verb present paticiple)
	{ "leer","leyendo","ley", VERB | VERB_PRESENT_PARTICIPLE, 0, ""},
	// Irregular imperatives
	{ "coger","coja","cog", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "coger","cojamos","cog", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I | VERB_IMPERATIVE, ""},
	{ "coger","cojan","cog", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "coger",u8"cojáis","cog", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "contar","cuente","cont", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "contar","cuenten","cont", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "contar","cuentes","cont", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "contar",u8"cuénta","cont", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "dar","da","d", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "dar","deis","d", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "dar","des","d", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "dar",u8"dáselo","d", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE | PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_SINGULAR, ""},
	{ "dar",u8"dé","d", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "decir","di","dec", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "girar","gire","gir", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "girar","gires","gir", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "hacer","haz","hac", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir","andante","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir","vaya","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir","vayamos","ir", VERB | VERB_PRESENT, PRONOUN_SINGULAR | PRONOUN_PLURAL | PRONOUN_I | VERB_IMPERATIVE, ""},
	{ "ir","vayan","ir", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir","vayas","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir","ve","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir","vete","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir",u8"váyase","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir",u8"andá","ir", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ir",u8"vayáis","ir", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I | VERB_IMPERATIVE, ""},
	{ "ir",u8"vámonos","ir", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I | VERB_IMPERATIVE, ""},
	{ "laxar",u8"lávenselas","lax", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE | PRONOUN_OBJECT_PLURAL | PRONOUN_OBJECT_SINGULAR, ""},
	{ "poner","pon","pon", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "repetir","repite","repit", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "separar","separa","separ", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_I | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "saltar","salte","sal", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_I | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "ser",u8"sé","ser", VERB | VERB_PRESENT | SINGULAR_PERSON,PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "tener","tenga","ten", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "tener","tengamos","ten", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_I | VERB_IMPERATIVE, ""},
	{ "tener","tengan","ten", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "tener","tengas","ten", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "tener",u8"tengáis","ten", VERB | VERB_PRESENT, PRONOUN_PLURAL | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "tenir","ten","ten", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	{ "venir","ven","ven", VERB | VERB_PRESENT | SINGULAR_PERSON, PRONOUN_SINGULAR | PRONOUN_YOU | VERB_IMPERATIVE, ""},
	// sentinal
	{ "", NULL, NULL, 0, 0, ""},
};

void MarkSpanishTags(WORDP OL, int i)
{ // we have to manual name concept because primary naming is from the english corresponding bit
	uint64 systemFlags = GetSystemFlags(OL);
	if (finalPosValues[i] & PRONOUN_OBJECT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_SINGULAR) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_singular")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_PLURAL) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_plural")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_I) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_i")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_YOU) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_you")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_HE_SHE_IT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_he_she_it")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_OBJECT_SINGULAR) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_singular")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_OBJECT_PLURAL) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_plural")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_OBJECT_I) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_i")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_OBJECT_YOU) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_you")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_OBJECT_HE_SHE_IT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_object_he_she_it")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_INDIRECTOBJECT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_INDIRECTOBJECT_SINGULAR) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_singular")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_INDIRECTOBJECT_PLURAL) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_plural")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_INDIRECTOBJECT_I) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_i")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_INDIRECTOBJECT_YOU) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_you")), i, i, CANONICAL);
	if (systemFlags & PRONOUN_INDIRECTOBJECT_HE_SHE_IT) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~pronoun_indirectobject_he_she_it")), i, i, CANONICAL);
	if (finalPosValues[i] & AUX_VERB_FUTURE)
	{
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~AUX_VERB_FUTURE")), i, i, CANONICAL);
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~SPANISH_FUTURE")), i, i, CANONICAL);
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~verb_future")), i, i, CANONICAL);
	}
	if (allOriginalWordBits[i] & NOUN_HE)
	{
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~NOUN_HE")), i, i, CANONICAL);
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~SPANISH_HE")), i, i, CANONICAL);
	}
	if (allOriginalWordBits[i] & NOUN_SHE)
	{
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~NOUN_SHE")), i, i, CANONICAL);
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~SPANISH_SHE")), i, i, CANONICAL);
	}
	if (allOriginalWordBits[i] & SINGULAR_PERSON) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~SINGULAR_PERSON")), i, i, CANONICAL);
	if (systemFlags & VERB_IMPERATIVE) MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~verb_imperative")), i, i, CANONICAL);
}

uint64 GetSpanishLemmaBits(char* wordx)
{
	WORDP D = FindWord(wordx, 0, PRIMARY_CASE_ALLOWED,true);
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
		WORDP X = FindWord(ptag,0,PRIMARY_CASE_ALLOWED,true);
		if (X) properties |= X->properties;
		tags = SkipWhitespace(tags);
	}
	return properties;
}

uint64 FindSpanishSingular(char* original, char* word, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	WORDP E = FindWord(word,0,PRIMARY_CASE_ALLOWED,true);
	if (E && E->properties & NOUN)
	{
		canonical = E;
		if (!exportdictionary)
		{
			entry = GetLanguageWord(original);
		}
		else entry = canonical;
		sysflags = E->systemFlags;
		return NOUN | NOUN_PLURAL;
	}
	return 0;
}

uint64 KnownSpanishUnaccented(char* original, WORDP& entry, uint64& sysflags)
{
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word, original);
	char copy[MAX_WORD_SIZE];
	char* at = word - 1;
	char c;
	//with the  marks over the five vowels(á, é, í, ó, ú), you will never see two of those in the same word.
	while ((c = *++at)) // try single accented change
	{
		char* change = NULL;
		if (c == 'a') change = "\xC3\xA1";
		else if (c == 'e') change = "\xC3\xA9";
		else if (c == 'i') change = "\xC3\xAD";
		else if (c == 'o') change = "\xC3\xB3";
		else if (c == 'u') change = "\xC3\xBA";
		if (change)
		{
			strcpy(copy, word);
			strcpy(copy + (at - word), change);
			strcpy(copy + (at - word) + 2, at + 1);

			WORDP canonical = NULL;
			uint64 newSysflags = 0;
			uint64 properties = ComputeSpanish1(2, copy, entry, canonical, newSysflags);
			if (properties)
			{
				sysflags |= newSysflags;
				return properties;
			}
		}
	}
	return 0;
}

static WORDP ConfirmSpanishInfinitive(char* stem, char* verbType, int stemLen, char* original, const char* suffix, char* rawlemma = NULL);
static WORDP ConfirmSpanishInfinitive(char* stem, char* verbType, int stemLen, char* original, const char* suffix, char* rawlemma)
{
	char word[MAX_WORD_SIZE];
	strcpy(word, stem);
	char rawword[MAX_WORD_SIZE];
	if (rawlemma) strcpy(rawword, rawlemma);
	else strcpy(rawword, stem);

	WORDP Lemma;
	if (*verbType) // we know the kind of verb
	{
		strcpy(word + stemLen, verbType); // make lemma from stem and type
		Lemma = FindWord(word,0,PRIMARY_CASE_ALLOWED,true);
		if (Lemma && Lemma->properties & VERB) // this is a lemma
		{
			strcpy(rawword + stemLen, suffix); // the actual word we would generate 
			if (!stricmp(rawword, original))
				return Lemma; // we would get this
		}
	}
	else // try all 3 verb types
	{
		strcpy(word + stemLen, "er"); // papxxx becomes paper
		Lemma = FindWord(word,0, PRIMARY_CASE_ALLOWED,true);
		if (Lemma && Lemma->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return Lemma; // we would get this
		}
		strcpy(word + stemLen, "ar");
		Lemma = FindWord(word, 0, PRIMARY_CASE_ALLOWED, true);
		if (Lemma && Lemma->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return Lemma; // we would get this
		}
		strcpy(word + stemLen, "ir");
		Lemma = FindWord(word, 0, PRIMARY_CASE_ALLOWED, true);
		if (Lemma && Lemma->properties & VERB)
		{
			strcpy(rawword + stemLen, suffix); // the actual
			if (!stricmp(rawword, original))
				return Lemma; // we would get this
		}
	}
	return NULL;
}

static uint64 FindSpanishInfinitive(char* original, const char* suffix, char* verbType, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	// Infinitives always end in ar, er or ir added to the stem - verbType names this suffix. 
	// If empty, means try all 3 types
	// Conjugation involves changing to different suffix on stem verb
	// So, given original and a suffix to consider, we know the stem part before that suffix.
	// Of course the stem may have changed spelling along the way, for added complexity.
	uint64 properties = 0;
	size_t originalLen = strlen(original);
	size_t suffixLen = strlen(suffix);
	size_t stemLen = originalLen - suffixLen;
	if (strcmp(original + stemLen, suffix)) 	return 0; // this conjugation doesnt end with suffix named
	WORDP D = GetLanguageWord(original);
	
	// original ended in designated suffix. See if we can get its infinitive form with no changes of spelling
	char stem[MAX_WORD_SIZE];
	strncpy(stem, original, stemLen);
	char lemma[MAX_WORD_SIZE];
	stem[stemLen] = 0;
	strcpy(lemma, stem);
	WORDP oldcanonical = canonical;
	canonical = ConfirmSpanishInfinitive(stem, verbType, stemLen, original, suffix);
	if (canonical && canonical->properties & (VERB_INFINITIVE | NOUN_GERUND)) // simple verb conjugation matched. we have canonical also.
	{
		entry = D;
		return VERB_INFINITIVE;
	}

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
	if (!canonical && found) // replace accented form with unaccented
	{
		*found = c;
		memmove(found + 1, found + 2, strlen(found + 1)); // know its 2 chars wide
		canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix);
		if (canonical) // simple verb conjugation matched. we have canonical also.
		{
			entry = D;
			return properties;
		}
	}


	strcpy(lemma1, stem);
	strcat(lemma1, suffix);
	if (properties & AUX_VERB_FUTURE)
	{
		canonical = FindWord(stem,0,PRIMARY_CASE_ALLOWED,true);
		if (canonical && canonical->properties & VERB && !stricmp(lemma1, original)) { ; }
		else canonical = NULL;
	}

	// stem changed from e to ie
	strcpy(lemma1, stem);
	char* change = strstr(lemma1, "ie");
	char* again = (change) ? strstr(change + 1, "ie") : NULL;
	if (again) change = again; // prefer 2nd one
	if (!canonical && change) // ie from e
	{
		memmove(change, change + 1, strlen(change));
		canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	// stem changed from o to ue
	strcpy(lemma1, stem);
	change = strstr(lemma1, "ue");
	again = (change) ? strstr(change + 1, "ue") : NULL;
	if (!canonical && change) // ue from o
	{
		memmove(change, change + 1, strlen(change));
		*change = 'o';
		canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	// stem changed from e to i
	if (!strcmp(verbType, "ir")) // ir verb i from e 
	{
		strcpy(lemma1, stem);
		char* change = strrchr(lemma1, 'i');
		if (!canonical && change) // i from e
		{
			*change = 'e';
			canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		}
	}
	else if (!stricmp(verbType, "er"))
	{
		strcpy(lemma1, stem);
		char* change = strstr(lemma1, "ie");
		char* again = (change) ? strstr(change + 1, "ie") : NULL;
		if (again) change = again;
		if (!canonical && change) // ie from e
		{
			memmove(change, change + 1, strlen(change));
			canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		}

		// stem o changed to ue
		strcpy(lemma1, stem);
		change = strstr(lemma1, "ue");
		again = (change) ? strstr(change + 1, "ue") : NULL;
		if (!canonical && change) // ue from o
		{
			memmove(change, change + 1, strlen(change));
			*change = 'o';
			canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		}
	}

	// stem changed from c to b (hacer conditional)
	strcpy(lemma1, stem);
	change = strrchr(lemma1, 'b');
	if (!canonical && change) // b from c
	{
		memmove(change, change + 1, strlen(change));
		*change = 'c';
		canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
	}

	// IRREGULAR VERB SPECIALS 
	// 
	// stem changed from c to r (hacer future)
	strcpy(lemma1, stem);
	change = strstr(lemma1, "r");
	if (!canonical && change && properties & AUX_VERB_FUTURE) // r from c
	{
		memmove(change, change + 1, strlen(change));
		*change = 'c';
		canonical = ConfirmSpanishInfinitive(lemma1, verbType, stemLen, original, suffix, stem);
		*change = 'r'; // revert
	}

	if (canonical)
	{// since we found infinitive for it, assign info to the word
		entry = D;
		return properties | (canonical->properties & VERB_BITS);
	}

	if (!canonical) canonical = oldcanonical;
	return properties;
}


static uint64 ReflexiveVerbSpanish(char* word, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	uint64 properties = 0;
	size_t len = strlen(word);
	// try reflexive forms of any regular verb
	if (len > 2 && word[len - 2] == 's' && word[len - 1] == 'e')
	{
		char base[MAX_WORD_SIZE];
		strcpy(base, word);
		base[len - 2] = 0;
		WORDP E = FindWord(base,0,PRIMARY_CASE_ALLOWED,true);
		if (E && E->properties & VERB)
		{
			canonical = E;
			if (!exportdictionary)
			{
				entry = GetLanguageWord(word);
				properties |= VERB_PRESENT;
			}
			else entry = canonical;
			sysflags |= PRONOUN_REFLEXIVE;
		// if (E->properties & VERB_INFINITIVE) RemoveProperty(E, VERB_INFINITIVE); // treetagger lied - direct change, not restorable
			properties |= VERB_PRESENT; // treetagger lied - direct change, not restorable
		//	if (entry->properties & VERB_INFINITIVE)  RemoveProperty(entry, VERB_INFINITIVE); // treetagger lied - direct change, not restorable
			properties |= E->properties;
		}
	}
	return properties;
}

static void MakeUnaccentedCopy(const char* word, char* copy)
{
	struct AccentEquivalence {
		const char accent[3];
		const char unaccent;
	};
	const AccentEquivalence table[] = {
		{ "\xC3\xA1", 'a' },
		{ "\xC3\xA9", 'e' },
		{ "\xC3\xAD", 'i' },
		{ "\xC3\xB3", 'o' },
		{ "\xC3\xBA", 'u' }
	};
	while (*word) {
		bool changed_letter = false;
		if (*word == '\xC3')
		{
			for (const AccentEquivalence& entry : table) {
				if (entry.accent[1] == word[1]) {
					*copy = entry.unaccent;
					changed_letter = true;
				}
			}
		}
		if (changed_letter) {
			word += 2;
		}
		else {
			*copy = *word;
			word++;
		}
		copy++;
	}
	*copy = '\0';
}

static uint64 IrregularSpanishVerb(char* wordToFind, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	uint64 properties = 0;
	// All searches done on unaccented version
	char unaccentedWord[MAX_WORD_SIZE];
	static unordered_map<std::string, IrregularSpanishInfo*> verb_map;
	if (verb_map.empty()) {
		IrregularSpanishInfo* entry = irregularSpanishVerbs;
		while (entry->lemma[0] != '\0') {
			MakeUnaccentedCopy(entry->original, unaccentedWord);
			if (verb_map.find(unaccentedWord) != verb_map.end()) {
				printf("Error: duplicate original %s unaccented %s\n",
					entry->original, unaccentedWord);
			}
			verb_map[unaccentedWord] = entry;
			entry->unaccented = unaccentedWord;
			entry++;
		}
	}

	MakeUnaccentedCopy(wordToFind, unaccentedWord);
	if (verb_map.find(unaccentedWord) != verb_map.end())
	{
		IrregularSpanishInfo* fn = verb_map[unaccentedWord];
		canonical = GetLanguageWord(fn->lemma);
		sysflags |= fn->sysFlags;
		properties = fn->properties;
	}

	return properties;
}

static WORDP FindSpanishWordRemoveAccent(char* base)
{
	WORDP D = FindWord(base,0,PRIMARY_CASE_ALLOWED,true);
	if (D) return D;
	char copy[MAX_WORD_SIZE];
	strcpy(copy, base);
	char* ptr = copy - 1;
	char c = 0;
	while (*++ptr)
	{
		if (!strnicmp(ptr, "\xC3\xA1", 2)) c = 'a';
		else if (!strnicmp(ptr, "\xC3\xA9", 2)) c = 'e';
		else if (!strnicmp(ptr, "\xC3\xAD", 2)) c = 'i';
		else if (!strnicmp(ptr, "\xC3\xB3", 2)) c = 'o';
		else if (!strnicmp(ptr, "\xC3\xBA", 2)) c = 'u';
		if (c)
		{
			*ptr = c;
			memmove(ptr + 1, ptr + 2, strlen(ptr + 1));
			D = FindWord(copy,0,PRIMARY_CASE_ALLOWED,true);
			if (D) strcpy(base, copy);
			return D;
		}
	}
	return NULL;
}

WORDP FindSpanishWord(char* base, char* full)
{ // presume it had accent added, try to remove it
	WORDP D = FindWord(full, 0, PRIMARY_CASE_ALLOWED, true); // dictionary knows it?
	if (D)
	{
		D = GetCanonical(D, VERB_INFINITIVE);
		if (D && D->properties & VERB_INFINITIVE) return D;
	}

	return FindSpanishWordRemoveAccent(base);
}

uint64 ImperativeSpanish(char* word, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	char base[MAX_WORD_SIZE];
	strcpy(base, word);
	size_t len = strlen(base);
	WORDP Dbase = FindSpanishWordRemoveAccent(base);
	uint64 properties = 0;
	if (Dbase)
	{
		entry = Dbase;
	}

	// tu form, add 'r' to form infinitive
	strcat(base, "r");
	WORDP Dcanon = FindSpanishWord(base, base);
	if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
	{
		canonical = Dcanon;
		sysflags |= PRONOUN_YOU | PRONOUN_SINGULAR | VERB_IMPERATIVE;
		return properties | VERB;
	}
	base[len] = '\0';
	// vosotros/vosotras form, remove 'd' and add 'r' to form infinitive
	// also, éis -> ar as infinitive,
	if (base[len - 1] == 'd') {
		base[len - 1] = 'r';
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties  | VERB;
		}
		base[len - 1] = 'd';
	}
	if (!stricmp(&base[len - 4], u8"éis")) {
		strcpy(&base[len - 4], "ar");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_SINGULAR | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 4], u8"éis");
	}
	// usted form, e -> ar as infinitive, a -> er, a -> ir
	if (base[len - 1] == 'e') {
		strcpy(&base[len - 1], "ar");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_SINGULAR | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 1], "e");
	}
	if (base[len - 1] == 'a') {
		strcpy(&base[len - 1], "er");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_SINGULAR | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 1], "ir");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_SINGULAR | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 1], "a");
	}
	// ustedes form, en -> ar as infinitive, an -> er, an -> ir
	if (!stricmp(&base[len - 2], "en")) 
	{
		strcpy(&base[len - 2], "ar");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 2], "en");
	}
	if (!stricmp(&base[len - 2], "an")) {
		strcpy(&base[len - 2], "er");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 2], "ir");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_YOU | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 2], "an");
	}
	// nosotros form, emos -> ar as infinitive, amos -> er an ir 
	if (!stricmp(&base[len - 4], "emos")) {
		strcpy(&base[len - 4], "ar");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_I | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 4], "emos");
	}
	if (!stricmp(&base[len - 4], "amos")) {
		strcpy(&base[len - 4], "er");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_I | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 4], "ir");
		Dcanon = FindSpanishWord(base, base);
		if (Dcanon && (Dcanon->properties & VERB_INFINITIVE))
		{
			canonical = Dcanon;
			sysflags |= PRONOUN_I | PRONOUN_PLURAL | VERB_IMPERATIVE;
			return properties | VERB;
		}
		strcpy(&base[len - 4], "amos");
	}

	// irregular form
	base[len] = '\0';
	uint64 transient = IrregularSpanishVerb(base, entry, canonical, sysflags);
	if (sysflags & VERB_IMPERATIVE) return properties | transient;

	return false;
}

uint64  PresentSpanish(char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	uint64 properties = PRONOUN_SUBJECT | VERB_PRESENT | VERB;

	// imperative tu form?  parar(to stop) = paras(you stop) → ¡Para!(Stop!)
	uint64 transient = 0;
	uint64 ans = ImperativeSpanish(original, entry, canonical, transient);
	if (ans)
	{
		sysflags |= transient;
		return properties | ans;
	}

	// we have the original. We know verb infinitive comes in 3 flavors. We try those flavors with the
	// various possible stems. 

// Spanish infinitives end in ar, er, or ir.
// 
	// 1st person  //		yo			(habl)o			(com)o		(abr)o
	transient = PRONOUN_SINGULAR | PRONOUN_I;
	if (FindSpanishInfinitive(original, "o", "ar", entry, canonical, transient)) // yo
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "o", "er", entry, canonical, transient)) /// él, ella, usted	habl + a	habla
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "o", "ir", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}

	// 2nd person //		tú				(habl)as		(com)es	(abr)es
	transient = PRONOUN_SINGULAR | PRONOUN_YOU;
	if (FindSpanishInfinitive(original, "as", "ar", entry, canonical, transient)) // tu
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "es", "er", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "es", "ir", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	// 3rd person //		él	/ ella		(habl)a			(com)e		(abr)e
	transient = PRONOUN_SINGULAR;
	if (FindSpanishInfinitive(original, "a", "ar", entry, canonical, transient)) /// él, ella, usted	habl + a	habla
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "e", "er", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "e", "ir", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_I;
	// 1st person plural //		nosotros	(habl)amos	(com)emos	(abr)imos
	if (FindSpanishInfinitive(original, "amos", "ar", entry, canonical, transient)) /// él, ella, usted	habl + a	habla
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "emos", "er", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "imos", "ir", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}

	transient = PRONOUN_PLURAL | PRONOUN_YOU;
	// 2nd person plural   //		vosotros	(habl)áis		(com)éis	(abr)ís
	if (FindSpanishInfinitive(original, u8"áis", "ar", entry, canonical, transient)) /// él, ella, usted	habl + a	habla
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"áis", "er", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ís", "ir", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	transient = PRONOUN_PLURAL;
	// 3rd person plural ellos, ellas, ustedes // ellos/ellas	(habl)an	(com)en	(abr)en
	if (FindSpanishInfinitive(original, "an", "ar", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "en", "er", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "en", "ir", entry, canonical, transient))
	{
		sysflags |= transient;
		return properties;
	}

	return 0;
}

uint64 PastSpanish(char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	// FindSpanishInfinitive(original, suffix, verb type, etnry, canonical, properties so far, new properties if match ) // yo
	uint64 properties = PRONOUN_SUBJECT | VERB_PAST | VERB;
	// 1st person  preterite  past //		yo				(habl)é			(com)í			(abr)í
	uint64 transient = PRONOUN_SINGULAR | PRONOUN_I;

	if (FindSpanishInfinitive(original, "\xC3\xBA", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "i", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "i", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR | PRONOUN_YOU;
	// 2nd person 	 past preterite  	tú	(habl)aste	(com)iste	(abr)iste
	if (FindSpanishInfinitive(original, "aste", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "iste", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "iste", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR;
	// 3rd person  //		él / ella			(habl)ó			(com)ió		(abr)ió
	if (FindSpanishInfinitive(original, u8"ó", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ió", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_I;
	// 1st person plural 		nosotros	(habl)amos	(com)imos	(abr)imos
	if (FindSpanishInfinitive(original, "amos", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "imos", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, "imos", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_YOU;
	// 2nd person plural //		vosotros		(habl)asteis	(com)isteis	(abr)isteis
	if (FindSpanishInfinitive(original, "asteis", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else  if (FindSpanishInfinitive(original, "isteis", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else  if (FindSpanishInfinitive(original, "isteis", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL;
	// 3rd person plural //		ellos/ellas		(habl)aron	(com)ieron	(abr)ieron
	if (FindSpanishInfinitive(original, "aron", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else 	if (FindSpanishInfinitive(original, "ieron", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else 	if (FindSpanishInfinitive(original, "ieron", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}

	transient = PRONOUN_SINGULAR | PRONOUN_I;
	// imperfect past 
	// 1st person  //	yo				(habl)aba		(com)ía
	if (FindSpanishInfinitive(original, "aba", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else 	if (FindSpanishInfinitive(original, "ia", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else 	if (FindSpanishInfinitive(original, "ia", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR | PRONOUN_YOU;
	// 2nd person //	tú					(habl)abas	(com)ías
	if (FindSpanishInfinitive(original, "abas", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ías", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ías", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR;
	// 3rd person //	él / ella			(habl)aba		(com)ía
	if (FindSpanishInfinitive(original, "aba", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ía", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ía", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_I;
	// 1st person plural //	nosotros		(habl)ábamos	(com)íamos
	if (FindSpanishInfinitive(original, u8"ábamos", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"íamos", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"íamos", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_YOU;
	// 2nd person plural //	vosotros		(habl)abais	(com)íais
	if (FindSpanishInfinitive(original, "abais", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"íais", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"íais", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL;
	// 3rd person plural //	ellos/ellas		(habl)aban	(com)ían
	if (FindSpanishInfinitive(original, "aban", "ar", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ían", "er", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	else if (FindSpanishInfinitive(original, u8"ían", "ir", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}

	return 0;
}

uint64 FutureSpanish(char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	// FindSpanishInfinitive(original, suffix, verb type, etnry, canonical, properties so far, new properties if match ) // yo
	uint64 properties = PRONOUN_SUBJECT | AUX_VERB_FUTURE | AUX_VERB_FUTURE | VERB;
	uint64 transient = PRONOUN_SINGULAR | PRONOUN_I;
	// 1st person //yo(hablar)é   all 3 verb types
	// simple future
	if (FindSpanishInfinitive(original, u8"é", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}

	transient = PRONOUN_SINGULAR | PRONOUN_YOU;
	// 2nd person //	tú(hablar)ás  all 3 verb types
	if (FindSpanishInfinitive(original, u8"ás", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR;
	// 3rd person //él / ella(hablar)á  all 3  verb types
	if (FindSpanishInfinitive(original, u8"á", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_I;
	// 1st person plural  //nosotros(hablar)emos  all 3 verb types
	if (FindSpanishInfinitive(original, "emos", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_YOU;
	// 2nd person plural  //vosotros(hablar)áis  all 3 verb types
	if (FindSpanishInfinitive(original, u8"áis", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL;
	// 3rd person plural //ellos / ellas(hablar)án  all 3 verb types
	if (FindSpanishInfinitive(original, u8"án", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR | PRONOUN_I;
	// conditional future 
	// 1st person // yo(hablar)ía  all 3 types
	if (FindSpanishInfinitive(original, u8"ía", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR | PRONOUN_YOU;
	// 2nd person // tú(hablar)ías			 all 3 types
	if (FindSpanishInfinitive(original, u8"ías", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_SINGULAR;
	// 3rd person // él / ella(hablar)ía   all 3 types
	if (FindSpanishInfinitive(original, u8"ía", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_I;
	// 1st person plural // nosotros(hablar)íamos  all 3 types
	if (FindSpanishInfinitive(original, u8"íamos", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL | PRONOUN_YOU;
	// 2nd person plural // vosotros(hablar)íais  all 3 types
	if (FindSpanishInfinitive(original, u8"íais", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}
	transient = PRONOUN_PLURAL;
	// 3rd person plural  //ellos / ellas(hablar)ían  all 3 types
	if (FindSpanishInfinitive(original, u8"ían", "", entry, canonical, transient))
	{
		sysflags = transient;
		return properties;
	}

	return 0;
}

static uint64  InfinitiveSpanish(char* word, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	WORDP D = NULL;
	size_t len = strlen(word);
	if (!stricmp(word + len - 2, "lo") ||
		!stricmp(word + len - 2, "la"))
	{
		char base[MAX_WORD_SIZE];
		strcpy(base, word);
		base[strlen(base) - 2] = 0;
		D = FindWord(base, 0, LOWERCASE_LOOKUP,true);
	}
	else D = FindWord(word, 0, LOWERCASE_LOOKUP, true);

	if (D && D->properties & VERB)
	{
		WORDP E = GetCanonical(D, VERB);
		if (E == D || !E)
		{
			entry = canonical = D;
			return VERB | VERB_INFINITIVE;
		}
	}
	return 0;
}

static uint64  GerundSpanish(char* word, char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	uint64 properties = NOUN | NOUN_GERUND;
	uint64 transient = 0;
	if (FindSpanishInfinitive(original, "ando", "ar", entry, canonical, transient))
	{
		return properties;
	}
	else if (FindSpanishInfinitive(original, "iando", "er", entry, canonical, transient))
	{
		return properties;
	}
	else if (FindSpanishInfinitive(original, "iando", "ir", entry, canonical, transient))
	{
		return properties;
	}
	return 0;
}

uint64 PastParticipleSpanish(char* word, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	char base[MAX_WORD_SIZE];
	strcpy(base, original);
	size_t len = strlen(original);
	if (!stricmp(original + len - 2, "lo") ||
		!stricmp(original + len - 2, "la"))
	{
		base[strlen(base) - 2] = '\0';
	}

	uint64 properties = VERB | VERB_PAST_PARTICIPLE | ADJECTIVE;
	uint64 transient = 0;
	if (FindSpanishInfinitive(base, "ado", "ar", entry, canonical, transient))
	{
		return properties;
	}
	else if (FindSpanishInfinitive(base, "ido", "er", entry, canonical, transient))
	{
		return properties;
	}
	else if (FindSpanishInfinitive(base, "ido", "ir", entry, canonical, transient))
	{
		return properties;
	}
	return 0;
}

uint64 ComputeSpanishLemmaNounGender(char* word, WORDP canonical)
{
	size_t len = strlen(word);
	char ending = word[len - 1];
	//https://vamospanish.com/discover/spanish-grammar-nouns-and-gender/

	// Nouns ending in - o, an accented vowel, -or , or -aje are usually masculine.
	// Nouns ending in - a, -ción, -ía, or -dad are usually feminine.
	//	Names of rivers, lakes, and oceans are usually masculine; names of mountains are usually feminine. NOT HANDLED
// The most well - known rule  is tending in - o are masculine and tending in - a are feminine, but there are numerous exceptions to this gender rule, especially for those ending in - a.
	uint64 properties = 0;
	int adjust = 0;
	if (ending == 'o')
	{
		adjust = 1;
		properties = NOUN_HE;
	}
	else if (!strcmp(word + len - 5, u8"ción"))
	{
		properties = NOUN_SHE;
		adjust = 5;
	}
	else if (!strcmp(word + len - 2, u8"á") || !strcmp(word + len - 2, u8"é") ||
		!strcmp(word + len - 2, u8"í") || !strcmp(word + len - 2, u8"ó") ||
		!strcmp(word + len - 2, u8"ú") || !strcmp(word + len - 2, "or"))
	{
		adjust = 2;
		properties = NOUN_HE;
	}
	else if (!strcmp(word + len - 3, "aje"))
	{
		adjust = 3;
		properties = NOUN_HE;
	}
	else if (ending == 'a')
	{
		properties = NOUN_SHE;
		adjust = 1;
	}
	else if (ending == 'z')
	{
		properties = NOUN_SHE;
		adjust = 1;
	}
	else if (!strcmp(word + len - 3, u8"ía") || !strcmp(word + len - 3, "dad"))
	{
		properties = NOUN_SHE;
		adjust = 3;
	}
	else properties |= NOUN_SHE | NOUN_HE; // default is ambi
	WORDP X = FindWord(word, len - adjust, LOWERCASE_LOOKUP,true);
	if (X && X->properties & NOUN) return properties;
	return 0;
}

uint64 ComputeSpanishNounGender(WORDP D, WORDP canonical)
{
	// Nouns ending in certain suffixes are usually feminine.
	//		- ción(usually the equivalent of "-tion"), -sión, -ía(usually the equivalent of "-y," although not in the diminutive sense), -za, -dad(often used like "-ty"), and -itis("-itis").
	// Nouns of Greek origin ending in - a, often - ma, are nearly always masculine.
	// Nouns with certain other endings are usually masculine. - aje(usually the equivalent of "-age"), -ambre, 
	// and -or . An exception is la flor(flower).
	// Infinitives used as nouns are masculine. NOT HANDLED
	// Months and days of the week are masculine.  NOT HANDLED
	// Letters are feminine while numbers are masculine.  NOT HANDLED
	// The gender of abbreviations and acronyms typically matches the gender of the main noun of what the shortened version stands for.
	// Compound nouns formed by following a verb with a noun are masculine.  NOT HANDLED
	// With the exception of la plata (silver), names of the chemical elements are masculine.  NOT HANDLED
	// Names of islands are usually feminine because la isla (island) is feminine.
	// Names of mountains are usually masculine, because el monte (mountain) is masculine.  NOT HANDLED
		// An exception is that the Rockies are usually referred to as las Rocosas or las Montañas Rocosas. NOT HANDLED
	// Names of companies usually are feminine, because la compañía (company) is feminine, as are 
		// sociedad anónima (corporation), corporación (corporation), and empresa (business). 
		// This rule is not consistently followed, however, and some well-known companies (such as Google) are referred to as either masculine or feminine.
	if (!(D->properties & NOUN)) return 0;

	char* word = D->word;
	size_t len = strlen(word);
	char ending = word[len - 1];
	char end2 = word[len - 2];
	uint64 properties = D->properties & (NOUN_HE | NOUN_SHE);
	if (properties & (NOUN_HE | NOUN_SHE)) {}
	else if (!stricmp(word, u8"sofá") || !stricmp(word, "sofa") ||
		!stricmp(word, "dia") || !stricmp(word, u8"día") ||
		!stricmp(word, "cura") || !stricmp(word, "planeta") ||
		!stricmp(word, "corazón") || !stricmp(word, "buzón") ||
		!stricmp(word, "agua") || !stricmp(word, "mapa") ||
		!stricmp(word, "problema")
		)
		properties |= NOUN_HE; // exception to next rule
	else if (!stricmp(word, "mano") || !stricmp(word, "radio") ||
		!stricmp(word, "foto") || !stricmp(word, "moto") ||
		!stricmp(word, "flor") || !stricmp(word, "leche") ||
		!stricmp(word, "carne") || !stricmp(word, "disco") ||
		!stricmp(word, "radio")
		)
		properties |= NOUN_SHE; // exception to next rule
	else if (len > 2 &&
		(!stricmp(word + len - 3, u8"ía") || !stricmp(word + len - 3, "ia") ||
			!stricmp(word + len - 2, "ua") || !stricmp(word + len - 3, u8"úa") ||
			!stricmp(word + len - 2, "ez")))
		properties |= NOUN_SHE;
	else if (len > 2 &&
		(!stricmp(word + len - 2, "or") || !stricmp(word + len - 3, u8"ío") ||
			!stricmp(word + len - 3, u8"io") || !stricmp(word + len - 2, "ma") ||
			!stricmp(word + len - 2, "an")))
		properties |= NOUN_HE;
	else if (len > 3 &&
		(!stricmp(word + len - 3, "cha") || !stricmp(word + len - 3, "bra") ||
			!stricmp(word + len - 3, "ara") || !stricmp(word + len - 3, "dad") ||
			!stricmp(word + len - 3, "tad") || !stricmp(word + len - 3, "tud")))
		properties |= NOUN_SHE;
	else if (len > 3 &&
		(!stricmp(word + len - 3, "aje") || !stricmp(word + len - 4, "zón") ||
			!stricmp(word + len - 3, "aro") || !stricmp(word + len - 3, "emo")))
		properties |= NOUN_HE;
	else if (len > 3 &&
		!stricmp(word + len - 3, "nte"))
		properties |= NOUN_HE | NOUN_SHE;
	else if (len > 4 &&
		(!stricmp(word + len - 4, "ecto") || !stricmp(word + len - 4, "ismo")))
		properties |= NOUN_HE;
	else if (len > 4 && !stricmp(word + len - 4, "ista"))
		properties |= NOUN_HE | NOUN_SHE;
	else if (len > 4 &&
		(!stricmp(word + len - 5, u8"ción") || !stricmp(word + len - 5, u8"sión") ||
			!stricmp(word + len - 4, "triz")))
		properties |= NOUN_SHE;
	else if (len > 5 && (!stricmp(word + len - 5, "umbre")))
		properties |= NOUN_SHE;
	else if (ending == 'r' || ending == 'o')
		properties |= NOUN_HE;
	else if (ending == 'a')
		properties |= NOUN_SHE;
	// plural of o word
	else if (ending == 's' && end2 == 'o')
		properties |= NOUN_HE;
	else if (ending == 's' && end2 == 'a')
		properties |= NOUN_SHE;
	else properties |= ComputeSpanishLemmaNounGender(word, canonical);
	return properties;
}

//https://vamospanish.com/discover/plural-nouns/#:~:text=In%20order%20to%20make%20a,that%20end%20in%20a%20vowel.&text=The%20definite%20articles%20also%20change,la%E2%80%9D%20becomes%20%E2%80%9Clas%E2%80%9C.
uint64 ComputeSpanishPluralNoun(char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	char base[MAX_WORD_SIZE];
	size_t len = strlen(word);

	//adding -s, -es, or -ies to the end. 
	entry = D;
	sysflags = D->systemFlags;
	uint64 properties = 0;
	char end1 = word[len - 1];
	char end2 = word[len - 2];
	char end3 = word[len - 3];

	if (!canonical)
	{
		// exception, no and yo plural add es even though vowel
		if (!stricmp(word, "noes") || !stricmp(word, "yoes"))
		{
			canonical = FindWord(word, len - 2, LOWERCASE_LOOKUP,true); // but may not be in dict
			if (canonical && canonical->properties & NOUN)
			{
				properties = (NOUN_PLURAL);
				return properties;
			}
		}

		else if (end2 == 'e' && end1 == 's' && !IsVowel(end3)) 	// add an -es to the end of words that end in consonant.
		{
			canonical = FindWord(word, len - 2, LOWERCASE_LOOKUP,true);
			if (canonical && canonical->properties & NOUN)
			{
				properties = (NOUN_PLURAL );
				return properties;
			}
			// If a noun ends in - z, add - es and change the z to c.
			char copy[MAX_WORD_SIZE];
			strcpy(copy, word);
			if (end3 == 'c')
			{
				word[len - 3] = 'z';
				canonical = FindWord(word, len - 3, LOWERCASE_LOOKUP,true);
				if (canonical && canonical->properties & NOUN)
				{
					properties = (NOUN_PLURAL );
					return properties;
				}
			}
		}
	}
	
	sysflags = 0; // because we transiently overwrite it

	// see if plural of a noun
	// If a singular noun ends in an unstressed vowel(a, e, i, o, u) or the stressed vowels á, é or ó, add - s 
	if (word[len - 1] == 's')  // s after normal vowel ending or after foreign imported word
	{
		bool singular = false;
		if (IsVowel(word[len - 2])) singular = true;
		else if ((unsigned char)word[len-2] > 0x7f) singular = true; // some utf8 char presumed stressed vowel
		if (singular)
		{
			strcpy(base, word);
			base[len - 1] = 0; // drop the s
			properties = FindSpanishSingular(word, base, entry, canonical, sysflags);
			if (properties) properties |= (NOUN_PLURAL);
		}
	}
	// siple ends in e s 
	if (word[len - 1] == 's' && word[len - 2] == 'e')  // s after normal vowel ending or after foreign imported word
	{
		strcpy(base, word);
		base[len - 2] = 0; // drop the s
		properties = FindSpanishSingular(word, base, entry, canonical, sysflags);
		if (properties) properties |= (NOUN_PLURAL);
	}

	// Singular nouns of more than one syllable which end in -en and don’t already have an accent, add one in the plural. los exámenes
	if (!properties && word[len - 1] == 's' && word[len - 2] == 'e')
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

		properties = FindSpanishSingular(word, base, entry, canonical, sysflags);
		if (properties)	 properties = (NOUN_PLURAL);
	}

	return properties; // SHOULD BE DETECTED
}

uint64 ComputeSpanishNoun(char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	entry = D;
	uint64 properties = D->properties & NOUN;

	// cheating ordinal number?  adding avo to normal number
	size_t len = strlen(word);
	if (len > 3 && !strcmp(word + len - 3, "avo")) 
	{
		WORDP D = FindWord(word, len - 3);
		if (D && D->properties & NOUN_NUMBER)
		{
			entry = StoreWord(word, AS_IS);
			canonical = D;
			properties = entry->properties | PLACE_NUMBER;
			return properties;
		}
	}
	WORDP num = FindWord(word, 0);
	if (num && num->properties & PLACE_NUMBER)
	{
		entry = num;
		canonical = num;
		properties = entry->properties;
		if (num->word[len - 1] == 'o') properties |= NOUN_HE;
		else if (num->word[len - 1] == 'a') properties |= NOUN_SHE;
		return properties;
	}

	uint64 pluralProperties = ComputeSpanishPluralNoun(word, D, entry, canonical, sysflags);
	if (pluralProperties) properties |= pluralProperties;
	else if (properties) properties |= (NOUN_SINGULAR | SINGULAR_PERSON);

	if (!canonical) canonical = D;

	char ending = word[len - 1];
	char end2 = word[len - 2];

	size_t lenc = strlen(canonical->word);
	char root = canonical->word[lenc - 1];

	// gender
	properties |= ComputeSpanishNounGender(D, canonical); // get gender from canonical form of noun

	if (properties) properties |= NOUN;
	return properties;
}

// summary table of Spanish adjective endings!
//Masculine		Feminine
//Singular	Plural	Singular	Plural
//- o - os - a - as
//- e - es - e - es
//- ista - istas - ista - istas
//- z - ces - z - ces
//- or -ores - ora - oras
//- ón - ones - ona - onas
//- ín - ines - ina - inas

uint64 ComputeSpanishAdjective(char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{ // determine lemma, plurality, and gender
	WORDP oldEntry = entry;
	WORDP oldCanonical = canonical;
	entry = D;
	uint64 properties = 0;
	char copy[MAX_WORD_SIZE];
	strcpy(copy, word);
	size_t len = strlen(copy);
	char end1 = word[len - 1];
	char end2 = word[len - 2];
	char end3 = word[len - 3];

	if (!stricmp(word, "gran") || !stricmp(word, "grandes"))
	{
		properties = NOUN_SHE | NOUN_HE;
		return properties;
	}
	if (end1 == 'a') // singular  female
	{
		//For adjectives that end in o, the feminine is created by changing to - a,
		copy[len - 1] = 'o';
		canonical = FindWord(copy, len, LOWERCASE_LOOKUP,true);
		copy[len - 1] = 'a'; // revert
		if (canonical && canonical->properties & ADJECTIVE)
		{
			properties = NOUN_SHE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
			return properties;
		}
	}
	if ((end2 == 'n' || end2 == 'r') && end1 == 'a') // singular exception female
	{
		//For adjectives that end in - n or -r, * the feminine is created by adding - a,
			// habladora
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			properties = NOUN_SHE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
			return properties;
		}
	}

	// For singular Spanish adjectives that end in any letter other than an ‘o’, you don’t have to do anything with the ending 
	if (end1 != 'o' && end1 != 's' && D->properties & ADJECTIVE) // but check for not plural s
	{
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE)
		{
			properties = SINGULAR_PERSON | NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
			return properties;
		}
	}

	// https://www.spanishdict.com/guide/descriptive-adjectives-in-spanish#:~:text=In%20Spanish%2C%20adjectives%20must%20agree,feminine%20AND%20plural%20as%20well.
	// https://www.lawlessspanish.com/grammar/adjectives/adjectives/
	// https://www.realfastspanish.com/grammar/spanish-adjectives
// PLURALIZING:
//	Masculine and feminine adjectives that end in the vowels ‘o’, ‘a’and ‘e’ such as largo, pasota and pobre.
// 	   	For adjectives that end in an ‘o’, ‘a’and ‘e’, all you have to do to match plural nouns is add the letter ‘s’.
// Adjectives that end in a consonant such as joven,  igual.
//		For adjectives that end in a consonant, you have to add ‘es’ :
/// Adjectives that end in a ‘z’ such as feliz, eficazand capaz.
//		For adjectives that end in a ‘z’, you have to replace the ‘z’ with a ‘c’ and add ‘es’:

	// RULE 0 some adjectives do not change at all, being both gender and both plurality
	if (!stricmp(word, "rosa") || !stricmp(word, "naranja") || !stricmp(word, "cada") || !stricmp(word, "violeta"))
	{
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE)
		{
			properties = NOUN_HE | NOUN_SHE | SINGULAR_PERSON  | ADJECTIVE | ADJECTIVE_NORMAL;
			return properties;
		}
	}
	// RULE 1 but legal plurals for above are also both genders
	if (!stricmp(word, "rosas") || !stricmp(word, "naranjas") || !stricmp(word, "cadas") || !stricmp(word, "violetas"))
	{
		properties = NOUN_HE | NOUN_SHE | ADJECTIVE | ADJECTIVE_NORMAL;
		copy[len - 1] = 0;
		canonical = GetLanguageWord(copy);
		return properties;
	}
	// RULE 3	Adjectives that end in e or -ista do not change according to gender.  do change for number.
	if (!stricmp(word + len - 4, "ista")) // RULE 3 FORM 1 ista singular
	{
		// Mi profesora es muy idealista.
		properties = NOUN_SHE | NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE) return properties;
	}
	else if (!stricmp(word + len - 5, "istas")) // RULE 3 FORM 1 ista plural
	{
		// Mi profesora es muy idealistas.
		properties = NOUN_SHE | NOUN_HE  | ADJECTIVE | ADJECTIVE_NORMAL;
		copy[len - 1] = 0;
		canonical = GetLanguageWord(copy);
		properties |= NOUN_SHE | NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		return properties;
	}
	else if (end1 == 'e' && stricmp("me", word))  // RULE 3 FORM 2 e singular and not "me"
	{
		// Mi abuela es interesante
		properties = NOUN_SHE | NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE) return properties;
	}
	else if (end2 == 'e' && end1 == 's')  // RULE 3 FORM 2 e plural
	{
		// Los árboles son verdes
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return  NOUN_HE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}

	// RULE 4 Most adjectives that end in a consonant do not change according to gender, but do change for number.
	// When the adjective ends in any consonant except - n, -r, or -z, there is no difference between the masculine and feminine forms.
	// Exception 1: Adjectives that end in -or , -ón, or -ín do have feminine forms.Simply add a or -as to the masculine singular form and delete the written accent if necessary.
	// Exception 2: ones ending in z change to c before pluralizing
	// Exception 3: Adjectives ending in -erior do not have a feminine form.
	if (!stricmp(copy + len - 6, "eriors")) // rule 4 exception 3 erior masculine plural
	{
		// Está en el patio exterior del edificio.
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return NOUN_HE | NOUN_SHE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (!stricmp(copy + len - 5, "erior")) // rule 4 exception 3 erior masculine singular
	{
		// Está en el patio exterior del edificio.
		properties = NOUN_HE | NOUN_SHE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE) return properties;
	}
	if (end1 != 's' && !IsVowel(end1) && (end1 != 'n' && end1 != 'r' && end1 != 'z')) // RULE 4 singular
	{
		// El coche es azul.
		properties = NOUN_SHE | NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE) return properties;
	}
	if (D->properties & ADJECTIVE && (end1 == 'n' || end1 == 'r')) // singular exception male
	{
		// hablador
		properties = NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		canonical = D;
		if (canonical && canonical->properties & ADJECTIVE) return properties;
	}

	if (!stricmp(copy + len - 3, "ora")) // rule 4 exception 1 female singular
	{
		// Mi hermana es trabajadora.
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return properties | SINGULAR_PERSON | NOUN_SHE | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (!stricmp(copy + len - 4, "oras")) // rule 4 exception 1 female plural
	{
		// Mis hermanas son trabajadoras.
		canonical = FindWord(copy, len - 2, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return properties | NOUN_SHE | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (!strcmp(copy + len - 4, u8"óna") || !strcmp(copy + len - 4, u8"ína")) // rule 4 exception 1 female singular
	{
		// Paula es cabezona.
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
		if (!canonical) // try without the accent
		{
			if (!strcmp(copy + len - 4, u8"óna")) strcpy(copy + len - 4, "on");
			else strcpy(copy + len - 4, "ina");
			canonical = FindWord(copy, 0, LOWERCASE_LOOKUP,true);
			strcpy(copy, word); // restore
		}
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return properties | SINGULAR_PERSON | NOUN_SHE | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (!stricmp(copy + len - 1 - 5, u8"ónas") || !stricmp(copy + len - 5, u8"ínas")) // rule 4 exception 1 female plural
	{
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
		if (!canonical)
		{
			if (!strcmp(copy + len - 5, u8"ónas")) strcpy(copy + len - 5, "on");
			else strcpy(copy + len - 5, "ina");
			canonical = FindWord(copy, 0, LOWERCASE_LOOKUP,true);
			strcpy(copy, word); // restore
		}
		if (canonical && canonical->properties & ADJECTIVE)
		{
			properties |= SINGULAR_PERSON | NOUN_SHE | ADJECTIVE | ADJECTIVE_NORMAL;
			return properties;
		}
	}
	if (end2 == 'e' && end1 == 's' && end3 == 'c') // RULE 4 exception 2  z  changed to c for plural
	{
		// Los gatos son felices.
		copy[len - 3] = 'z';
		canonical = FindWord(copy, len - 2, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return  NOUN_HE | NOUN_SHE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (end2 == 'e' && end1 == 's' && !IsVowel(end3) && (end3 != 'n' && end3 != 'r' && end3 != 'z')) // RULE 4 exception 2 plural
	{
		// Los enigmas son fáciles.
		canonical = FindWord(word, len - 2, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return NOUN_HE | NOUN_SHE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	// RULE 2: Adjectives that end in o in the masculine singular form have four possible endings, one each for masculine, feminine, singular, and plural. 
	// These types of adjectives make up the majority of adjectives in Spanish.
	if (!strcmp(copy + len - 2, "os")) // RULE2 ENDING 1 male plural ending?
	{
		// El hombre es alto. (The man is tall.)	Los hombres son altos. (The men are tall.)
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);  // see if male singular canonical there
		if (canonical && canonical->properties & ADJECTIVE)
		{
			canonical = D;
			return NOUN_HE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}

	if (!strcmp(copy + len - 2, "es")) // 
	{
		if (end3 == 'n' || end3 == 'r') // canonical ends in n or r
		//When the masculine adjective ends in - a, there is no difference between the masculineand feminine forms.
			canonical = FindWord(copy, len - 2, LOWERCASE_LOOKUP,true); // canoncal ends in n or r
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return NOUN_HE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (!strcmp(copy + len - 2, "as")) // RULE2 ENDING 2  female plural ending?
	{
		//When the masculine adjective ends in - a, there is no difference between the masculineand feminine forms.
		canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true); // canoncal ends in a?
		if (!canonical && (end3 == 'n' || end3 == 'r')) // canonical ends in n or r
		{
			canonical = FindWord(copy, len - 2, LOWERCASE_LOOKUP,true);  // see if male singular canonical there
		}
		if (!canonical) // otherwise ending in o?
		{
			// Soy delgada. (I'm thin.)  	Somos delgadas. (We're thin )  		realista	 	realistas
			copy[len - 2] = 'o'; // change to male singular ending and look
			canonical = FindWord(copy, len - 1, LOWERCASE_LOOKUP,true);
			copy[len - 2] = 'a'; // change back
		}

		if (canonical && canonical->properties & ADJECTIVE)
		{
			return NOUN_SHE  | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (end1 == 'o') // RULE2 ENDING 4 default male singular ending? already the lemma
	{
		canonical = FindWord(copy, len, LOWERCASE_LOOKUP,true);
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return NOUN_HE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL;
		}
	}
	if (end1 == 'a') // RULE2 ENDING 3 default female singular ending?
	{
		//When the masculine adjective ends in - a, there is no difference between the masculineand feminine forms.
		canonical = FindWord(copy, len, LOWERCASE_LOOKUP,true); // canoncal ends in a?
		if (!canonical)
		{
			//	Es un perro negro. (	It's a black dog-m.) 		Es una camisa negra. (It's a black shirt-f.)
			copy[len - 1] = 'o'; // change to male singular ending and look
			canonical = FindWord(copy, len, LOWERCASE_LOOKUP,true);
			copy[len - 1] = 'a'; // change back
		}
		else properties = NOUN_HE; // observe this
		if (canonical && canonical->properties & ADJECTIVE)
		{
			return properties | NOUN_SHE | SINGULAR_PERSON | ADJECTIVE | ADJECTIVE_NORMAL; // using OR intentionally
		}
	}

	entry = oldEntry;
	canonical = oldCanonical;
	return 0;
}

struct SpanishVerbParams {
	uint64 properties;
	uint64 sysflags;
	uint64 baseflags;           // flags for base == legitimate for object
};

static SpanishVerbParams RemoveDirectObjectEndings(char* base)
{
	SpanishVerbParams dop{};   // direct object params
	size_t len = strlen(base);

	if (len < 3) {}
	else if (!stricmp(base + len - 3, "nos"))
	{
		base[len - 3] = '\0';
		dop.properties = PRONOUN_OBJECT;
		dop.baseflags = VERB_INFINITIVE | NOUN_GERUND;
		dop.sysflags = PRONOUN_OBJECT_I | PRONOUN_OBJECT_PLURAL;
	}
	else if (!stricmp(base + len - 3, "los") || !stricmp(base + len - 3, "las"))
	{
		base[len - 3] = '\0';
		dop.properties = PRONOUN_OBJECT;
		dop.baseflags = VERB_INFINITIVE | NOUN_GERUND;
		dop.sysflags = PRONOUN_OBJECT | PRONOUN_OBJECT_PLURAL;
	}
	else if (len > 4 && ( // defer to regular conjugation, this will repeat later cominamos protected
		!stricmp(base + len - 4, "amos") ||
		!stricmp(base + len - 4, "emos") ||
		!stricmp(base + len - 4, "imos"))) {
	}
	else if (!stricmp(base + len - 2, "me"))
	{
		base[len - 2] = '\0';
		dop.properties = PRONOUN_OBJECT;
		dop.baseflags = VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		dop.sysflags = PRONOUN_OBJECT_I | PRONOUN_OBJECT_SINGULAR;
	}
	else if (!stricmp(base + len - 2, "te") ||
		!stricmp(base + len - 2, "os"))
	{
		base[len - 2] = '\0';
		dop.properties = PRONOUN_OBJECT;
		dop.baseflags = VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		dop.sysflags = PRONOUN_OBJECT_YOU | PRONOUN_OBJECT_SINGULAR;
	}
	else if (!stricmp(base + len - 2, "lo") || !stricmp(base + len - 2, "la"))
	{
		base[len - 2] = '\0';
		dop.properties = PRONOUN_OBJECT;
		dop.baseflags = VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		dop.sysflags = PRONOUN_OBJECT_SINGULAR | PRONOUN_OBJECT_HE_SHE_IT;
	}

	return dop;
}

static SpanishVerbParams RemoveIndirectObjectEndings(char* base)
{                               // iop == indirect object params
	size_t len = strlen(base);
	SpanishVerbParams iop{};

	if (len < 3) {}
	else if (!stricmp(base + len - 3, "nos"))
	{
		base[len - 3] = '\0';
		iop.baseflags |= VERB_INFINITIVE | NOUN_GERUND; // need to compute participles in some regular verbs
		iop.sysflags |= PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_I | PRONOUN_INDIRECTOBJECT_PLURAL;
	}
	else if (!stricmp(base + len - 3, "les"))
	{
		base[len - 3] = '\0';
		iop.baseflags |= VERB_INFINITIVE | NOUN_GERUND; // need to compute participles in some regular verbs
		iop.sysflags |= PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_PLURAL;
		iop.sysflags |= PRONOUN_INDIRECTOBJECT_HE_SHE_IT;
	}
	// nos must be tested before os
	else if (!stricmp(base + len - 2, "me"))
	{
		base[len - 2] = '\0';
		iop.baseflags |= VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		iop.sysflags |= PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_I | PRONOUN_INDIRECTOBJECT_SINGULAR;
	}
	else if (!stricmp(base + len - 2, "te") || !stricmp(base + len - 2, "os"))
	{
		base[len - 2] = '\0';
		iop.baseflags |= VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		iop.sysflags |= PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_YOU | PRONOUN_INDIRECTOBJECT_SINGULAR;
	}
	else if (!stricmp(base + len - 2, "le"))
	{
		base[len - 2] = '\0';
		iop.baseflags |= VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		iop.sysflags |= PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_SINGULAR;
		iop.sysflags |= PRONOUN_INDIRECTOBJECT_HE_SHE_IT;
	}
	else if (!stricmp(base + len - 2, "se"))
	{
		base[len - 2] = '\0';
		iop.baseflags |= VERB_INFINITIVE | NOUN_GERUND | VERB_PRESENT_3PS;
		iop.sysflags |= PRONOUN_INDIRECTOBJECT | PRONOUN_INDIRECTOBJECT_PLURAL;
	}
	return iop;
}

static WORDP checkLegitimate(char* base, SpanishVerbParams params)
{
	WORDP D{ FindSpanishWordRemoveAccent(base) };
	WORDP tmp_entry{ nullptr };
	WORDP tmp_canonical{ nullptr };
	uint64 tmp_flags{ 0 };

	if (D && (D->properties & params.baseflags)) return D;
	if (ImperativeSpanish(base, tmp_entry, tmp_canonical, tmp_flags)) return tmp_canonical;
	return nullptr;
}

uint64 HandleObjectSuffixesSpanish(char* word, int at, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	char tmp[MAX_WORD_SIZE];  // contains the working version of the word without the suffixes
	char base[MAX_WORD_SIZE];   // contains the word without the suffixes
	strcpy(tmp, word);

	uint64 irprops = IrregularSpanishVerb(tmp, entry, canonical, sysflags);
	if (irprops) return 0;

	SpanishVerbParams indirect_only_params = RemoveIndirectObjectEndings(tmp);
	SpanishVerbParams return_params = {};
	WORDP indirect_only_canonical{};
	if (indirect_only_params.sysflags)
	{
		indirect_only_canonical = checkLegitimate(tmp, indirect_only_params);
		if (indirect_only_canonical)
		{
			return_params = indirect_only_params;
			strcpy(base, tmp);
		}
	}

	strcpy(tmp, word);
	SpanishVerbParams direct_only_params = RemoveDirectObjectEndings(tmp);
	WORDP direct_only_canonical{};
	SpanishVerbParams direct_and_indirect_params{};
	WORDP direct_and_indirect_canonical{};
	if (direct_only_params.sysflags || direct_only_params.properties)
	{
		direct_only_canonical = checkLegitimate(tmp, direct_only_params);
		if (direct_only_canonical)
		{
			return_params.sysflags |= direct_only_params.sysflags;
			return_params.properties |= direct_only_params.properties;
			strcpy(base, tmp);
		}

		direct_and_indirect_params = RemoveIndirectObjectEndings(tmp);
		if (direct_and_indirect_params.sysflags)
		{
			direct_and_indirect_canonical = checkLegitimate(tmp, direct_and_indirect_params);
			if (direct_and_indirect_canonical)
			{
				return_params.sysflags |= direct_only_params.sysflags;
				return_params.sysflags |= direct_and_indirect_params.sysflags;
				return_params.properties |= direct_only_params.properties;
				return_params.properties |= direct_and_indirect_params.properties;
				strcpy(base, tmp);
			}
		}
	}

	WORDP object_canonical{};
	if (direct_only_canonical) object_canonical = direct_only_canonical;
	else if (indirect_only_canonical) object_canonical = indirect_only_canonical;
	else if (direct_and_indirect_canonical) object_canonical = direct_and_indirect_canonical;

	if (object_canonical)
	{
		sysflags |= return_params.sysflags;
		strcpy(word, base);
		canonical = object_canonical;
	}

	return return_params.properties;
}

uint64 ComputeSpanishAdverb(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags) // case sensitive, may add word to dictionary, will not augment flags of existing words
{
	uint64 properties = 0;
	// adverb from adjective
	const char* mente = strstr(original, "mente");
	if (mente && !mente[5]) // see if root is adjective
	{
		WORDP Q = FindWord(original, strlen(original) - 5,PRIMARY_CASE_ALLOWED,true);
		if (Q && Q->properties & ADJECTIVE)
		{
			if (!exportdictionary) entry = GetLanguageWord(original);
			else entry = canonical = dictionaryBase + 1;
			sysflags = 0;
			properties |= ADVERB;
		}
	}
	return properties;
}

static WORDP FindCanonicalFromStem(const char* word, const char* const suffixes[4], size_t stem_len)
{
	char base[MAX_WORD_SIZE];
	strcpy(base, word);
	int index = 0;
	const char* cur_suffix = suffixes[index++];
	while (*cur_suffix) {
		strcpy(base + stem_len, cur_suffix);
		WORDP D = FindSpanishWordRemoveAccent(base);
		if (D && (D->properties & VERB_INFINITIVE)) return D;
		cur_suffix = suffixes[index++];
	}
	return nullptr;
}

static uint64 OtherVerbSpanish(const char* word, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	struct TableEntry {
		const uint64 properties;
		const uint64 sysflags;
		const char* infinitive_suffixes[4];
		const char* word_suffixes[4];
	};
	TableEntry endTable[] = {
		// Present progressive
		{
			VERB | VERB_PRESENT_PARTICIPLE, // properties
			PRONOUN_I | PRONOUN_YOU | PRONOUN_HE_SHE_IT | PRONOUN_SINGULAR | PRONOUN_PLURAL, // sysflags
			{"ar", ""},         // .infinitive_suffixes
			{"ando", ""}        // .word_suffixes
		},
		{
			VERB | VERB_PRESENT_PARTICIPLE, // properties
			PRONOUN_I | PRONOUN_YOU | PRONOUN_HE_SHE_IT | PRONOUN_SINGULAR | PRONOUN_PLURAL, // sysflags
			{"er", "ir", ""},     // infinitive_suffixes 
			{"iendo", ""}         // word_suffixes 
		}
	};

	uint64 tmp_properties = 0;
	uint64 tmp_sysflags = 0;
	size_t word_len = strlen(word);
	WORDP tmp_canon = nullptr;

	for (const TableEntry& te : endTable) {
		const auto& word_suffixes = te.word_suffixes;
		int word_index = 0;
		const char* word_suffix = word_suffixes[word_index++];
		while (*word_suffix) {
			size_t word_suffix_len = strlen(word_suffix);
			if (strlen(word) > word_suffix_len)
			{
				int stem_size = word_len - word_suffix_len;
				if (!stricmp(word_suffix, word + stem_size)) {
					WORDP Dcanon = FindCanonicalFromStem(word, te.infinitive_suffixes, stem_size);
					if (Dcanon) {
						tmp_properties |= te.properties;
						tmp_sysflags |= te.sysflags;
						tmp_canon = Dcanon;
					}
				}
			}
			word_suffix = word_suffixes[word_index++];
		}
	}
	if (tmp_properties) {
		sysflags |= tmp_sysflags;
		canonical = tmp_canon;
	}
	return tmp_properties;
}

uint64 ComputeSpanishVerb(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags) // case sensitive, may add word to dictionary, will not augment flags of existing words
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

	uint64 ans = ReflexiveVerbSpanish(word, entry, canonical, sysflags);
	if (ans) return ans;

	// if handleobjectsuffixes finds one, it is stripped off and returned in word, 
	// which should now be a verb conjugation normal, no object refs
	properties |= HandleObjectSuffixesSpanish(word, at, entry, canonical, sysflags);
	
	// irregular verb
	uint64 tense = IrregularSpanishVerb(word, entry, canonical, sysflags);
	if (tense)
	{
		return properties | tense; // full description provied by function
	}

	// present tense
	tense = PresentSpanish(word, entry, canonical, sysflags);
	if (tense)
	{
		return properties | tense;
	}

	// past  tense 
	tense = PastSpanish(word, entry, canonical, sysflags);
	if (tense)
	{
		sysflags = entry->systemFlags;
		return properties | tense;
	}

	// future   tense 
	tense = FutureSpanish(word, entry, canonical, sysflags);
	if (tense)
	{
		return properties | tense;
	}
	// past participle   tense 
	tense = PastParticipleSpanish(word, original, entry, canonical, sysflags);
	if (tense)
	{
		return properties | tense;
	}
	// gerund
	tense = GerundSpanish(word, word, entry, canonical, sysflags);
	if (tense)
	{
		return properties | tense;
	}
	// infinitive
	tense = InfinitiveSpanish(word, word, entry, canonical, sysflags);
	if (tense)
	{
		return properties | tense;
	}

	tense = OtherVerbSpanish(word, entry, canonical, sysflags);
	if (tense) 
	{
		return properties | tense;
	}

	// deferred os ending
	if (len > 4 && (!stricmp(base + len - 4, "amos") || !stricmp(base + len - 4, "emos") || !stricmp(base + len - 4, "imos"))) //deferred from above
	{
		char base[MAX_WORD_SIZE];
		strcpy(base, word);
		base[len - 2] = 0; // os ending
		WORDP D = FindSpanishWord(base, word);
		if (D && D->properties & (VERB_INFINITIVE | NOUN_GERUND))
		{
			uint64 flags = PRONOUN_OBJECT_SINGULAR;
			if (word[len - 2] == 'm') flags |= PRONOUN_OBJECT_I;
			else if (word[len - 2] == 't') flags |= PRONOUN_OBJECT_YOU;
			entry = GetLanguageWord(word);
			return properties | PRONOUN_OBJECT;
		}
	}
	return properties;
}

// primary entry point from spellcheck or GetPosData
uint64 ComputeSpanish1(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	// tt dictionary may have excess data (noun singular and plural, which is bogus)
	// 
	// can we find this word as a conjugation of something?
	char word[MAX_WORD_SIZE];
	sysflags = 0;
	strcpy(word, original);
	MakeLowerCase(word); // only nouns MIGHT really be uppercase and we dont worry about their listings in the dictionary
	WORDP D = GetLanguageWord(word); // ensure in dictionary
	canonical = GetCanonical(D, D->properties);

	// we need to check for pronouns because dict doesnt have them
	// dont use verb if conflict conjunction or preposition (para is one as it is also a command parar form)
	// WE DONT exclude adjective here because some verb forms are adjective as well 
	// verb infinitive we dont do here, because we might have decided it is a pronoun conjugation of an infinitive buscarlo
	if (D->properties & (DETERMINER | CONJUNCTION | PREPOSITION) ||
		(D->properties & PRONOUN_SUBJECT && !(D->properties & VERB)))// pure pronoun
	{
		if (!canonical) canonical = D;
		entry = D;
		sysflags = D->systemFlags;
		return D->properties;
	}

	uint64 properties = D->properties & ADVERB; // adverb code sucks (like adverb for ahí)
	uint64 old_sysflags;

	// Note for a sentence of  a single word, there is no way to disambituate
	old_sysflags = sysflags; // inherit old sysflags
	uint64 ans = ComputeSpanishAdjective(word, D, entry, canonical, sysflags);
	if (!ans) sysflags = old_sysflags;
	else properties |= ans;

	old_sysflags = sysflags;
	ans = ComputeSpanishNoun(word, D, entry, canonical, sysflags);
	if (!ans) sysflags = old_sysflags;
	else properties |= ans;

	// note botas is noun (plural) but verb (singular)
	old_sysflags = sysflags;
	ans = ComputeSpanishVerb(at, original, entry, canonical, sysflags); // case sensitive, may add word to dictionary, will not augment flags of existing words
	if (!ans) sysflags = old_sysflags;
	else properties |= ans;

	old_sysflags = sysflags;
	ans = ComputeSpanishAdverb(at, original, entry, canonical, sysflags); // case sensitive, may add word to dictionary, will not augment flags of existing words
	if (!ans) sysflags = old_sysflags;
	else properties |= ans;

	// accept what dict has (desperation, lacks lemma) or adjective or conjugates 
	if (!properties && D->properties & (VERB | ADJECTIVE | NOUN))
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
	if (properties)
	{
		entry = D;
		AddProperty(entry, properties);
		AddSystemFlag(entry, sysflags);
		properties = entry->properties; // The merge of entry and our new properties
	}

	return properties;
}

static uint64 ComputeFilipinoNoun(int at,char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	entry = D;
	uint64 properties = D->properties & NOUN;
	if (properties && at > 1 && !stricmp(wordStarts[at - 1], "mga"))
	{
		entry = D;
		canonical = D;
		return NOUN | NOUN_PLURAL;
	}

	// affixes indicating plural
	if (!strnicmp(word, "mag", 3))
	{
		WORDP E = FindWord(word + 3);
		if (E && E->properties & NOUN)
		{
			entry = D;
			canonical = E;
			return NOUN | NOUN_PLURAL;
		}
	}
	if (!strnicmp(word, "magka", 5))
	{
		WORDP E = FindWord(word + 5);
		if (E && E->properties & NOUN)
		{
			entry = D;
			canonical = E;
			return NOUN | NOUN_PLURAL;
		}
	}
	
	// s plural?
	size_t len = strlen(word);
	if (word[len - 1] == 's')
	{
		WORDP E = FindWord(word, len - 1);
		if (E && E->properties & NOUN_SINGULAR)
		{
			entry = D;
			canonical = E;
			return NOUN | NOUN_PLURAL;
		}
	}

	if (!properties) return 0; // not a basic noun

	// default singular
	return NOUN | NOUN_SINGULAR;
}

static uint64 ComputeFilipinoVerb(int at, char* word, WORDP D, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	entry = D;
	uint64 properties = D->properties & VERB;
	// MAG verbs
	if (!strnicmp(word, "mag", 3) && !strncmp(word+3,word+5,2)) // repeated syllable pair (maybe more)
	{
		WORDP D = FindWord(word + 5);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_INFINITIVE; // actually future
		}
	}
	if (!strnicmp(word, "mag", 3)) 
	{
		WORDP D = FindWord(word + 3);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_INFINITIVE; 
		}
	}
	if (!strnicmp(word, "nag", 3) && !strncmp(word + 3, word + 5, 2) )
	{
		WORDP D = FindWord(word + 5);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_PRESENT; 
		}
	}
	if (!strnicmp(word, "nag", 3))
	{
		WORDP D = FindWord(word + 3);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_PAST;
		}
	}

	// MA verbs
	if (!strnicmp(word, "ma", 2) && !strncmp(word + 2, word + 4, 2)) // repeated syllable pair (maybe more)
	{
		WORDP D = FindWord(word + 4);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_INFINITIVE; // actually future
		}
	}
	if (!strnicmp(word, "ma", 2))
	{
		WORDP D = FindWord(word + 2);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_INFINITIVE;
		}
	}
	if (!strnicmp(word, "na", 2) && !strncmp(word + 2, word + 4, 2))
	{
		WORDP D = FindWord(word + 2);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_PRESENT;
		}
	}
	if (!strnicmp(word, "na", 2))
	{
		WORDP D = FindWord(word + 2);
		if (D && D->properties & VERB)
		{
			entry = D;
			return VERB | VERB_PAST;
		}
	}

	// UM verbs

	// IN Verbs

	// O to U verbs

	// I verbs

	if (!properties) return 0;

	return VERB | VERB_INFINITIVE;
}

static uint64 ComputeFilipino1(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{
	char word[MAX_WORD_SIZE];
	sysflags = 0;
	strcpy(word, original);
	MakeLowerCase(word); // only nouns MIGHT really be uppercase and we dont worry about their listings in the dictionary
	WORDP D = GetLanguageWord(word); // ensure in dictionary
	canonical = GetCanonical(D, D->properties);

	// we need to check for pronouns because dict doesnt have them
	// dont use verb if conflict conjunction or preposition (para is one as it is also a command parar form)
	// WE DONT exclude adjective here because some verb forms are adjective as well 
	// verb infinitive we dont do here, because we might have decided it is a pronoun conjugation of an infinitive buscarlo
	if (D->properties & (DETERMINER | CONJUNCTION | PREPOSITION) ||
		(D->properties & PRONOUN_SUBJECT && !(D->properties & VERB)))// pure pronoun
	{
		if (!canonical) canonical = D;
		entry = D;
		sysflags = D->systemFlags;
		return D->properties;
	}

	uint64 ans = ComputeFilipinoNoun(at,word, D, entry, canonical, sysflags);
	return ans;
}

uint64 ComputeFilipino(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{ // this interface actually changes a dict entry
	WORDP old = FindWord(original, 0, PRIMARY_CASE_ALLOWED, true);
	uint64 properties = ComputeFilipino1(at, original, entry, canonical, sysflags);
	if (properties) return properties;

	WORDP D = GetLanguageWord(original); // find actual entry for spanish itself
	D->properties |= properties;
	D->systemFlags |= sysflags;

	WORDP E = FindWord(original); // find appropriate entry which may be universal
	if (E != D && E->internalBits & UNIVERSAL_WORD && old != D) // update universal (transient)
	{
		if (E->internalBits & UNIVERSAL_WORD)
		{// BUG could become freed
			WORDP* foreign = E->foreignFlags;
			ongoingUniversalDictChanges = AllocateHeapval(HV1_WORDP | HV2_INT | HV3_INT, ongoingUniversalDictChanges, (uint64)D, (uint64)D->foreignFlags, true);
			E->foreignFlags = (WORDP*)AllocateHeap(NULL, languageCount, sizeof(WORDP), true);
			if (foreign) memcpy(ongoingUniversalDictChanges, E->foreignFlags, sizeof(WORDP*) * languageCount);
			E->foreignFlags[GET_FOREIGN_INDEX(E)] = D;
		}
	}
	return properties;
}

uint64 ComputeSpanish(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags)
{ // this interface actually changes a dict entry

	WORDP old = FindWord(original, 0, PRIMARY_CASE_ALLOWED, true);
	uint64 properties = ComputeSpanish1(at, original,  entry, canonical, sysflags);

	WORDP D = GetLanguageWord(original); // find actual entry for spanish itself
	D->properties |= properties;
	D->systemFlags |= sysflags;

	WORDP E = FindWord(original); // find appropriate entry which may be universal
	if (E != D && E->internalBits & UNIVERSAL_WORD && old != D) // update universal (transient)
	{
		if (E->internalBits & UNIVERSAL_WORD)
		{// BUG could become freed
			WORDP* foreign = E->foreignFlags;
			ongoingUniversalDictChanges = AllocateHeapval(HV1_WORDP | HV2_INT | HV3_INT, ongoingUniversalDictChanges, (uint64)D, (uint64)D->foreignFlags, true);
			E->foreignFlags = (WORDP*)AllocateHeap(NULL, languageCount, sizeof(WORDP), true);
			if (foreign) memcpy(ongoingUniversalDictChanges, E->foreignFlags, sizeof(WORDP*) * languageCount);
			E->foreignFlags[GET_FOREIGN_INDEX(E)] = D;
		}
	}
	return properties;
}

static void ShowSpanishProperties(uint64 properties)
{
	if (properties & VERB) printf("~verb ");
	if (properties & VERB_INFINITIVE) printf("~verb_infinitive ");
	if (properties & VERB_PRESENT) printf("~verb_present ");
	if (properties & VERB_PAST) printf("~verb_past ");
	if (properties & AUX_VERB_FUTURE)
	{
		printf("~AUX_VERB_FUTURE ");
		printf("~verb_future ");
	}
	if (properties & PRONOUN_OBJECT) printf("~pronoun_object ");
	if (properties & NOUN) printf("~noun ");
	if (properties & NOUN_SINGULAR) printf("~noun_singular ");
	if (properties & NOUN_PLURAL) printf("~noun_plural ");
	if (properties & NOUN_NUMBER) printf("~noun_number ");
	if (properties & NOUN_GERUND) printf("~noun_gerund ");

	if (properties & ADVERB) printf("~adverb ");
	if (properties & ADJECTIVE) printf("~adjective ");
	if (properties & PREPOSITION) printf("~preposition ");
	if (properties & CONJUNCTION) printf("~conjunction ");
	if (properties & DETERMINER) printf("~determiner ");
	if (properties & NOUN_HE) printf("~NOUN_HE ");
	if (properties & NOUN_SHE) printf("~NOUN_SHE ");
	if (properties & SINGULAR_PERSON) printf("~SINGULAR_PERSON ");
	printf("\r\n");
}

static void ShowSpanishSysflags(uint64 systemFlags)
{
	if (systemFlags & VERB_IMPERATIVE) printf("~verb_imperative ");
	if (systemFlags & PRONOUN_SINGULAR) printf("~pronoun_singular ");
	if (systemFlags & PRONOUN_PLURAL) printf("~pronoun_plural ");
	if (systemFlags & PRONOUN_I) printf("~pronoun_I ");
	if (systemFlags & PRONOUN_YOU) printf("~pronoun_you ");

	if (systemFlags & PRONOUN_OBJECT_SINGULAR) printf("~pronoun_object_singular ");
	if (systemFlags & PRONOUN_OBJECT_PLURAL) printf("~pronoun_object_plural ");
	if (systemFlags & PRONOUN_OBJECT_I) printf("~pronoun_object_i ");
	if (systemFlags & PRONOUN_OBJECT_YOU) printf("~pronoun_object_you ");
	if (systemFlags & PRONOUN_INDIRECTOBJECT) printf("~pronoun_indirectobject ");
	if (systemFlags & PRONOUN_INDIRECTOBJECT_SINGULAR) printf("~pronoun_indirectobject_singular ");
	if (systemFlags & PRONOUN_INDIRECTOBJECT_PLURAL) printf("~pronoun_indirectobject_plural ");
	if (systemFlags & PRONOUN_INDIRECTOBJECT_I) printf("~pronoun_indirectobject_i ");
	if (systemFlags & PRONOUN_INDIRECTOBJECT_YOU) printf("~pronoun_indirectobject_you ");
	printf("\r\n");
}

static void ShowSpanishAttributes(uint64 properties, uint64 systemFlags)
{
	if (properties) printf("Properties: ");
	ShowSpanishProperties(properties);
	if (systemFlags) printf("SystemFlags: ");
	ShowSpanishSysflags(systemFlags);
}

void C_ComputeSpanish(char* word)
{
	WORDP entry, canonical = NULL;
	uint64 sysflags = 0;
	SetLanguage("spanish");
	wordCount = 2; // avoid the 1 ignore restriction
	WORDP D = GetLanguageWord(word);
	uint64 origprop = D->properties;
	uint64 origsys = D->systemFlags;
	uint64 properties = ComputeSpanish1(1, word, entry, canonical, sysflags);

	// unknown, try for shift to accented forms
	if (!properties) properties = KnownSpanishUnaccented(word, entry, sysflags);

	WORDP X = GetCanonical(entry, (uint64)-1);
	if (X) canonical = X;
	if (!canonical) canonical = D;
	printf("word: %s  actual: %s   lemma: %s\r\n", word, entry->word, canonical->word);
	printf("Computed properties : ");
	ShowSpanishAttributes(properties, sysflags);
	printf("DICT properties: ");
	ShowSpanishAttributes(origprop, origsys);
}

static int test_word(
	const char* word,
	const char* expected_canonical,
	uint64 expected_properties,
	uint64 expected_sysflags)
{
	WORDP entry = NULL;
	WORDP canonical = NULL;
	uint64 sysflags = 0;
	uint64 properties = ComputeSpanish1(1, word, entry, canonical, sysflags);
	bool got_difference = false;

	if (!canonical || stricmp(expected_canonical, canonical->word) ||
		(expected_properties & properties) != expected_properties ||
		(expected_sysflags & sysflags) != expected_sysflags)
	{
		printf("Error tagging: %s\r\n", word);
		got_difference = true;
	}
	if (!canonical) 
	{
		printf("Error: could not find canonical\n");
	}
	else if (stricmp(expected_canonical, canonical->word)) 
	{
		printf("  Expected canonical word %s doesn't match canonical %s\n",
			expected_canonical, canonical->word);
	}
	if (got_difference) 
	{
		if ((expected_properties & properties) != expected_properties)
		{
			printf("  Expect Properties: 0x%16llx   ", expected_properties);
			ShowSpanishProperties(expected_properties);
			printf("  Actual Properties: 0x%16llx   ", properties);
			ShowSpanishProperties(properties);
		}

		if ((expected_sysflags & sysflags) != expected_sysflags)
		{
			printf("  Expect sysflags: 0x%16llx   ", expected_sysflags);
			ShowSpanishSysflags(expected_sysflags);
			printf("Actual sysflags: 0x%16llx   ", sysflags);
			ShowSpanishSysflags(sysflags);
			printf("\r\n");
		}
	}
	else printf(".");
	fflush(stdout);
	return got_difference;
}

int C_JaUnitTestSpanish()
{
	const uint64 V_I{ VERB_IMPERATIVE };
	const uint64 V_INF{ VERB_INFINITIVE };
	const uint64 V_PP{ VERB_PRESENT_PARTICIPLE };
	const uint64 P_S{ PRONOUN_SINGULAR };
	const uint64 P_P{ PRONOUN_PLURAL };
	const uint64 P_Y{ PRONOUN_YOU };
	const uint64 P_I{ PRONOUN_I };
	const uint64 P_3{ PRONOUN_HE_SHE_IT };
	const uint64 P_IO{ PRONOUN_INDIRECTOBJECT };
	const uint64 P_IOP{ PRONOUN_INDIRECTOBJECT_PLURAL };
	const uint64 P_IOS{ PRONOUN_INDIRECTOBJECT_SINGULAR };
	const uint64 P_IOI{ PRONOUN_INDIRECTOBJECT_I };
	const uint64 P_IOY{ PRONOUN_INDIRECTOBJECT_YOU };
	const uint64 P_IO3{ PRONOUN_INDIRECTOBJECT_HE_SHE_IT };
	const uint64 P_O{ PRONOUN_OBJECT };
	const uint64 P_OI{ PRONOUN_OBJECT_I };
	const uint64 P_OY{ PRONOUN_OBJECT_YOU };
	const uint64 P_O3{ PRONOUN_OBJECT_HE_SHE_IT };
	const uint64 P_OS{ PRONOUN_OBJECT_SINGULAR };
	const uint64 P_OP{ PRONOUN_OBJECT_PLURAL };

	int errors = 0;

	printf("Starting spanish tests\n");
	errors += test_word("giren", "girar", VERB, P_Y | P_P | V_I);
	errors += test_word("mostrar", "mostrar", VERB | V_INF, 0); // infinitive verb form
	errors += test_word(u8"giréis", "girar", VERB, P_Y | P_S | V_I);
	errors += test_word("giremos", "girar", VERB, P_I | P_P | V_I);
	errors += test_word("hablales", "hablar", VERB, P_Y | P_S | V_I | P_IO | P_IOP);
	errors += test_word("salten", "saltar", VERB, P_Y | P_P | V_I);
	errors += test_word("hacerme", "hacer", VERB, P_IOS | P_IOI);
	errors += test_word(u8"sepárame", "separar", VERB, P_S | V_I);
	errors += test_word("algo", "algo", 0, 0);
	errors += test_word("siente", "sentir", VERB, P_Y | P_S | V_I | P_IOS | P_IOY);
	errors += test_word(u8"váyase", "ir", VERB, P_Y | P_S | V_I);
	errors += test_word("para", "parar", PREPOSITION, 0);
	errors += test_word("come", "comer", VERB, P_S | V_I);
	errors += test_word("tome", "tomar", VERB, P_S | V_I);
	errors += test_word("repite", "repetir", VERB, P_Y | P_S | V_I);
	errors += test_word("di", "decir", VERB, P_S | V_I);
	errors += test_word(u8"háblales", "hablar", VERB, P_Y | P_S | V_I | P_IO | P_IOP);
	errors += test_word(u8"cuéntanos", "contar", VERB, P_Y | P_S | V_I | P_IOP | P_IOI);
	errors += test_word("cuentanos", "contar", VERB, P_Y | P_S | V_I | P_IOP | P_IOI);
	errors += test_word("cantad", "cantar", VERB, P_Y | P_P | V_I);
	errors += test_word("bebed", "beber", VERB, P_Y | P_P | V_I);
	errors += test_word("venid", "venir", VERB, P_Y | P_P | V_I);
	errors += test_word("salte", "saltar", VERB, P_Y | P_S | V_I);
	errors += test_word("coja", "coger", VERB, P_Y | P_S | V_I);
	errors += test_word("suba", "subir", VERB, P_Y | P_S | V_I);
	errors += test_word("suban", "subir", VERB, P_Y | P_P | V_I);
	errors += test_word("gires", "girar", VERB, P_Y | P_S | V_I);
	errors += test_word("gire", "girar", VERB, P_Y | P_S | V_I);
	errors += test_word(u8"váyase", "ir", VERB, P_Y | P_S | V_I);
	errors += test_word(u8"dáselo", "dar", VERB, P_Y | P_S | V_I | P_IO | P_IOS);
	errors += test_word(u8"lávenselas", "laxar", VERB, P_S | P_Y | V_I | P_OP | P_OS);
	errors += test_word(u8"ayúdame", "ayudar", VERB, P_S | P_Y | V_I | P_IOI | P_IOS); // (you) help me
	errors += test_word("ayudame", "ayudar", VERB, P_S | P_Y | V_I | P_IOI | P_IOS);
	errors += test_word(u8"dámelo", "dar", VERB | P_O, P_S | P_Y | V_I | P_IOI | P_IOS | P_OS); // (you) give it to me
	errors += test_word("damelo", "dar", VERB | P_O, P_S | P_Y | V_I | P_IOI | P_IOS | P_OS); // (you) give it to me
	errors += test_word(u8"quitármelo", "quitar", VERB | P_O, P_IOI | P_IOS | P_OS);
	errors += test_word("quitarmelo", "quitar", VERB | P_O, P_IOI | P_IOS | P_OS);
	errors += test_word("dime", "decir", VERB | P_O, P_IO | P_IOS | P_OS);
	errors += test_word(u8"escribiéndole", "escribir", VERB, P_IO);
	errors += test_word("ayudarme", "ayudar", VERB | P_O, P_IO | P_IOI);
	errors += test_word("ayudarte", "ayudar", VERB | P_O, P_IO | P_IOY);
	errors += test_word("ayudarla", "ayudar", VERB | P_O, 0);
	errors += test_word("ayudarlo", "ayudar", VERB | P_O, 0);
	errors += test_word("ayudarlos", "ayudar", VERB | P_O, 0);
	errors += test_word("ayudarlas", "ayudar", VERB | P_O, 0);
	errors += test_word("ayudaros", "ayudar", VERB | P_O, 0);
	errors += test_word("ayudarnos", "ayudar", VERB | P_O, 0);
	errors += test_word("tuvo", "tener", VERB, P_3);
	errors += test_word("acercarme", "acercar", VERB | P_O, P_OI | P_IO | P_IOI);
	errors += test_word("leerlo", "leer", VERB | V_INF | P_O, P_O3); // infinitive and direct object
	errors += test_word("leyendolo", "leer", VERB | P_O, P_O3); // present participle and direct object
	errors += test_word(u8"leyéndolo", "leer", VERB | P_O, P_O3); // present participle and direct object
	errors += test_word(u8"léanlo", "leer", VERB | P_O, V_I | P_O3); // imperative and direct object
	errors += test_word(u8"mostrándole", "mostrar", 0, P_IO3); // present participle (gerund) and indirect object
	errors += test_word("muestro", "mostrar", VERB, P_I); // verb form with irregular stem

	return errors;
}

void C_JaUnittest(char* word)
{
	int errors = 0;
	uint64 start_time_ms = ElapsedMilliseconds();
	char currentlang[100];
	strcpy(currentlang, current_language);
	SetLanguage("spanish");
	errors += C_JaUnitTestSpanish();
	if (errors > 0) printf("\nErrors %d,", errors);
	else printf("\nNo errors,");
	SetLanguage(currentlang);
}
