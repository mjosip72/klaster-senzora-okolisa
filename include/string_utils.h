
#pragma once

#include <Arduino.h>

bool equals(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

bool starts_with(const char* a, const char* x) {
    return strncmp(a, x, strlen(x)) == 0;
}

const char* extract_value(const char* x) {
    const char* data = strchr(x, '=');
    if(data == NULL) return NULL;
    return data + 1;
}
