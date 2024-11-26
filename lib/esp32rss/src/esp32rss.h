// This all started with Code from https://github.com/varind/ESP8266-Web-To-LCD/blob/master/Web-To-LCD-2/Web-To-LCD-2.ino
#pragma once

#ifndef ESP32RSS_H
#define ESP32RSS_H
#include <SafeString.h>
#include "Arduino.h"
#include <array>
#define MAX_ELEMENTS 5        // Maximum  number of elements in the Elements array
#define MAX_PARSERULES   1      // Maximum number of parserules
#define MAX_RESULTS 16         // Maximum number of results that will be stored
#define ELEMENT_LENGTH 32      // Maximum length of HTML tags
#define RESULT_LENGTH 256      // Maximum length of the RESULTING LINE
#define MAX_FEEDURL_LENGTH 256 // maximum length of the feed url char[]

// eat some memory:
typedef std::array<char[ELEMENT_LENGTH], MAX_ELEMENTS> elementArray;
typedef std::array<char[RESULT_LENGTH], MAX_RESULTS> resultArray;

class parseRule
{
public:
    parseRule(elementArray &, unsigned int s = 0);
    parseRule();
    bool executeRule(const char *, SafeString &);
    elementArray elements;
    int cleanResults();
    resultArray results;

private:
    void initialize();
    unsigned int skip;
    unsigned int currentElement;
    char intermediaryResult[RESULT_LENGTH];
    unsigned int currentResult;
    bool cleanFromHTMLtags(SafeString &);
};

typedef std::array<parseRule, MAX_PARSERULES> ruleArray;
typedef std::array<char[RESULT_LENGTH], MAX_RESULTS * MAX_PARSERULES> contentArray;

class esp32RSS
{
public:
    esp32RSS(const char *);
    char feedUrl[MAX_FEEDURL_LENGTH]; //  URL of RSS feed
                                      //    std::array<parseRule, MAX_PARSERULES> parseRules;
    ruleArray parseRules;

    bool addRuleSet(elementArray);
    void logRuleSets();
    void rssContent(contentArray &);
    int getData(void);

private:
    int cleanResults();
    bool clientConnected;
    bool connectOverTLS = true;
    String url2host(String, uint16_t *);
    unsigned int ruleCounter;
};

#endif // ESP32RSS_H