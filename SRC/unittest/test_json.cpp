#include <string.h>
#include "../common.h"
#include "../factSystem.h"
#include "../json.h"
#include "../os.h"
#include "catch2.h"

static void trim_newlines(char* in_buf, char* out_buf) {
    int o_i = 0;
    int i_i = 0;
    while (in_buf[i_i]) {
        if (in_buf[i_i] != '\r' && in_buf[i_i] != '\n') {
            out_buf[o_i++] = in_buf[i_i];
        }
        i_i++;
    }
    out_buf[o_i] = '\0';
}

TEST_CASE("json create array", "[json]") {
    InitFacts();
    InitStackHeap();
    InitDictionary();
    InitJSONNames();

    char* jmessage = " [{\"dataid\": \"2021-08-23\"}, {\"groovy\": \"shoes\"}] ";
    char buffer[BUFSIZ];
    buffer[0] = '\0';
    const bool nofail = false;
    FunctionResult rv = ParseJson(buffer, jmessage, strlen(jmessage) + 1, nofail);
    REQUIRE( rv == NOPROBLEM_BIT);

    char* top_name = "ja-t+0";
    WORDP ja_t0_word = FindWord(top_name);
    FACT* fact_ja_t0_1 = GetSubjectNondeadHead(ja_t0_word);
    REQUIRE( fact_ja_t0_1->subject == 1);
    char* ja_t0_1_subject_word = Meaning2Word(fact_ja_t0_1->subject)->word;
    REQUIRE( strcmp( ja_t0_1_subject_word, "ja-t+0" ) == 0 );

    MEANING ja_t0_1_verb = fact_ja_t0_1->verb;
    char* ja_t0_1_verb_word = Meaning2Word(ja_t0_1_verb)->word;
    REQUIRE( strcmp( ja_t0_1_verb_word, "1" ) == 0 );

    MEANING ja_t0_1_object = fact_ja_t0_1->object;
    char* ja_t0_1_object_word = Meaning2Word(ja_t0_1_object)->word;
    REQUIRE( strcmp( ja_t0_1_object_word, "jo-t+1001" ) == 0 );

    FACT* fact_ja_t0_0 = GetSubjectNondeadNext(fact_ja_t0_1);
    MEANING ja_t0_0_subject = fact_ja_t0_0->subject;
    char* ja_t0_0_subject_word = Meaning2Word(ja_t0_0_subject)->word;
    REQUIRE( strcmp( ja_t0_0_subject_word, "ja-t+0" ) == 0 );

    MEANING ja_t0_0_verb = fact_ja_t0_0->verb;
    char* ja_t0_0_verb_word = Meaning2Word(ja_t0_0_verb)->word;
    REQUIRE( strcmp( ja_t0_0_verb_word, "0" ) == 0 );
    
    MEANING ja_t0_0_object = fact_ja_t0_0->object;
    char* ja_t0_0_object_word = Meaning2Word(ja_t0_0_object)->word;
    REQUIRE( strcmp( ja_t0_0_object_word, "jo-t+1000" ) == 0 );

    REQUIRE( fact_ja_t0_0->subjectNext == 0);

    char arg_array[5][132];
    for (int i=0; i<5; ++i) {
        callArgumentList[i] = arg_array[i];
    }
    callArgumentBase = 0;
    currentOutputBase = buffer;
    strcpy(ARGUMENT(1), "ja-t+0");
    strcpy(ARGUMENT(2), "0");
    strcpy(ARGUMENT(3), "");
    rv = JSONTreeCode(buffer);
    REQUIRE( rv == NOPROBLEM_BIT );
    char expected[] = "ja-t+0["
        "  {    # jo-t+1000"
        "    \"dataid\": \"2021-08-23\""
        "  },"
        "  {    # jo-t+1001"
        "    \"groovy\": \"shoes\""
        "  }"
        " ]# ja-t+0"
        "<=JSON ";

    char trim_buffer[BUFSIZ];
    trim_newlines(buffer, trim_buffer);
    REQUIRE( strcmp(trim_buffer, expected) == 0);

    FreeDictionary();
    FreeStackHeap();
    CloseFacts();
}

TEST_CASE("json merge arrays", "[json]") {
    InitFacts();
    InitStackHeap();
    InitDictionary();
    InitJSONNames();

    const bool nofail = false;
    char buffer[BUFSIZ];
    FunctionResult rv;    

    char* jmessage_a = " [{\"dataid\": \"2021-08-23\"}, {\"groovy\": \"shoes\"}] ";
    buffer[0] = '\0';
    rv = ParseJson(buffer, jmessage_a, strlen(jmessage_a) + 1, nofail);
    REQUIRE( rv == NOPROBLEM_BIT);

    char* jmessage_b = "["
        "{\"dataid\": \"2022-08-10\", \"fubar\": {\"foo\": \"bar\"} }, "
        "{\"dataid\": \"2022-08-06\"} "
        "]";
    buffer[0] = '\0';
    rv = ParseJson(buffer, jmessage_b, strlen(jmessage_b) + 1, nofail);
    REQUIRE( rv == NOPROBLEM_BIT);

    char arg_array[5][132];
    for (int i=0; i<5; ++i) {
        callArgumentList[i] = arg_array[i];
    }
    callArgumentBase = 0;
    currentOutputBase = buffer;
    strcpy(ARGUMENT(1), "key");
    strcpy(ARGUMENT(2), "ja-t+0");
    strcpy(ARGUMENT(3), "ja-t+1002");

    buffer[0] = '\0';
    rv = JSONMergeCode(buffer);
    REQUIRE( rv == NOPROBLEM_BIT);
    char merged_name[10];
    strcpy(merged_name, buffer);

    buffer[0] = '\0';
    callArgumentBase = 0;
    currentOutputBase = buffer;
    strcpy(ARGUMENT(1), merged_name);
    strcpy(ARGUMENT(2), "0");
    strcpy(ARGUMENT(3), "");
    rv = JSONTreeCode(buffer);
    REQUIRE( rv == NOPROBLEM_BIT );

    char expected[] = "["
        "  {    # jo-t+2006"
        "    \"dataid\": \"2021-08-23\""
        "  },"
        "  {    # jo-t+2007"
        "    \"groovy\": \"shoes\""
        "  }"
        " ]# ja-t+2005"
        "<=JSON ";

    char trim_buffer[BUFSIZ];
    trim_newlines(buffer, trim_buffer);
    REQUIRE( strcmp(trim_buffer, expected) == 0 );

    FreeDictionary();
    FreeStackHeap();
    CloseFacts();
}

