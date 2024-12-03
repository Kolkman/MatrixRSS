#pragma once

#ifndef MatrixRSS_Debug_H
#define MatrixRSS_Debug_H

//  Code blatently copied from the ESPAsync_WifiManager_Debug.h source 
//   by Khoi Hoang https://github.com/khoih-prog/ESPAsync_WiFiManager

#ifdef MATRIXRSS_DEBUG_PORT
  #define DBG_PORT_ESP_EM      MATRIXRSS_DEBUG_PORT
#else
  #define DBG_PORT_ESP_EM      Serial
#endif

// Change _MATRIXRSS_LOGLEVEL_ to set tracing and logging verbosity
// 0: DISABLED: no logging
// 1: ERROR: errors
// 2: WARN: errors and warnings
// 3: INFO: errors, warnings and informational (default)
// 4: DEBUG: errors, warnings, informational and debug

#ifndef _MATRIXRSS_LOGLEVEL_
  #define _MATRIXRSS_LOGLEVEL_       2
#endif

/////////////////////////////////////////////////////////

const char ESP_EM_MARK[] = "[MatrixRSS] ";
const char ESP_EM_SP[]   = " ";

#define ESP_EM_PRINT        DBG_PORT_ESP_EM.print
#define ESP_EM_PRINTLN      DBG_PORT_ESP_EM.println

#define ESP_EM_PRINT_MARK   ESP_EM_PRINT(ESP_EM_MARK)
#define ESP_EM_PRINT_SP     ESP_EM_PRINT(ESP_EM_SP)

/////////////////////////////////////////////////////////

#define LOGERROR(x)        if(_MATRIXRSS_LOGLEVEL_>0) { ESP_EM_PRINT(x); }
#define LOGERROR0(x)         if(_MATRIXRSS_LOGLEVEL_>0) { ESP_EM_PRINT_MARK; ESP_EM_PRINTLN(x); }
#define LOGERROR1(x,y)      if(_MATRIXRSS_LOGLEVEL_>0) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(y); }
#define LOGERROR2(x,y,z)    if(_MATRIXRSS_LOGLEVEL_>0) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP;  ESP_EM_PRINTLN(z); }
#define LOGERROR3(x,y,z,w)  if(_MATRIXRSS_LOGLEVEL_>0) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP; ESP_EM_PRINT(z); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(w); }

/////////////////////////////////////////////////////////

#define LOGWARN(x)         if(_MATRIXRSS_LOGLEVEL_>1) { ESP_EM_PRINT(x); }
#define LOGWARN0(x)          if(_MATRIXRSS_LOGLEVEL_>1) { ESP_EM_PRINT_MARK; ESP_EM_PRINTLN(x); }
#define LOGWARN1(x,y)       if(_MATRIXRSS_LOGLEVEL_>1) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(y); }
#define LOGWARN2(x,y,z)     if(_MATRIXRSS_LOGLEVEL_>1) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP;  ESP_EM_PRINTLN(z); }
#define LOGWARN3(x,y,z,w)   if(_MATRIXRSS_LOGLEVEL_>1) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP; ESP_EM_PRINT(z); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(w); }

/////////////////////////////////////////////////////////

#define LOGINFO(x)         if(_MATRIXRSS_LOGLEVEL_>2) { ESP_EM_PRINT(x); }
#define LOGINFO0(x)          if(_MATRIXRSS_LOGLEVEL_>2) { ESP_EM_PRINT_MARK; ESP_EM_PRINTLN(x); }
#define LOGINFO1(x,y)       if(_MATRIXRSS_LOGLEVEL_>2) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(y); }
#define LOGINFO2(x,y,z)     if(_MATRIXRSS_LOGLEVEL_>2) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(z); }
#define LOGINFO3(x,y,z,w)   if(_MATRIXRSS_LOGLEVEL_>2) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP; ESP_EM_PRINT(z); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(w); }

/////////////////////////////////////////////////////////

#define LOGDEBUG(x)        if(_MATRIXRSS_LOGLEVEL_>3) { ESP_EM_PRINT(x); }
#define LOGDEBUG0(x)         if(_MATRIXRSS_LOGLEVEL_>3) { ESP_EM_PRINT_MARK; ESP_EM_PRINTLN(x); }
#define LOGDEBUG1(x,y)      if(_MATRIXRSS_LOGLEVEL_>3) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(y); }
#define LOGDEBUG2(x,y,z)    if(_MATRIXRSS_LOGLEVEL_>3) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(z); }
#define LOGDEBUG3(x,y,z,w)  if(_MATRIXRSS_LOGLEVEL_>3) { ESP_EM_PRINT_MARK; ESP_EM_PRINT(x); ESP_EM_PRINT_SP; ESP_EM_PRINT(y); ESP_EM_PRINT_SP; ESP_EM_PRINT(z); ESP_EM_PRINT_SP; ESP_EM_PRINTLN(w); }

/////////////////////////////////////////////////////////

#endif    
