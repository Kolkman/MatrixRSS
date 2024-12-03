#pragma once

#ifndef CONTENTCONTAINER_H
#define CONTENTCONTAINER_H

#include "Arduino.h"
#include <array>

#define MAX_ELEMENTS 16    // Maximum  number of elements in the Elements array
#define ELEMENT_LENGTH 256 // Maximum length of HTML tags

typedef std::array<char[ELEMENT_LENGTH], MAX_ELEMENTS> contentArray;

class ContentContainer {
public:
  ContentContainer();
  void init();
  void addcontent(char *);
  void readcontent(char *);
  unsigned int current;
  contentArray content;
  unsigned int filled;

private:

  void utf8AsciiEnhanced(char*);
};

#endif