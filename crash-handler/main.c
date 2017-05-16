#include "../include/mk64/mk64.h"
extern "C" {

extern char* strAppend(char *dest, const char *src);
extern char* printHex(char *buf, u32 num, int nDigits);
extern char* printNum(char *buf, u32 num);
extern OSThread *currentThread;

#include "font.inc"


void crash_main_init() {
    //Called at boot once our code is loaded into RAM.
    if(sdrv_init()) {
        char text[512] __attribute__ ((aligned (16)));
        char *buf = text;

        buf = strAppend(buf, "Detected 64drive rev ");
        u32 rev = sdrv_getVariant();
        if(rev & 0xFF000000) *buf++ =  rev >> 24;
        if(rev & 0x00FF0000) *buf++ = (rev >> 16) & 0xFF;
        if(rev & 0x0000FF00) *buf++ = (rev >>  8) & 0xFF;
        if(rev & 0x000000FF) *buf++ =  rev        & 0xFF;

        buf = strAppend(buf, ", FW ");
        u32 ver = sdrv_getVersion() & 0xFFFF;
        buf = printHex(buf, (ver / 100), 1);
        *buf++ = '.';
        buf = printHex(buf, (ver % 100), 2);
        //buf = printHex(buf, ver, 8);

        buf = strAppend(buf, ", mem: ");
        buf = printNum (buf, sdrv_getRamSize() / (1024*1024));
        buf = strAppend(buf, "MB\r\n");
        sdrv_dprint(text);
    }
}


void* crash_titleHook(void *buf, int x1, int y1, int x2, int y2,
uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
    //(*(u32*)0xDEADBEEF) = 0xFFFFFFFF; //crash the game to test crash handler
}


void dmaThreadHook(u32 direction, u32 devAddr, void *vAddr, u32 nBytes) {
    //Called by the PI thread when a DMA is about to begin.

    //if(currentThread != (void*)0x80195270) return;

    //log this DMA to debug print
    char text[512] __attribute__ ((aligned (16)));
    char *buf = text;
    buf = strAppend(buf, "DMA ");
    buf = printHex (buf, devAddr, 6);
    *buf++ = ' ';
    *buf++ = direction ? '>' : '<';
    *buf++ = ' ';
    buf = printHex (buf, (u32)vAddr, 8);
    buf = strAppend(buf, " len ");
    buf = printHex (buf, nBytes, 8);
    buf = strAppend(buf, " T ");
    buf = printHex (buf, (u32)currentThread, 8);
    *buf++ = '\r';
    *buf++ = '\n';
    *buf++ = '\0';
    sdrv_dprint(text);
}

static const char *regName[] = { //only those in __OSThreadContext
          "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9",             "gp", "sp", "s8", "ra",
    "lo", "hi",
    NULL
};

static const char *causes[] = {
    "interrupt",
    "TLB modification exception",
    "TLB exception on load",
    "TLB exception on store",
    "address error on load",
    "address error on store",
    "bus error on fetch",
    "bus error on data",
    "syscall",
    "breakpoint",
    "reserved instruction",
    "coprocessor unusable",
    "arithmetic overflow",
    "trap exception",
    "error 14", //reserved
    "floating point exception",
    "error 16",
    "error 17",
    "error 18",
    "error 19",
    "error 20",
    "error 21",
    "error 22",
    "watchpoint",
    "error 24",
    "error 25",
    "error 26",
    "error 27",
    "error 28",
    "error 29",
    "error 30",
    "error 31",
};


void drawChar(u16 *framebuffer, u16 X, u16 Y, char c) {
    u32 fx = (c & 0xF) * 8;
    u32 fy = (c >>  4) * 8;

    for(int cy=0; cy<8; cy++) {
        for(int cx=0; cx<8; cx++) {
            u32 idx = ((fy+cy) * 128) + (fx+cx);
            u8 *src = (u8*)&ibmfont.pixel_data[idx / 8];
            u8 mask = *src & (0x80 >> cx);
            u16 col = mask ? 0xFFFF : 0x0001;
            framebuffer[((Y+cy) * 320) + X+cx] = col;
        }
    }
}


static u16 _print_X, _print_Y, _print_startX; //XXX make static vars work right

static void print_fbuf(u16 *framebuffer, const char *text) {
    for(int i=0; text[i]; i++) {
        char c = text[i];
        if(c == '\n') {
            _print_X = _print_startX;
            _print_Y += 8;
        }
        else {
            drawChar(framebuffer, _print_X, _print_Y, c);
            _print_X += 8;
        }
        if(_print_X >= 320 - 8) {
            _print_X = _print_startX;
            _print_Y += 8;
        }
        if(_print_Y >= 240 - 8) _print_Y = 0;
    }
}


void drawCrashScreen(u16 *framebuffer, OSThread *thread) {
    _print_X = 20;
    _print_Y = 20;
    _print_startX = _print_X;

    char text[2048];
    char *buf = text;
    buf = strAppend(buf, "FATAL ERROR - THREAD ");
    buf = printNum (buf, thread->id); *buf++ = '\n';
    buf = strAppend(buf, causes[(thread->context.cause >> 2) & 0x1F]);
    *buf++ = '\n';
    print_fbuf(framebuffer, text); buf = text;

    buf = strAppend(buf, "PC:");
    buf = printHex (buf, thread->context.pc, 8);
    buf = strAppend(buf, " SR:");
    buf = printHex (buf, thread->context.sr, 8);
    buf = strAppend(buf, " CR:");
    buf = printHex (buf, thread->context.cause, 8);
    buf = strAppend(buf, "\nVA:");
    buf = printHex (buf, thread->context.badvaddr, 8);
    buf = strAppend(buf, " RC:");
    buf = printHex (buf, thread->context.rcp, 8);
    if(thread->context.pc >= 0x80000000 && thread->context.pc <= 0x807FFFFC) {
        buf = strAppend(buf, " OP:");
        buf = printHex (buf, *(u32*)(thread->context.pc & ~3), 8);
    }
    buf = strAppend(buf, "\n");
    print_fbuf(framebuffer, text); buf = text;

    u32 *reg = (u32*)&thread->context.at;
    buf = text;
    for(int i=0; regName[i]; i++) {
        buf = strAppend(text, regName[i]); *buf++ = ':';
        reg++; //skip high word
        buf = printHex (buf, *reg++, 8);
        buf = strAppend(buf, ((i % 3) == 2) ? "\n" : " ");
        print_fbuf(framebuffer, text);
    }

    extern int mainThreadTask, mainThreadPrevTask, mainThreadSubTask;

    buf = text;
    buf = strAppend(buf, "HE:"); buf = printHex (buf, (u32)heapEndPtr, 8);
    buf = strAppend(buf, " HS:");
    buf = printHex (buf, (u32)&heapEnd - (u32)heapEndPtr, 8);
    buf = strAppend(buf, " MT:"); buf = printHex (buf, mainThreadTask, 8);
    buf = strAppend(buf, " PT:"); buf = printHex (buf, mainThreadPrevTask, 8);
    buf = strAppend(buf, " ST:"); buf = printHex (buf, mainThreadSubTask, 8);
    buf = strAppend(buf, "\nSM:");buf = printHex (buf, screenMode, 8);
    buf = strAppend(buf, " RT:"); buf = printHex (buf, raceType, 8);
    buf = strAppend(buf, " TN:"); buf = printHex (buf, curCourse, 8);
    buf = strAppend(buf, "\nNP:");buf = printHex (buf, numPlayers, 8);

    print_fbuf(framebuffer, text);
}

extern void osWriteBackDCacheAll();

void crashHook(u16 *framebuffer, OSThread *thread) {
    //called when a thread crashes.

    //memset(framebuffer, 0x8888, 320*240*2);
    drawCrashScreen(framebuffer, thread);
    osWriteBackDCacheAll();

    char text[512] __attribute__ ((aligned (16)));
    char *buf = text;
    buf = strAppend(buf, "\r\n *** Crash in thread ");
    buf = printNum (buf, thread->id);
    *buf++ = ' '; *buf++ = '(';
    buf = printHex (buf, (u32)thread, 8);

    buf = strAppend(buf, ") @ ");
    buf = printHex (buf, thread->context.pc, 8);
    buf = strAppend(buf, " ***\r\nstatus:   ");
    buf = printHex (buf, thread->context.sr, 8);
    buf = strAppend(buf, "\r\ncause:    ");
    buf = printHex (buf, thread->context.cause, 8); *buf++ = ' ';
    buf = strAppend(buf, causes[(thread->context.cause >> 2) & 0x1F]);

    buf = strAppend(buf, "\r\nbadvaddr: ");
    buf = printHex (buf, thread->context.badvaddr, 8);
    buf = strAppend(buf, "\r\nrcp:      ");
    buf = printHex (buf, thread->context.rcp, 8);
    buf = strAppend(buf, "\r\n");
    sdrv_dprint(text);

    u32 *reg = (u32*)&thread->context.at;
    for(int i=0; regName[i]; i++) {
        buf = text;
        buf = strAppend(buf, regName[i]);
        buf = strAppend(buf, ": ");
        buf = printHex (buf, *reg++, 8); *buf++ = ' ';
        buf = printHex (buf, *reg++, 8);
        if((i & 3) == 3) buf = strAppend(buf, "\r\n");
        else buf = strAppend(buf, "  ");
        sdrv_dprint(text);
    }

    sdrv_dprint("\r\nStack:\r\n");
    u32 *stack = (u32*)thread->context.sp;
    for(int i=0; i<256; i += 8) {
        buf = text;
        buf = printHex (buf, (u32)stack, 8);
        buf = strAppend(buf, ": ");
        for(int j=0; j<8; j++) {
            buf = printHex(buf, *stack++, 8);
            *buf++ = ' ';
            if(j == 3) *buf++ = ' ';
        }
        buf = strAppend(buf, "\r\n");
        sdrv_dprint(text);
    }

    sdrv_dprint("Code:\r\n");
    u32 *code = (u32*)thread->context.pc - 64;
    for(int i=0; i<256; i += 8) {
        buf = text;
        buf = printHex(buf, (u32)code, 8);
        *buf++ = ':';
        for(int j=0; j<8; j++) {
            *buf++ = ' ';
            if((u32)code == thread->context.pc) {
                buf = strAppend(buf, "\x1B[7m");
            }

            buf = printHex(buf, *code, 8);

            if((u32)code == thread->context.pc) {
                buf = strAppend(buf, "\x1B[0m");
            }
            //else *buf++ = ' ';

            if(j == 3) *buf++ = ' ';
            code++;
        }
        buf = strAppend(buf, "\r\n");
        sdrv_dprint(text);
    }
}

} //extern "C"
