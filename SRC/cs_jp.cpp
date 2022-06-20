/*** Tokenizes for japanese chars */
#include "common.h"
#include <stdio.h>
#ifndef DISCARD_JAPANESE
#include "mecab.h"
static MeCab::Tagger* tagger;
#endif

// structure for japanese parsing.
static jp_tokens_t jpt;


int malloc_jp_token_buffers() {
    jpt.input = (char*)malloc(maxBufferSize);
    jpt.token_str = (char*)malloc(maxBufferSize);
    jpt.canon_str = (char*)malloc(maxBufferSize);
    jpt.pos_str = (char*)malloc(maxBufferSize);
    if (jpt.input==NULL || jpt.token_str==NULL || jpt.canon_str==NULL || jpt.pos_str == NULL) {
        return -1;
    }
#ifndef DISCARD_JAPANESE
    tagger = MeCab::createTagger(""); 
#endif
    return 0;
}

void JapaneseNLP()
{
    const char* at = get_jp_canon_str();
    for (int i = 1; i <= wordCount; ++i)
    {
        originalLower[i] = StoreWord(wordStarts[i], AS_IS); // japanese is case insensitive

        const char* separator = (char*)strchr(at, ' ');
        if (separator != NULL) { // nominal path
            wordCanonical[i] = AllocateHeap(at, separator - at);
            canonicalLower[i] = StoreWord(wordCanonical[i], AS_IS);
            at = separator + 1;
        }
        else if (*at != 0) {    // got a token with no space at the end
            separator = at + strlen(at);
            wordCanonical[i] = AllocateHeap(at, separator - at);
            canonicalLower[i] = StoreWord(wordCanonical[i], AS_IS);
            at = separator;
        }
        else {                  // got an empty token (unexpected)
            wordCanonical[i] = AllocateHeap("");
            canonicalLower[i] = StoreWord(wordCanonical[i], AS_IS);
        }
    }
}

void MarkJapaneseTags(int i, char* testDictionary)
{
    char* at = (char*)get_jp_pos_str();
    int base = 0;
    while (++base <= i && *at)
    {
        char* end = strchr(at, ' '); // find end
        if (end) *end = 0;
        char pos[MAX_WORD_SIZE];
        *pos = '~';
        strcpy(pos + 1, at);
        if (end) *end = ' ';
        if (testDictionary == NULL) // nominal case
        {
            MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord(pos, AS_IS)), i, i);
        }
        else                    // test case
        {
            strcat(testDictionary, pos);
            strcat(testDictionary, " ");
        }
        at = strchr(at, ' '); // find next pos
        if (!at) return; // failed to find
        at += 1; // pointing to pos
    }
}

void free_jp_token_buffers() {
#ifndef DISCARD_JAPANESE
    delete tagger;
#endif

    free(jpt.input);
    jpt.input = NULL;

    free(jpt.token_str);
    jpt.token_str = NULL;

    free(jpt.canon_str);
    jpt.canon_str = NULL;

    free(jpt.pos_str);
    jpt.pos_str = NULL;
}

static void maybe_trace() {
        char* in = jpt.input;
        Log(USERLOG, "input: %s \r\n", in);
        
        char utfcharacter[20];
        int utfsize = 0;
        while (*in)
        {
            in = IsUTF8(in, utfcharacter);
            Log(USERLOG, "%s  -  ", utfcharacter);
            ++utfsize;
        }
        Log(USERLOG, "    %d kanji/alpha \r\n", utfsize);

        int count = 0;
        char* tokens = (char*) get_jp_token_str();
        char* lemmas = (char*)get_jp_canon_str();
        char token[MAX_WORD_SIZE];
        char lemma[MAX_WORD_SIZE];
        while (*tokens)
        {
            ++count;
            char* endtoken = strchr(tokens, ' ');
            char* endlemma = strchr(lemmas, ' ');
            if (!endtoken) strcpy(token, tokens);
            else
            {
                strncpy(token, tokens, endtoken - tokens);
                token[endtoken - tokens] = 0;
            }
            if (!endlemma) strcpy(lemma, lemmas);
            else
            {
                strncpy(lemma, lemmas, endlemma - lemmas);
                lemma[endlemma - lemmas] = 0;
            }
            Log(USERLOG, "#%d  %s - %s\r\n",count, token, lemma);
            if (!endtoken || !endlemma) break;
            tokens = endtoken + 1;
            lemmas = endlemma + 1;
        }
}

static void process_mecab_result(char* data, struct jp_tokens_t& jpt)
{
    char* at = data;
    int count = 0;
    char last_token[MAX_WORD_SIZE];

    // walk each node
    while (1)
    {
        char* tab = strchr(at, '\n'); // locate node end
        if (tab)
        {
            *tab = 0; // ensure comma search cannot leave this node
            if (!stricmp(at, "EOS")) break; // end of sentence
            bool merge = false;

            // process fields of current node
            for (int j = 0; j < 7; ++j)
            {
                char* comma = strchr(at+1, ','); // skip in case its token , 
                if (comma) *comma = 0;
                if (j == 0) // word and part of speech
                {
                    char* tab = strchr(at, '\t');
                    if (tab)
                    {
                        *tab = 0;
                        if (trace & TRACE_PREPARE) Log(USERLOG, "%d:  token: %s   pos: %s  ", ++count, at, tab + 1);

                        // if we have english . or ?, presume url and merge
                        if (*at == '.' || *at == '?')
                        {
                            size_t len = strlen(jpt.token_str);
                            jpt.token_str[len - 1] = 0; // remove trailing blank
                            jpt.pos_str[len - 1] = 0; // remove trailing blank
                            jpt.canon_str[len - 1] = 0; // remove trailing blank
                            strcat(jpt.token_str, at);
                            strcpy(last_token, at);
                            strcat(jpt.pos_str, tab + 1);
                            merge = true;
                        }
                        else
                        {
                            strcat(jpt.token_str, at);
                            strcat(jpt.token_str, " ");
                            strcpy(last_token, at);
                            strcat(last_token, " ");
                            strcat(jpt.pos_str, tab + 1);
                            strcat(jpt.pos_str, " ");
                        }
                    }
                    else printf("missing tab after processing string <%s>\r\n", data);
                }
                else if (j == 6) // lemma
                {
                    if (trace & TRACE_PREPARE) Log(USERLOG, "  lemma:  %s\r\n", at);
                    if (*at != '*')
                    {
                        strcat(jpt.canon_str, at);
                        if (!merge) strcat(jpt.canon_str, " ");
                    }
                    else strcat(jpt.canon_str, last_token);
                    break;
                }
                if (!comma) break;
                at = comma + 1; 
            }
            at = tab + 1;
        }
        else
        {
            ReportBug("Mecab missing eos");
            jpt.canon_str[0] = 0;
            jpt.token_str[0] = 0;
            jpt.pos_str[0] = 0;
            break;
        }
    }
}

int jp_tokenize(char* buffer) {
    jpt.error_flag = 0;
    strcpy(jpt.input, buffer);
    jpt.token_str[0] = '\0';
    jpt.canon_str[0] = '\0';
    jpt.pos_str[0] = '\0';
    jpt.error_flag = 0;

    // Gets tagged result in string format.
#ifndef DISCARD_JAPANESE
    const char *result = tagger->parse(jpt.input);
    if (trace & TRACE_PREPARE) maybe_trace();
    process_mecab_result((char*)result, jpt);
#endif

    return jpt.error_flag;
}

const char* get_jp_token_str() {
    return jpt.token_str;
}

const char* get_jp_canon_str() {
    return jpt.canon_str;
}

const char* get_jp_pos_str() {
    return jpt.pos_str;
}
