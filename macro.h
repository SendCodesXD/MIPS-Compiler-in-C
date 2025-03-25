#ifndef __MACRO_H__
#define __MACRO_H__
#include <stdint.h>

struct Macro
{
    char name[128];
    struct StringTable* code;
    struct StringTable* args;
    uint32_t lineNum;
};

struct MacroTable
{
    uint32_t nMacro;
    struct Macro* macros;
};


struct MacroTable* parseMacroFromFile(const char* fileName);
int isMacroCall(struct MacroTable* table, const char* line);
void printMacroTable(struct MacroTable* table);
void expandMacroCall(struct MacroTable* macroTable, const char* line, struct StringTable* codeTable);
struct Macro* getMacroFromLine(struct MacroTable* table, const char* line);
#endif