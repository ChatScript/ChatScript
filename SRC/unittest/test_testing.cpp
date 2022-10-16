#include <string.h>
#include "../common.h"
#include "catch2.h"

TEST_CASE("ingestlog dse", "[ingestlog]") {
    IngestStruct is;
    int limit = 10000;
    mallocIngestStruct(is, limit);

    const char* response = "Respond: user:37224444 bot:Pearl ip:184.106.28.86 (~consumer_electronics_expert) 0 [reset type: FunnelQuestion category: consumer_electronics partner: 1 source: sip DeviceCategory: desktop]  ==> [assistanttitle=Technician's Assistant|assistantname=Pearl Wilson|why=~consumer_electronics_expert.491.0=GENERAL_START.~handle_oob.12.0=GIVEN_CATEGORY |v=33.0 ] Welcome! How can I help with your electronics question?  When:May02 18:24:25 8ms Why:~xpostprocess.5.0.~control.9.0 ~consumer_electronics_expert.491.0=GENERAL_START.~handle_oob.12.0=GIVEN_CATEGORY";
    strcpy(is.readbuf, response);
    bool result = parseIngestLogEntry(is);
    REQUIRE(result == true);

    REQUIRE(strcmp(is.user, "37224444") == 0);
    REQUIRE(strcmp(is.bot, "Pearl") == 0);
    REQUIRE(strcmp(is.ip, "184.106.28.86") == 0);
    REQUIRE(strcmp(is.topic, "~consumer_electronics_expert") == 0);
    REQUIRE(strcmp(is.turn, "0") == 0);
    REQUIRE(strcmp(is.lastCategory, "consumer_electronics") == 0);
    REQUIRE(strcmp(is.lastSpecialty, "") == 0);
    const char* command = "[reset type: FunnelQuestion category: consumer_electronics "
        "partner: 1 source: sip DeviceCategory: desktop]  ";
    REQUIRE(strcmp(is.buffer, command) == 0);
    const char* outputStart = "[assistanttitle=Technician's Assistant|assistantname=Pearl Wilson";
    REQUIRE(strstr(is.output, outputStart) != NULL);
    freeIngestStruct(is);
}

TEST_CASE("ingestlog direct_command", "[ingestlog]") {
    IngestStruct is;
    int limit = 10000;
    mallocIngestStruct(is, limit);

    const char* response = "Jun 04 07:13:05.019 2021 Respond: user:dt bot:test len:16 ip: (~test) 2 this is my life  ==> [why=~test.0.0.~control.11.0  ] Welcome  When:Jun 04 07:13:05.027 2021 8ms 0wait Why:~xpostprocess.1.0=OOBRESULT.~control.11.0 ~test.0.0.~control.11.0  JOpen:0/0";
    strcpy(is.readbuf, response);
    bool result = parseIngestLogEntry(is);
    REQUIRE(result == true);

    REQUIRE(strcmp(is.user, "dt") == 0);
    REQUIRE(strcmp(is.bot, "test") == 0);
    REQUIRE(strcmp(is.ip, "") == 0);
    REQUIRE(strcmp(is.topic, "~test") == 0);
    REQUIRE(strcmp(is.turn, "2") == 0);
    REQUIRE(strcmp(is.lastCategory, "") == 0);
    REQUIRE(strcmp(is.lastSpecialty, "") == 0);
    const char* command = "this is my life  ";
    REQUIRE(strcmp(is.buffer, command) == 0);
    const char* outputStart = "[why=~test.0.0.~control.11.0  ] Welcome";
    REQUIRE(strstr(is.output, outputStart) != NULL);

    freeIngestStruct(is);
}

TEST_CASE("ingestlog no input", "[ingestlog]") {
    IngestStruct is;
    int limit = 10000;
    mallocIngestStruct(is, limit);

    is.readbuf[0] = '\0';
    bool result0 = parseIngestLogEntry(is);
    REQUIRE(result0 == false);

    strcpy(is.readbuf, "this is a test");
    bool result1 = parseIngestLogEntry(is);
    REQUIRE(result1 == false);
    
    freeIngestStruct(is);
}

TEST_CASE("ingestlog with_label", "[ingestlog]") {
    IngestStruct is;
    int limit = 10000;
    mallocIngestStruct(is, limit);

    const char* response = "Jun 25 17:01:02.888 2022 Respond: user:306 bot:Pearl len:0 ip:192.168.53.87 (~introductions) 1 [{\"dse\": {\"testpattern\": {\"concepts\": [], \"input\": \"88 no made 55k last year married filing jointly.\", \"language\": \"English\", \"patterns\": [{\"label\": \"TAX_GENERAL_3A\", \"pattern\": \"^( ( !$filing_status !$income ) ) \"}, {\"label\": \"TAX_GENERAL_3B\", \"pattern\": \"^( ( !$filing_status ) ) \"}, {\"label\": \"TAX_GENERAL_3C\", \"pattern\": \"^( ( !$income ) ) \"}, {\"label\": \"TAX_GENERAL_3D\", \"pattern\": \"^( !$roo_coronavirus_2d :l$roo_coronavirus_2d:=1 ) \"}], \"style\": \"earliest\", \"variables\": {\"$an_shortexpertsingular\": \"an Accountant\", \"$assistantname\": \"Pearl Wilson\", \"$assistanttitle\": \"Accountant's Assistant\", \"$category\": \"tax\", \"$country\": \"Seattle\", \"$filing_status\": \"joint\", \"$income\": \"true\", \"$indentlevel\": \"3\", \"$roo_coronavirus_1a\": \"1\", \"$roo_coronavirus_2a\": \"1\", \"$shortexpertsingular\": \"Accountant\", \"$specialty\": \"tax\", \"$why\": \"TAX_GENERAL_2A\"}}}}] ==> [{\"why\": \"~handle_oob.2.0\", \"v\": 41.0, \"serverip\": 0, \"path\": \"|\", \"output\": {\"newglobals\": {\"$roo_coronavirus_2d\": \"1\"} , \"match\": 3} }  ]  When:Jun 25 17:01:02.913 2022 25ms 1qq 0 Why:~xpostprocess.7.0=OOBRESULT.~control.21.0=GENERAL_ML  JOpen:0/0 Timeout:0";
    strcpy(is.readbuf, response);
    bool result = parseIngestLogEntry(is);
    REQUIRE(result == true);

    REQUIRE(strcmp(is.label, "TAX_GENERAL_3A") == 0);

    freeIngestStruct(is);
}

TEST_CASE("cleanlog Hi.", "[ingestlog]") {
    const char* input = "\"output\": \"Hi. Can you help me? \"";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": \"Can you help me? \"";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog Hello.", "[ingestlog]") {
    const char* input = "\"output\": \"Hello. Can you help me? \"";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": \"Can you help me? \"";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog ruleshown", "[ingestlog]") {
    const char* input = "[{\"path\": \"|RBET_DELETE |TEST_RBET_SHOW \", "
        "\"output\": \"Rbet data {}  ip 0 pid 0\", "
        "\"data\": {\"ruleshown\": [\"TEST_RBET_SHOW \"] } }  ]";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": \"Rbet data {}  ip 0 pid 0\"}  ]";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog data", "[ingestlog]") {
    const char* input = "[{ \"path\": \"|RBET_DELETE |TEST_RBET_SHOW \", "
        "\"output\": \"Rbet data {}  ip 0 pid 0\", "
        "\"data\": {\"ruleshown\": [\"TEST_RBET_SHOW \"] } }  ]";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": \"Rbet data {}  ip 0 pid 0\"}  ]";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog bruce-cheat", "[ingestlog]") {
    const char* input = "\"output\": \"$bwtrace\": \"bruce-cheat\" bar";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\":  bar";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog bruce-cheat with comma", "[ingestlog]") {
    const char* input = "\"output\": \"$bwtrace\": \"bruce-cheat\", bar";
    char buffer[1000];
    strcpy(buffer, input);
    CleanLogEntry(buffer);
    char* expected = "\"output\": bar";
    REQUIRE(strcmp(buffer, expected) == 0);
}

TEST_CASE("cleanlog newglobals", "[ingestlog]") {
    const char* input = "\"output\": "
        "{\"newglobals\": {} , \"match\": 0}";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": {\"match\": 0}";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog newglobals with extra space", "[ingestlog]") {
    const char* input = "\"output\": "
        "{ \"newglobals\": {} , \"match\": 0}";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": {\"match\": 0}";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog output", "[ingestlog]") {
    const char* input = "\"output\": "
        "{\"newglobals\": {} , \"match\": 0}";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": {\"match\": 0}";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog output with space", "[ingestlog]") {
    const char* input = "\"output\": "
        "{ \"newglobals\": {} , \"match\": 0}";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": {\"match\": 0}";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog with space in front", "[ingestlog]") {
    const char* input = "     \"output\": "
        "{ \"newglobals\": {} , \"match\": 0}";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": {\"match\": 0}";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("cleanlog with space in back", "[ingestlog]") {
    const char* input = "     \"output\": "
        "{ \"newglobals\": {} , \"match\": 0}";
    char buffer[1000];
    strcpy(buffer, input);
    char* actual = CleanLogEntry(buffer);
    REQUIRE(actual != NULL);
    char* expected = "\"output\": {\"match\": 0}";
    REQUIRE(strcmp(actual, expected) == 0);
}

TEST_CASE("verify default char is unsigned", "[ingestlog]") {
    char read[10];
    read[1] = '\x80';
    REQUIRE(read[1] == 0x80);
}
