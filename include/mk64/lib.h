//Functions added by `lib` patch.
char* strAppend(char *dest, const char *src);
char* printNumInBase(char *buf, u32 num, u32 base, int minDigits);
char* printHex(char *buf, u32 num, int nDigits);
char* printNum(char *buf, u32 num);
char* printNumPadded(char *buf, u32 num, int nDigits);
char* printFloat(char *buf, float num, int nChars);
char* printDouble(char *buf, double num, int nChars);

//Functions added by `hooks` patch.
#define PATCH32(addr, val) (*(u32*)UNCACHED(addr) = (val))
#define PATCH16(addr, val) (*(u16*)UNCACHED(addr) = (val))
#define PATCH8(addr, val) (*(u8*)UNCACHED(addr) = (val))
#define PATCHJAL(addr, func) hookJal((u32)(addr), (void*)(func))
u32 hookJal(u32 addr, void *func);
