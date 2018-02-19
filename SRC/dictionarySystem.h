#ifndef _DICTIONARYSYSTEM_H
#define _DICTIONARYSYSTEM_H

#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

typedef unsigned int DICTINDEX;	//   indexed ref to a dictionary entry

//   This file has formatting restrictions because it is machine read by AcquireDefines. All macro and function calls
//   must keep their ( attached to the name and all number defines must not.
//   All tokens of a define must be separated by spaces ( TEST|TEST1  is not legal).

//   These are the properties of a dictionary entry.
//   IF YOU CHANGE THE VALUES or FIELDS of a dictionary node,
//   You need to rework ReadDictionary/WriteDictionary/ReadBinaryEntry/WriteBinaryEntry

// D->properties bits   -- when you names add to these, you have to rebuild dict

// property flags -- THIS IS A MARKER FOR BELOW ZONE words marked `value

//   BASIC_POS POS bits like these must be on the bottom of any 64-bit word, so they match MEANING values (at top of 32 bit) as well.
#define NOUN					0x0000000080000000ULL	// bit header PENNBANK: NN NNS NNP NNPS
#define VERB					0x0000000040000000ULL	// bit header PENNBANK: V*
#define ESSENTIAL_POS			( NOUN | VERB  )
#define ADJECTIVE				0x0000000020000000ULL	// bit header  PENNBANK: JJ JJR JJS
#define ADVERB					0x0000000010000000ULL   // bit header  PENNBANK: RB RBR RBS
#define BASIC_POS				( ESSENTIAL_POS | ADJECTIVE | ADVERB )
//   the above four should be first, reflecting the Wordnet files (leaving room for wordnet offset)

#define PREPOSITION			0x0000000008000000ULL	// Pennbank: IN (see also CONJUNCTION_SUBORDINATE)
#define ESSENTIAL_FLAGS			( BASIC_POS | PREPOSITION ) //  these are type restrictions on MEANINGs and are shared onto systemflags for most likely pos-tag candidate type

// kinds of conjunctions
#define CONJUNCTION_COORDINATE  0x0000000004000000ULL	// Pennbank: CC
#define CONJUNCTION_SUBORDINATE 0x0000000002000000ULL	// Pennbank: IN (see also PREPOSIITON)
#define CONJUNCTION		( CONJUNCTION_COORDINATE | CONJUNCTION_SUBORDINATE ) // direct enumeration as header

// kinds of determiners 
#define PREDETERMINER 			0x0000000001000000ULL  // Pennbank: PDT
#define DETERMINER  			0x0000000000800000ULL   // Pennbank: DT
#define POSSESSIVE				0x0000000000400000ULL	// is a possessive like 's or Taiwanese (but not possessive pronoun) Pennbank: POS
#define PRONOUN_POSSESSIVE		0x0000000000200000ULL	// my your his her its our their	Pennbank: PRP$			
#define POSSESSIVE_BITS			( PRONOUN_POSSESSIVE | POSSESSIVE )
#define DETERMINER_BITS		   ( DETERMINER | PREDETERMINER | POSSESSIVE_BITS ) // come before adjectives/nouns/ adverbs leading to those

//		0x0000000000100000ULL 	

// punctuation
#define COMMA 					0x0000000000080000ULL	
#define REQUIRE_CONCEPT			COMMA					// concept recursively demands members be of named pos type (noun, verb, adjective, adverb)
#define PAREN					0X0000000000040000ULL 	
#define PUNCTUATION				0x0000000000020000ULL	// this covers --  and  -  and [  ] etc BUT not COMMA or PAREN or QUOTE
#define QUOTE 					0x0000000000010000ULL	// double quoted string	
#define CURRENCY				0x0000000000008000ULL	// currency marker as stand alone from PENNBANK

// kinds of adjectives
#define ADJECTIVE_NORMAL		0x0000000000004000ULL	// "friendly" // "friendlier" // "friendliest"
#define ADJECTIVE_NUMBER		0x0000000000002000ULL 	// the five dogs, the 5 dogs, and the fifth dog
#define ADJECTIVE_NOUN			0x0000000000001000ULL 	// noun used as an adjective in front of another ((char*)"bank clerk") "attributive noun"
#define ADJECTIVE_PARTICIPLE	0x0000000000000800ULL 	// verb participle used as an adjective (before or after noun) the "walking" dead  or  the "walked" dog
#define ADJECTIVE_BITS ( ADJECTIVE_NOUN | ADJECTIVE_NORMAL | ADJECTIVE_PARTICIPLE | ADJECTIVE_NUMBER )

// unusual words
#define INTERJECTION			0x0000000000000400ULL	 
#define THERE_EXISTENTIAL		0x0000000000000200ULL	// "There" is no future in it. There is actually a unique kind of pronoun.  Pennbank: EX
#define FOREIGN_WORD			0x0000000000000100ULL 	// pennbank: FW
#define TO_INFINITIVE	 		0x0000000000000080ULL 	// attaches to NOUN_INFINITIVE

// kinds of nouns
#define NOUN_ADJECTIVE			0x0000000000000040ULL 	// adjective used as a noun - "the rich"   implied people as noun  -- also past verb: (char*)"the *dispossessed are fun"
#define NOUN_SINGULAR			0x0000000000000020ULL	// Pennbank: NN
#define NOUN_PLURAL				0x0000000000000010ULL	// Pennbank: NNS
#define NOUN_PROPER_SINGULAR	0x0000000000000008ULL	//   A proper noun that is NOT a noun is a TITLE like Mr.	Pennbank: NP
#define NOUN_PROPER_PLURAL		0x0000000000000004ULL	// Pennbank: NPS
#define NOUN_GERUND				0x0000000000000002ULL	// "Walking" is fun
#define NOUN_NUMBER				0x0000000000000001ULL	// I followed the "20".
#define NOUN_INFINITIVE 		0x0000800000000000ULL	// To *go is fun
#define NOUN_PROPER	 ( NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL  )
#define NORMAL_NOUN_BITS ( NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER | NOUN_NUMBER  | NOUN_ADJECTIVE )
#define NOUN_BITS ( NORMAL_NOUN_BITS | NOUN_GERUND | NOUN_INFINITIVE )

// kinds of verbs (tenses)
#define VERB_PRESENT			0x0000400000000000ULL	// present plural (usually infinitive)  Pennbank: VBP
#define VERB_PRESENT_3PS		0x0000200000000000ULL	// 3rd person singular singular  Pennbank: VBZ
#define VERB_PRESENT_PARTICIPLE 0x0000100000000000ULL	// GERUND,  Pennbank: VBG
#define VERB_PAST				0x0000080000000000ULL	// Pennbank: VBD
#define VERB_PAST_PARTICIPLE    0x0000040000000000ULL	// Pennbank VBN
#define VERB_INFINITIVE			0x0000020000000000ULL	//   all tense forms are linked into a circular ring   Pennbank: VB
#define PARTICLE				0x0000010000000000ULL	//   multiword separable verb (the preposition component) (full verb marked w systemflag SEPARABLE_PHRASAL_VERB or such)  Pennbank: RP
#define VERB_BITS (  VERB_INFINITIVE | VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST | VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE  )

// kinds of pronouns
#define PRONOUN_SUBJECT  		0x0000008000000000ULL	// I you he she it we  they
#define PRONOUN_OBJECT			0x0000004000000000ULL	// me, you, him, her, etc
#define PRONOUN_BITS	( PRONOUN_OBJECT | PRONOUN_SUBJECT ) // there is no pronoun flag, just the bits

// aux verbs
#define	AUX_DO 					0x0000002000000000ULL	
#define AUX_HAVE				0x0000001000000000ULL 
#define	AUX_BE					0x0000000800000000ULL	
#define AUX_VERB_PRESENT		0x0000000400000000ULL
#define AUX_VERB_FUTURE			0x0000000200000000ULL
#define AUX_VERB_PAST			0x0000000100000000ULL
#define AUX_VERB_TENSES ( AUX_VERB_PRESENT | AUX_VERB_FUTURE | AUX_VERB_PAST ) // modal verbs
#define AUX_VERB ( AUX_VERB_TENSES | AUX_BE | AUX_HAVE | AUX_DO )

#define STARTTAGS				0x0000800000000000ULL	// the top bit of the 48 bits visible to tagger

//////////////////////////////16 bits below here can not be used in posValues[] of tagger  
#define AS_IS					0x8000000000000000ULL   //  TRANSIENT INTERNAL dont try to reformat the word (transient flag passed to StoreWord)

#define NOUN_HUMAN				0x4000000000000000ULL  //   person or group of people that uses WHO, he, she, anyone
#define NOUN_FIRSTNAME			0x2000000000000000ULL  //   a known first name -- wiULL also be a sexed name probably
#define NOUN_SHE				0x1000000000000000ULL	//   female sexed word (used in sexed person title detection for example)
#define NOUN_HE					0x0800000000000000ULL	//   male sexed word (used in sexed person title detection for example)
#define NOUN_THEY				0x0400000000000000ULL   
#define NOUN_TITLE_OF_ADDRESS	0x0200000000000000LL	//   eg. mr, miss
#define NOUN_TITLE_OF_WORK		0x0100000000000000ULL
#define LOWERCASE_TITLE			0X0080000000000000ULL	//   lower case word may be in a title (but not a noun)
#define NOUN_ABSTRACT			0x0040000000000000ULL	// can be an abstract noun (maybe also concrete)
#define MORE_FORM				0x0020000000000000ULL  
#define MOST_FORM				0x0010000000000000ULL  

#define QWORD 	 				0x0008000000000000ULL 	// who what where why when how whose -- things that can start a question

#define PLACE_NUMBER			0x0004000000000000ULL	// hidden refinement of ADJECTIVE_NUMBER
#define SINGULAR_PERSON			0x0002000000000000ULL	// for conjugation of "to be" (am/was)- present3ps is a marked tense, 2nd person and plural is default of "verb_present"

#define IDIOM 					0x0001000000000000ULL	// multi word expression of which end is at TAIL (like multiword prep)

//////////////////////////////bits above cannot used in posValues[] of tagger  

#define NOUN_TITLE		 (  NOUN | NOUN_TITLE_OF_WORK | NOUN_PROPER_SINGULAR )
#define NOUN_HUMANGROUP		( NOUN | NOUN_HUMAN | NOUN_TITLE_OF_WORK | NOUN_THEY )
#define NOUN_MALEHUMAN     ( NOUN | NOUN_HUMAN | NOUN_HE | NOUN_PROPER_SINGULAR )
#define NOUN_FEMALEHUMAN    ( NOUN |  NOUN_HUMAN | NOUN_SHE | NOUN_PROPER_SINGULAR )
#define NOUN_HUMANNAME ( NOUN_HUMAN | NOUN | NOUN_PROPER_SINGULAR )

#define TAG_TEST ( INTERJECTION | IDIOM | PUNCTUATION | QUOTE | COMMA | CURRENCY | PAREN | PARTICLE | VERB_BITS | NOUN_BITS | FOREIGN_WORD | DETERMINER_BITS | ADJECTIVE_BITS | AUX_VERB | ADVERB  | PRONOUN_BITS | CONJUNCTION | POSSESSIVE_BITS | THERE_EXISTENTIAL | PREPOSITION | TO_INFINITIVE )

#define NOUN_DESCRIPTION_BITS ( ADJECTIVE_BITS | DETERMINER_BITS  | ADVERB  | NOUN_BITS )

// system flags -- THIS IS A REQUIRED MARKER FOR BELOW ZONE and also ends PROPERTIES flags   words marked ``value

// english word attribues 
#define ANIMATE_BEING 				0x0000000000000001ULL 
#define	HOWWORD 				    0x0000000000000002ULL
#define TIMEWORD 					0x0000000000000004ULL
#define LOCATIONWORD				0x0000000000000008ULL	// prepositions will be ~placepreposition, not this - concepts have to mark this for place named nouns

// verb conjucations
#define VERB_CONJUGATE3				0x0000000000000010ULL	//	3rd word of composite verb extensions	
#define VERB_CONJUGATE2				0x0000000000000020ULL	//   2nd word of composite verb extensions  (e.g., re-energize)
#define VERB_CONJUGATE1				0x0000000000000040ULL	//   1st word of composite verb extensions  (e.g. peter_out)

#define PRESENTATION_VERB			0x0000000000000080ULL 	// will occur after THERE_EXISTENTIAL
#define COMMON_PARTICIPLE_VERB		0x0000000000000100ULL	// will be adjective after "be or seem" rather than treated as a verb participle

#define NOCONCEPTLIST				0x0000000000000200ULL   // conceptlist should ignore this on a concept (keep in low order bits for query)
#define PRONOUN_RELATIVE			0x0000000000000400ULL   

// phrasal verb controls
#define INSEPARABLE_PHRASAL_VERB		0x0000000000000800ULL	//  cannot be split apart ever
#define MUST_BE_SEPARATE_PHRASAL_VERB	0x0000000000001000ULL 	// phrasal MUST separate - "take my mother into" but not "take into my mother" 	
#define SEPARABLE_PHRASAL_VERB			0x0000000000002000ULL  // can be separated
#define PHRASAL_VERB 					0x0000000000004000ULL  // accepts particles - when lacking INSEPARABLE and MUST_SEPARABLE, can do either 

// verb objects
#define VERB_NOOBJECT  				0x0000000000008000ULL 	
#define VERB_INDIRECTOBJECT 		0x0000000000010000ULL	
#define VERB_DIRECTOBJECT 			0x0000000000020000ULL	 
#define VERB_TAKES_GERUND			0x0000000000040000ULL	// "keep on" singing
#define VERB_TAKES_ADJECTIVE		0x0000000000080000ULL	//  be seem etc (copular/linking verb) links adjectives and adjective participles to subjects
#define VERB_TAKES_INDIRECT_THEN_TOINFINITIVE	0x0000000000100000ULL    // proto 24  --  verbs taking to infinitive after object: (char*)"Somebody ----s somebody to INFINITIVE"  "I advise you *to go" + ~TO_INFINITIVE_OBJECT_verbs
#define VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE	0x0000000000200000ULL 	// proto 25  -  verbs that take direct infinitive after object  "Somebody ----s somebody INFINITIVE"  "i heard her sing"  + ~causal_directobject_infinitive_verbs
#define VERB_TAKES_TOINFINITIVE					0x0000000000400000ULL    // proto 28 "Somebody ----s to INFINITIVE"   "we agreed to plan" -- when seen no indirect object
#define VERB_TAKES_VERBINFINITIVE				0x0000000000800000ULL 	// proto 32, 35 "Somebody ----s INFINITIVE"   "Something ----s INFINITIVE"  -- when seen no indirect object  "you *dare call my lawyer"
//  ~factitive_adjective_Verbs take object complement adjective after direct object noun
// ~factitive_noun_verbs take object complement noun after direct object noun
// ~adjectivecomplement_taking_noun_infinitive adjectives can take a noun to infinitive after them as adjective "I was able to go"

#define OMITTABLE_TIME_PREPOSITION	0x0000000001000000ULL // can be used w/o preposition
#define ALWAYS_PROPER_NAME_MERGE	0x0000000002000000ULL // if see in any form in input, do proper name merge upon it
#define CONJUNCT_SUBORD_NOUN 		0x0000000004000000ULL // can be used to simultaneously start a subordinate clause and be its subject

// these match the values in properties above
#define ONLY_NONE					0x0000000008000000ULL
#define PROBABLE_ADVERB 			0x0000000010000000ULL
#define PROBABLE_ADJECTIVE 			0x0000000020000000ULL
#define PROBABLE_VERB 				0x0000000040000000ULL
#define PROBABLE_NOUN 				0x0000000080000000ULL

#define PRONOUN_PLURAL				0x0000000100000000ULL	
#define PRONOUN_SINGULAR			0x0000000200000000ULL	
#define ACTUAL_TIME  				0x0000000400000000ULL // a time word like 14:00
#define PREDETERMINER_TARGET		0x0000000800000000ULL	// predeterminer can come before these (a an the)

#define ADJECTIVE_POSTPOSITIVE		0x0000001000000000ULL	// adjective can occur AFTER the noun (marked on adjective)
#define DETERMINER_SINGULAR			0x0000002000000000ULL		
#define DETERMINER_PLURAL 			0x0000004000000000ULL	
#define TAKES_POSTPOSITIVE_ADJECTIVE 	0x0000008000000000ULL	// word takes adjectives AFTER like something, nothing, etc (marked on word, not on adjective) 

#define GRADE5_6					0x0000010000000000ULL
#define GRADE3_4					0x0000020000000000ULL
#define GRADE1_2  					0x0000040000000000ULL
#define KINDERGARTEN				( GRADE1_2 | GRADE3_4 | GRADE5_6 )
#define AGE_LEARNED ( KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6 )  // adult is all the rest  (3 bits)

#define SUBSTITUTE_RECIPIENT		0x0000080000000000ULL

#define NOUN_NODETERMINER			0x0000100000000000ULL	// nouns of location that require no determiner (like "Home is where the heart is") or mass nouns

#define	WEB_URL						0x0000200000000000ULL	
#define ORDINAL						0x0000400000000000ULL  // for adjectives and nouns labeled ADJECTIVE_NUMBER or NOUN_NUMBER, it is cardinal if not on and ordinal if it is	

#define PRONOUN_REFLEXIVE			0x0000800000000000ULL 

// 16 bits CANNOT REFERENCE BELOW IN POS RULES - chatscript internal markers
/////////////////////////////////////////////////////////////////////////////////
#define PRONOUN_INDIRECTOBJECT		0x0001000000000000ULL	// can be an indirect object
#define MODEL_NUMBER				0x0002000000000000ULL  // Noun_number can be a normal number word like "one" or a model number like "Cray-3"
#define COMMON1						0x0004000000000000ULL 
#define COMMON2						0x0008000000000000ULL  // above 10 K
#define COMMON3						( COMMON1 | COMMON2 )  // above 100 K
#define COMMON4						0x0010000000000000ULL	 // above 1 million
#define COMMON5						( COMMON1 |  COMMON4 ) // above 10 million
#define COMMON6						( COMMON2  | COMMON4 )  // above 100 million
#define COMMON7						( COMMON1 |  COMMON2  | COMMON4 ) // above 1 billion
#define COMMONNESS					COMMON7  // word frequency in 1 gram

#define MONTH						0x0020000000000000ULL 	// is a month name
#define SPELLING_EXCEPTION			0x0040000000000000ULL	// dont double final consonant making past tense
#define ADJECTIVE_NOT_PREDICATE		0x0080000000000000ULL	
#define ADJECTIVE_ONLY_PREDICATE	0x0100000000000000ULL	
#define CONSTANT_IS_NEGATIVE		0x0200000000000000ULL	

#define CONDITIONAL_IDIOM			0x0400000000000000ULL	// word may or may not merge into an idiom during pos-tagging -  blocks automerge during tokenization
#define NO_PROPER_MERGE				0x0800000000000000ULL	// do not merge this word into any proper name
#define MARKED_WORD					0x1000000000000000ULL	// transient word marker that USER can store on word and find facts that connect to it && building dictionary uses it to mean save this word regardless
#define PATTERN_WORD 				0x2000000000000000ULL
#define DELAYED_RECURSIVE_DIRECT_MEMBER	0x4000000000000000ULL  // concept will be built with all indirect members made direct
#define EXTENT_ADVERB				0x8000000000000000ULL	
/////////////////////////////////////////////////////////////////////////////////

// CANNOT REFERNCE ABOVE IN POS TAGGING RULES
// See MARK_FLAGS for which system flags are exposed as concepts
// end system flags -- THIS IS A REQUIRED MARKER FOR ABOVE ZONE

// parse flags -- THIS IS A REQUIRED MARKER FOR BELOW ZONE   words marked ```value  

#define FACTITIVE_ADJECTIVE_VERB	0x00000001		// ~factitive_adjective_Verbs
#define FACTITIVE_NOUN_VERB			0x00000002	// ~factitive_noun_Verbs
#define ADJECTIVE_GOOD_SUBJECT_COMPLEMENT 0x00000004 // adjectives in conflict with adverbs which WILL be subject complements  ~adjective_good_subject_complement
#define QUOTEABLE_VERB				0x00000008 // he said "I love you" ~quotable_verbs 
#define ADJECTIVE_TAKING_NOUN_INFINITIVE 0x00000010 // "he is *determined to go home" -- complement ~adjectivecomplement_taking_noun_infinitive 
#define OMITTABLE_THAT_VERB			0x00000020 // that in object clause might be omitted ~omittable_that_verbs 
#define NEGATIVE_ADVERB_STARTER		0x00000040 // "never can I see him" - before aux is NOT a question ~negative_adverb_starter
#define NON_COMPLEMENT_ADJECTIVE	 0x00000080	// cannot be used as adjective complement in "I am xxx" ~non_complement_adjectives 
#define PREP_ALLOWS_UNDETERMINED_NOUN 0x00000100 //  men *of faith  two *per box
#define CONJUNCTIONS_OF_TIME		0x00000200	// ~conjunctions_of_time
#define CONJUNCTIONS_OF_SPACE		0x00000400	 // ~conjunctions_of_space
#define CONJUNCTIONS_OF_ADVERB		0x00000800 // ~conjunctions_of_adverb  
#define NONDESCRIPTIVE_ADJECTIVE	0x00001000  // not usable as adjective complement  ~nondescriptiveadjective 
#define NEGATIVE_SV_INVERTER		0x00002000 // no never not at start of sentence can flip sv order ~negativeinverters
#define LOCATIONAL_INVERTER			0x00004000 // here there nowhere at start of sentence can flip sv order  ~locationalinverters
#define ADVERB_POSTADJECTIVE		0x00008000  // modified a subject complement adjective postpositively  ~postadjective_adverb
#define QUANTITY_NOUN				0x00010000 // can have adverb modifying it like "over half" ~counting_nouns
#define CONJUNCTIVE_ADVERB			0x00020000  // join two main sentences with ; xxx ,   ~conjunctive_adverb
#define CORRELATIVE_ADVERB			0x00040000  // I like either boys or girls   ~correlative_adverb
#define POTENTIAL_CLAUSE_STARTER	0x00080000 // who whomever, etc might start a clause ~potential_clause_starter 
#define VERB_ALLOWS_OF_AFTER		0x00100000 // most cant-- used by LIVEDATA rules ~verb_with_of_after
#define INDEFINITE_PRONOUN			0x00200000 // can have postnominal adjective
#define PERSONAL_PRONOUN			0x00400000
#define ADJECTIVE_NOT_PRE			0x00800000 // cannot come before a noun (like "on" which is object complement or post position)
#define OBJECT_AS_ADJECTIVE			0x01000000 // verb can take object-as like "i view him as silly
#define DISTANCE_NOUN  			    0x02000000 // "six *feet under
#define DISTANCE_ADVERB  			0x04000000 // "six feet *under
#define TIME_ADVERB  				0x08000000 // "six years *hence"
#define TIME_NOUN  				    0x10000000 // "six *years *hence"
#define TIME_ADJECTIVE				0x20000000 // " a year *old "
#define DISTANCE_ADJECTIVE  		0x40000000 // "six feet *tall
#define OF_PROPER		     		0x80000000 // Bank of *America always merge these of 
// end parse flags -- THIS IS A REQUIRED MARKER FOR ABOVE ZONE

// ALL VALUES BELOW HERE DO NOT NEED TO DECODE A BIT VALUE BACK TO A NAME VALUE

// repeated internal bits
#define HAS_SUBSTITUTE			0x00000800		//   word has substitute attached 

// flags on facts  FACT FLAGS

#define FACTATTRIBUTE	    0x10000000  // fact is an attribute fact, object can vary while subject/verb should be fixed 
// unused 0x20000000 
// unused 0x40000000 
// unused 0x80000000 
// DO NOT MOVE FLAGS AROUND, since user topic files assume them as they are
// transient flags
#define MARKED_FACT         0x08000000  //   TRANSIENT : used during inferencing sometimes to see if fact is marked, also in user save to avoid repeated save
#define ITERATOR_FACT		MARKED_FACT	// used by iterator
#define MARKED_FACT2        0x04000000  //   TRANSIENT: used during inferencing sometimes to see if 2ndary fact is marked
#define FACTDEAD			0x02000000   //   has been killed off
#define FACTTRANSIENT		0x01000000   //   save only with a set, not with a user or base system

// normal flags
#define FACTAUTODELETE      0x00800000 // delete when json deletes
#define ORIGINAL_ONLY       0x00400000  //  dont match on canonicals
#define FACTBUILD2			0x00200000 
#define FACTBUILD1	        0x00100000  // fact created during build 1 (for concepts)

// user flags
#define USER_FLAG4			0x00080000
#define USER_FLAG3			0x00040000
#define USER_FLAG2			0x00020000
#define USER_FLAG1			0x00010000

#define USER_FLAGS			0x000F0000 
#define SYSTEM_FLAGS		0xFFF0FFF0 // system used top 12 bits and bottom 12
// unused 0x00004000 
// unused 0x00008000
#define JSON_OBJECT_FACT	0x00002000 // on subject side of triple
#define JSON_ARRAY_FACT		0x00001000	// on subject side of triple

#define JSON_ARRAY_VALUE	0x00000800  // on object side of triple
#define JSON_OBJECT_VALUE	0x00000400  // on object side of triple
#define JSON_STRING_VALUE	0x00000200 // on object side of triple
#define JSON_PRIMITIVE_VALUE 0x00000100 // on object side of triple
#define JSON_FLAGS ( JSON_PRIMITIVE_VALUE | JSON_STRING_VALUE | JSON_OBJECT_VALUE | JSON_ARRAY_VALUE | JSON_OBJECT_FACT | JSON_ARRAY_FACT )
#define JSON_OBJECT_FLAGS ( JSON_ARRAY_VALUE |  JSON_OBJECT_VALUE | JSON_STRING_VALUE | JSON_PRIMITIVE_VALUE )
// permanent flags
#define FACTSUBJECT         0x00000080  //   index is - relative number to fact 
#define FACTVERB			0x00000040	//   is 1st in its bucket (transient flag for read/WriteBinary) which MIRRORS DICT BUCKETHEADER flag: 
#define FACTOBJECT		    0x00000020  //   does not apply to canonical forms of words, only the original form - for classes and sets, means dont chase the set
#define FACTDUPLICATE		0x00000010	//   allow repeats of this fact
//unused		0x00000001	
//unused		0x00000002	
//unused		0x00000004	
//unused		0x00000005	

// end of fact flags

// pos tagger roles and states on roles[] && needRoles[] (32 bit limit)
// needRoles values can be composed of multiple choices. roles are exactly one choice
#define MAINOBJECT			0x00000008
#define MAININDIRECTOBJECT	0x00000004 
#define MAINVERB			0x00000002
#define MAINSUBJECT			0x00000001			// noun roles like Mainsubject etc are ordered lowest first for pronoun IT priority

#define SUBJECT2			0x00000010
#define VERB2				0x00000020
#define INDIRECTOBJECT2		0x00000040 
#define OBJECT2				0x00000080

#define CLAUSE 					0x00000100 	
#define VERBAL 					0x00000200	 
#define PHRASE					0x00000400	// true postnominal adjective or verbal that acts as postnominal adjective  (noun role is named also, so anonymous means adverb role)
		 	
#define QUOTATION_UTTERANCE  	0x00000800		
#define OBJECT_COMPLEMENT		0x00001000	// can be noun or adjective or to/infinitive after direct object..."that should keep them *happy" "I knight you *Sir *Peter" (verb expects it)
#define TO_INFINITIVE_OBJECT 	0x00002000	// expecting either indirectobject then to infinitve or just to infinitive
#define VERB_INFINITIVE_OBJECT	0x00004000  // expecting either indirectobject then infinitive or just infinitive
#define OMITTED_SUBJECT_VERB	0x00008000 // for clauses like "if necessary"

// end of needRoles, the rest are roles

// what does coordinate conjunction conjoin (7 choices = 3 bits) -- NOT available on needRoles  // 		there can be no conjoined particles
#define CONJUNCT_CLAUSE 		0x00010000 	
#define CONJUNCT_SENTENCE		0x00020000	
#define CONJUNCT_NOUN			0x00030000	
#define CONJUNCT_VERB 			0x00040000	
#define CONJUNCT_ADJECTIVE		0x00050000	
#define CONJUNCT_ADVERB			0x00060000
#define CONJUNCT_PHRASE			0x00070000
#define CONJUNCT_KINDS			( CONJUNCT_PHRASE | CONJUNCT_CLAUSE | CONJUNCT_SENTENCE | CONJUNCT_NOUN | CONJUNCT_VERB | CONJUNCT_ADJECTIVE | CONJUNCT_ADVERB )

#define SENTENCE_END            0x00080000
#define WHENUNIT				0x00100000
#define WHEREUNIT				0x00200000
#define WHYUNIT					0x00300000
#define HOWUNIT					0x00400000
#define ADVERBIALTYPE ( WHENUNIT | HOWUNIT | WHEREUNIT | WHYUNIT )
#define REFLEXIVE		 		0x00800000   	
#define COMMA_PHRASE 			0x01000000  // describes noun before it
#define BYOBJECT2				0x02000000
#define OFOBJECT2				 0x04000000
#define POSTNOMINAL_ADJECTIVE	 0x08000000  			// true postnominal adjective or verbal that acts as postnominal adjective  (noun role is named also, so anonymous means adverb role)

#define APPOSITIVE				0x10000000		//	2nd noun restates first
#define ADJECTIVE_COMPLEMENT	0x20000000		//2ndary modifier of an adjective "he was ready *to *go"	    noun clause or a prepositional phrase 
#define ABSOLUTE_PHRASE			0x40000000	  // "wings flapping", the man runs
#define SUBJECT_COMPLEMENT		0x80000000  // main object after a linking verb is an adjective "he seems *strong" (aka subject complement) -- a prep phrase instead is also considered a subject complement

// above are 32 bit available to needRoles and roles. Below only suitable for 64bit roles
#define OMITTED_OF_PREP			0x0000000100000000ULL
#define OMITTED_TIME_PREP		0x0000000200000000ULL 
#define ADVERBIAL				0x0000000400000000ULL 
#define ADJECTIVAL				0x0000000800000000ULL 
#define ADDRESS 				0x0000001000000000ULL  // for factitive verbs
#define TAGQUESTION				0x0000002000000000ULL
#define DISTANCE_NOUN_MODIFY_ADVERB	0x0000004000000000ULL
#define TIME_NOUN_MODIFY_ADVERB		0x0000008000000000ULL
#define TIME_NOUN_MODIFY_ADJECTIVE	    0x0000010000000000ULL
#define DISTANCE_NOUN_MODIFY_ADJECTIVE	0x0000020000000000ULL
// end of roles/needroles


// topic control flags

// permanent flags 
#define TOPIC_KEEP 1		// don't erase rules
#define TOPIC_REPEAT 2      // allow repeated output
#define TOPIC_RANDOM 4      // random access responders (not gambits)
#define TOPIC_SYSTEM 8		// combines NOSTAY, KEEP, and special status on not accessing its gambits  
#define TOPIC_NOSTAY 0x00000010		// do not stay in this topic
#define TOPIC_PRIORITY 0x00000020	// prefer this more than normal
#define TOPIC_LOWPRIORITY 0x00000040 // prefer this less than normal
#define TOPIC_NOBLOCKING 0x00000080 // :verify should not complain about blocking
#define TOPIC_NOKEYS 0x00000100	// 0x00000100  :verify should not complain about keywords missing
#define TOPIC_NOPATTERNS 0x00000200	// :verify should not complain about patterns that fail
#define TOPIC_NOSAMPLES 0x00000400 // :verify should not complain about sample that fail
#define TOPIC_NOGAMBITS 0x00000800 // gambittopics should not use these topics
#define TOPIC_SAFE	0x00001000		// update safe
#define TOPIC_SHARE	0x00002000		// sharable topic among bots
#define TOPIC_RANDOM_GAMBIT	0x00004000		// r: exists in topic

//   TRANSIENT FLAGS
#define TOPIC_GAMBITTED 0x00008000	//   (transient per user) gambit issued from this topic
#define TOPIC_RESPONDED 0x00010000	//   (transient per user) responder issued from this topic
#define TOPIC_REJOINDERED 0x00020000	//   (transient per user) rejoinder issued from this topic
#define ACCESS_FLAGS ( TOPIC_GAMBITTED | TOPIC_RESPONDED | TOPIC_REJOINDERED ) 
#define TOPIC_BLOCKED	0x00040000		//   (transient per user) disabled by some users 
#define TOPIC_USED		0x00080000		//   (transient per user) indicates need to write out the topic
#define TRANSIENT_FLAGS ( TOPIC_BLOCKED | TOPIC_USED | ACCESS_FLAGS ) 

// TRACE FLAGS
// simple
#define TRACE_ON			0x00000001	
#define TRACE_MATCH 		0x00000002
#define TRACE_VARIABLE		0x00000004

// mild
#define TRACE_PREPARE		0x00000008
#define TRACE_OUTPUT		0x00000010
#define TRACE_PATTERN		0x00000020

// deep
#define TRACE_HIERARCHY		0x00000040
#define TRACE_INFER			0x00000080
#define TRACE_SUBSTITUTE	0x00000100
#define TRACE_FACT			0x00000200
#define TRACE_VARIABLESET	0x00000400
#define TRACE_USERFN		0x00000800
#define TRACE_USER			0x00001000
#define TRACE_POS			0x00002000
#define TRACE_QUERY			0x00004000
#define TRACE_TCP			0x00008000
#define TRACE_USERCACHE		0x00010000
#define TRACE_SQL			0x00020000
#define TRACE_LABEL			0x00040000
#define TRACE_SAMPLE		0x00080000
#define TRACE_INPUT			0x00100000
#define TRACE_SPELLING		0x00200000
#define TRACE_TOPIC			0x00400000
#define TRACE_SCRIPT		0x00800000
#define TRACE_JSON			0x01000000
#define TRACE_NOT_THIS_TOPIC 0x02000000
#define TRACE_FLOW			0x04000000
#define TRACE_COVERAGE		0x08000000
#define TRACE_ALWAYS		0x10000000

#define TRACE_ECHO			0x20000000	// echo trace
#define TRACE_USERFACT		0x40000000
#define TRACE_TREETAGGER	0x80000000

// TIME FLAGS
// simple
#define TIME_ON				0x00000001	

// mild
#define TIME_PREPARE		0x00000008
#define TIME_PATTERN		0x00000020

// deep
#define TIME_USERFN			0x00000800
#define TIME_USER			0x00001000
#define TIME_QUERY			0x00004000
#define TIME_TCP			0x00008000
#define TIME_USERCACHE		0x00010000
#define TIME_SQL			0x00020000
#define TIME_TOPIC			0x00400000
#define TIME_JSON			0x01000000
#define TIME_NOT_THIS_TOPIC 0x02000000
#define TIME_FLOW			0x04000000
#define TIME_ALWAYS			0x10000000

// pos tagger result operators
#define DISCARD 1
#define KEEP 2
#define TRACE 8

#define ONE_REJOINDER 0x00010000		// get next rejoinder

// pos tagger pattern values   6 bits (0-63) + 3 flags

// ops on pos bits
#define HAS 1 // any bit matching 
#define IS 2	// is exactly this
#define INCLUDE 3 // has bit AND has other bits
#define CANONLYBE  4	// has all these bits by category 
#define HASORIGINAL 5	// all original bits	
#define PRIORPOS 6		
#define POSTPOS 7
#define PASSIVEVERB 8
// ops on system flags
#define HASPROPERTY 9
#define HASALLPROPERTIES 10	
#define HASCANONICALPROPERTY 11
#define NOTPOSSIBLEVERBPARTICLEPAIR 12
// ops on supplementental parse marks
#define PARSEMARK 13

// ops on actual word
#define ISORIGINAL 14 // is this word
#define ISCANONICAL 15 // canonical word is this
#define PRIORCANONICAL 16
#define ISMEMBER 17	// is member of this set
#define PRIORORIGINAL 18	
#define POSTORIGINAL 19

// ops on position
#define POSITION 20 // sentence boundary
#define START POSITION
#define END POSITION
#define RESETLOCATION 21	

#define HAS2VERBS 22	
#define ISQWORD 23
#define ISQUESTION 24
#define ISABSTRACT 25
#define POSSIBLEINFINITIVE 26
#define POSSIBLEADJECTIVE 27
#define POSSIBLETOLESSVERB 28
#define POSSIBLEADJECTIVEPARTICIPLE 29
#define HOWSTART 30
#define POSSIBLEPHRASAL 31
#define POSSIBLEPARTICLE 32
#define ISCOMPARATIVE 33
#define ISEXCLAIM 34
#define ISORIGINALMEMBER 35
#define ISSUPERLATIVE 36
#define SINGULAR 37
#define ISPROBABLE 38
#define PLURAL 39
#define LASTCONTROL PLURAL  // add new ops to optable as well

#define SKIP 1 // if it matches, move ptr along, if it doesnt DONT
#define STAY 2
#define NOTCONTROL 4 


// values of parseFlags parseBits on dictionary  -- when you names add to these, you have to rebuild dict
  
// control over tokenization (tokenControl set from user $cs_token variable)
// these values MIRRORED as resulting used values in %tokenflags (tokenFlags)
#define DO_ESSENTIALS			0x0000000000000001ULL 
#define DO_SUBSTITUTES			0x0000000000000002ULL
#define DO_CONTRACTIONS			0x0000000000000004ULL
#define DO_INTERJECTIONS		0x0000000000000008ULL
#define DO_BRITISH				0x0000000000000010ULL 
#define DO_SPELLING				0x0000000000000020ULL 
#define DO_TEXTING				0x0000000000000040ULL 
#define DO_NOISE				0x0000000000000080ULL	// file specific to scripter, not in common release

#define DO_PRIVATE				0x0000000000000100ULL	// 256
#define DO_NUMBER_MERGE			0x0000000000000200ULL	// 512
#define DO_PROPERNAME_MERGE		0x0000000000000400ULL	// 1024
#define DO_SPELLCHECK				0x0000000000000800ULL	// 2048
#define DO_INTERJECTION_SPLITTING	0x0000000000001000ULL  // 4096

// this do not echo into tokenFlags
#define DO_POSTAG				0x0000000000002000ULL   // 8192
#define DO_PARSE				0x0000000000006000ULL  // controls pos tagging and parsing both
#define NO_IMPERATIVE			0x0000000000008000ULL
#define NO_WITHIN				0x0000000000010000ULL // dont look inside composite words
#define DO_DATE_MERGE			0x0000000000020000ULL
#define NO_SENTENCE_END			0x0000000000040000ULL  
#define NO_INFER_QUESTION		0x0000000000080000ULL  // require ? in input

#define DO_SUBSTITUTE_SYSTEM	( DO_ESSENTIALS | DO_SUBSTITUTES | DO_CONTRACTIONS | DO_INTERJECTIONS | DO_BRITISH | DO_SPELLING | DO_TEXTING | DO_NOISE ) // DOES NOT INCLUDE PRIVATE

// tenses do not echo into tokenControl
#define PRESENT					0x0000000000002000ULL	 
#define PAST					0x0000000000004000ULL    // basic tense- both present perfect and past perfect map to it
#define FUTURE					0x0000000000008000ULL   
#define PRESENT_PERFECT			0x0000000000010000ULL   // distinguish PAST PERFECT from PAST PRESENT_PERFECT
#define CONTINUOUS				0x0000000000020000ULL    
#define PERFECT					0x0000000000040000ULL    
#define PASSIVE					0x0000000000080000ULL

// tokencontrol parallel values  echoed to tokenflags
#define NO_HYPHEN_END			0x0000000000100000ULL  // dont end sentences using hypens  - shares PRESENT BIT
#define NO_COLON_END			0x0000000000200000ULL  // dont end sentences using colons  - shares PAST BIT
#define NO_SEMICOLON_END		0x0000000000400000ULL  // dont end sentences using semicolons  - shares FUTURE BIT
#define STRICT_CASING			0x0000000000800000ULL  // trust that user means casing (on non-start words)
#define ONLY_LOWERCASE			0x0000000001000000ULL  // never recognize uppercase words, treat them all as lowercase
#define TOKEN_AS_IS				0x0000000002000000ULL  // let pennbank tokens be untouched
#define SPLIT_QUOTE				0x0000000004000000ULL  // separate expressions in single quotes
#define LEAVE_QUOTE				0x0000000008000000ULL  // do not remove quotes around single words
// tokencontrol not reflected into tokenflags
#define UNTOUCHED_INPUT			0x0000000010000000ULL 

//   values of tokenFlags (seen processing input not eched into tokencontrol) 
#define QUESTIONMARK			0x0000000020000000ULL  
#define EXCLAMATIONMARK			0x0000000040000000ULL   
#define PERIODMARK				0x0000000080000000ULL   
#define USERINPUT				0x0000000100000000ULL  
#define COMMANDMARK 			0x0000000200000000ULL
#define IMPLIED_YOU 			0x0000000400000000ULL // commands and some why questions where you is implied
#define FOREIGN_TOKENS			0x0000000800000000ULL
#define FAULTY_PARSE			0x0000001000000000ULL   
#define QUOTATION				0x0000002000000000ULL
#define NOT_SENTENCE			0x0000004000000000ULL   

// in tokencontrol, not tokenflags
#define NO_PROPER_SPELLCHECK		0x0000008000000000ULL   
#define NO_LOWERCASE_PROPER_MERGE	0x0000010000000000ULL   
#define DO_SPLIT_UNDERSCORE			0x0000020000000000ULL   
#define MARK_LOWER					0x0000040000000000ULL   

// in tokenflags not token control
#define NO_FIX_UTF					0x0000080000000000ULL   
#define NO_CONDITIONAL_IDIOM        0x0000100000000000ULL
//	0x0000200000000000ULL   
#define JSON_DIRECT_FROM_OOB		0x0000400000000000ULL   

// end of tokenflags

// these change from parsing
#define SENTENCE_TOKENFLAGS  ( QUOTATION | COMMANDMARK | IMPLIED_YOU | FOREIGN_TOKENS | FAULTY_PARSE  | NOT_SENTENCE | PRESENT | PAST | FUTURE | PRESENT_PERFECT | CONTINUOUS | PERFECT | PASSIVE )

// flags to control output processing
#define    OUTPUT_ONCE			0x00000001 
#define    OUTPUT_KEEPSET		0x00000002	// don't expand class or set
#define    OUTPUT_KEEPVAR		0x00000004 			// don't expand a var past its first level
#define    OUTPUT_KEEPQUERYSET	0x00000008		// don't expand a fact var like @1object
#define    OUTPUT_SILENT		0x00000010				// don't show stuff if trace is on
#define    OUTPUT_NOCOMMANUMBER 0x00000020		// don't add to numbers
#define    OUTPUT_NOTREALBUFFER 0x00000040		// don't back up past start at all
#define	   OUTPUT_ISOLATED_PERIOD 0x00000080	// don't join periods onto numbers
#define    OUTPUT_NOQUOTES		0x00000100			// strip quotes off strings
#define	   OUTPUT_LOOP			0x00000200				// coming from a loop, fail does not cancel output
#define	   OUTPUT_UNTOUCHEDSTRING 0x00000400	// leave string alone
#define	   OUTPUT_FACTREAD		0x00000800			// reading in fact field
#define    OUTPUT_EVALCODE		0x00001000		// UNUSED now	
#define	   OUTPUT_DQUOTE_FLIP	0x00002000
#define	   OUTPUT_ECHO			0x00004000
#define	   OUTPUT_STRING_EVALED 0x00008000	// format string should be treated like an output call
#define    OUTPUT_NOUNDERSCORE  0x00010000	// convert underscore to blanks
#define    OUTPUT_FNDEFINITION  0x00020000	// this is a function being run
#define    OUTPUT_RAW			0x00040000	// there are no special characters, except variable references		
#define    OUTPUT_RETURNVALUE_ONLY		0x00080000	// just return the buffer, dont print it out		
#define    OUTPUT_FULLFLOAT		0x00100000		// dont truncate

// flags to control response processing continue from output flags $cs_response
#define RESPONSE_UPPERSTART		0x00200000  // start each output sentence with upper case
#define RESPONSE_REMOVESPACEBEFORECOMMA		0x00400000  // remove spaces before commas
#define RESPONSE_ALTERUNDERSCORES			0x00800000
#define RESPONSE_REMOVETILDE				0x01000000
#define RESPONSE_NOCONVERTSPECIAL			0x02000000
#define RESPONSE_CURLYQUOTES				0x04000000
#define ALL_RESPONSES ( RESPONSE_UPPERSTART | RESPONSE_REMOVESPACEBEFORECOMMA | RESPONSE_ALTERUNDERSCORES | RESPONSE_REMOVETILDE | RESPONSE_NOCONVERTSPECIAL ) 

#define ASSIGNMENT				0x01000000 //used by performassignment


typedef void (*DICTIONARY_FUNCTION)(WORDP D, uint64 data);

#include "dictionaryMore.h"

#endif
