#ifndef SYS_CALL_H
#define SYS_CALL_H
#include <processes.h>
#include <sys_req.h> 
#include <string.h>
#include <memory.h>

struct context {
    // segment registers
    int ds;
    int es;
    int fs;
    int gs; 
    int ss;

    //General Purpose
    int eax;     // Accumulator register
    int ebx;     // Base register
    int ecx;     // Counter register
    int edx;     // Data register
    int esp;     // Stack pointer
    int ebp;     // Base pointer
    int esi;     // Source index
    int edi;     // Destination index

    int eip;     // Instruction pointer (program counter)
    int cs;
    int flags;  // Flags register (for example, to store CPU flags)   
};

struct context* sys_call(struct context *);





#endif
