#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__
#include <stdint.h>

struct JType
{
    uint32_t address: 26;
    uint32_t opcode: 6; 
};

struct IType
{
    uint32_t imm: 16;
    uint32_t rt: 5;
    uint32_t rs: 5;
    uint32_t opcode: 6; 
};

struct RType
{
    uint32_t funct: 6;
    uint32_t shamt: 5;
    uint32_t rd: 5;
    uint32_t rt: 5;
    uint32_t rs: 5;
    uint32_t opcode: 6; 
};

enum InstructionType
{
    instructionTypeUnknown,
    instructionTypeJ,
    instructionTypeI,
    instructionTypeSyscall,
    instructionTypeR
};

struct Instruction
{
    union
    {
        struct JType j;
        struct IType i;
        struct RType r;
        uint32_t asUint32;
    };
    enum InstructionType type;
};

int parseLineSimple(const char* line, char* m, char* a1, char* a2, char* a3);
int parseLine(const char* line, struct Instruction* pIns, uint32_t currentAddress);
int getImmValue(const char* imm);
#endif