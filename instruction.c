#include "instruction.h"
#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"

int parseLineSimple(const char* line, char* m, char* a1, char* a2, char* a3)
{
    a1[0] = 0;
    a2[0] = 0;
    a3[0] = 0;
    const char* c = line;
    while (*c && (isalpha(*c)))
        c++;
    memcpy(m, line, c - line);
    m[c - line] = 0;
    while (*c && isspace(*c))
        c++;
    if (!(*c))
        return 1;
    const char* start = c;
    int currentArg = 1;
    char* tmpLine = strdup(start);
    char* token = strtok(tmpLine, ",");
    while (token)
    {
        if (currentArg == 1)
        {
            strcpy(a1, token);
            stripInplace(a1);
            currentArg++;
        }
        else if (currentArg == 2)
        {
            strcpy(a2, token);
            stripInplace(a2);
            currentArg++;
        }
        else if (currentArg == 3)
        {
            strcpy(a3, token);
            stripInplace(a3);
            currentArg++;
            break;
        }
        token = strtok(NULL, ",");
    }

    free(tmpLine);
    return 1;
}

const char* g_rIns[] = {"add", "addu", "sub", "and", "or", "slt", "jr"};
const char* g_iIns[] = {"addi", "addiu", "beq", "bne", "lw", "sw", "ori", "lui"};
const char* g_jIns[] = {"j", "jal"};

enum InstructionType getInstructionType(const char* m)
{
    for (uint32_t i = 0; i < sizeof(g_rIns)/sizeof(g_rIns[0]); ++i)
        if (0 == strcmp(g_rIns[i], m))
            return instructionTypeR;
    for (uint32_t i = 0; i < sizeof(g_iIns)/sizeof(g_iIns[0]); ++i)
        if (0 == strcmp(g_iIns[i], m))
            return instructionTypeI;
    for (uint32_t i = 0; i < sizeof(g_jIns)/sizeof(g_jIns[0]); ++i)
        if (0 == strcmp(g_jIns[i], m))
            return instructionTypeJ;
    if (0 == strcmp(m, "syscall"))
        return instructionTypeSyscall;
    return instructionTypeUnknown;
}

const char* g_allRegs1[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31"};
const char* g_allRegs2[] = {"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9", "$s0", "$s1", "$s2", "$s3" ,"$s4", "$s5", "$s6", "$s7", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

int getRegNum(const char* reg)
{
    // printf("getRegNum %s\n", reg);
    const char* r = reg;
    for (int i = 0; i < 32; ++i)
        if (0 == strcmp(g_allRegs1[i], r) || 0 == strcmp(g_allRegs2[i], r))
            return i;
    if (0 == strcmp(reg, "$0"))
        return 0;
    assert(0);
    return -1;
}

int isReg(const char* reg)
{
    return getRegNum(reg) >= 0;
}

int isImm(const char* imm)
{
    if (startsWith(imm, "0x") || startsWith(imm, "0X"))
        return 1;
    for (int i = 0; i < strlen(imm); ++i)
    {
        if (!isdigit(imm[i]))
            return 0;
    }
    return 1;
}

int getRFunc(const char* m)
{
    if (0 == strcmp(m, "add"))
        return 32;
    if (0 == strcmp(m, "addu"))
        return 33;
    if (0 == strcmp(m, "sub"))
        return 34;
    if (0 == strcmp(m, "and"))
        return 36;
    if (0 == strcmp(m, "or"))
        return 37;
    if (0 == strcmp(m, "slt"))
        return 0x2A;
    if (0 == strcmp(m, "jr"))
        return 0x8;
    assert(0);
    return -1;
}

int getImmValue(const char* imm)
{
    assert(isImm(imm));
    if (startsWith(imm, "0x") || startsWith(imm, "0x"))
        return strtoul(imm + 2, NULL, 16);
    return strtoul(imm, NULL, 10);
}

int getIOpcode(const char* m)
{
    //printf("getIopcode %s\n", m);
    if (0 == strcmp(m, "addi"))
        return 0x8;
    if (0 == strcmp(m, "addiu"))
        return 0x9;
    if (0 == strcmp(m, "beq"))
        return 0x4;
    if (0 == strcmp(m, "bne"))
        return 0x5;
    if (0 == strcmp(m, "lw"))
        return 0x23;
    if (0 == strcmp(m, "sw"))
        return 0x2b;
    if (0 == strcmp(m, "ori"))
        return 0xd;
    if (0 == strcmp(m, "lui"))
        return 0xf;
    assert(0);
    return -1;
}

int getJOpcode(const char* m)
{
    if (0 == strcmp(m, "j"))
        return 0x2;
    if (0 == strcmp(m, "jal"))
        return 0x3;
    assert(0);
    return -1;
}

int parseLine(const char* line, struct Instruction* pIns, uint32_t currentAddress)
{
    char m[128];
    char a1[128];
    char a2[128];
    char a3[128];
    parseLineSimple(line, m, a1, a2, a3);
    enum InstructionType type = getInstructionType(m);
    pIns->type = type;
    if (type == instructionTypeI)
    {
        // addi a1,a2,0x2
        if (0 == strcmp(m, "lui"))
        {
            pIns->i.imm = getImmValue(a2);
            pIns->i.rt = getRegNum(a1);
            pIns->i.rs = 0;
            pIns->i.opcode = getIOpcode(m);
        }
        else if (0 == strcmp(m, "bne") || 0 == strcmp(m, "beq"))
        {
            pIns->i.imm = (pIns->i.imm - currentAddress - WORD_SIZE) >> 2;
            pIns->i.rs = getRegNum(a1);
            pIns->i.rt = getRegNum(a2);
            pIns->i.opcode = getIOpcode(m);
        }
        else
        {
            pIns->i.imm = getImmValue(a3);
            pIns->i.rt = getRegNum(a1);
            pIns->i.rs = getRegNum(a2);
            pIns->i.opcode = getIOpcode(m);
        }
    }
    else if (type == instructionTypeR)
    {
        if (0 == strcmp(m, "jr"))
        {
            pIns->r.funct = getRFunc(m);
            pIns->r.rd = 0;
            pIns->r.rs = getRegNum(a1);
            pIns->r.rt = 0;
            pIns->r.shamt = 0;
            pIns->r.opcode = 0;
        }
        else
        {
            pIns->r.funct = getRFunc(m);
            pIns->r.rd = getRegNum(a1);
            pIns->r.rs = getRegNum(a2);
            pIns->r.rt = getRegNum(a3);
            pIns->r.shamt = 0;
            pIns->r.opcode = 0;
        }
    }
    else if (type == instructionTypeJ)
    {
        pIns->j.opcode = getJOpcode(m);
        pIns->j.address = getImmValue(a1) >> 2;
    }
    else if (type == instructionTypeSyscall)
    {
        pIns->asUint32 = 0;
    }
    else
    {
        // Do nothing
    }
    return 1;
}