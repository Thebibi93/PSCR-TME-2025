// strutil.cpp
#include "strutil.h"
#include <cstddef>
#include <cstring>

namespace pr {

size_t length(const char* s) {
    size_t i = 0;
    while(s[i]){
        i++;
    }
    return i;
}

char* newcopy(const char* s) {
    size_t len = length(s);
    char* res = new char[len + 1];
    memcpy(res, s, len + 1); 
    return res;
}

int compare(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return (unsigned char)(a[i]) - (unsigned char)(b[i]);
        }
        i++;
    }
    return (unsigned char)(a[i]) - (unsigned char)(b[i]);
}

}
