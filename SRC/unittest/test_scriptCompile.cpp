#include <cassert>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"

#include "catch2.h"

#include <stddef.h>

#ifdef __GNUC__ // ignore warnings from common.h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wconversion-null"
#include "../common.h"
#pragma GCC diagnostic pop

#else  // not __GNUC__ means Visual Studio
#include "../common.h"
#endif // __GNUC__

#include "../scriptCompile.h"

// #define SKIP_TEST = 1

extern char* readBuffer;

#ifndef SKIP_TEST
TEST_CASE("compile_pattern", "[scriptCompile]")
{
    char ptr[] = "#! Output comment !#\\nHello!";
    char word[512];
    char emptyBuffer[512];
    readBuffer = emptyBuffer;
    word[0] = '\0';
    csapicall = COMPILE_PATTERN;
    
    char* rv = ReadSystemToken(ptr, word);
    char* expected = "Hello!";
    REQUIRE(strcmp(expected, word) == 0);
}
#endif // SKIP_TEST

#ifndef SKIP_TEST
TEST_CASE("compile_output", "[scriptCompile]")
{
    char ptr[] = "#! Output comment !#\\nHello!";
    char word[512];
    char emptyBuffer[512];
    readBuffer = emptyBuffer;
    word[0] = '\0';
    csapicall = COMPILE_OUTPUT;
    
    char* rv = ReadSystemToken(ptr, word);
    char* expected = "Hello!";
    REQUIRE(strcmp(expected, word) == 0);
}
#endif // SKIP_TEST


