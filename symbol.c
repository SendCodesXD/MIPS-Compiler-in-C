#include "symbol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
struct SymbolTable* createSymbolTable()
{
    struct SymbolTable* table = (struct SymbolTable*)malloc(sizeof(struct SymbolTable));
    table->nSymbol = 0;
    table->symbols = NULL;
    return table;
}

int addSymbol(struct SymbolTable* table, const char* symbol, uint32_t address, enum SymbolType type)
{
    table->nSymbol++;
    table->symbols = (struct Symbol*)realloc(table->symbols, table->nSymbol * sizeof(struct Symbol));
    table->symbols[table->nSymbol - 1].address = address;
    table->symbols[table->nSymbol - 1].symbol = strdup(symbol);
    table->symbols[table->nSymbol - 1].type = type;
    return 1;
}

int destroySymbolTable(struct SymbolTable* table)
{
    for (uint32_t i = 0; i < table->nSymbol; ++i)
        free(table->symbols[i].symbol);
    free(table->symbols);
    free(table);
    return 1;
}

int findSymbolByName(struct SymbolTable* table, const char* name, uint32_t* pAddr)
{
    int ret = 0;
    for (int i = 0; i < table->nSymbol; ++i)
    {
        if (0 == strcmp(name, table->symbols[i].symbol))
        {
            ret = 1;
            *pAddr = table->symbols[i].address;
            break;
        }
    }
    return ret;
}

int getSymbol(const char* line, char* symbol, char* lineWithoutSymbol)
{
    int ret = 0;
    int isInDoubleQuotes = 0;
    const char* c = line;
    strcpy(lineWithoutSymbol, line);
    while (*c)
    {
        if (*c == ':')
        {
            if (!isInDoubleQuotes)
            {
                ret = 1;
                memcpy(symbol, line, c - line);
                symbol[c - line] = 0;
                strcpy(lineWithoutSymbol, c + 1);
                stripInplace(lineWithoutSymbol);
            }
            break;
        }
        else if (*c == '"')
        {
            isInDoubleQuotes = 1 - isInDoubleQuotes;
        }
        c++;
    }
    return ret;
}

void printSymbolTable(struct SymbolTable* table)
{
    for (uint32_t i = 0; i < table->nSymbol; ++i)
    {
        printf("symbol: \"%s\", address: 0x%08x, type=%s\n", 
            table->symbols[i].symbol,
            table->symbols[i].address,
            table->symbols[i].type == symbolTypeCode ? "code" : 
            table->symbols[i].type == symbolTypeData ? "data" : "unk"
        );
    }
    puts("-------------------------------");
}