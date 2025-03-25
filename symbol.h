#ifndef __SYMBOL_H__
#define __SYMBOL_H__
#include <stdint.h>

enum SymbolType
{
    symbolTypeUnknown,
    symbolTypeCode,
    symbolTypeData
};

struct Symbol
{
    char* symbol;
    uint32_t address;
    enum SymbolType type;
};

struct SymbolTable
{
    uint32_t nSymbol;
    struct Symbol* symbols;
};

struct SymbolTable* createSymbolTable();
int addSymbol(struct SymbolTable* table, const char* symbol, uint32_t address, enum SymbolType type);
int destroySymbolTable(struct SymbolTable* table);
int findSymbolByName(struct SymbolTable* table, const char* name, uint32_t* pAddr);
int getSymbol(const char* line, char* symbol, char* lineWithoutSymbol);
void printSymbolTable(struct SymbolTable* table);
#endif