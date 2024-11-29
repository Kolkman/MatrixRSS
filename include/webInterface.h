#ifndef MATRIX_WEB_H
#define MATRIX_WEB_H
#include <ESPAsyncWebServer.h>
#include "MD_Parola.h"
#include "webinterface.h"
#include <AsyncTCP.h>
#include <WiFi.h>



class webInterface {
public:
  webInterface();
  ~webInterface();

  void setupWebSrv();
  void InitPages();

private:
  String _password;
  bool _authRequired = false;
  AsyncWebServer * server;
  // HTTPUpdateServer *httpUpdater;
  void handleNotFound(AsyncWebServerRequest *);
  void handleIndex(AsyncWebServerRequest *);
  void handleRoot(AsyncWebServerRequest *);
  void handleReset(AsyncWebServerRequest *);
  void handleFile(AsyncWebServerRequest *, const char *, const unsigned char *,
                  const size_t);

  bool isIp(const String &);
  void asyncpause(const unsigned long);
};

const char htmlHeader[] =
    "<!DOCTYPE HTML><html><head><meta name=\"viewport\" "
    "content=\"width=device-width,initial-scale=1.0\"/><link "
    "rel=\"stylesheet\" type=\"text/css\" href=\"RssMatrix.css\" "
    "media=\"all\"/>"
    "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">"
    "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>"
    "<link "
    "href=\"https://fonts.googleapis.com/"
    "css2?family=Sixtyfour+Convergence&display=swap\" rel=\"stylesheet\">"
    " <script src=\"helpers.js\"></script>"
    "</head> <body> ";
const char htmlFooter[] = "</body></html>";

struct EffectStruct_t {
  textEffect_t effect;
  String label;
};

struct PositionStruct_t {
  textPosition_t position;
  String label;
  String tooltip;
};

#endif
