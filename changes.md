# version 9.0 1/1/2019

1. param inputlimit=n truncates user input to this size.
    On a server, users have a default limit of 80K per volley
    but malicious users abuse this. You can set this to explicitly limit
    how much input users can actually provide.  Consequences of excess
    input are, for example, sluggish performance because it may try to spell correct junk input.
2. ^eval1(arg)
    like ^eval(x) but evaluates its argument before passing into ^eval.
3. mark ~PASSIVE_VERB on such.
4. !~set in concept declaration - see advanced concepts: exclusion.
    e.g., concept: ~wildanimals (!~pet_animals ~animals)
5. "fundamental meanings" can now be used as keywords in concepts, topics, and patterns.
    see advanced cs: advanced concepts: fundamental meaning (quoted below)
Fundamental meaning consists of an actor, an action, and an optional actee.
In the active voice sentence "I love you", the actor is "I", the action is "love",
and the actee is "you". In the passive voice sentence "I was arrested", there is no actor,
the verb is "arrested", and the actee is "I". Wherease in the passive voice sentence
"I was arrested by the police", the actor is "police". 

Fundamental meaning patterns always have a verb, which as a keyword is designated as  
"|arrest|" or whatever word or concept you want to detect.
A pattern which includes a fundamental actor is shown as  
"~pronoun|arrest|". One that includes an actee is
"|arrest|~police", whereas one that has both actor and actee is
"~pronoun|arrest|~police".

# Version 8.8 11/26/2018

1. ^spellcheck(input dictionary)
input is tokenized words separated by spaces
dictionary is json array of words
It outputs the input words adjusted by any spelling correction.
Useful if you read dynamic menus from an API endpoint and then want
to match user input against that menu, whose elements may not be
in the main CS dictionary

2. Interjections from LIVEDATA now also mark their words as a normal
concept set entries, so you can suppress changing the words to an
interjection and yet still match on the interjection concept. This
make writing scripts for interjections significantly easier. See new
document below.

3.  new document Practicum- spelling and interjections

4. you cannot use { or ( immediately after a bidirectional wildcard
    and *~0b is now legal# Version 8.7 11/11/2018

1. $cs_userfactlimit = * means keep all facts
2. documented #SPLIT_QUOTE tokencontrol (removes double quotes from input)
3. ^stats(TEXT) bytes of heap left, ^stats(DICT) dictionary entries left
4. ^jsontext(FactId) adds quotes if object of fact is a json text 
    FAILS if not a json fact.  CS represents json text as ordinary
    unquoted text, but text that looks like float numbers would be
    subject to possible float truncations or expansion of digits.
5. ^replaceword(word location) arguments are like ^mark and ^unmark, except this changes the
actual original word at the position (which is just 1 word of the sentence). This changes no concept
markings (which you can do yourself with mark and unmark). With this, for example, you could unrevise a spellfix
like this:
	u: (_~emosad) $_tmp = ^original(_0)  ^replaceword($_tmp _0)  
6. Parameter:  repeatLimit=n
Servers are subject to malicious inputs, often generated as repeated words over and over.
This detects repeated input and if the number of sequential repeats is non-zero and equal or
greater to this parameter, such inputs will be truncated to just the initial repeats. All
other input in this volley will be discarded.

# Version 8.6 10/4/2018

1. configurl=http://xxx as a command line or config file parameter allows you to 
request additional parameters from a url. This would be important if for security reasons
you didn't want some parameters visible in a text file or on the run command of CS.

2. Normally facts created by user script only impact that user (saved in their topic file).
Now you can create facts that can affect all users. Maybe you want to write a bot that
learns from users like Microsoft's Tay did.
a) (secondary) When you modify some pre-user fact (layer 0, layer 1, boot layer) the change
will move into the boot layer and thereafter be visible to all users.
b) (primary) When you create JSON data in a special way, it will migrate to the boot layer at the
end of the user's turn and not be saved in the user topic file. To do this,
merely use BOOT instead of PERMANENT or TRANSIENT on the initial args to a 
json structure creator, eg ^jsoncreate(BOOT OBJECT). 
Facts moved to boot will be lost if the server restarts or you call the boot
function. A command line argument of "recordboot" will direct CS to write the
these facts into a top level file  "bootfacts.txt" as they are migrated to boot.  You would be responsible
for writing a boot function that reads it on execution to recover these facts on startup. Direct modification of
system facts in (a) are not saved. You would have to write your own scripts to track
those changes.

There is no way of collecting garbage from abandoned pre-user data, so do the
above too often and the server may run out of memory and die.

3. ^stats(FACTS) returns how many free facts are left.

# Version 8.5
1. ^walktopics('^func) finds the topics current bot can access
and calls ^func with topic name, iteratively.
2. ^walkvariables('^func)
3. ^reset(VARIABLES) - sets all global user variables to NULL
4. ^reset(FACTS) - kills all permanent user facts
5. ^reset(HISTORY) forget what was said previously
6. not new but now documented %input = n  sets that system variable
   Other system variables can be set as well, sometimes locking them
   into that value until you do %xxx = .
7. indirect function call ^$_xx()  if $_xx holds a function name

# version 8.4 8/12/2018
1. ^findrule(label) finds a rule with that label (not tag) anywhere in all topics
    and returns the tag for it (presumes unique label)
2. in filesxxx build file, if you name a directory with two tailing slashes, 
	then the system will compile all files recursively within and below that folder.
3. $cs_responseControl RESPONSE_NOFACTUALIZE to suppress fact creation of bots output
4. script compiler directive  ignorespell:  to block some kinds of spelling warnings
	ignorespell:  word1 word2 ...    (use lower case form of word, will not warn about upper and lower case mixtures)
 	ignorespell: *    turn off all spelling warnings on casing
	ignorespell: !*   turn on all spell warnings on casing

# version 8.31 6/18/2018
1. ^readfile(line filename 'function) will read lines from the file and pass them untouched
   as the sole argument of function. This is formerly called: ^jsonreadcvs which is legal but
   deprecated.
2. max match variable is now _30 instead of _20

# version 8.3 6/9/2018
1. new manual Practicum - Messaging
2. loop now allows function call as argument:  loop( ^length(@0)) {...}
3. new manual Practicum-ControlFlow
4. may now use match variables and quoted match variables in json indirections:  
    $_x[_5] = 4
    $_x._5 = 5
    $_x['_5) = 5
    $_x.'_5 = 5
    $_tmp = $_x._5
    etc
5. :allmembers ~concept ~nonconcept ~nonconcept dumps the members of ~concept into TMP/tmp.txt, one per line, 
   but excludes any that are member of the ~nonconcept sets.
6. command line param "authorize" allows all server users to use : commands, regardless of authorized.txt.
7. new manual Practicum - Gleaning

# version 8.2 4/1/2018
1. debugger has autosizing to your screen and save/restore for size/location adjustments you make
2. ^query(exact_svrange x y ? -1 ? ? lowvalue highvalue) Finds facts whose object is x<=object<=y numeric
3. ^query(exact_vrange ? y ? -1 ? ? lowvalue highvalue)  see Predefined  queries section in Facts Manual
 
# version 8.1 2-18-2018
1.CS Debugger now has been released in Release mode, so maybe it works for you, and has new features-- read the manual again.
2. new manuals: Practicum- Rejoinders and Practicum- Patterns
3. $cs_sequence wins over default sequence limit of 5 words in a row
4. !<< >> is now legal

# version 8.0  1/31/2018
1. files to compile by script compiler must now end with suffix .top or .tbl so you can
   allow other files in same directories (like readme.txt, etc).
2. :timelog computes avg/min/max of a server log response times
3. for json arrays can now do:  $_array1 +=  $_array2 and $_array1 += value
4. Windows GUI debugger for CS.  See ChatScript debugger manual.

# version 7.73 12/7/2017
1. several fixes to pattern matcher

# version 7.72  IMPORTANT FIX FOR BUG INTRODUCTION IN 7.6 -- KNOWLEDGE of adverbs in dictionary lost
1. param traceuser=username to make a server trace that user only

# version 7.71 11/29/2017
1. :spellit some sentence, tells you what spellcheck found to modify it

# version 7.7 11/19/2017
1. DLL version of ChatScript now part of std release (untested)
2. $cs_outputchoice to force random output choice
3. :trace treetagger
4. Spellcheck will fix words with excessive repeated letters will be adjust.  3 or more in a row truncated to 2. If not recognized, each single pair of 
   2 will be tested as one. so hellllloooooo becomes helloo becomes hello
5. ~noun_phrase added to allow you to grab noun phrases in input
6. treetagger chunks (if available) get marked
7. :trace INPUT to see input w/o all the concept bindings
8. :tokenize - subset of :prepare that just shows resulting transformations on input, no pos and concept marking.
9. :trace all universal - sets engine to trace everything listed (even in server mode)
10. System now handles multiple upper case forms of a word, so it can
	memorize the particular ones you do differently in concepts and return
	the correct one.

# version 7.61
1. ^pos(canonical xx all)  get all canonical forms (for foreign use)
2. ^mark(_0 value  single)  dont mark the entire implication chain, just the argument given
3. Advanced manual now has a section on advanced tokenization


# version 7.6 10/22/2017
1. ~capacronym -  acronyms that are  all caps, all letters
2. :trim now supports optional quoted first param :trim "keepname"
	which instead of dumping all files it sees into a single tmp/tmp.txt file
	keeps the names of the files it sees separated into tmp/*.txt
3. you can now specify db name to postgres
4, ^pos(isalluppercase xxx)
5. implicit concept set for [] and {} in patterns when simple words/phrases/concepts
6. :dedupe  filepath, outputs into tmp/filename just unique lines from the input
7. command line parameter defaultbot=name gives the name of the default bot to use, overriding the defaultbot table value.
8. ^timeinfofromseconds now returns 2 more values, the months index and the dayofweek index

# Version 7.55 9/24/2017
1. new PDF document: ChatScript Coding Standards
2. RESPONSE_CURLYQUOTES converts on output plain quotes to curly ones

# Version 7.54  8/27/2017
1. limit on arguments to outputmacros raised to 31 from 15
2. ^setresponse(index message) # revises existing response to this (can be used in postprocessing)
3. CS server protocol allows a single null-terminated string, with user and bot components separated by ascii 1 instead of ascii 0.
4. ^pos(canonical xxx yyy) takes optional 3rd argument yyy, the pos-tag of the word (since words using foreign dictionaries may have different canonical values based on pos-tag).

# Version 7.53 8/12/2017
1. :tsv  convert tsv to table form (putting quotes around things without it)
2. %externaltagging
3. improved control script manual

# Version 7.52 7/8/2017
1. ^respond(~xxx TEST) conditional execution of a topic to see if a match would occur (no output)
2. revision of german noun pos tags in dictionary

# Version 7.51  6/25/2017
1. ^pos(isuppercase xx)  does it begin with uppercase letter
2. $cs_proxycredentials  // "myname:thesecret"
   $cs_proxyserver    // "http://local.example.com:1080"
   $cs_proxymethod   1 is most common value to use- see https://curl.haxx.se/libcurl/c/CURLOPT_HTTPAUTH.html
3 tmp= command line  -- reassign location of TMP directory
4. ^jsonopen allows json composite reference to be postdata argument, will write it into text as the data for you
5. :quotelines file, reads lines and puts doublequotes around them
6. Spanish concepts (ontology) and Livedata

# Version 7.5  6/18/2017
1. :dualupper - list words that have more than one uppercase form
2. hidefromlog param - list json fields not to save into user or server log
3. ^jsonparse autoconverts \unnnn into corresponding utf8 characters
4. :trim 12 - put out rule label then input then output 
5. $cs_saveusedJson - destory any json not referred to via a user variable when saving user
6. new spelling-marking manual

# Version 7.42 5/28/2017
1.  *~8b  for bidirectional search
2. ^burst($val digitsplit)  splits into two pieces a word part and a digit part (dd12 => dd  12) and
	(12dd => dd 12) - if it starts with digit, gets all consecutive digits as second value and all the
	rest as first. Otherwise if ends with digit, gets all consecutive rear digits as second value and
 	rest of word before as first value.
3. may now call outputmacros from patterns, it protects match variables across the call


# Version 7.411 5/1/2017
1. fixing numeric bugs

# Version 7.41 4/30/2017
1. ^setposition now allows a wildcard id as the second and last argument
2. the ? operator can now check for value in json array.
3. ^format( integer/float  formatstring value) does format of doublefloat or int64 integer
4. ^clearmatch() resets all match variables to unmatched
5. ^wordatindex() now support ranges, allows optional 3 argument end. Or second argument can be "_0"
6. $cs_numbers =  french, indian, american or other

# Version 7.4 4/24/2017
1. $x.y[] = 1  autogenerates a json array
2. $cs_fullfloat & 64bit float & e-notation means the system gives you full precision, not 2 digit
3. JSONOPen now performs urlencoding automatically
4. servertrace command line param - forces all users to trace
5. erasename command parameter - full user reset if found in input
6. compile outputmacros in any order (but must already be defined if used by a table)
7. ^pos(IsModelNumber x) and ^isInteger and ^isFloat

# Version 7.31 4/8/2017
**--- NEED TO RECOMPILE YOUR BOT! format has changed in TOPIC folder**
1. ^wordAtIndex({original, canonical} n) retrieves word at index either original or canonical
2. ^cs_reboot and ^reboot
3. $x.y = 1  autogenerates
4. replace:  now takes quoted expressions on the right side, decoding to x+y
5. Livedata- internalconcepts.top is a concept listing all internal non-enumerated concepts
6. Livedata- numbers.txt (per language) describes canonical numeric value of numerically oriented words
7. Previously undocumented ^makereal function changed and documented to take an argument. It allows to make transient facts
in a factset permanent (or such facts created after a given fact)

# Version 7.3 3-4-2017
1. topic files of user stored by language as well as user id and bot name if not english
2. `NO_CONDITIONAL_IDIOM`
3. `^tokenize(WORD xxx)` and `tokenize(FULL)`
4. MemoryGC
5. `?$var` pattern test
6. `%timeout`
7. `^isnormalword(value)`   `^isnumber(value)`
8. string comparison in IF or pattern now supports `<` `>` `<=` `>=` for string ordering (case insensitive)
9.  `:redo FILE filename xxxxx` -- to name file to open instead of std backup of last turn
10. `loglimit=n` -- where n is number of MB  log rolling
11. `^removeproperty` supports `HAS_SUBSTITUTE` to turn off a substitution from `LIVEDATA/`
12. Italian dictionary support

# version 7.2  2-9-2017
1. `^jsonreadcvs` takes optional 3rd argument - function to invoke with the fields instead of returning json
2. `^tokenize($_text)` returns facts of (sentence `^tokenize` `^tokenize`)
3. `:mixedcase` - lists all words which have multiple case forms
4. `%language` returns current dictionary language
5. engine concept `~model_number` marks words with both alpha and digit in them
6. command line `buildfiles=xxxx` to tell where `filesxxx.txt` are
7. French, German, Spanish dictionarys and utf8 spellcheck support

# version 7.12  -- 1/28/2017 recompile your bot. TOPIC format has changed slightly
1. ^jsonreadcvs  to read tab-delimited spreadsheet files
2. fixed concurrency bug  -- CHANGING to VS2015 since source using C++11 no longer compiles in VS2010 and you cant easily
 get that compiler version any more.
3. :trim 11
4. Command line parameter manual split off from system concepts and variables
5. topic=  command line parameter
6. renamed $cs_factowner to $cs_botid
7. Moved and expanded discussion of multiple bots from advanced manual to new ChatScript Multiple Bots manual

# version 7.111   1/15/2017
1. fixed major bug in concept reading I introduced in 7.11

# version 7.11
1. allowing @ and . in user file names at login
2. bot: 1  harry    -- sets bot fact owner at same time (see $cs_factowner)
3. allow bot: command as a line in the filesxxx.txt build file to change bot restrictions from there

# version 7.1
1. command line parameter apikey for `:translateconcept`
2. `:translateconcept` to use google translate on concepts from english
3. conditional block comments now supported:  
```
##<<German .....  
    ..  
##>>
```
4. `noboot` command line param
5. cyclomatic complexity listed in map file (described in [Debugger manual](WIKI/ChatScript-Debugger.md))
6. command line parameter `userencrypt` enables encrypting user topic file if `encrypt=` and `decrypt=` is set
7. command line parmater `config=` (default is cs_init.txt)
8. optional `{}` around output macro
9. consolidated all command line parameter descriptions into [ChatScript System Variables and Engine-defined Concepts and Parameters](WIKI/ChatScript-System-Variables-and-Engine-defined-Concepts.md)
10. new manual [ChatScript-Foreign-Languages](WIKI/ESOTERIC-CHATSCRIPT/ChatScript-Foreign-Languages.md)

# version 7.01 1/1/2017
1. fixing $_variables in topics where changing rules destroys the value assigned

# version 7.0
1. logsize - bytes for the log buffer to use (see advanced manual)
2. outputsize - bytes for main output buffer to use (see advanced manual)
3. autoinitfile:  username-init.txt at top level (see advanced manual)
4. new manual- ChatScript Debugger and corresponding :debug command described within
5. @_10 anchor pattern element (see advanced manual)
6. ~integer, ~float, ~positiveinteger, ~negativeinteger refine ~number
7.  RESPONSE_NOCONVERTSPECIAL on $cs_ response or as flag to ^log will block conversion of escaped n,r,t into their ascii counterparts
8. conditional compilation per line supported- see advanced manual conditional build
9. language= command line param (defaults english) revised DICT/LIVEDATA data access per language-  
	WARNING: data organization within LIVEDATA has changed. If you are running your own copy, mimic the new structure.

# version 6.91
1. autodelete and USER_flag1...4 for json
2. :trace -userfact
3. deleting a json fact will recurse and delete the object referred to UNLESS it has some other reference to it still on another json fact.
4. Match(~concept)
5. added ^callstack(@1)  --- removed ^backtrace()
6. $cs_factowner can be assigned a bit value to control what facts you can see


# version 6.9 11/20/2016
1. ^nth now accepts json object or arrays, returns the factid of the nth member
2. command line parameter root=xxxx to tell where CS root directory is
3. for ^jsonpath, if you give a field name first without ., CS defaults the dot
	^jsonpath(field $_obj)  ==  ^jsonpath(".field" $_obj)
4. ^jsonkind($_obj) returns array or object if a json thingy, or fails if not

# version 6.87 11/5/2016
1. Backtrace() to show calls to get to you
2. :trace output ^function  --- you can control function tracing bits in detail
3. ^pick(jsonarray or jsonboject) returns factid
4. $xx.subject or $xx.verb or $xx.object if $xx has fact id
5. %restart - can be set and then retrieved across a system restart (:restart)
6. can use local vars in function definition: outputmacro: ^x($_arg1 $_myarg)

# version 6.86 10/23/2016
1. ^delete now always succeeds. For undeletable things like text, it merely ignores it
2. JSON_DIRECT_OOB tokencontrol for json that is too large for being a token
3. optional 5th param to jsonopen, timeout in seconds for this call (applied 1st to connection, then to transfer)
so total max is double
4. ^environment(variablename) retrieves environment variable value
5. like json object references $x.y.z you can now do array references like $x[$tmp].y[5]
BUT you cannot do assignment into an array for a new index, only on an existing index.
6. input tokenizer from user input NO LONGER auto converts "&" into "and"
7. ^jsoncopy no longers fails if what you pass is not a json structure, it merely returns what you passed

# version 6.85 10-8-2016
1. If (^function()) fails     if function returns false (as well as previously returning 0 or failing
2. @4 = ^jsongather(jsonstructure) - now allowed in addition to ^jsongather(@4 jsonstructure)
3. source= command line parameter will start up using that file as input
4. ^jsongather(xx xx limit) gets facts thru the limit level deep.
5 :trim xx xx nooob  - display outputs without oob data
6. $x.a = null clears as does ^jsonobjectinsert($x a)  use of "" or ^"" sets json literal null
9. $$csmatch_start $$csmatch_end tell range of where ^match matched
8. ^jsonarraydelete(VALUE $array value ALL)
9. ^jsonobjectinsert(DUPLICATE $object name value)
10:  $t.test.$foo =   or $test.test.foo +=    are now legal
11: :coverage and :showcoverage -  the first dumps data on what rules have been output executed so far into
	/TMP/coverage.txt. The latter displays an abstract of all of your scripts, marking rules that
	have been executed and those that have not (by omission)
12: ^import and ^export have special behaviors if the name includes the substring "ltm"
13: outputmacros may now be called with factset references as arguments
14: safe as argument to jsonobjectinsert or jsonarraydelete will not recursively delete json content
	^jsonarraydelete("safe index" $array 3)
	^jsonobjectinsert(safe $object $key null)

# version 6.84 9/25/2016
1. supports json path argument with string for key so you can do ^jsonpath(."st. helens".data $$x)
2. dotted notation supports $x.y.z and $x.y.z =
3. findtext now also returns $$findtext_word which is what word it was found at
4. ^actualinputrange(start end) from ^original data, naming a range of words, what actual words are covered
	returns range (begin<<16) | end
5. ^sleep
6. ^originalinputrange(start end) from ^actual data, naming a range of words, what original words generated it
	returns range (begin<<16) | end
7. command line paramter login=   (documented in advanced manual)
8. ^return(null) now made to be equivalent to ^return() to avoid errors
9. ^jsonopen(direct - returns the read text directly to the answer, rather than converting to json facts
10. %pid - process id
11. encryption, encrypt=  decrypt=    command line parameter (documented in clients and servers manual under encryption)
12. :time (performance measurement) documented in debugging manual
13. bootcmd=  command line parameter executed before CSBOOT is run (if it is run) (documented in advanced manual)

# version 6.83 9/11/2016
1. ^extract now also takes signed arguments to perform relative or backwards extractions,
	eg 	($$source 5 +2) to extract 2 characters beginning at position 5
		($$source 5 -2) to extract 2 characters ending at position 5
		($$source -1 +1) to extract last character – from end, start 1 character before and get 1 character
		($$source -5 -1) from end, start 5 characters before and get 1 character before, i.e. the 6th char from end
2. $cs_usermessagelimit sets max user/bot message saves. range 0...20
3. $var.key
4. clarify ^all = 1 and such assignments to function variables
5. Full wiki documentation up-to-date, with HTMLDocumentation folder corresponding. I have not
	proofread the wiki or html, so it may have minor glitches still.
	PDFDocumentation not generated from Wiki yet (manually updated).
	7. boottrace param to request trace over a startup boot loading of data
	8. proofread of all documentation

# version 6.8a  8/27/2016
1. :trace full   renamed  :trace always,  merely allows you to pass thru ^notrace but you can still choose bits you are tracing
2. mongo support for filesystem changed...
3. ^authorized() allows a script to test for authorization of current id just as debug commands do
4. Giorgio Robino has transcribed 1st half of the CS documentation into wiki format in directory WIKI.
In the future I will update it and remaining documents as I ongoingly revise documents. So currently wiki matches 6.8
Renamed DOCUMENTATION to PDFDOCUMENTATION

# version 6.8 8-13-2016
1. JsonParse allows optional argument before the data of NOFAIL - will return null without error if something is wrong
2. ^jsonpath can end a path with *, which means return fact id rather than object of fact. EG
	^jsonpath(.value* $$id)  or ^jsonpath(".foo[5]*" $$id)
3. full local variables in topics and outputmacros, $_xxx  (see Advanced manual- Local Variables)

# version 6.7b 8-5-2016
USER TOPIC FILE FORMAT now detects the old format again, and accepts it, along with newer format.
1. %zulutime - format: 2016-07-27T11:38:35.0Z
2. documented   :trace factcreate x y z in debugging manual to trace specific fact creations

# version 6.7a 7-31-2016
1. ^'xxxxx'  -- active json string. Advanced manual.
2. postgres user storage name now includes USERS/ and .txt at end
3. renamed %http_response to %httpresponse. Json manual
4. Mongo DB now supported. Mongo manual
5. param nosuchbotrestart to force server restart if bot not recognized. Advanced Manual or Server manual
6 ^layer(word) tells you when word entered into the dictionary. System functions manual
7. no longer building linux postres # version as part of standard. You can build it or mongo as needed.

# version 6.62   7-24-2016
WARNING- USER TOPIC FILE FORMAT CHANGED, old ones will self destruct and clear to empty
1 :restart can take a 1st argument of "erase" which means system should erase topic file of current user (changing systems)
	and it can take up to 4 replacement command line parameters
2. ^jsonlabel(label)
3. ^jsonundecodeString(field)
4. $cs_externaltag ^setoriginal, ^setcanon, ^settag, ^setrole

# version 6.61 7-17-2016
1. jsonobjectinsert will delete old value if exists, then insert replacement.
2. ^jsonarraysize is now deprecated in favor of ^length - you get nonfatal warning messages
3. ^jsondelete is now deprectated in favor of  ^delete - you get nonfatal warning messages

# version 6.6  7-4-2016
1. ^matches - returns string of word indices detected by a match
2. :trace $var
3. DO_SPLIT_UNDERSCORES
4. LEAVE_QUOTE documented from $cs_token
5. ^wordinconcept(wordreference ~concept)
6. ^conceptlist returns facts whose third argument is now start<<8 + end (range rather than just start)
7. MARK_LOWER tokenflag
8. ^clearcontext()

# version 6.5b 6-11-2016
1. ^serialize(@set) ^deserialize(string)  convert factset to a string and back out again.
2. ^query can now also directly return a field value from a match by naming field on the TO argument
	$$tmp = ^query(exact_sv meat eat ? 1 ? @1object)
3: :restart now accepts up to 4 commandline parameters
4. ^savesentence(label) and ^restoresentence(label) will save and restore an entire
   sentence preparation, so you can jump back and forth between sentences in pattern matching w/o real cost.


# version 6.5a 5-18-2016
1. ChatScript now enforces a rule that a function variable name should not be the name of an existing function (hides the function).
This may cause your builds to break until you rename your variables in question.
2. :trace full -- when set, disables NOTRACE() so everything is traced using all flags
   :trace ignorenotrace just turns on the flag, leaving your other bits intact
   :trace none turns off all bits and any ignorenotrace flag
3. PRIVATE_CODE now requires PrivateInit() and PrivateRestart() and PrivateShutdown() functions and
file privatetestingtable.cpp  for any :test overrides, a command line paramemter private= passes its value
thru to PrivateInit
4. ResponseRuleID(-1) does all to date

# version 6.5 -- 5-7-2016  WARNING- change of directory structure in TOPIC
0. TOPIC now gets subdirectories for each of the build layers. While it will work with content in the old format (all at top level)
or the new format (subdivided into BUILD0 and BUILD1 subdirectories, it will only :build into the new format. If you have
deployment scripts, make sure when they perform builds that CS can create these subdirectories.

1. when using pattern-  u: ($$test?)  which would see if word was found in sentence,
if it is a quoted string "buy me" it will be able to find that sequence as matching.
2. ^jsonarraydelete([INDEX,VALUE] array index) to remove element and renumber later ones down
3.  while !not in pattern checks that not doesnt appear later in the sentence, !!not checks that it is not the next word
4.  :trace RULEFLOW will just show the rules as it does their outputs. It is a terse flow of control view.

# version 6.4  4/30/2016
1. ^importfacts may have null as its set value. It wont store the facts created in a factset.
2. ^conceptlist takes optional 3rd arg, prefix to filter by
3. ^words(word) get all dictionary entries with this spelling, upper and lower case, spaces vs underscores
4. #define DISCARDDATABASE renamed to DISCARDPOSTGRES
5. ^dbinit(EXISTS ...) will return normally when db is already open
6. :restart autocloses a postgres db
7. ^CSBOOT will now print its results to console and if a server, to server log
8. ^CSSHUTDOWN added for system going down behaviors


# version 6.3a 4/16/2016
1. jsonarrayinsert allows a leading flag UNIQUE, in which case it does not add to array if already there.
2. ^notrace(... ) turns off regular tracing for the given stream. Explicit function and topic traces are not blocked, just normal :trace xxdx
3. $cs_jsontimeout  sets json wait delay (300 second default)
4. save/restore local variables allow you to avoid naming collisions -- see advanced manual outputmacros:
e.g. outputmacro: ^myfunc(^arg1) ($$tmp $$tmp1)
	code
 the values of $$tmp and $$tmp1 are saved and restored across the call, so the macro may safely write on them.
5. ^return(...)   see advanced manual outputmacros:


# version 6.3 4/10/2016
1. ~twitter_name detected as concept for @xxx names  and ~hashtag_label detected as concept
2. $cs_beforereset and $cs_afterreset if, assigned to functions, will execute them before and
after a :reset. This is your chance to pass some information across to the newly reinited user.
I store data in _match variables before and retrieve them after.
3. Outputmacros can be declared as executable javascript code. New manual in Esoterica for that.

# version 6.2g 3/27/2016
1. fixed slowdown introduced into the engine a few versions ago.

# version 6.2f  3/25/2016
1. ^jsonparse now accepts a flag argument of SAFE, which means you can use it against data the extends beyond the minimal, meaning
if you memorize OOB data after [, you dont have to care to find the closing ], which will be hard since JSON itself has ] in it. Eg
	u: ( \[ _* ) $$jsonfacts ^jsonparse(safe _0)
2. support for spanish spellchecking

# version 6.2e 3/20/2016

# version 6.2d 3/14/2016
1. ^substitute 1st arg can now be "insensitive", meaning character search and case insensivive. Or you can
provide a string like "word insensitive" to pass 2 arguments.
2. %originalSentence will give you the raw user input for the current sentence (after tokenization)
3. ^original(_0) will give you the original text of the user that this match derives from (see docs for more details)


# version 6.2c  3/5/2016
1. You no longer have to backslash [ in an active string. You can do ^"[$count]" with impunity.
   That means that using [] [] notation to designate a choice of text is no longer effective inside
   an active string"
2.  ^Next(loop) will skip the rest of code in a loop and begin the next interation (NO MATTER how far
   up the call chain the loop actually is)
3. CS can now be run old-style (all executables at top level) or within BINARIES
4. %host gives you the name of the currently running host.
5. ^pos(verb be present/past XXX) optional 4th arg to verb form, names pronoun or noun used, so better handles irregular verbs
6. $cs_language, if set to spanish, alters spellchecking for spanish

# version 6.2b  2/24/2016
1. Moved executables and DLLS to folder BINARIES. You must now run CS from there rather than top level.
   CS changes current working directory up 1 level after starting.
   Revise Cron jobs and shortcuts to the exe appropriately.

# version 6.2a  2/20/2016
1. ^norejoinder()  makes this rule unable to set its rejoinder.
2. CS now allows multiple forms of uppercase words, e.g.,  ID and Id
3. Recently a major security hole in glibc, open source c library was announced and patched. CS uses glibc.
   This build uses the new version. If you are running older versions of CS, there is some security risk and
   you are advised to upgrade to this executable linux version, or apply sudo yum update to glibc (or equivalent
   commands) and then rebuild your existing sources.
4. optional 3rd argument to ^jsonpath(xx yy safe) tells it to return ugly data with doublequotes around it. Things with
   embedded json data or whitespace need to be passed into jsonformat using safe data.



# version 6.2 2/14/2016
1. documented existing assignment abilities:  @3 = @2  @3 -= @2  @3 += @2  @3 = $$factid
  @3 += $$factid  @3 -= $$factid
2. new command line parameter timer=15000x10 to control how long a volley can last before being abandoned
by time. 1st number is millisecond limit, 2nd is a checking frequency (reducing load of getting the time
frequently)
3. %maxmatchvariables and %maxfactsets  given the highest legal number for those respective things
4. UNLIKELY INCOMPATIBLE CHANGE:  previously if you used match variables inside a nested pattern component, e.g.,
	u: ( _this ( my _*1 ) )
it would be discarded on return to the top level. THEREFORE except in unusual cases, you would not have nested match
requests.  NOW... they are not discarded. If the above matches, _0 and _1 are both defined. Same for
	u: (this _(my _{often} dog))
which sets 3 match variables

# version 6.1d 2/7/2016
1. new command line parameter debug=   allows you to execute a :debug command on bootup and then exit.
eg. "debug=:trim USERS 4"
2. IF tests can now be made of a std rule pattern (including memorization etc) using form
if (pattern ....)   {}  which means you can match input and even memorize during the if test


# version 6.1c  2/2/2016
1. ^sequence added, similar to refine, except instead of only executing one rule, executes all matching rules.
2. Added :common word1 word2 to tell you the concepts they both particpate in, closest first.
3. $bot has been renamed $cs_bot and $login has been renamed $cs_login with script errors if you use the older names.
4. added internal ~timewords as an enumerated concept as well.

# version 6.1b 1/25/2016
1. variables owned by the bot can be defined (always resident unchanged accessable to all users) - see Advanced Variables in
	advanced manual
2. $cs_trace enables and tracks traces by user
3. right hand side of a comparison in a pattern may now be a double-quoted string, looks at the inside of the quotes

# version 6.1a 1/21/2016

# version 6.1 1/17/2016
1. concept from ontology concepts: ~daynumber renamed to ~dayindex so as not to collide with engine concept.
2. ~common7 redacted
3. ^load(name) to dynamically load a layer of topics (analogous to layers 0 and 1 that load on startup)

# version 6.01a 1-9-2016
1. DICT/BASIC has a small dictionary of words thru grade 6, suitable for use on mobile devices seeking
to minimize CS memory use. See end of Finalizing a bot: Mobile size issues

# version 6.01 1-7-2016
1. easier to follow output tracing
2. jsonopen now supports PUT as a type (with arguments just like POST)
3. ^length now accepts the name of a json object or array, and returns the number of elements it has
4. ^match can take a rule tag instead of a pattern, using the pattern of that rule.

# version 6.0 1-02-2016
1. easier to understand pattern tracing
2. ^print accepts more flags (RESPONSE_ ) controlling output
3. %timenumbers for a complete second minute hour dayinweek dateinmonth  month year data

# version 5.93 12-31-2015
1. New manual "ChatScript Common Beginner Mistakes"
2. time.tbl file removed from worlddata, its 2 concepts merged into ontology concepts, whose order of data has changed
concept: ~month_names (Apr April Aug August Dec December Feb February Jan January Jul July Jun June Mar March May may Nov November Oct October Sep Sept September )
concept: ~month_names_index DUPLICATE (3 3 7 7 11 11 1 1 0 0 6 6 5 5 2 2 4 4 10 10 9 9 8 8 8  )
concept: ~daysinmonth DUPLICATE (30 30 31 31 31 31 28 28 31 31 31 31 30 30 31 31 30 30 30 30 31 31 30 30  30) # 0-based
concept: ~month_proper_names DUPLICATE (April April August August Decemeber December February February January January July July June June March March May May November November October October September September September )
3. :trace none now also turns off all topic and macro tracing
4. $cs_utcoffset now accepts time notation as well, like -02:30 to adjust 2.5 hours before utc.
5. :trace notthis ~topic  will suppress tracing in this topic AND below it

# version 5.92 12-27-2015
1. Jsondelete now only takes 1 argument, name of json composite to delete
2. :prepare now takes optional initial argument NOPREPASS to avoid using prepass topic


# version 5.91  12-23-2015
1. all debug commands have a 2character abbreviation of 1st and last letter (those that conflict
tie goes to the the first in the table).
2. src directory renamed SRC (must use new executables which have correct directory case in LINUX)
3. ~email_url split off from ~web_url and recognized by the engine
4. add ^jsoncopy( jsonref) to duplicate a json fact structure given its name
5. ^jsonparse now accepts extended object references that get or dereference existing json structures
6. %originalinput gives the original volley input
7. new manual ChatScript System Variables & Engine-defined concepts split off from various manuals.
8. the json argument "unique" has been deprecated. it is no longer needed.
9. PRIVATE_CODE define supports you appending code to functionexecute.cpp w/o changing engine source.
   See new manual INSTALLING AND UPDATING CHATSCRIPT


# version 5.9 12-14-2015
1. renamed ~verbs to ~verblist, ~prepositions to ~prepositionlist, ~adverbs to ~adverblist and
~adjectives to ~adjectivelist - these lists are not from the pos-tagger and should not be used
in scripts. They are aggregation data about what is in the corresponding files in ONTOLOGY.
2. :show newline forces newlines to remain in the log file for respond: and start: lines.
3. renamed ^jsonprint to ^jsontree so you can remember it vs ^jsonwrite
4 ^jsonformat( textstring) takes a json string and writes it so that all keys have double
quotes around them. So you can build a json string from a CS format string w/o having to
have (and escape) quotes around the field names
5. new system variable %http_response returns most recent response code from libcurl (for ^jsonopen)
6. optional 1st argument to ^jsonopen, ^jsonparse, ^jsoncreate,
^jsonobjectinsert, ^jsonarrayinsert  is word "permanent" to tell the facts to
not be transient and "unique" to make the facts unique across volleys.
7. ^decodeinputtoken(number) given %token or $cs_token will give english values for the enabled bits.
8. Added writeup just after OutputMacros in advanced manual, comparing them and ^reuse().
9. added ^jsonarraysize(string) to count how many elements are in the given json array name.
10. added discussion of using complex headers to JsonOpen

# version 5.8f 12/10/2015
1. Jsonprint takes optional 2nd argument depth to print to
2. optional first argument to :reset makes it safe to call from inside a script

# version 5.8e  12/5/2015
1. removed ^setposition(value). Now you must use ^setposition(_var start end) you can use @_x+ to create an equivalent effect.

# version 5.8d 11/29/2015
1 ^undelete can take a field restriction like ^undelete(@0object) which insures
that the fact and/or the object is unique

# version 5.8c 11/28/2015
1. improved :testpattern trace

# version 5.8b 11/26/2015

# version 5.8a 11/22/2015
1. :build option NOSUBSTITUTION will disable warning you about various substitutions.
2. Sample comment can now include F to indicate the pattern is NOT supposed to match
3. $cs_utcoffset is hours from utc, %time returns current time in that timezone
4. ^timefromseconds takes optional second argument, timezone (hour) displacement (+ or -)


# version 5.8 11/13/2015
1. ^jsongather(set jsonid) takes the facts involved in the json structure named by jsonid and stores
them in the factset set.
2. ^jsonparse and ^jsonopen take optional 1st argument "UNIQUE" allowing array and object naming
to be unique based on volleyCount
3. $cs_looplimit, if defined, will replace the default 1000 value limit on how many iterations of a
loop the system will stop at to protect against runaway loops
4. Findtext will now substitute _ to space in source and target before matching, to provide
matching either notation.
5. ^canon(word canonical) is analogous to :canon word canonical, and only works during :build. Used
for table control over setting canonicals.
6. ^jsondelete(factset jsonfact) will delete the fact and all facts referred to by it and if the
fact is an array fact, will renumber all later array value facts down 1.
7. ^jsonobjectinsert( object key value)  inserts key and value into object named
8. ^jsonarrayinsert(array value) adds value to end of array
9 ^jsoncreate({object array}) creates an empty json composite.
10 New CS manual  ChatScript JSON

# version 5.72d 10/24/2015
1 ^conceptlist fixed (bad edit destroyed the name)

# version 5.72c 10/23/2015
1. :build will ignore files ending in .bak and ~

# version 5.72b 10/18/2015
1. OUTPUT_NOSTORE renamed OUTPUT_RETURNVALUE_ONLY

# version 5.72 10/11/2015
1. new ^print flags:  OUTPUT_RAW - does not try to interpret " or [  or { or (
       OUTPUT_NOSTORE - does NOT save the answer, but instead just returns it.
2. ^JSONWRITE(id)  given json facts referred to by the root "id", creates the corresponding JSON string sans
   any line feeds
3. new manual ChatScript Engine Manual (in esoterica)
4. Harry (in topic keywordless) now can illustrate Json calls by calling wikipedia for an extract if you say:
	"what is a root"
5. documented that \ can escape an entire token in patterns, not just a character. hence
	( \test=5 ) means the entire token test=5 and not a comparison of test with 5
6.  No longer providing 32bit LINUX # version executables. You need to build them yourself. Or tell me you want it.
	if there is enough demand, I will continue them.

# version 5.7 9/27/2015
1. ^define takes a second argument of "all" or a third argument of "all" to display all meanings or of a particular sort.
2. user flags on facts extended to 8 from 4 and JSON_xxx fact flags named as well.  FACTS manual advanced section now documents a bunch of fact flags.
3. :trace JSON added
4. JSONOPEN & JSONPRINT & JSONPARSE & JSONPATH provide support for internet JSON access


# version 5.61B 9/16/2015
1. added optional argument ("trace") to build to trace the rules it compiles so you can see where it last succeeded before dying...
   :build harry trace
2. ^cs_topic_enter(^topic ^mode) and ^cs_topic_exit(^topic ^result) allow you to intercept calls to do a topic.


# version 5.6 8/28/2015
1. changed LIVEDATA path to be the unchanged path for initsystem embedded CS users.


# version 5.54 8/24/2015
1. new tokencontrol value UNTOUCHED_INPUT. if set to exactly that, the system will tokenize based on spaces only.
2 :tracedfunctions  List all user defined macros currently being traced
3. :tracedtopics List all topics currently being traced



# version 5.53a 8/6/2015
1. ^respond now takes multiple arguments to try in order (similar to the new ^gambit)

# version 5.53
1. in debugging manual documented all the individual :trace options


# version 5.52
1.  filesxxx.txt files hunt order now includes RAWDATA folder.
2. Gambit now takes a series of things to try in order (up to 15) with FAIL as a final legal choice.
WARNING: Back at the beginning of CS 5.xxx ^gambit(PENDING) changed meaning to no longer include the
current topic. But this can be "recreated" by doing ^GAMBIT(~ PENDING)
^gambit(~ PENDING ~mygeneral)
will do current topic, if no gambit then PENDING topics, if no gambit then ~mygeneral topic.
3. new command line parameter authorize=" ... " to supplement authorizations system for debug commands
4 new command line parameter nodebug   disables user issued debug commands
5. documented existing command line parameters  build0= and build1= which do command line builds

# version 5.51
1. speech input/ouput in WEBINTERFACE (for chrome/safari)
2. GetRule(x y) takes 0 as y, meaning the top level rule above us.
3: :verify takes optional 1st param a variable indicating what tokencontrol value to use.


# version 5.5
1. new script marker -
describe: $myvar "used to store data"  _10 "tracks pos tag"
takes a $var or an _var or an @set or a ^macro or ~topic  and a text string and saves that as documentation. See :list
2. :list {$ ^ ~ _ @} will list documented items (and undocumented variables). See Finalizing a bot
3: ^sort now takes optional first argument alpha or alphabetic meaning sort by name instead of value. Also takes instead  "age" meaning put oldest facts first.
4. loop (@2) now uses count of set as loop control. Previously you could also use @2 to represent a count in patterns and IF tests.
5. :prepare now takes optional first argument, a user variable to set the tokencontrol to.
6. You can now control what tracing is done within a topic.  :trace ~topicname takes the current trace flags, uses them for that topic, and sets trace to 0.
Therefore:  :trace basic match  ~topicname all  will set ~topicname to do those 2 kinds of traces and turn off regular trace, then turn on regular trace for everything else
7. :trace input  adds current input to the trace
7: removed the comment in a concept definition (use describe: instead).
8. concept definitions can now be annotated with ONLY_NOUNS or ONLY_VERBS or ONLY_ADJECTIVES or ONLY_ADVERBS. This requires that only the corresponding
interpretation of words can match, both in this set and in ALL concepts recursively referred to by this set. It is similar to marking a concept with
NOUN but doesn't actually change any existing property bits, merely requires them if the pos tagger can determine. ONLY_NONE added to a concept blocks
any propogate thru it

# version 5.4 (extra stable)
1. serious bugs with << and %tense fixed
2. optional first flag to createfacts STRIP_QUOTES to remove quotes from arguments
3. userbugs table in postgres user client to record CS detected bugs

# version 5.33
1. loebner.exe updated to current system
2. WriteFact(factindex) will dump the fact to text. eg
$$f = first(@1fact)
$$tmp = WriteFact($$f)
and
CreateFact($$tmp)  can get the fact back again

# version 5.32
1. indirection can now be applied on a value:
   $tmp = ^$x   or   gambit(^$x)  will, if $x holds the name of a user or match variable, cycle indirectly thru $x to get the value of the named variable.
2.  :build can now define private canonical values for words using  canon: word value word value ...
3. ^burst(wordcount ...) replaced with ^burst(count ...)
3. ^explode removed, replace with ^burst(xxx "")
4. You can now use postgres as a fileserver to horizontally scale CS onto multiple servers - see postgres manual in Esoterica


**********************************
WARNING- Backward Incompatible 5.3 vs 5.2 version

Deprecated $response, $token, $userfactlimit, $control_main, $control_pre, $control_post,$randindex
Now named $cs_xxx - user topic files will be ignored. Script compiler will find your uses and flag them as errors, telling you to fix them appropriately.

***********************************


# version 5.3
1. SetWildcardSeparator redacted. Replaced with setting variable
$cs_wildcardseparator
2. $cs_prepass documented under advanced input in advanced manual. Allows you extra conrol between pos-parsing and your main program.
3. ^reviseFact(factid x y z)  can replace any or all fields of existing fact. use null for fields you don't want to replace. Can only replace non-dead user facts fields. Can only replace compatibly (if subject if a fact, new value must be a factid also)

# version 5.23
1. new FAIL code: LOOP, terminates a loop and also the enclosing rule. new END code: LOOP, terminates a loop but not the enclosing rule
2. concept control word DUPLICATE allows repeated keywords
3. ^timetoseconds(seconds minutes hours date-of-month month year) converts to unix epoch (month can be name or number 1-12. inverse of ^timefromseconds.
4. %daylightsavings - bool 1 if in effect
5. ^timeinfofromseconds(%fulltime) takes an epoch time and decodes it into its components, spreading across 7 match variables. default begins at _0, but _3 = ^timeinfofromseconds(2155) starts on _3. In order:
seconds, minutes, hours, date in month, month (name), year, day of week (name)
6. :show now takes optional 2nd argument, value to set flag to
7. $userfactlimit, if defined, overrides the normal value of how many facts you can track with your user
8. changed command parameter from serverprelog to noserverprelog and inverted default.
9. ^SetWildcardSeparator(char) uses this character to separate words when storing wildcard match values

# version 5.22
1. %outputrejoinder and %inputrejoinder can now be set.

# version 5.21
1. automatic retry when pattern fails by shifting starting match now infinite, no longer just once.
2. ^enable(usedrules) re-enables rules that were disabled so far this volley.
3. ^reset(output) flush all pending output that has been stored away (not including current buffered but uncommitted output)
4. ^save() deprecated to ^enable(write @1) ^disable(write @1)


# version 5.2
1. retry(INPUT) will restart doing the entire input again.
2. :notrace ~topic  ~topic1 -- do not trace during this topic
3. ^disable(save) blocks saving out the user state for this volley.  ^enable(save) unblocks it.
4. ^csbootfunction as outputmacro, if it exists, will execute on startup of the system as a whole. A place to define more system facts and variables that cross all users.
5. :directories shows the execution path to cs.exe, the current working directory, and any assignments to the read/write/static directories.
6. operator |^ and |^=  turn off bits, equal to x & (-1 ^ y)
7. :retry  is safe for multiple users when enabled for a server.
8. new command line parameter "redo" keeps backups of every volley (and prints out current volley) so you can return to any prior volley via :retry @n  new input to revise input leading to the numbered output.
9. :redo 3 replacement input   - returns to turn and reenters.
10. nth(~set count) returns nth member of set, starting with 1
11. ^pos(allupper word) converts acronyms and state abbreviations to all upper case
12. documentation for PerformChatGivenTopic in client/server, along with :topicdump and :extratopic

# version 5.1
1. %revisedinput - current input comes from ^input()
2. outputmacro: ^mymacro variable (^a ^b ^c) allows you to supply fewer args on the call, rest default to null
3. ^argument(2) and ^argument(2 ^mycall) allow you to access arguments of macros (including table generation) by index and even name a calling scope above you
4. order of arguments to ^phrase has been flipped
5
6. ^phrase() now takes optional 3rd argument "canonical" to request the canonical forms of words
7. new command line param   "serverretry"  allows :retry to be used from a server
8. ^respond(PENDING) analogous to ^gambit(PENDING) and also
   ^respond(~) and ^respond(keyword) analogous to the ^gambit args
9. ##<<  starts a block comment and ##>> ends one  (can span any number of lines in a file)
10. :permanentvariables lists all such variables that exist in your script
11. optional 6th arg to InitSystem is a USERFILESYSTEM struct ptr to replace loading user topic files from a filesystem with some other mechanism (like from a database)

# version 5.0
1. added $response to control reply autoprocessing:
	a: #RESPONSE_UPPERSTART makes 1st character of output be upper case (default)
	b: RESPONSE_REMOVESPACEBEFORECOMMA trims spaces before commas (default)
	c: RESPONSE_ALTERUNDERSCORES a variety of things including
	converting underscores to spaces and removing ~ from concept names
2. mark(* _0) will reenable marks on the range given by _0
3. unmark(*) turns off marks on each word of sentence
4. ^mark() and ^unmark() no argument forms, have their meanings switched

# version 4.91
1. bug fix for rejoinder

# version 4.9
1. IsNumber(x) fails if x is not an integer, float, or currency number. It succeeds otherwise, with no output.
2. ^find(@3 $F) can now return position of fact in set
3. Now legal to have null facts in a factset.
4. ^nth(@3subject 12) returns the 12th entry in the set
5. ^setrejoinder now takes optional 1st argument of input or output
6. ^rejoinder takes optional argument which is the rule above the rejoinder block to use

# version 4.83
1. :context to display current valid context
2. removed  .<_0 and .>_0
3. :testpattern can now name ~topic.label instead of a pattern, to test an existing rule.
4. new manual- ChatScript Pattern Redux

# version 4.82  Heavily Stabilized version- Rose # version for Loebners
1. %crosstalk is a 4k buffer you can store data in across bots or users or document reads... it's engine resident.
2. ^unmark(word all)  remove ALL references that match this word in sentence
3. %freetext (kb of available text space) %freedict (unused words) %freefact (unused facts)
4. revised nltk bot to manage memory by default so it can read anything

# version 4.81
1. adding a concept modifier MORE allows you to extend a concept previously declared.  concept: ~city MORE (...)
2. rules with label prefix CX_ will also set context even if they dont generate output
3. ^incontext now returns the volley that matched if it succeeds.
4 added old paper ChatBots 101 to papers.


# version 4.8 - WARNING- save file format change. All prior user topic files are ignored.

1. added query  direct_all   to allow you to get all user facts into a factset (query needs to make from = user
2. any concept defined with a major pos restriction (NOUN,VERB,ADJECTIVE,ADVERB) will make member facts that only allow that use of the word to be a member. And this is recursive through any referenced concept.
3. ^DELETE can now kill an individual fact
4. ^conceptlist now requires a first argument of TOPIC CONCEPT or BOTH
5. ^incontext(label or topic.label) fails if the given label has not generated output within the 4 previous volleys. Succeeds if label has.
6. ^addcontext(topic label) sets a context. Label does not have to exist as a real label.  Topic can be ~ meaning current topic.
6. new command line params users= and logs= to set directories.
7. optional 1st argument "once" to ^burst, makes the burst return the first separation and then the rest of the value unburst.
8. you can now wildcard word spellings with an * in front as well. Eg  u: ( *ator ) matches "I ate a reactor"
9. ^setwildcardindex(val)  tells a pattern match to resume dishing out wildcard slots at the given val (so you don't clobber prexisting useful values)

# version 4.7b
1. ^conceptlist takes optional 1st argument of (CONCEPT or TOPIC or BOTH), returns the set referenced.
2. :extratopic filename  will import that file which was written by :topicdump of a topic, and convert it into internal binary format suitable for a PerformChatWithTopic, and in local mode will disable the topic named in the binary(if there is one) and add the binary and use PerformChatWithTopic instead of PerformChat. Used to test the PerformChatWithTopic interface.
3. :identify prints out the starter label of # version and compile date and ^identify()
4; ^removeinternalflag (word HAS_HAS_SUBSTITUTE) will disable substition from that word (permanently)
5. ^disable(rejoinder) now ^disable(outputrejoinder) and ^disable(inputrejoinder) added

# version 4.7
1. ^respond(topic.tag) or ^respond(topic.label) skips over all responders before the tag/label, starting at the given place instead. If the topic is declared Random, it will ignore that and perform sequential scanning instead.
2. EVServer log format made conformant with non-evserver and user log format
3. :regress terse file now allowed, only comments on bad changed things.
4. PerformChatWithTopic mirrors PerformChat but has extra argument of a topic from external (documented in External Communication)
5. :topicdump dumps named topic or all topics into lines of source, 1 per rule of the topic, in internal text format. (documented in External Communication)

# version 4.6b
1. prefixing username on startup with * in client or stand-alone mode, allows
user to speak first.

# version 4.6
1. the stand alone system recognizes 3 oob messages:
	alarm=xxx
	callback=xxx
	loopback=xxx
If alarm is set, an incoming [alarm] will be sent to CS after the xxx milliseconds have been completed after completion of any current volley.

If a callback is set, an incoming [callback] will be sent to CS after xxx milliseconds, unless the user starts typing first, in which case the callback is cancelled

If a loopback is set, an incoming [loopback] will be sent to CS xxx milliseconds after
the most recent output, unless the user starts typing first, in which case it resets and tries again after the next volley

2. new parameter output=  sets a line length limit for output of chatbot forcing \r\n as needed
3. disable(rule ~) disables current rule
4. :abstract story (display just topics with gambits and their gambits (+ rejoinders)
5. :abstract responder (display topics and their responders (+ rejoinders)
6. :abstract allows a list of topics to do
7. new folder WEBINTERFACE holds the old index.php script under SIMPLE, and  improved scripts under BETTER support the 3 oob messages as well as scrollable log

# version 4.5
1. if the filesxxx file is not found at the top level of CS, it will also look at the containing folder. THerefore you can put your own rawdata and filesxxx build files in there, put the CS folder in there, and update CS by merely replacing the CS folder entirely. If you have your own livedata, it can be in that outer folder and a command
line parameter tells CS to use it instead of the default folder.
2. system= and english=  can be used to redirect accessing those folders within LIVEDATA. This means you can make your own copy of livedata (eg above cs) and then redirect CS to that to have your substitutions, but redirect these 2 folders to stay within chatscript for updating.
3. :conceptlist to list all concepts or all matching a pattern like ~ye*
4. documented :concepts which if given a word, lists all concepts affiliated with it.
 (faster and more convenient that :prepare sometimes)


# version 4.4e
1. ^findtext now takes optional 4th argument "insensitive" to test with case insensitivity
2. ^burst, if you dont tell it what to use to separate, will default to using BOTH
underscore and space.
3. improved placenumber labelling and 24'th is now accepted as a placenumber (the ')
4. new document Overview Input to Output
5. :regress file format changed, now works with :trim as well. You need to redo any regression file using init again.
6. documented more codes for query scripts (S V O in section 1)
7. altered ^first,last,pick to accept @0raw and field to accept @0 raw

# version 4.4d
1. noticable bug relating to multiword recognition like "creme brulee" in dictionary fixed.
2. improved display on :verify pattern and :testpattern
3. better display for :regress
4. bug with concepts declared with a systemflag (which didnt get set) fixed


# version 4.4c
1. optional second argument to ^first,^last,^last is KEEP which means select but dont erase the fact member
2. ^Getverify(tag) will retrieve the verify tag content if it exists in the VERIFY directory
3. concept declaration can now have property UPPERCASE_MATCH, when used with _~concept in pattern match, result will force uppercase wildcard storage regardless of how you entered the words (storing the title or name of someone)
4. :trace reject  shows you what messages were not given because they repeated.
5. ^result(stream)  evaluates the stream, traps ANY return code and returns the text of the return code.

# version 4.4b
1. :trace SAMPLE displays the sample input when tracing a rule.
2. definition of << >> pattern match modified to set the matching location AFTER the >> to be just like the start of a match, ie something that looks like < * (match the next listed token anywhere in the sentence).
3. ^phase(_0 noun) returns noun phrase running thru here
   ^phrase(_0 preposition) does it for prep phrases
   ^phrase(_0 verbal) does it for verbals;
4: script compiler now accepts the declaration
	query:  name  "query string data"  
to define private queries analogous to LIVEDATA/SYSTEM/queries.txt
5. :queries lists all defined queries
6. ^field now accepts flag as a type
7. ^hasanyproperty and ^hasallproperty now accepts CONCEPT and TOPIC as choices
8. USER_FLAG1 .. USER_FLAG4 now defined as marks you can put on facts.
9. if 1st arg to ^setposition is a match var, then it will take 2 more args (start + end) and set the position data for that matchvar.

# version 4.4
1. ~dateinfo marked on dates (some combination of month day, month year, month/day year
2.  tokenization control DO_DATE_MERGE now exists so dates involving month + year/day can be merged into one token
3. indirect function calls are now supported...
	e.g outputmacro: ^x(^arg1 ^arg2) ^arg1(^arg2) or
	$$tmp(argument) or _9(argument)
4. format strings are safe to use directly as arguments to functions.
5. Advanced users manual split into that + ChatScript System Functions Manual
6. Use of ^mark with a regular word (not concept) will force that word to reflect in ^conceptlist even though it is not a concept.
7. :trace label reports as it passes thru labelled rules
8: ^dbopen(^"null") will fake a dummy postgress database whereby you can run code without actually storing anything away.
9. :commands on    :commands off    to locally disable :commands



# version 4.33d
1. format strings ^"xxx" are allowed to continue onto successive lines. Will have only a single space at each line break, regardless of how many were before or after
2.  rename: can now declare user defined number constants:  ##myconstant 244
(check for hex also)
3. ^END(CALL) to termiate a user function call but appear normal afterwards.
4. ^pos(RAW number) returns the word at that posn in the sentence
5. builtin concept ~daynumber defined (for numbers 1 - 31)
6 optional exclude arguments to ^conceptlist removed. Instead ^addproperty(~set NOCONCEPTLIST) to sets to ignore
7. :document reader will react to any :command given at start of lines in document file.

8. concept ~being_list merged onto ~beings and thus deleted
9.  ~BE_VERBS removed from ~special_activity_verbs



# version 4.33c
1.  ^burst now allows optional first arg WORDCOUNT to return how many words appear if you burst (doesnt do the burst).
2. ^sort now accepts additional factset args and sorts them to match the sort of the first argument
3. _3 = ^field(factid all) will spread subject/verb/object onto _3, _4, _5
4. OUTPUT_NOUNDERSCORE flag for log will convert underscores to spaces (which is what output to user do automatically)


# version 4.33b (extra stable version)
1. when running CS as client=, if you enter a :restart command to the server it will do that, but also restart the client= asking you for a new login.
2. ^decodepos(pos index) gives text value of POS bits at that word location or
   ^decodepos(role index) gives text value of ROLE bits
3. ~uppercase and ~utf8 concepts added to system
4. conceptlist takes any number of additional set arguments to exclude including
	~pos, ~sys, ~role  to exclude those underlying sets
5. extended documentation of ^pos() includin new:
	^pos(hex32 number) ^pos(hex64 number) converts ints to hex 0x...
	^pos(conjugate word POSCODE) returns inflected word


# version 4.33a
0. fixed crash bug found by user
1. fixed infinite loop bug found by user

# version 4.33
0. ^GetRole  ^getPos returning parse data about a word in a sentence
1. login= command line parameter
2  :trace SQL added
3. auto double quoting for postgres text args that contain a quote
4. extra optional initial arg to ^eval like ^print, flags to control how it generates its answer.
5. output macroargs accept optional control specifier .HANDLE_QUOTES (good with postgres)
6. :variables match shows match variable bindings
7. rename: _goodname _10  as top level script element to relabel match variables
	and @myname @10 to relable factsets


# version 4.32
0. removed :debug (debugger system)
1. bug fixes to :testtopic, script compiler, possible termination of engine on certain input sequences
2 substitutes.txt file split to noise.txt which covers all noise words that erase themselfs. DO_NOISE is now included in DO_SUBSTITUTE_SYSTEM so it is compatible, but
if you turned off some files and used DO_SUBSTITUTE, you may want to add in DO_NOISE
3. ~topic added to detect words that are names of topic (with or without ~)
4. ^conceptlist() given a wildcard reference or absolute index in a sentence, generates facts of the concepts referenced by that location.
5. for composite proper names, mark will now mark also the last word, and if the name is a human name, the first word as well.
6. %bot added, parallels $bot (and is better to use system variable)
7. systemflags for pos-tag tie values now exposed via names:
PROBABLE_PREPOSITION, PROBABLE_ADVERB, PROBABLE_ADJECTIVE,PROBABLE_VERB,PROBABLE_NOUN usable with ^GetProperty
8. ^"xxx" strings (both format and functional) are now allowed to use [][] choices and make function calls.


# version 4.31  --- fixes FUNDAMENTAL bug in dictionary code...
0. fixed script in :document writeup, enhanced :wikitext
1. added ^uniquefact to intersect two sets and returns items in 1 not in 2
2. allowing ^analyze within main control and not just postprocessing
3. ^postprocessprint now split into ^postprintbefore and ^postprintfafter based on prepending or postpending the output to existing output
4. redocumented *-1, an example of a backwards wildcard to retrieve the word before the current location in pattern matching
5. ^memorymark() and ^memoryfree() to manage memory when reading really large documents
6. disabled the optional language argument to :restart, removed language.txt file
7. :facts @3  now displays contents of a factset
8. server default volley limit (save=) now defaults to 1, save every time. High volume servers might optimize by raising this, but there is no need on low-volume servers.  userhold= server parameter removed.
9.  :quit can be placed in a file you :source or a top file you :build and it skips the rest of the file.
10. emergency allocation scheme added for if user topic file gets bigger than your cache allocation value

# version 4.3
0. :wikitext to extract plain text from mediawiki format (wikipedia, simplepedia)
1. :document revised and expanded document of its own (in esoterica) along with :wikitext
2. ^postprocessprint changed to append its output to the existing output, rather than putting it before the existing output.
3.  file open path revisions to allow IOS to do :build and other commands

# version 4.24
0. default builds all are sans postgress support. Additional builds that support postgres are all labeled ChatScriptpg. Some machines like Windows Server 2008 do not have some std visual studio libraries that postgres client uses, unless you have installed the postgres server.
1. tcpopen now url-encodes its data argument SO YOU SHOULDN'T. Except you can use GETU and POSTU to indicate you are supplying preencoded data. Also it sets $$tcpopen_error with a message if the function fails. GET bug fixed.
2. ^log now takes OPEN and CLOSE arguments that can replace FILE, to keep a file open for writing when used heavily in a volley (like during :document)
3. ^findtext( source pattern startoffset) returns index of the pattern first find in source starting at offset
4. ^extract( source from to) returns the substring designated -  combined with #3 and it's handy for using tcpopen to read web pages for specific data.
5. fixed bugs with serverctrlz, :document xxx single, tcpopen,
6. new document "What is ChatScript"

# version 4.23
1. %os tells what os you are: windows, mac, linux, ios, android (if you #define ANDROID)
2. talk.vbs script added to chatscript root and harry now speaks by default on windows. See ~Xpostprocess in simplecontrol.top
3. tcpopen and popen accept null for function argument
4. paramater serverprelog tells server to log input before working on it, helps see crashes
5. fixed crash in tcpopen
6. parameter serverctrlz tells server to complete output with ascii 0 then 0xfe 0xff
7. prior change of server socket parameters from SO_REUSEADDR to SO_LINGER seemed to cause problems. Revised to SO_REUSEADDRE false.

# version 4.22
1. new dlls for postres added, now reenabled by default for windows
2. new :regress  facility for testing large programs (see Finalizing a Bot)

# version 4.21
1. Disabled by default postgress code in windows #define DISCARDDATABASE since some dll's it seems to require are not available unless postgresql is installed.

# version 4.2
1. %user and %ip added to make  login information  available
2: datum:  one-liner tablemacro use that can be declared at top level within a topic
3:  can declare a table as variable to avoid saying ... to fill missing args- eg table: ^x variable (^arg1 ^arg2)
4.  new advanced manual section ADVANCED VARIABLES better described indirection variables
5. ChatScript now directly supports PostgresSQL database access and a new manual ChatScript PostgreSQL (in esoterica folder) describes how to use it and added bot postgres

# version 4.1
1. DEFAULT Burst character is now " " instead of "_"
   Several versions ago the storing on variables of multiple words captured on input
   was changed from _ separation to " " separation, but I neglected to change burst
   default. Now rectified. BUT... if you had ^burst(_3 _) code you've written, it is
   probably wrong and should have been changed to ^burst(_3 " ").
2.  :diff now writes the differences list also to LOGS/diff.txt
3. nouserlog command line parameter now works in non-server mode
4. added %engine %script %dict  for when those components were made
5  ^lastsaid() returns what bot just said
6. SHARE (bots keep some state in common) see Advanced Topics
7. added ? as standalone (not op) to IF test
8. LIVEDATA subfolder SYSTEM created and canonical.txt, queries.txt, systemessentials.txt moved into it, less likely you will change them.
9. Gambit() and GambitTopics() fixed so it doesnt do topics with the NOGAMBITS flag on unless explicitly asked for.
10. :autoreply generalized to repeat whatever input you tell it to repeat
11. :abstract improved to show reuse() gambit()  refine() respond() instead of just labelling them { code }

# version 4.03
1. new manual ChatScript App Client Manual and new folder subsections in documentation
2. fixed :reset user bug
3. improved server machine loading of windows chatscript
4. ^getparse() deleted, as now marks can cover as much of the sentence as needed
5. improvements to postagging so src, DICT, LIVEDATA/ENGLISH are incompatible with prior such folders (has no effect on your TOPIC folder data)

# version 4.02
0. Short debug commands like :do, :why, :say had accidently been disabled with 4.0. Fixed
1. # version number hadn't been incremented either.

# version 4.01
0. Bug fix for canonical of 1.46 shouldn't be 1
1. Fixing loss of John proper name and others in dictionary
2. Fixed missing ' in output from contractions
3. bug in ^setrejoinder using a labelled rule fixed
4. revision on dictionary -   ~verb_tenses now ~verb_bits and dictionary rebuilt along with LIVEDATA/ENGLISH revisions

# version 4.00
0. setting position reference and direction changed from @_0++ to @_0+ and same for --  and you can now use @_0_i  (inclusive) to make the _0 location be the next thing scanned.
1. the livedata PRIVATE substitutions file has been moved into the scripting language. A new top-level declaration is replace: xxx yyy  which names an individual
substitution to add which is enabled with the DO_PRIVATE $token flag.
2. fixed :verify infinite loop and various other potential crash bugs


# version 3.81  
0. new document Paper- WinningTheLoebners
1. mark xref per concept/word raised from 7 to 32
2. ^tally to get and set an integer value on a word (transient to a volley)
3. ^walkdictionary('fn)  to execute named outputmacro on every dictionary entry
4. new document Bot Harry - moves initial discussion of harry from basic manual
5. new document Bot Stockpile - describes how to run the planner bot
6. new bot and document Bot NLTK - describes how to run natural language analyses

# version 3.8
0. Fixed bug in :shutdown that crashes on shutdown
1. moved hardtrace.log to LOGS and renamed exitlog.txt
2. fixed potential crash bug in spellcheck
3. :shutdown merged into :quit and removed, use :quit for server or non-server exiting chatscript
4. fixed bugs when using :build or :restart on a server
5. new DICT, bits on dictionary entry, data in LIVEDATA/ENGLISH

# version 3.73
0. 3.72 introduced serious tokenization bug that broke contraction processing.   Fixed.
1. new manual ChatScript External Communcations- moves stuff from advanced on calling outside things
2. :topicstats, :abstract, and :topicinfo now accept wildcard topic like ~ne*
3. :commands will now display their results across a server connection (subject to buffer restrictions)
4. #DO_CONDITIONAL_POSTAG removed

# version 3.72
Most of the work has been to increase the accuracy of english pos-tagging.
extensive changes in engine and dictionary entries properties and systemflags and concepts related to pos-tagging and parsing
1. parserdata.top split off from verbhierachy.top in ONTOLOGY and put in own folder
2.  warning removed - now allows creation of topics with no rules.
3. %sentence is true if input had a subject (possibly implied) and verb.


# version 3.71
1. severe bug in pattern matcher fixed

# version 3.7
0. :document now takes file or directory
1 %document now exists to tell you you are in document mode
2  %command (sentence type) and %impliedyou (you as implied subject)
3. added ^RETRY(TOPRULE) to return from a ^refine() to the outermost rule to retry
4. pattern matching can now change direction using @_n++ and @_n--
5. added documentation old paper "Speaker for the Dead"
6: added documentation paper "Writing a Chatbot"
7: added :concepts to tell you quickly how to generalize a word

# version 3.64
0. ^iterator allowed in any rule not just in planning rule and planning example now also uses iterators.
1. ^Tcpopen allows get/post from script
2. :trace display cleaned up, you can do collections like :trace basic  :trace mild  :trace detail,  and :trace tcp added for ^tcpopen.

# version 3.63
0. dualmacro:   allows you to use same code as an output macro and as a pattern macro
1. new command line parameter  livedata=   allows you to name your own directory to load livedata from.
2. :trace now accepts - xxx to turn off stuff, so :trace all - infer - pattern can be used
3. bugfixes and revisions to how Planning system works - read revised doc
4. :build stockpile and stockpile planner demo now exists
5. :build 1  is now :build Harry

# version 3.62
0. authorizedIP.txt file now also takes L_loginnames that can be authorized.
1. new manual - finalizing a bot
2. you can name a range of topics to :verify by ending with * , eg,  :verify ~f*
3. :verify sampletopic to restrict warnings to only samples resulting in wrong topics.
4. ^Log(file filename new)  use of new to clear out any existing content in file
5.  out-of-band sentences (ones leading with [...]) no long spell check inside the [ ] area nor do they postag/parse.
6.  :silent toggles showing output or not (good for regression tests)
7. :diff file1 file2 - reports lines that differ between these two files.
8. new manual- installing an amazon cloud server.

# version 3.61
0:  NOSAMPLES added as topic control flag to block :verify sample on topic
1. named locations in worlddata and concepts are marked with systemflag LOCATIONWORD  and
	concept ~locatedword has been renamed to ~locatedname
2:  :gambittest moved into :verify gambit
3. ^findfact(subject verb object) added
4. new query control field 1 values: S, V, O for queing incoming item as a fact reference.
5.  ^flushfacts(factid) to kill off all facts created after this one
6. added << and >> as additional arithmetic operators on assignment  $$tmp = $$a << 4
7. added NOGAMBITS as topic control flag to block :verify gambit on topic

# version 3.6
0. WARNING- DICT binary format changed, src corresponds to it.
1. ^createattribute  function similar to createfact, but insures this is the only fact with this combination of subject and verb.
2. plan:  script element added and ChatScript Planning document added
3. ^Iterator() added for plans
4. ^marked() now returns FAILRULE bit if word is not marked, instead of null.

# version 3.54
1. :say xxx yyy zzz  forces chatbot to say that message as its output (good for testing postprocessing scripts)
2. :facts xxxx  displays all facts involving word or meaning given
3. WORLDATA facts on authors changed from "exemplar" to "write"  for their works
4. WORLDATA facts on musicians changed from "exemplar" to "sing" for their works
5. added NOPATTERNS flag to topic define, so can avoid pattern tests using :verify on that topic

# version 3.53
1. can now use :trace on functions (turns on all tracing on patternmacro or outputmacro)
2. documented how to deal with issues in proper-name merging in pos-parser documenation.
3. ^uppercase(word) returns 1 if word starts with uppercase letter and 0 otherwise.
3. changed a bunch of worlddata geographic names to have the property ALWAYS_PROPER_NAME_MERGE
4. undocumented pattern notation metal~~  which matches metal and all direct synonyms of it (e.g., alloy).
5. ^trimhistory(who count) added to chop off history
6. restored ... ellipsis as a final argument to a table to fill in rest of args with *
7. added ^reviseoutput(n value) to overwrite existing generated response n with given value
8. added systemproperty for word  NO_PROPER_MERGE to mark concept words which should never merge into a proper name (eg URL)

# version 3.52
1. documented ^position() which returns location information on a match variable
2. added ^setposition() to allow you to arbitrarily set current match location in a pattern match.
3. added topic flag nokeys similar to newly documented noblocking, suppresses all keyword :verify tests for that topic
4. documented IGNORESPELLING feature on concepts- suppresses compiler warnings
5. removed documentation on old server feature TIME (server no longer times out, client must if it wants to)

# version 3.51
1. :revert renamed to :retry
2. new document ChatScript memorization
3. crash bug fixes for mac

# version 3.5
1. added NOUNPHRASE to GetParse()
2. added Set match Position ability to jump backwards during pattern match:  @_0 being an example
3. ^join(AUTOSPACE ...) adds blanks between each item automatically
4. double underscores in output to user will allow a single underscore to show to user
5. bot: xxx  will restrict all topics in a file occurring after this top level command
6. :show topics  will display all topics that the input would trigger by keyword

# version 3.4
1. :topics sentence   displays the topics marked, value, and words triggering it
2. :definition displays code of the macro named
3. function argument variables now allowed within format output strings
4. HARRY project and filesx.txt file reorganized
5. new documents PAPER Winning 15 minute conversation & ChatScript Exotica Examples

# version 3.3
1. added ^available to see if rule has been used up or not
2. :document mode to read an entire document (in advanced manual not debugging manual)
3. :abstract now can take a filename instead of a topicname
4. :abstract pretty will prettyprint your topic(s)
5. :abstract canon will prettyprint, replacing noncanonical keywords in patterns with canonical ones
6. ^GetParse added to retrieve phrases, clauses, and verbals
7. added :overlap to see what members of set1 are in set2
8. :memstats to show memory use
9. :noreact blocks reacting to input
10. :show pos turns on pos summary
11: parser manual updated to explain how to access parse data from patterns
11: ^flags(word) and ^properties(word) to retrieve dictionary flags on word
12  new tokenflag: ONLY_LOWERCASE ignores all proper nouns and name merging to use lower case forms of words

# version 3.2a
1. revised popen to strip out \ off embedded doublequotes in command string

# version 3.2
1. function calls to SYSTEM functions can now take relation operations inside patterns.
2. improved pos-tagging and parsing for english
3. optional 3rd argument to export to append to a file rather than overwrite
4. optional preliminary argument to ^log to name file to log to other than userlog
5. ^popen function added
6. new manuals on pos-tagging (for foreign language developers) and control scripts

# version 3.1
1. bug fixes - loebner entry # version

# version 3.06
1. fixed :trace ~topicname and various other bugs
2. added tokencontrol NO_INFER_QUESTION and added "Controlling Input" to advanced documentation
3. %fact  - most recent fact id
4. brought back the parser

# version 3.05c
1. fixed NOUN_TITLE_OF_WORK should not merge proper name into one token "the fox" is a movie name but shouldn't become a single token
2. fixed script compiler failing to notice a; xxxx as a faulty rejoinder
3. fixed tokenization bug
4. added ^position, ^capitalized
5. reinstated English Pos-tagging
6. fixed tablemacro bugs

# version 3.04
1. tables accept .KEEP_QUOTE on arguments
2. crash in NEXT fixed

# version 3.03
1. fixed crash in :restart (server + standalone)
2. fixed output mode OUTPUT_NOQUOTES

# version 3.02  
1. fixed bug in scriptcompiler for u: (!^query())- not before call failed
2. fixed manuals

# version 3.0
1. reverted some newer capabilities from deleted versions above 2.0
2. can now escape pattern comparisons - u: (\gesture=smile)
3. support for foreign language dictionaries
4. embedded debugger
5. rule tags to increase introspection

# version 2.0
1. added ~money and improved money tokenization and canonization in the system
2. added DO_POSTAG and DO_PARSE and DO_NO_IMPERATIVE as tokenControl values for bots
3. added :postrace command to watch postagging/parsing
4. added dynamic query abilty, using _xx and $xxx variables as arguments for subject, verb, object and '$$ and '~xxx mean dont expand the choice, while '_xxx still means orignal form
5. can block con# version of & to and in script compile, by preceeding & with \
6. added lowercase and canonical to ^POS
7. raised max function argument count to 19

# version 1.99	--- last # version of 2011
1. fixed SYSTEM command
2. fixed token errors related to questions
3. added |=  &=  ^=  to assignment
4. made responders allow more data

# version 1.28
1. MAC OS compiles server now
2. TIME= command line option
3. USERLOG NOUSERLOG SERVERLOG NOSERVERLOG
4. LOGS subdirecotry added

# version 1.27
1. allow utf8 and support DO_UTF8_CONVERT to change some accented to normal ascii.
2. renamed :undo to :retry
3. added capitalize to ^pos()
4. $crashmsg

# version 1.26
1. bug fixes for 64-bit linux and other crashes

# version 1.25
1. added ^hasProperty(word, bit)
2. documented ~ as pattern word for current topic
3. added %all
4. altered :undo to take replacement sentence on same line

# version 1.24
1. added :debug to set internal debug flag
2. added a couple of pdf papers about chatscript into documentation
3. added :reset analogous to ^reset(USER)
4. added xs xv xo to test sentence marks in inference system for part 3
5. adding data to :help function to briefly describe most functions
6. added :all to assist in debugging
7: added :undo to retry user input with replacement
8: documented the ?  option in pattern matching (advanced)

# version 1.23
1. system variable %userInput added
2. automated pronoun handling supported along with improved pos tagging

# version 1.22
1. help now takes "function" or "variables"
2. Pos tagging improved, migrating toward full parsing but not there yet
3. linux server no longer busy-waits - Linux build now requires -lrt as well

# version 1.21
bug fixes, more pos tagging  and doc added for ^match()

# version 1.20
1. improved POS tagging, with rules put into Livedata
2. tables accept ... to mean fill in remaining arguments with *
3. added :testpos to verify pos tagging and regress/postest.txt
4. added :pos to display pos tagging and how it happened
5. :word now can take a series of words to display
6. tables now allow a short-entry to default missing tail entries
7. :help lists all the : commands
8. argument "sandbox" tells server to disallow access to OS from script
9: revised various tables using verb exemplar to use more direct choices:
	star  - for actors/actresses and the movies/tv in which they starred
	write - for authors and the books/poems they wrote
10: chatscript tutorial written by  Erel Segal

# version 1.17
1. fixed bugs in Linux server mode
2. added optional "echo" to :source
3. substitutes adds negation testing on the result arg
4. ~introduction basic sample topic renamed ~introductions

# version 1.16
1. :prepare now takes all and none to mean set as mode for future
2. revised memory usage--- Linux # version was dying in malloc

# version 1.15
1. Added ^settopicflags()
2. :topics added
3. An output choice may now be [!$value ...] in addition to [$value ...]

# version 1.14
1. ^keywordtopics now also accepts topics labeled nostay
2. ^mark allowed optional second argument, a match variable and optional 3rd argument, a match_variable
3. ^unmark order of arguments flipped, and 2nd argument made optional
4. script compiler now spell checks patterns and outputs and warns you about questionable words
5: :build optional 2nd argument "nospell" to suppress spell checking and "output" to add output checking
6. documented unipropogate query
7. pos(snake noun 2) -> snakes

# version 1.13
1. revised autogeneration mechanism
2. now accepting [] instead of () for topic and concept definitions
3. manual revised to use [] instead of ()
4. removed ~teenword (now considered ~adultword)
5. handling of ~infinitive improved
6. added :used to see what responders are used up in a topic
7. :verify rule changed to :verify pattern

# version 1.12
1. added POS(noun,word,proper) to uppercase to proper name
2. allowed to have global user variables from a :build commands
3. added autogeneration of responders

# version 1.11
1. fixed serious bug in script compiler

# version 1.1
1. added %rejoinder (indicates system has a rejoinder it can expect)
2. clarified eraseure in esoterica
3. added ^import and ^export to read and write fact files
4. redefined factset decompositions (@1subject, @1object) and added  @1fact @1verb @1+ @1- @1all

# version 1.01
1. stopped deleting the "to" in front of an infinitive.
2. bug fix with repeat topic flag
3. other bug fixes

# version 1.0
1. supporting unique names for public servers (if user name comes in from webpage with . prefix).
2. insuring all boolean system variables return "1"/defined and null/undefined
3. removed $defaulttopic variable used by engine which caused a modification to the control script
4. optional 2nd arg to reuse  allows it to skip over a responder that has been marked d
5. added :bot to change bots on the fly
6. table arguments allows to specify a designation that the entry is script by prepending ^ to a "xxx" table entry string. Eval() will execute it
7. modified doublequoted string handling on output side.... normal strings display with their " ".
   strings with \ in front no longer have any special meaning
   strings with ^ in front mean output string w/o quotes as a format string
8. added documentation section Esoterica and Fine Detail
9. earlier optimization added introduced a mega serious bug which was reported and now fixed, screws up a lot.
10. added ^nofail and revised control script to use it

# version 0.95
1. bug fixes
2. redescribed noerase so that gambits are always erased. It only controls responders.
3. Continuation  renamed rejoinder

# version 0.94
1. multiple copies of topics allowed for multiple-bot handling-- see WHICH BOT in documentation
2. :abstract allowed a topic name argument
3: added flag to mark object-pronouns to improve pos-tagging commands
4. extended assignment syntax to allow any length of operations, so $var = 1 + 2 % 3 * 4 is now legal
5. enhanced output of things like :testtopic and :testoutput to show variables changed as well.
6. changed file format of user save data... any files in USERS folder you have should be removed.
7. moved %token over to $token
8. allowing #define constants as ordinary items in output
9. added %question
10 moves ~qwords from script to engine

# version 0.93 This is considered a stable build and will remain as new builds arrive
1. fixed bug crash on misspelled words and other crashes found by a massive regression test.
2. added windows hardware exception trapping to server so it cant crash
3. split ~singular and ~plural into: ~nounsingular, ~nounplural, ~verbsingular,~verbplural
4. moved ~auxverb from script to engine

# version 0.92
1. fixed bugs introduced by spellcheck, which destroyed the interjections system and some canonical forms
2. documented the interjections system in the manual
3. renamed concept set ~aux to ~auxverb
4. added %token and changed the default to allow assignment to all %systemvars.

# version 0.91
1. modified :revert to automatically redo your last input
2. fixed serious bug in marking used responses

# version 0.9
0. added ^fact() and removed fact duplicates
1. added :prepare and :postprocess and :testtopic and :real and :testouput
2. added spell checking
3. documented ^analyze and _0=#define for values of dictionarysystem.h
4. bug fixes
5. documented global macrovars ^$var and ^_0
6. added pattern positioning @_2
7. removes :noreact and %noreact
8. :regresssub renamed :verifysub  :dummyinput renamed :fakereply   :writefacts renamed :facts
9. upgraded POS analysis

# version 0.8
0. :shutdown added
1. :verify upgraded with new ability
2. LoebnerVS2008 fixed to compilable
3. misc bug fixes
4. added files for testing servers, better server logging, and writeup on using servers

# version 0.7

0. Internal representation for "refines" changed to use member verb
1. Misc bug fixes, particularly to ^query and the \"xxx" construct
2. added :variables

# version 0.6

0. misc bug fixes and improvements to :word and startup display data
1. removed %bot  (redundant with $bot)
2. added %server (server mode vs standalone mode)
3. added ~unknownword (unrecognized input words)
4. added $var? (can var content be found in sentence) and _9? (can match var be found in sentence)
5. added :stats
6. added ^save(@setref true/false)
7. added support for Windows server
8. added overlay to run Loebner protocol (different project) under vs2010
9. _var (match variables) restricted to _9 max
10. :define documented
11. added VS2008 build data

# version 0.5 --- base line
