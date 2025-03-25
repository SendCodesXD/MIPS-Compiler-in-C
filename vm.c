#include "vm.h"
#include "instruction.h"
#include "utils.h"
#include "constants.h"
#include <stdlib.h>
#include <stdio.h>


struct VirtualMachine* createVirtualMachine() 
{
    struct VirtualMachine* vm = (struct VirtualMachine*)malloc(sizeof(struct VirtualMachine));
    memset(vm->regs, 0, sizeof(vm->regs));
    vm->data = calloc(1 * 1024 * 1024, sizeof(char));
    vm->stack = calloc(STACK_SIZE, sizeof(char));
    vm->pc = TEXT_BASE;
    vm->r.sp = STACK_BASE + STACK_SIZE - WORD_SIZE;
    vm->exit = 0;
    return vm;
}

int doSyscall(struct VirtualMachine* vm)
{
    int ret = 0;
    uint32_t syscallNumber = vm->r.v0;
    //printf("syscall(%u, a0=0x%08x, a1=0x%08x)\n", syscallNumber, vm->r.a0, vm->r.a1);
    if (syscallNumber == 1)
    {
        printf("%d\n", vm->r.a0);
        ret = 1;
        vm->pc += WORD_SIZE;
    }
    else if (syscallNumber == 4)
    {
        uint8_t* mem = (uint8_t*)vm->data + (vm->r.a0 - DATA_BASE);
        printf("%s", mem);
        ret = 1;
        vm->pc += WORD_SIZE;
    }
    else if (syscallNumber == 8)
    {
        uint8_t* mem = (uint8_t*)vm->data + (vm->r.a0 - DATA_BASE);
        memset(mem, 0, vm->r.a1);
        fgets(mem, vm->r.a1, stdin);
        vm->r.v0 = strlen(mem) - 1;
        ret = 1;
        vm->pc += WORD_SIZE;
    }
    else if (syscallNumber == 10)
    {
        vm->exit = 1;
        ret = 1;
        vm->pc += WORD_SIZE;
    }
    else if (syscallNumber == 11)
    {
        putc(vm->r.a0, stdout);
        ret = 1;
        vm->pc += WORD_SIZE;
    }
    else
    {
        printf("Unregconize syscall number %u\n", syscallNumber);
        vm->pc += WORD_SIZE;
    }
    return ret;
}

int runOneInstruction(struct VirtualMachine* vm, uint32_t insBinary, uint32_t* pNextPc)
{
    /*
        const char* g_rIns[] = {"add", "addu", "sub", "and", "or", "slt", "jr"};
        const char* g_iIns[] = {"addi", "addiu", "beq", "bne", "lw", "sw", "ori", "lui"};
        const char* g_jIns[] = {"j", "jal"};
    */
    int ret = 0;
    struct Instruction ins = {.asUint32 = insBinary};
    if (ins.asUint32 == 0) // syscall
    {
        ret = doSyscall(vm);
    }
    else
    {
        uint32_t opcode = ins.r.opcode;   
        if (opcode == 0) // R type
        {
            uint32_t funct = ins.r.funct;
            uint32_t rd = ins.r.rd;
            uint32_t rs = ins.r.rs;
            uint32_t rt = ins.r.rt;
            switch (funct)
            {
                case 32: // add
                    vm->regs[rd] = vm->regs[rs] + vm->regs[rt];
                    vm->pc += WORD_SIZE;
                    break;
                case 33: // addu
                    vm->regs[rd] = vm->regs[rs] + vm->regs[rt];
                    vm->pc += WORD_SIZE;
                    break;
                case 34: // sub
                    vm->regs[rd] = vm->regs[rs] - vm->regs[rt];
                    vm->pc += WORD_SIZE;
                    break;
                case 36: // and
                    vm->regs[rd] = vm->regs[rs] & vm->regs[rt];
                    vm->pc += WORD_SIZE;
                    break;
                case 37: // or
                    vm->regs[rd] = vm->regs[rs] | vm->regs[rt];
                    vm->pc += WORD_SIZE;
                    break;
                case 0x2A: // slt
                    vm->regs[rd] = vm->regs[rs] < vm->regs[rt] ? 1 : 0;
                    vm->pc += WORD_SIZE;
                    break;
                case 0x8: // jr
                    vm->pc = vm->regs[rs];
                    break;
                default:
                    printf("unregconize funct: %u, ins 0x%08x\n", funct, insBinary);
                    ret = 0;
                    assert(0);
            }
        }
        else
        {
            uint32_t rt = ins.i.rt;
            uint32_t rs = ins.i.rs;
            uint32_t imm = ins.i.imm;
            switch (opcode)
            {
                case 0x8: // addi
                    vm->regs[rt] = vm->regs[rs] + imm;
                    vm->pc += WORD_SIZE;
                    break;
                case 0x9: // addiu
                    vm->regs[rt] = vm->regs[rs] + imm;
                    vm->pc += WORD_SIZE;
                    break;
                case 0x4: // beq
                    if (vm->regs[rt] == vm->regs[rs])
                        vm->pc += (imm << 2);
                    vm->pc += WORD_SIZE;
                    break;
                case 0x5: // bne
                    if (vm->regs[rt] != vm->regs[rs])
                        vm->pc += (imm << 2);
                    vm->pc += WORD_SIZE;
                    break;
                case 0x23: // lw
                    ret = 0;
                    assert(0);
                    break;
                case 0x2b: // sw
                    ret = 0;
                    assert(0);
                    break;
                case 0xd: // ori
                    vm->regs[rt] = vm->regs[rs] | imm;
                    vm->pc += WORD_SIZE;
                    break;
                case 0xf: // lui
                    vm->regs[rt] = imm << 16;
                    vm->pc += WORD_SIZE;
                    break;
                case 0x2: // j
                    vm->pc = ins.j.address << 2;
                    break;
                case 0x3: // jal
                    vm->r.ra = vm->pc + WORD_SIZE;
                    vm->pc = ins.j.address << 2;
                    break;
                default:
                    ret = 0;
                    assert(0);
            }
        }
    }
    *pNextPc = vm->pc;
    return ret;
}

int destroyVirtualMachine(struct VirtualMachine* vm)
{
    free(vm->data);
    free(vm->stack);
    free(vm);
    return 1;
}

int initDataForVirtualMachine(struct VirtualMachine* vm, struct DataTable* dataTable)
{
    uint8_t* start = (uint8_t*)vm->data;
    for (uint32_t i = 0; i < dataTable->nData; ++i)
    {
        struct Data* pData = &dataTable->data[i];
        if (pData->hasValue)
            memcpy(start + (pData->address - DATA_BASE), pData->dataValue, pData->size);
        else
            memset(start + (pData->address - DATA_BASE), 0xCC, pData->size);
    }
    return 1;
}