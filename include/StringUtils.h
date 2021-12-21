
#pragma once

#include <Arduino.h>

namespace StringUtils {

bool equals(const char* a, const char* b) {
  return strcmp(a, b) == 0;
}

bool notEquals(const char* a, const char* b) {
  return strcmp(a, b) != 0;
}

bool startsWith(const char* a, const char* x) {
  return strncmp(a, x, strlen(x)) == 0;
}

const char* extractValue(const char* x) {
  const char* data = strchr(x, '=');
  if(data == NULL) return "0";
  return data + 1;
}

int toInt(const char* x) {
  return atoi(x);
}

float toFloat(const char* x) {
  return atof(x);
}

}
