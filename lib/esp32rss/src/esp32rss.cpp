// This all started with code from https://github.com/varind/ESP8266-Web-To-LCD/blob/master/Web-To-LCD-2/Web-To-LCD-2.ino
// and uses code fragments from the rrsRead Library (author @chrmlinux03)
#include "debug.h"

#include "esp32rss.h"
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

parseRule::parseRule()
{
    for (int i = 0; i < MAX_ELEMENTS; i++)
    {
        cSFA(sfElement, elements[i]);
        sfElement = "";
    }
    skip = 0;
    initialize();
}

parseRule::parseRule(elementArray &e, unsigned int s)
{
    elements = e;
    skip = s;
    initialize();
}

void parseRule::initialize()
{
    currentElement = 0;
    for (int i = 0; i < MAX_RESULTS; i++)
    {
        cSFP(sfTmpResult, results[i]);
        sfTmpResult = "";
    }
    cSFPS(sfIntermediaryResult, intermediaryResult, RESULT_LENGTH);
    sfIntermediaryResult = "";
    currentResult = 0;
    return;
}
/*
 parseRule(const char * line, SafeString & result));
 * will recursively parse input line against the elements tree supplied
 * for the rule.
 * Maintains internal state between calls.
 */
bool parseRule::executeRule(const char *inPut, SafeString &sfResult)
{
    cSF(line, strlen(inPut));
    line = inPut;
    int idx = 0;
    // LOGDEBUG1("executeRule arg:", line);
    cSF(sfCurrentElement, ELEMENT_LENGTH + 2); // +2 to hold dtags
    cSF(sfNextElement, ELEMENT_LENGTH + 2);
    cSF(sfPreviousElement, ELEMENT_LENGTH + 2);

    sfCurrentElement = elements[currentElement];
    //    LOGDEBUG1("sfCurrentElement: ", sfCurrentElement.c_str());
    if (sfCurrentElement == "")
    {
        //      LOGINFO1("Scanning content of: ", elements[currentElement - 1]);
        if (currentElement > 0)
        { // we are now parsing the content of our final element.
            // There is a sfResultug in this code. If the final element has  nested elements of the same trype
            // their closing tags will not be ignored.
            sfPreviousElement = F("</");
            sfPreviousElement += elements[currentElement - 1];
            sfPreviousElement += F(">");

            idx = line.indexOf(sfPreviousElement);
            if (idx >= 0)
            {
                // we found a closing element on the same line, Everyghing in between is game

                currentElement--;
                cSFPS(sfIntermediaryResult, intermediaryResult, RESULT_LENGTH);
                cSF(sfTmp, line.length());

                line.substring(sfTmp, 0, idx);
                sfIntermediaryResult += sfIntermediaryResult;
                // Store the result internally.
                if (currentResult < MAX_RESULTS)
                {
                    //                    LOGDEBUG1("Result found on line: ", line.c_str());
                    //                    LOGDEBUG3("Result number: ", currentResult, " Added:", sfIntermediaryResult.c_str());
                    cSFPS(sfCurrentResult, results[currentResult], RESULT_LENGTH);
                    sfCurrentResult = intermediaryResult;
                    currentResult++;
                }
                else
                {
                    LOGWARN1("ResultBuffer full, ignoring:", intermediaryResult)
                }
                // Clear the result line
                // return whatever is not parsed
                line.substring(sfResult, idx + sfPreviousElement.length());
                return true;
            }
            else
            {
                // No closing tag on this line, adding the whole line to the result
                cSFPS(sfIntermediaryResult, intermediaryResult, RESULT_LENGTH);
                sfIntermediaryResult += line;
                sfResult = "";
                return false;
            }
        }
        else
        {
            // there is really nothing to look at, just return an empty string
            LOGWARN("Not Well understood error condition: This only happens when the elemnts array is empty.")
            LOGWARN(elements[0]);
            throw("Not well unerstood error condition");
        }
    }
    sfCurrentElement = F("<");
    sfCurrentElement += elements[currentElement];
    idx = line.indexOf(sfCurrentElement);
    if (idx >= 0)
    {
        //        LOGDEBUG5("Found at", idx, " :", sfCurrentElement, " in ", line.c_str());
        idx = line.indexOf(">"); // TBD: Potential problem if the closing bracket of a tag is on a new line
        cSFPS(sfIntermediaryResult, intermediaryResult, RESULT_LENGTH);
        sfIntermediaryResult = "";
        currentElement++;

        cSF(sfToBeParsed, line.length());
        line.substring(sfToBeParsed, idx + 1);
        executeRule(sfToBeParsed.c_str(), line);
        line.substring(sfResult, idx + 1);
        return true;
    }
    else
    {
        // We found nothing on this line. We can go on.
        sfResult = "";
        return false;
    }
}

int parseRule::cleanResults()
{
    int counter = 0;
    for (int i = 0; i < MAX_RESULTS; i++)
    {

        cSFP(sfResult, (char *)results.at(i));
        LOGDEBUG1("Cleaning: ", sfResult.c_str());
        if (sfResult != "")
        {
            counter++;
            sfResult.replace("<![CDATA[", "");
            sfResult.replace("]]>", "");
            sfResult.replace("'", "\"");
            sfResult.replace("`", "\"");
            sfResult.replace("’", "'");
            sfResult.replace("‘", "'");
            sfResult.replace("„", "\"");
            sfResult.replace("", "\"");
            sfResult.replace("”", "\"");
            sfResult.replace("é", "e");
            sfResult.replace("ë", "e");
            sfResult.replace("ö", "o");
            sfResult.replace("í", "i");
            sfResult.replace("&#8216;", "'");
            sfResult.replace("&#8217;", "'");
            sfResult.replace("&#x2019;", "\'"); // replace special characters
            sfResult.replace("&#39;", "\'");
            sfResult.replace("&apos;", "\'");
            sfResult.replace("’", "\'");
            sfResult.replace("–", "-");
            sfResult.replace("ï", "i");
            sfResult.replace("…", "...");
            sfResult.replace(";", "&");
            sfResult.replace("&amp;", "&");
            sfResult.replace("&quot;", "\"");
            sfResult.replace("&gt;", ">"); // We should do this after parsing of the XML
            sfResult.replace("&lt;", "<"); // We should do this after parsing of the XML
            while (sfResult.startsWith(" "))
            {
                sfResult.substring(sfResult, 1);
            }
            while (cleanFromHTMLtags(sfResult))
            {
            };
        }
    }
    return (counter);
}

bool parseRule::cleanFromHTMLtags(SafeString &in)
{

    if (in == "")
    {
        return false;
    }
    int p1, p2;
    p1 = in.indexOf("<");
    p2 = in.indexOf(">");
    if (p1 >= 0)
    {
        if (p2 >= 0 && p2 > p1)
        {
            cSF(sfTMP1, in.length());
            cSF(sfTMP2, in.length());

            in.substring(sfTMP1, 0, p1);
            in.substring(sfTMP2, p2 + 1);
            in = sfTMP1;
            in += sfTMP2;
            LOGDEBUG1("CleanFromHTMLtags:", in.c_str());
            return (true);
        }
    }
    return (false);
}

esp32RSS::esp32RSS(const char *url)
{

    cSFA(sfFeedUrl, feedUrl); // wrap in SafeStrings for processing
    sfFeedUrl = url;

    ruleCounter = 0;
}

int esp32RSS::getData()
{
    if (parseRules.empty())
    {
        LOGWARN("getData() has no parseRules to compare against");
        return -1;
    }
    static WiFiClientSecure *https;
    static WiFiClient *http;
    HTTPClient webClient;
    byte memPos = 0; // Memory offset
    LOGINFO1("Connecting to ", feedUrl);

    u_int8_t rtn = 0;
    uint16_t _port = 0;
    String host = url2host(String(feedUrl), &_port);

    if (_port == 80)
        connectOverTLS = false;
    clientConnected = false;
    if (connectOverTLS)
    {

        https = new WiFiClientSecure;
        https->setInsecure();
        if (webClient.begin(*https, feedUrl))
        {
            clientConnected = true;
        }
    }
    else
    {
        http = new WiFiClient;
        if (webClient.begin(*http, feedUrl))
        {
            clientConnected = true;
        }
    }
    if (!clientConnected)
    {
        LOGWARN("ClientConnection Failed");
        rtn = -1;
    }
    else
    {
        int httpCode = webClient.GET();
        if (httpCode > 0)
        {
            int len = webClient.getSize();
            // HTTP header has been send and Server response header has been handled
            LOGINFO3("[HTTPS] GET... code: ", httpCode, "Response length: ", len);
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                int i;
                while (i = ((connectOverTLS ? https : http)->available()))
                {

                    String line = (connectOverTLS ? https : http)->readStringUntil(0x0a);
                    cSFP(sfLine, (char *)line.c_str()); // wrap String's char[] in a SafeString

                    // LOGDEBUG1("sfLine:", sfLine.c_str());
                    while (parseRules[0].executeRule(line.c_str(), sfLine))
                    {
                        if (sfLine == "")
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                LOGINFO1("[HTTPS] GET... failed, error: ", webClient.errorToString(httpCode).c_str());
            }
            webClient.end();
            return (cleanResults());
        }
    }
    return rtn;
}

//=================================================
//
// cleanResults
//
//=================================================
int esp32RSS::cleanResults()
{
    LOGDEBUG("cleanResults() called");
    int counter = 0;
    for (auto &rule : parseRules)
    {
        counter += rule.cleanResults();
    }
    return counter;
}

//=================================================
//
// url2host
//
//=================================================
String esp32RSS::url2host(String src, uint16_t *port)
{
    const char * url443 = "https://";
    const char *  url80 = "http://";

    
    String dst = "";
    int st, en;

    if (src.indexOf(url443) >= 0)
    {
        st = src.indexOf(url443) + strlen(url443);
        *port = 443;
        Serial.println(st);
    }
    else if (src.indexOf(url80) >= 0)
    {

        st = src.indexOf(url80) + strlen(url80);
        *port = 80;
    }
    else
    {
        Serial.println("unknown URL");
        *port = 0;
        return "";
    }
    en = src.indexOf("/", st);
    dst = src.substring(st, en);
    return dst;
}

bool esp32RSS::addRuleSet(elementArray in)
{
    if (ruleCounter < MAX_PARSERULES)
    {
        parseRules[ruleCounter] = parseRule(in);
        ruleCounter++;
        return true;
    }
    else
    {
        LOGWARN("No Bufferspace to add Rule");
        return false;
    }
}

void esp32RSS::logRuleSets()
{
    for (const auto &rule : parseRules)
    {
        if (rule.elements[0] != "")
        {
            LOGINFO("RULESET -------");
            for (const auto element : rule.elements)
            {
                if (element != "")
                {
                    LOGINFO1("------ ", element);
                }
            }
        }
    }
}

void esp32RSS::rssContent(contentArray &content)
{

    for (int i = 0; i < MAX_PARSERULES; i++)
    {
        for (int j = 0; j < MAX_RESULTS; j++)
        {
            cSFPS(sfContent, content[i * MAX_RESULTS + j],RESULT_LENGTH);
            sfContent = parseRules[i].results[j];
        }
    }
    return;
}
