#include "../include/mk64/mk64.h"
extern "C" {

extern char* strAppend(char *dest, const char *src);
extern char* printHex(char *buf, u32 num, int nDigits);
extern char* printNum(char *buf, u32 num);

#include "font.inc"

static u16 *framebuffer;
static OSThread *faultThread;


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


//XXX move this
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


void drawChar(u16 X, u16 Y, char c) {
    u32 fx = (c & 0xF) * 8;
    u32 fy = (c >>  4) * 8;

    for(int cy=0; cy<8; cy++) {
        for(int cx=0; cx<8; cx++) {
            u32 idx = ((fy+cy) * 128) + (fx+cx);
            u8 *src = (u8*)&ibmfont.pixel_data[idx / 8];
            u8 mask = *src & (0x80 >> cx);
            u16 col = mask ? 0xFFFF : 0x0021;
            framebuffer[((Y+cy) * 320) + X+cx] = col;
        }
    }
}


static u16 _print_X, _print_Y, _print_startX; //XXX make static vars work right

static void print_fbuf(const char *text) {
    for(int i=0; text[i]; i++) {
        char c = text[i];
        if(c == '\n') {
            _print_X = _print_startX;
            _print_Y += 8;
        }
        else {
            drawChar(_print_X, _print_Y, c);
            _print_X += 8;
        }
        if(_print_X >= 320 - 8) {
            _print_X = _print_startX;
            _print_Y += 8;
        }
        if(_print_Y >= 240 - 8) _print_Y = 0;
    }
}


static void drawCrashScreen() {
    OSThread *thread = faultThread;

    _print_X = 20;
    _print_Y = 20;
    _print_startX = _print_X;

    char text[2048];
    char *buf = text;

    //print crash info
    buf = strAppend(buf, "FATAL ERROR - THREAD ");
    buf = printNum (buf, thread->id); *buf++ = '\n';
    buf = strAppend(buf, causes[(thread->context.cause >> 2) & 0x1F]);
    buf = strAppend(buf, "\n");
    print_fbuf(text); buf = text;

    //print PC, opcode, and other regs
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
    buf = strAppend(buf, " OP:");
    if(thread->context.pc >= 0x80000000 && thread->context.pc <= 0x807FFFFC) {
        buf = printHex (buf, *(u32*)(thread->context.pc & ~3), 8);
    }
    else buf = strAppend(buf, "--------");
    buf = strAppend(buf, "\n");
    print_fbuf(text); buf = text;

    //print GPRs
    u32 *reg = (u32*)&thread->context.at;
    buf = text;
    for(int i=0; regName[i]; i++) {
        buf = strAppend(text, regName[i]); *buf++ = ':';
        reg++; //skip high word
        buf = printHex (buf, *reg++, 8);
        buf = strAppend(buf, ((i % 3) == 2) ? "\n" : " ");
        print_fbuf(text);
    }

    //print game-specific info
    buf = text;
    buf = strAppend(buf, "HE:"); buf = printHex (buf, (u32)heapEndPtr, 8);
    buf = strAppend(buf, " HS:");
    buf = printHex (buf, (u32)&heapEnd - (u32)heapEndPtr, 8);
    buf = strAppend(buf, "\nMT:");buf = printHex (buf, mainThreadTask, 8);
    buf = strAppend(buf, " PT:"); buf = printHex (buf, mainThreadPrevTask, 8);
    buf = strAppend(buf, " ST:"); buf = printHex (buf, mainThreadSubTask, 8);
    buf = strAppend(buf, "\nSM:");buf = printHex (buf, screenMode, 8);
    buf = strAppend(buf, " RT:"); buf = printHex (buf, raceType, 8);
    buf = strAppend(buf, " TN:"); buf = printHex (buf, curCourse, 8);
    buf = strAppend(buf, "\nNP:");buf = printHex (buf, numPlayers, 8);
    print_fbuf(text);

    //print hardware info
    buf = text;
    buf = strAppend(buf, "\nHW:");
    if(sdrv_isInit) {
        buf = strAppend(buf, "64drive rev ");
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

        buf = strAppend(buf, ", ");
        buf = printNum (buf, sdrv_getRamSize() / (1024*1024));
        buf = strAppend(buf, "MB\n");
    }
    else { //XXX ED64
        buf = strAppend(buf, "ROM\n");
    }
    print_fbuf(text);
    //XXX show CP0 regs
}


static void drawFloats() { //print FPRs
    char text[2048];
    char *buf = text;
    u32 *fp = (u32*)&faultThread->context.fp0;

    buf = strAppend(buf, "FPCSR: ");
    buf = printHex (buf, faultThread->context.fpcsr, 8);
    buf = strAppend(buf, "\n");
    print_fbuf(text);

    buf = text;
    for(int i=0; i<32; i++) {
        *buf++ = 'f';
        *buf++ = '0' + (i / 10);
        *buf++ = '0' + (i % 10);
        *buf++ = ':';
        buf    = printHex(buf, *fp, 8); //XXX print as floats.
        fp++;
        if(i & 1) {
            buf = strAppend(buf, "\n");
            print_fbuf(text);
            buf = text;
        }
        else {
            buf = strAppend(buf, "  ");
        }
    }

    print_fbuf("\nI LOVE YOU 00000000"); //lol OoT reference
}


static void drawStack(u16 buttons) { //print stack dump
    //use up/down to page through data
    static u32 *addr = 0;
    if(addr == 0 || (buttons & Z_TRIG)) addr = (u32*)faultThread->context.sp;
    if(buttons & U_JPAD) addr += 3 * 24;
    if(buttons & D_JPAD) addr -= 3 * 24;

    u32 *data = addr;
    char text[2048];
    char *buf = text;

    buf = strAppend(buf, "STACK DUMP: ");
    buf = printHex(buf, (u32)data, 8);
    buf = strAppend(buf, " SP=");
    buf = printHex(buf, faultThread->context.sp, 8);
    buf = strAppend(buf, "\n");
    print_fbuf(text);

    for(int i=0; i<24; i++) {
        buf = text;
        buf = printHex(buf, ((u32)data) & 0xFFFF, 4);
        buf = strAppend(buf, ":");
        for(int j=0; j<3; j++) {
            *buf++ = (data == (u32*)faultThread->context.sp) ? '>' : ' ';
            buf = printHex(buf, *data, 8);
            data--;
        }
        buf = strAppend(buf, "\n");
        print_fbuf(text);
    }
}


static void drawMem(u16 buttons) { //print memory dump
    //use up/down to page through data
    static u32 *addr = (u32*)0x800DC400;
    static int digit = 4;

    if(buttons & B_BUTTON) addr -= 3 * 24;
    if(buttons & A_BUTTON) addr += 3 * 24;
    if(buttons & L_JPAD) digit--;
    if(buttons & R_JPAD) digit++;
    digit &= 7;

    if(buttons & (U_JPAD | D_JPAD)) {
        u32 newAddr = (u32)addr;
        u32 shift   =   4  * (7 - digit);
        u32 mask    = 0xF << shift;
        u32 cur     = (newAddr >> shift) & 0xF;
        cur += (buttons & U_JPAD) ? 1 : -1;
        newAddr = (newAddr & ~mask) | ((cur & 0xF) << shift);
        addr = (u32*)newAddr;
    }

    u32 *data = addr;
    char text[2048];
    char *buf = text;

    _print_Y = 12;
    buf = strAppend(buf, "          ");
    for(int i=0; i<8; i++) *buf++ = (i == digit) ? 'V' : ' ';
    buf = strAppend(buf, "\n");
    buf = strAppend(buf, "MEM DUMP: ");
    buf = printHex(buf, (u32)data, 8);
    buf = strAppend(buf, "\n");
    print_fbuf(text);

    //ensure addr is in bounds. XXX better method
    u32 a = (u32)addr;
    if((a <  0x80000000)
    || (a >= 0x80800000 && a < 0xA0000000)
    || (a >= 0xA0800000 && a < 0xA4000000)
    || (a >= 0xA4080000 && a < 0xB0000000)
    || (a >= 0xC0000000)) return;

    for(int i=0; i<24; i++) {
        buf = text;
        buf = printHex(buf, ((u32)data) & 0xFFFF, 4);
        buf = strAppend(buf, ":");
        for(int j=0; j<3; j++) {
            *buf++ = ' ';
            u32 val = *data;
            buf = printHex(buf, val >> 16, 4); *buf++ = ' ';
            buf = printHex(buf, val & 0xFFFF, 4);
            data++;
        }
        buf = strAppend(buf, "\n");
        print_fbuf(text);
    }
}


static void printCrashInfoToConsole() {
    OSThread *thread = faultThread;

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

    buf = text;
    buf = strAppend(buf, "\r\nHeap: ");
    buf = printHex (buf, (u32)heapEndPtr, 8);
    buf = strAppend(buf, "; size: ");
    buf = printHex (buf, (u32)&heapEnd - (u32)heapEndPtr, 8);
    sdrv_dprint(text);

    buf = text;
    buf = strAppend(buf, "\r\nMain thread task: ");
    buf = printHex (buf, mainThreadTask, 4);
    buf = strAppend(buf, ", prev: ");
    buf = printHex (buf, mainThreadPrevTask, 4);
    buf = strAppend(buf, ", sub: ");
    buf = printHex (buf, mainThreadSubTask, 4);
    sdrv_dprint(text);

    buf = text;
    buf = strAppend(buf, "\r\nScreen mode: ");
    buf = printHex (buf, screenMode, 1);
    buf = strAppend(buf, "; race mode: ");
    buf = printHex (buf, raceType, 1);
    buf = strAppend(buf, "; course: ");
    buf = printHex (buf, curCourse, 2);
    buf = strAppend(buf, "; players: ");
    buf = printHex (buf, numPlayers, 1);
    sdrv_dprint(text);

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
            if     ((u32)code == thread->context.pc) *buf++ = '<';
            else if((u32)code == thread->context.pc + 4) *buf++ = '>';
            else *buf++ = ' ';

            buf = printHex(buf, *code, 8);

            if(j == 3) *buf++ = ' ';
            code++;
        }
        buf = strAppend(buf, "\r\n");
        sdrv_dprint(text);
    }
}


static void doButtons() {
    static int page = 0;
    static u16 prevButtons = 0;
    u16 curButtons = player1_controllerState.buttons;
    u16 buttons = curButtons & ~prevButtons;
    if(!buttons) {
        prevButtons = curButtons;
        return;
    }

    if(buttons & L_TRIG) page--;
    if(buttons & R_TRIG) page++;
    if(page < 0) page = 5;
    if(page > 5) page = 0;

    //reset framebuffer
    _print_X = 20;
    _print_Y = 20;
    _print_startX = _print_X;
    memset(framebuffer, 0x0021, SCREEN_SIZE*2); //x2 for 2 bytes per pixel

    switch(page) {
        case 0: //show GPRs and exception info
            drawCrashScreen();
            break;

        case 1: //show FPRs
            drawFloats();
            break;

        case 2: //show stack
            drawStack(buttons);
            break;

        case 3: //show memory
            drawMem(buttons);
            break;

        case 4: //show first framebuffer other than this one
            if(framebuffer == frameBuffer0)
                memcpy(framebuffer, frameBuffer2, SCREEN_SIZE*2);
            else
                memcpy(framebuffer, frameBuffer0, SCREEN_SIZE*2);
            break;

        case 5: //show second framebuffer
            if(framebuffer == frameBuffer1)
                memcpy(framebuffer, frameBuffer2, SCREEN_SIZE*2);
            else
                memcpy(framebuffer, frameBuffer1, SCREEN_SIZE*2);
            break;
    }

    osWriteBackDCacheAll();
    prevButtons = curButtons;
}

void crashHook(u16 *framebuf, OSThread *thread) {
    //called when a thread crashes.

    faultThread = thread;
    framebuffer = framebuf;
    memset(framebuffer, 0x0021, 320*240*2);
    drawCrashScreen();
    //printCrashInfoToConsole();
    while(1) {
        osYieldThread();
        readControllers();
        doButtons();
    }
}

} //extern "C"
