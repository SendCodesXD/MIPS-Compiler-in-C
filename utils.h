#ifndef __UTILS_H__
#define __UTILS_H__
#include <string.h>
#include <stdint.h>
#include <assert.h>

struct StringTable
{
    char** lines;
    uint32_t nLine;
};

struct StringTable* createStringTable();
int addString(struct StringTable* table, const char* s);
int destroyStringTable(struct StringTable* table);

int startsWith(const char* s1, const char* s2);
char* stripInplace(char* s);
char *stringReplace2(char *orig, char *rep, char *with);
char* stripDoubleQuotesInplace(char* s);
void printStringTable(struct StringTable* table);
#endif