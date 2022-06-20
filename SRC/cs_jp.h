#ifndef CS_JP_H
#define CS_JP_H

struct jp_tokens_t {
    int error_flag;
    char* input;
    char* token_str;
    char* canon_str;
    char* pos_str;
};

int malloc_jp_token_buffers();
void free_jp_token_buffers();
int jp_tokenize(char* buffer);
const char* get_jp_token_str();
const char* get_jp_canon_str();
const char* get_jp_pos_str();
// exposed for testing
void JapaneseNLP();
void MarkJapaneseTags(int i, char* testDictionary = NULL);

#endif // CS_JP_H
