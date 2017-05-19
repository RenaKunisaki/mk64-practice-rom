#include <n64.h>
extern "C" {
static const char *hex = "0123456789ABCDEF";


char* strAppend(char *dest, const char *src) {
    //same as strcpy, but returns pointer to end of string.
    while(*dest++ = *src++);
    return dest-1;
}


char* printNumInBase(char *buf, u32 num, u32 base, int minDigits) {
    //print unsigned integer into buffer.
    //base can be up to 36; higher will work but use strange characters.
    //returns pointer to null terminator.
    if(base < 2) base = 10;
    char digits[512];
    char *d = &digits[511];
    *d-- = '\0';
    while(num > 0 || d == &digits[510] || minDigits > 0) {
        //print digits into buffer
        char c = num % base;
        if(c > 9) c = 'A' + (c-10);
        else c = '0' + c;
        *d-- = c;
        num /= base;
        minDigits--;
        if(d == &digits[0]) break;
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
    return printNumInBase(buf, num, 10, 1);
}

char* printNumPadded(char *buf, u32 num, int nDigits) {
    //print decimal number into buffer.
    //returns pointer to null terminator.
    return printNumInBase(buf, num, 10, nDigits);
}


static char* _printDouble(char *buf, double num, int nChars) {
    //internal method, used when num is known to be not inf or NaN
    double nMax = 1;
    for(int i=0; i<nChars - 4; i++) nMax *= 10.0d; //XXX find pow()
    nMax -= 1;

    if(num < 0.0d) num = -num;
    int e = 0;

    while(num < 1.0d && num > 0.0d && e > -100) {
        num *= 10.0d;
        e--;
    }

    while(num > nMax) {
        num /= 10.0d;
        e++;
    }

    buf = printNumPadded(buf, num, nChars - 3);
    *buf++ = 'e';
    *buf++ = (e >= 0) ? '+' : '-';
    if(e < 0) e = -e;
    return printNumPadded(buf, e, 3);
}


char* printFloat(char *buf, float num, int nChars) {
    //print float into buffer.
    //returns pointer to null terminator.
    u32 flt = *(u32*)&num;

    *buf++ = flt & 0x80000000 ? '-' : '+';
    nChars--; if(nChars < 1) return buf;

    u32 expn = (flt >> 23) & 0xFF;
    u32 frac =  flt & 0x7FFFFF;
    if(expn == 255) {
        if(frac != 0) buf = strAppend(buf, "NaN         ");
        else buf = strAppend(buf, "inf         ");
        //XXX why does this hang?
        //nChars -= 3;
        //while(nChars --> 0) *buf++ = ' '; //pad with spaces
        *buf = 0;
    }
    else {
        return _printDouble(buf, num, nChars);
    }
    return buf;
}


char* printDouble(char *buf, double num, int nChars) {
    //print float into buffer.
    //returns pointer to null terminator.
    u32 *b = (u32*)&num;
    u32 hi = b[0], lo=b[1];

    *buf++ = hi & 0x80000000 ? '-' : '+';
    nChars--; if(nChars < 1) return buf;

    u32 expn = (hi >> 20) & 0x7FF;
    u32 frac =  hi & 0xFFFFF;
    if(expn == 2047) {
        if(frac != 0 || lo != 0) buf = strAppend(buf, "NaN         ");
        else buf = strAppend(buf, "inf         ");
        //XXX why does this hang?
        //nChars -= 3;
        //while(nChars --> 0) *buf++ = ' '; //pad with spaces
        return buf;
    }
    else if(expn == 0 && frac != 0) {
        //XXX handle subnormal numbers
        return strAppend(buf, "------------");
    }
    else if(expn == 0 && frac == 0) {
        return _printDouble(buf, 0, nChars);
    }
    else {
        return _printDouble(buf, num, nChars);
    }
}

} //extern "C"
