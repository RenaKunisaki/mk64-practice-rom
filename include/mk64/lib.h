//Functions added by `lib` patch.

char* strAppend(char *dest, const char *src);
char* printNumInBase(char *buf, u32 num, u32 base, int minDigits);
char* printHex(char *buf, u32 num, int nDigits);
char* printNum(char *buf, u32 num);
char* printNumPadded(char *buf, u32 num, int nDigits);
char* printFloat(char *buf, float num, int nChars);
char* printDouble(char *buf, double num, int nChars);
