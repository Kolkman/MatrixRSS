#include "contentcontainer.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

ContentContainer::ContentContainer() {
  filled = 0;
  current = 0;
  sprintf(content[0], "No News");
  for (int i = 0; i < MAX_ELEMENTS; i++) {
    sprintf(content[1], "");
  }
  return;
}

void ContentContainer::init() {
  filled = 0;
  current = 0;
  sprintf(content[0], "No News");
  for (int i = 0; i < MAX_ELEMENTS; i++) {
    sprintf(content[1], "");
  }
  return;
}

void ContentContainer::addcontent(char *addstring) {
  strncpy(content[filled], addstring, ELEMENT_LENGTH - 1);
  content[filled][ELEMENT_LENGTH - 1] = '\0';
  filled++;
  if (filled == MAX_ELEMENTS) {
    filled = 0;
  }
  return;
}

void ContentContainer::readcontent(char *input) {
  strncpy(input, content[current], ELEMENT_LENGTH - 1);
  input[ELEMENT_LENGTH - 1] = '\0';
  current++;
  if (current == MAX_ELEMENTS) {
    current = 0;
  }

  return;
}