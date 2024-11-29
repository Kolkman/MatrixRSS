#include "webInterface.h"

#include "debug.h"
#include "pgmspace.h"

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebserver.h>
#include <WiFi.h>



#include "pages/RssMatrix.css.h"
#include "webInterfaceOTAUpdate.h"


webInterface::webInterface() {
  LOGDEBUG0("Webinterfce Constructor");
  server= new AsyncWebServer(80);
}

webInterface::~webInterface() { LOGDEBUG0("Webinterfce Destructor"); }

void webInterface::setupWebSrv() {

  LOGINFO0("Setting up Webserver");

  // We set this for later. Wnen there are no credentials set we want to keep
  // the captive portal open - ad infinitum


  LOGDEBUG0("Resetting the Webserver");
  server->reset();
  // closing the API for unauthorized
 
  LOGDEBUG0("Starting the Webserver");
  server->begin(); /// Webserver is now running....


  server->onNotFound(
      std::bind(&webInterface::handleNotFound, this, std::placeholders::_1));
  server->on("/", HTTP_GET,
             std::bind(&webInterface::handleRoot, this, std::placeholders::_1));
  server->on(
      "/reset", HTTP_GET,
      std::bind(&webInterface::handleReset, this, std::placeholders::_1));
  server->on(
      "/index.html", HTTP_GET,
      std::bind(&webInterface::handleIndex, this, std::placeholders::_1));

  
  // respond to GET requests on URL /heap
  server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  InitPages(); // sets some default pages.

  
  webOTAUpdate.begin(server);

  server->begin();
  LOGINFO0("HTTP server started");
}

void webInterface::InitPages() {
  server->onNotFound(
      std::bind(&webInterface::handleNotFound, this, std::placeholders::_1));

  DEF_HANDLE_RssMatrix_css;
}

void webInterface::handleNotFound(AsyncWebServerRequest *request) {

  String message = htmlHeader;
  message += "<H1>Error 400 <br/> File Not Found</H1>\n\n";
  message += "<div id=\"notFoundInfo\"><div id=\"notFoundURI\">URI: <span "
             "id=\"notFoundURL\">";
  message += request->url();
  message += "</span></div>\n<div id=\"notFoundMethod\">Method: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "</div>\n<div id=\"notFoundArguments\">Arguments: ";
  message += request->args();
  message += "</div>\n";

  for (uint8_t i = 0; i < request->args(); i++) {
    message +=
        "<div class=\"notFoundArgument\"><span class=\"notFoundargName\">" +
        request->argName(i) + "</span>:<span class=\"notFoundarg\"> " +
        request->arg(i) + "</span></div>\n";
  }
  message += "</div>";
  message += htmlFooter;
  request->send(404, "text/html", message);
}

void webInterface::handleReset(AsyncWebServerRequest *request) {
  String message =
      "<head><meta http-equiv=\"refresh\" content=\"2;url=/\">\n<meta "
      "name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" "
      "/><title>RssMatrix</title></head>";
  message += "<h1> Reseting Device ! </h1>";
  request->send(200, "text/html", message);
  ESP.restart();
}

void webInterface::handleRoot(AsyncWebServerRequest *request) {
  request->redirect("/index.html");
}


void webInterface::handleFile(AsyncWebServerRequest *request,
                              const char *mimetype,
                              const unsigned char *compressedData,
                              const size_t compressedDataLen) {
  AsyncWebServerResponse *response =
      request->beginResponse(200, mimetype, compressedData, compressedDataLen);
  response->addHeader("Server", "ESP Async Web Server");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}


void webInterface::handleIndex(AsyncWebServerRequest *request) {

  
  String message = htmlHeader;
  message += "<H1>RssMatrix</H1>";

        message += "<div id=\"wecomeMsg\">Welcome to the RSSMatrix - not much to see here\n";
      
      message += "</div>\n";
   
  message +=
      "<div class=\"firmware\"> Firmware version: " + String(FIRMWAREVERSION) +
      " - " + String(F(__DATE__)) + ":" + String(F(__TIME__)) + "</div>";

  message += htmlFooter;
  request->send(200, "text/html", message);
}





// helper function for below checks whether a value is in a vectpr
using namespace std;
template <typename T>

bool contains(vector<T> vec, const T &elem) {
  bool result = false;
  if (find(vec.begin(), vec.end(), elem) != vec.end()) {
    result = true;
  }
  return result;
}


// Is this an IP?
bool webInterface::isIp(const String &str) {
  for (unsigned int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);

    if (c != '.' && c != ':' && (c < '0' || c > '9')) {
      return false;
    }
  }

  return true;
}
