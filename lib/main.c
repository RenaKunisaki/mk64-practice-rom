#include <n64.h>
extern "C" {
static const char *hex = "0123456789ABCDEF";


char* strAppend(char *dest, const char *src) {
    //same as strcpy, but returns pointer to end of string.
    while(*dest++ = *src++);
    return dest-1;
}


char* printNumInBase(char *buf, u32 num, u32 base) {
    //print unsigned integer into buffer.
    //base can be up to 36; higher will work but use strange characters.
    //returns pointer to null terminator.
    if(base < 2) base = 10;
    char digits[34];
    char *d = &digits[33];
    *d-- = '\0';
    while(num > 0 || d == &digits[32]) { //print digits into buffer
        char c = num % base;
        if(c > 9) c = 'A' + (c-10);
        else c = '0' + c;
        *d-- = c;
        num /= base;
    }

    //copy digits to buf
    do {
        *buf = *++d;
    } while(*buf++);

    return --buf;
}


char* printHex(char *buf, u32 num, int nDigits) {
    //print hex number into buffer.
    //will zero-pad to specified number of digits.
    //will truncate numbers larger than specified length.
    //returns pointer to null terminator.
    char *bufEnd = &buf[nDigits];
    *bufEnd = 0;
    while(nDigits--) {
        buf[nDigits] = hex[num & 0xF];
        num >>= 4;
    }
    return bufEnd;
}


char* printNum(char *buf, u32 num) {
    //print decimal number into buffer.
    //returns pointer to null terminator.
    return printNumInBase(buf, num, 10);
}

} //extern "C"
