/* This patch is part of the framework for other patches. It provides
 * methods for patching into the game code without stepping on toes.
 */
#include <n64.h>
extern "C" {

/* typedef struct {
    u32 instr[16];
} stubCode;

static stubCode stubs[256]; */

/* hook before:
    addiu $sp, -0xXXX
    sw $ra, 0x10($sp)
    # more regs to back up here?
    jal newfunc
    nop
    old instr 1
    old instr 2
    lw $ra, 0x10($sp)
    jr $ra
    addiu $sp, 0xXXX
*/


#define MAKE_JAL(addr) (0x0C000000 | ((((u32)addr) & 0x007FFFFF) >> 2))
#define DECODE_JAL(op) ((((op) & ((1 << 26) - 1)) << 2) | 0x80000000)


u32 hookJal(u32 addr, void *func) {
    u32 oldOp = *(u32*)addr;
    *(u32*)addr = MAKE_JAL(func);
    return DECODE_JAL(oldOp);
}

/* int hookAddress(u32 addr, void *func, int before) {
    u32 *ops = (u32*)addr;
    u32 oldInstr[] = {ops[0], ops[1]}; //save original instructions


} */


} //extern "C"
