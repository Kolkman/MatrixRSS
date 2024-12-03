#include "contentcontainer.h"
#include "debug.h"
#include <bits/stdc++.h>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <vector>

ContentContainer::ContentContainer() { return; }

void ContentContainer::init() {
  filled = 0;
  current = 0;

  for (int i = 0; i < MAX_ELEMENTS; i++) {
    sprintf(content[i], "");
  }
  sprintf(content[0], "Nog geen nieuw nieuws geladen");
  return;
}

void ContentContainer::addcontent(char *addstring) {
  // Instring conversion will only shorten the string.
  utf8AsciiEnhanced(addstring);
  // utf8Ascii(addstring);

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
}

struct utf8 {
  char mask;      /* char data will be bitwise ANDed with this binary number */
  char start;     /* start index of the bytes of current char in a utf-8 encoded
                     character */
  uint32_t begin; /* beginning of codepoint range */
  uint32_t end;   /* end of codepoint range */
  uint32_t bits_stored; /* the number of bits from the codepoint that fit in the
                           char */
};

const std::vector<utf8> utf8s = {
    /*    mask                          start        begin    end       bits */
    utf8{0b00111111, static_cast<char>(0b10000000), 0, 0, 6},
    utf8{0b01111111, static_cast<char>(0b00000000), 0000, 0177, 7},
    utf8{0b00011111, static_cast<char>(0b11000000), 0200, 03777, 5},
    utf8{0b00001111, static_cast<char>(0b11100000), 04000, 0177777, 4},
    utf8{0b00000111, static_cast<char>(0b11110000), 0200000, 04177777, 3}};

uint32_t codepoint_size(const uint32_t &codepoint) {
  uint32_t size = 0;
  for (const utf8 &utf : utf8s) {
    if ((codepoint >= utf.begin) && (codepoint <= utf.end)) {
      break;
    }
    size++;
  }
  return size;
}

uint32_t utf8_size(const char &ch) {
  uint32_t size = 0;
  for (const utf8 &utf : utf8s) {
    if ((ch & ~utf.mask) == utf.start) {
      break;
    }
    size++;
  }
  return size;
}

std::vector<char> to_utf8(const uint32_t &codepoint) {
  const uint32_t byte_count = codepoint_size(codepoint);
  std::vector<char> result{};

  uint32_t shift = utf8s[0].bits_stored * (byte_count - 1);
  result.emplace_back((codepoint >> shift & utf8s[byte_count].mask) |
                      utf8s[byte_count].start);
  shift -= utf8s[0].bits_stored;
  for (uint32_t i = 1; i < byte_count; ++i) {
    result.emplace_back((codepoint >> shift & utf8s[0].mask) | utf8s[0].start);
    shift -= utf8s[0].bits_stored;
  }
  return result;
}

uint32_t to_codepoint(const std::vector<char> &chars) {
  const uint32_t byte_count = utf8_size(chars[0]);
  uint32_t shift = utf8s[0].bits_stored * (byte_count - 1);
  uint32_t codepoint = (chars[0] & utf8s[byte_count].mask) << shift;

  for (uint32_t index = 1; index < byte_count; ++index) {
    shift -= utf8s[0].bits_stored;
    codepoint |= (chars[index] & utf8s[0].mask) << shift;
  }
  return codepoint;
}

// Code from https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1

typedef uint32_t u8chr_t;
static uint8_t const u8_length[] = {
    // 0 1 2 3 4 5 6 7 8 9 A B C D E F
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 3, 4};

#define u8length(s) u8_length[(((uint8_t *)(s))[0] & 0xFF) >> 4];

int u8chrisvalid(u8chr_t c) {
  if (c <= 0x7F)
    return 1; // [1]

  if (0xC280 <= c && c <= 0xDFBF) // [2]
    return ((c & 0xE0C0) == 0xC080);

  if (0xEDA080 <= c && c <= 0xEDBFBF) // [3]
    return 0;                         // Reject UTF-16 surrogates

  if (0xE0A080 <= c && c <= 0xEFBFBF) // [4]
    return ((c & 0xF0C0C0) == 0xE08080);

  if (0xF0908080 <= c && c <= 0xF48FBFBF) // [5]
    return ((c & 0xF8C0C0C0) == 0xF0808080);

  return 0;
}

int u8next(char *txt, u8chr_t *ch) {
  int len;
  u8chr_t encoding = 0;

  len = u8length(txt);

  for (int i = 0; i < len && txt[i] != '\0'; i++) {
    encoding = (encoding << 8) | txt[i];
  }

  errno = 0;
  if (len == 0 || !u8chrisvalid(encoding)) {
    encoding = txt[0];
    len = 1;
    errno = EINVAL;
  }

  if (ch)
    *ch = encoding;

  return encoding ? len : 0;
}

// from UTF-8 encoding to Unicode Codepoint
uint32_t u8decode(u8chr_t c) {
  uint32_t mask;

  if (c > 0x7F) {
    mask = (c <= 0x00EFBFBF) ? 0x000F0000 : 0x003F0000;
    c = ((c & 0x07000000) >> 6) | ((c & mask) >> 4) | ((c & 0x00003F00) >> 2) |
        (c & 0x0000003F);
  }

  return c;
}

void ContentContainer::utf8AsciiEnhanced(char *s) {

  LOGWARN1("Parsing:", s);
  // In place conversion UTF-8 string to Extended ASCII
  // The extended ASCII string is always shorter.
  // uint8_t c;
  // char utf8[4]; // RFC3629 limits to 4 bytes (only UTF-16)
  uint i = 0; // follows the unicode
  int j = 0;  // follows the asci
  uint s_len = strlen(s);

  char logmsg[255];
  while (i < s_len) {
    uint32_t decoded;
    u8chr_t encoding;

    uint u8len = u8next(s + i, &encoding);
    LOGINFO(u8len);
    decoded = u8decode(encoding);
    sprintf(logmsg, "  0x%x -> ", decoded);
    LOGINFO(logmsg);
    i += u8len;

    if (decoded < 0x7f) {
      s[j] = decoded;
      j++;
    } else if (decoded == 0xa0) {
      // NO-BREAK SPACE
      s[j] = ' ';
      j++;

    } else if (u8len == 2 && (encoding >> 8) == 0xC2) {
      // see
      // https://github.com/MajicDesigns/MD_Parola/blob/main/examples/Parola_UFT-8_Display/Parola_UFT-8_Display.ino
      s[j] = (0x00ff & encoding);
      j++;
    } else if (u8len == 2 && (encoding >> 8) == 0xC3) {

      // see
      // https://github.com/MajicDesigns/MD_Parola/blob/main/examples/Parola_UFT-8_Display/Parola_UFT-8_Display.ino
      s[j] = (0x00ff & encoding) | 0x00C0;
      j++;
    } else if (decoded == 0x20AC) {
      s[j] = 0x80; // Euro symbol special case
      j++;
    } else if (decoded == 0xE280) {
      s[j] = 133; // ellipsis special case (turn to dash)
      j++;
    } else if (decoded == 0x2019 || decoded == 0x2018) {
      // RIGHT SINGLE QUOTATION MARK or LEFT SINGLE QUOTATION MARK
      s[j] = '\'';
      j++;
    } else if (0x200f < decoded && decoded < 0x2016) {
      // hyphens
      s[j] = '-';
      j++;
    }

    else {

      sprintf(logmsg, "  0x%x (%d) NOT HANDLED \n", decoded ,u8len);
      LOGWARN(logmsg);
    }
    sprintf(logmsg, "  0x%x \n", s[j]);
    LOGINFO(logmsg);
  }
  s[j] = '\0';
}
