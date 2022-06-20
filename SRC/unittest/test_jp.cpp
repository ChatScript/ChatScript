#include <string.h>
#include "../common.h"
#include "../tokenSystem.h"
#include "../cs_jp.h"
#include "catch2.h"

TEST_CASE("call get_jp_token_str_1 entry", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );
    
    strcpy(language, "japanese");
    char input[1024] = "太郎は次郎が持っている本を花子に渡した";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "太郎 は 次郎 が 持っ て いる 本 を 花 子 に 渡し た ";
    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "太郎 は 次郎 が 持つ て いる 本 を 花 子 に 渡す た ";
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str_2 entries", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "ピンチの時には必ずヒーローが現れる"
        "太郎は次郎が持っている本を花子に渡した";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] =
        "ピンチ の 時 に は 必ず ヒーロー が 現れる "
        "太郎 は 次郎 が 持っ て いる 本 を 花 子 に 渡し た ";

    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] =
        "ピンチ の 時 に は 必ず ヒーロー が 現れる "
        "太郎 は 次郎 が 持つ て いる 本 を 花 子 に 渡す た ";
    
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str english", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "he lives.";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "he lives.";

    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "he lives.";
    
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str url", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "www.go.com";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "www.go.com ";

    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "www.go.com ";
    
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str jp 1", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "現在チャットサポートは大変混み合っております";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "現在 チャット サポート は 大変 混み 合っ て おり ます ";
    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "現在 チャット サポート は 大変 混む 合う て おる ます ";
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    char exp_pos[] = "名詞 名詞 名詞 助詞 名詞 動詞 動詞 助詞 動詞 助動詞 ";
    const char* actual_pos = get_jp_pos_str();
    REQUIRE(strcmp(actual_pos, exp_pos) == 0);
    
    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str jp 2", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "自然言語処理を勉強してください";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "自然 言語 処理 を 勉強 し て ください ";
    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "自然 言語 処理 を 勉強 する て くださる ";
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    char exp_pos[] = "名詞 名詞 名詞 助詞 名詞 動詞 助詞 動詞 ";
    const char* actual_pos = get_jp_pos_str();
    REQUIRE(strcmp(actual_pos, exp_pos) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str jp 3", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "ピンチの時には必ずヒーローが現れる";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "ピンチ の 時 に は 必ず ヒーロー が 現れる ";
    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "ピンチ の 時 に は 必ず ヒーロー が 現れる ";
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    char exp_pos[] = "名詞 助詞 名詞 助詞 助詞 副詞 名詞 助詞 動詞 ";
    const char* actual_pos = get_jp_pos_str();
    REQUIRE(strcmp(actual_pos, exp_pos) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str jp 4", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "今日もしないとね";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "今日 も し ない と ね ";
    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "今日 も する ない と ね ";
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    char exp_pos[] = "名詞 助詞 動詞 助動詞 助詞 助詞 ";
    const char* actual_pos = get_jp_pos_str();
    REQUIRE(strcmp(actual_pos, exp_pos) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str jp 5", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "すもももももももものうち";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "すもも も もも も もも の うち ";
    char exp_can[] = "すもも も もも も もも の うち ";

    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    char exp_pos[] = "名詞 助詞 名詞 助詞 名詞 助詞 名詞 ";
    const char* actual_pos = get_jp_pos_str();
    REQUIRE(strcmp(actual_pos, exp_pos) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call get_jp_token_str jp 6", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    strcpy(language, "japanese");
    char input[1024] = "毎日本を読まされました";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char exp_tok[] = "毎 日本 を 読ま さ れ まし た ";
    const char* actual_token = get_jp_token_str();
    REQUIRE( strcmp(actual_token, exp_tok) == 0);
    
    char exp_can[] = "毎 日本 を 読む する れる ます た ";
    const char* actual_canon = get_jp_canon_str();
    REQUIRE(strcmp(actual_canon, exp_can) == 0);

    char exp_pos[] = "接頭詞 名詞 助詞 動詞 動詞 動詞 助動詞 助動詞 ";
    const char* actual_pos = get_jp_pos_str();
    REQUIRE(strcmp(actual_pos, exp_pos) == 0);

    free_jp_token_buffers();
}

TEST_CASE("call tokenize with multiple sentences", "[jp]") {
    maxHeapBytes = 10000000;    // 10 MB
    InitStackHeap();
    malloc_jp_token_buffers();

    strcpy(language, "japanese");
    char buffer[1024] = "私の妻は良いです。x ？www.go.com！he lives.";
    char* ptr = buffer;
    // char buffer[1024] = "太郎は次郎が持っている本を花子に渡した。";
    int count = 2;
    char* starts[MAX_SENTENCE_LENGTH];
    memset(starts, 0, sizeof(char*) * MAX_SENTENCE_LENGTH);
    char* separators = NULL;
    bool all1 = false;
    bool oobStart = false;
    int sentance_number = 0;
    char expected_strings[4][100] = {
        "私 の 妻 は 良い です ",
        "x ",
        "www.go.com ",
        "he lives."
    };
    while (ptr && *ptr) {
        REQUIRE( sentance_number < 4 );
        ptr = Tokenize(ptr,
                       count,
                       starts,
                       separators,
                       all1,
                       oobStart);
        const char* token = get_jp_token_str();
        const char* canon = get_jp_canon_str();
        const char* expected_str = expected_strings[sentance_number];
        REQUIRE( strcmp(expected_str, token) == 0 );
        REQUIRE( strcmp(expected_str, canon) == 0 );
        sentance_number++;
    }

    free_jp_token_buffers();
    FreeStackHeap();
}

TEST_CASE("JapaneseNLP", "[jp]") {
    maxHeapBytes = 10000000;    // 10 MB
    InitStackHeap();
    malloc_jp_token_buffers();

    strcpy(language, "japanese");
    char buffer[1024] = "私の妻は良いです。x ？www.go.com！he lives.";
    char* ptr = buffer;
    int count = 2;
    char* starts[MAX_SENTENCE_LENGTH];
    memset(starts, 0, sizeof(char*) * MAX_SENTENCE_LENGTH);
    char* separators = NULL;
    bool all1 = false;
    bool oobStart = false;
    int sentance_number = 0;
    char* expected_first[4] = { "私", "x", "www.go.com", "he" };
    char* expected_last[4] = { ".", "?", "!", "lives." };
    int expected_word_counts[4] = { 7, 2, 2, 2 };
    while (ptr && *ptr) {
        REQUIRE( sentance_number < 4 );
        wordCount = 0;
        ptr = Tokenize(ptr,
                       count,
                       starts,
                       separators,
                       all1,
                       oobStart);
        JapaneseNLP();
        REQUIRE(count == expected_word_counts[sentance_number]);
        REQUIRE(strcmp(expected_first[sentance_number], starts[1]) == 0);
        REQUIRE(strcmp(expected_last[sentance_number], starts[count]) == 0);
        sentance_number++;
    }

    free_jp_token_buffers();
    FreeStackHeap();
}

TEST_CASE("call get_jp_token_str_1 english with commas", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );
    
    strcpy(language, "japanese");
    char input[1024] = "Hello, I'm, the Expert's "
        "Assistant on JustAnswer. How can I help?";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    char expected_token[] = "Hello , I ' m , the Expert ' s "
        "Assistant on JustAnswer.How can I help?";
    const char* actual_token = get_jp_token_str();
    REQUIRE(strcmp(expected_token, actual_token) == 0);

    free_jp_token_buffers();
}

TEST_CASE("find_closest_jp_period 1", "[jp]") {
    char* input = "私の妻は良いです。x";
    char* result = find_closest_jp_period(input);
    char* expected = "。x";
    REQUIRE(strcmp(expected, result) == 0);
}

TEST_CASE("find_closest_jp_period 2", "[jp]") {
    char* input = "私の妻は良いです。xす。x";
    char* result = find_closest_jp_period(input);
    char* expected = "。xす。x";
    REQUIRE(strcmp(expected, result) == 0);
}

TEST_CASE("find_closest_jp_period 3", "[jp]") {
    char* input = "私．の妻は良いです。xす。x";
    char* result = find_closest_jp_period(input);
    char* expected = "．の妻は良いです。xす。x";
    REQUIRE(strcmp(expected, result) == 0);
}

TEST_CASE("find_closest_jp_period 4", "[jp]") {
    char* input = "私の妻は良いですxすx";
    char* result = find_closest_jp_period(input);
    char* expected = NULL;
    REQUIRE(expected == result);
}

TEST_CASE("check parts of speech, single token", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    char input[1024] = "太郎";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    JapaneseNLP();
    char testDictionary[MAX_BUFFER_SIZE] = { '\0' };
    MarkJapaneseTags(1, testDictionary);
    char* expected_pos = "~名詞 ";
    REQUIRE(strcmp(expected_pos, testDictionary) == 0);

    free_jp_token_buffers();
}

TEST_CASE("check parts of speech, partial list", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    char input[1024] = "太郎は次郎が持っている本を花子に渡した";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    JapaneseNLP();
    char testDictionary[MAX_BUFFER_SIZE] = { '\0' };
    MarkJapaneseTags(2, testDictionary);
    char* expected_pos = "~名詞 ~助詞 ";
    REQUIRE(strcmp(expected_pos, testDictionary) == 0);

    free_jp_token_buffers();
}

TEST_CASE("check parts of speech, full list", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    char input[1024] = "太郎は";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    JapaneseNLP();
    char testDictionary[MAX_BUFFER_SIZE] = { '\0' };
    MarkJapaneseTags(2, testDictionary);
    char* expected_pos = "~名詞 ~助詞 ";
    REQUIRE(strcmp(expected_pos, testDictionary) == 0);

    free_jp_token_buffers();
}

TEST_CASE("check parts of speech, bad index", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    char input[1024] = "太郎";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    JapaneseNLP();
    char testDictionary[MAX_BUFFER_SIZE] = { '\0' };
    MarkJapaneseTags(2, testDictionary);
    char* expected_pos = "~名詞 ";
    REQUIRE(strcmp(expected_pos, testDictionary) == 0);

    free_jp_token_buffers();
}

TEST_CASE("check parts of speech, english", "[jp]") {
    int rv;
    rv = malloc_jp_token_buffers();
    REQUIRE( rv == 0 );

    char input[1024] = "help me.";
    rv = jp_tokenize(input);
    REQUIRE( rv == 0 );

    JapaneseNLP();
    char testDictionary[MAX_BUFFER_SIZE] = { '\0' };
    MarkJapaneseTags(3, testDictionary);
    char* expected_pos = "~名詞 ~名詞 ";
    REQUIRE(strcmp(expected_pos, testDictionary) == 0);

    free_jp_token_buffers();
}
TEST_CASE("check ConvertJapanText patterns", "[jp]") {
    char input[1024] = "abcＢＹｂｙ０９。．－＿。｡（）｛｝［］def";
    ConvertJapanText(input, true);
    char expected[] = "abcＢＹｂｙ０９..-_..(){}()def";
    REQUIRE(strcmp(expected, input) == 0);
}

TEST_CASE("check ConvertJapanText output in variables", "[jp]") {
    char input[1024] = "abc$ＢＹｂｙ０９。．－＿。｡（）｛｝［］def";
    ConvertJapanText(input, false);
    char expected[] = "abc$BYby09..-_..(){}()def";
    REQUIRE(strcmp(expected, input) == 0);
}

TEST_CASE("check ConvertJapanText output in variables 2", "[jp]") {
    char input[1024] = "$ＢＹ－ＢＹ　ＢＹ－ＢＹ";
    ConvertJapanText(input, false);
    char expected[] = "$BY-BY BY-BY";
}
