#ifndef __VM_H__
#define __VM_H__
#include <stdint.h>
#include "utils.h"
#include "constants.h"
#include "data.h"

struct VirtualMachine
{
    union
    {
        uint32_t regs[32];
        struct 
        {
            uint32_t zero;
            uint32_t at;
            uint32_t v0,v1;
            uint32_t a0,a1,a2,a3;
            uint32_t t0,t1,t2,t3,t4,t5,t6,t7,t8,t9;
            uint32_t s0,s1,s2,s3,s4,s5,s6,s7;
            uint32_t k0,k1;
            uint32_t gp;
            uint32_t sp;
            uint32_t fp;
            uint32_t ra;
        } r;
    };
    void* data;
    uint32_t* stack;
    uint32_t pc;
    uint32_t exit;
};

struct VirtualMachine* createVirtualMachine();
int initDataForVirtualMachine(struct VirtualMachine* vm, struct DataTable* dataTable);
int runOneInstruction(struct VirtualMachine* vm, uint32_t insBinary, uint32_t* pNextPc);
int destroyVirtualMachine(struct VirtualMachine* vm);

#endif