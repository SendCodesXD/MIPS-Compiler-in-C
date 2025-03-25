#include "macro.h"
#include "constants.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MacroTable* createMacroTable()
{
    struct MacroTable* table = (struct MacroTable*)malloc(sizeof(struct MacroTable));
    table->nMacro = 0;
    table->macros = NULL;
    return table;
}

int addMacro(struct MacroTable* table, struct Macro* macro)
{
    table->nMacro++;
    table->macros = (struct Macro*)realloc(table->macros, table->nMacro * sizeof(struct Macro));
    memcpy(&table->macros[table->nMacro - 1], macro, sizeof(*macro));
    return 1;
}

int destroyMacroTable(struct MacroTable* table)
{
    return 1;
}

void getMacroName(const char* line, char* name)
{
    const char* start = line;
    if (startsWith(line, ".macro"))
        start = line + 7;
    const char* end = strchr(start, '(');
    if (!end)
        end = start + strlen(start);
    uint32_t len = end - start;
    memcpy(name, start, len);
    name[len] = 0;
}

struct Macro* findMacroByName(struct MacroTable* table, const char* name)
{
    for (uint32_t i = 0; i < table->nMacro; ++i)
        if (0 == strcmp(name, table->macros[i].name))
            return &table->macros[i];
    return NULL;
}

void getMacroArg(const char* line, struct StringTable* table)
{
    char tmp[LINE_SIZE];
    const char* start = strchr(line, '(');
    const char* cur;
    char c;   
    if (!start) // No arg?
        return;
    start++;
    cur = start;
    while (*cur)
    {
        while ((*cur) && (*cur != ',') && (*cur != ')'))
            cur++;
        uint32_t len = cur - start;
        memcpy(tmp, start, len);
        tmp[len] = 0;
        stripInplace(tmp);
        addString(table, tmp);
        if (*cur == 0)
            break;
        start = cur + 1;
        cur = start;
    }
}

void appendMacroLine(const char* line, struct StringTable* table)
{
    addString(table, line);
}

void replaceMacroCallWithCode(struct MacroTable* macroTable, const char* line, struct StringTable* argTable, struct StringTable* codeTable)
{
    char name[128];
    getMacroName(line, name);
    struct Macro* macro = findMacroByName(macroTable, name);
    for (uint32_t i = 0; i < macro->code->nLine; ++i)
    {
        char* newLine = strdup(macro->code->lines[i]);
        for (uint32_t j = 0; j < macro->args->nLine; ++j)
        {
            char* oldLine = newLine;
            newLine = stringReplace2(newLine, macro->args->lines[j], argTable->lines[j]);
            free(oldLine);
        }
        addString(codeTable, newLine);
        free(newLine);
    }
}

void expandMacroCall(struct MacroTable* macroTable, const char* line, struct StringTable* codeTable)
{
    struct StringTable* argTable = createStringTable();
    getMacroArg(line, argTable);
    replaceMacroCallWithCode(macroTable, line, argTable, codeTable);
    destroyStringTable(argTable);
}

struct MacroTable* parseMacroFromFile(const char* fileName)
{
    struct MacroTable* table = createMacroTable();
    char line[LINE_SIZE];
    char tmp[LINE_SIZE];
    FILE* f = fopen(fileName, "rb");
    char* cur = NULL;
    struct Macro* macro = NULL;
    uint32_t lineNum = 0;
    while ((cur = fgets(line, sizeof(line), f)) != NULL)
    {
        lineNum++;
        stripInplace(line);
        if (line[0] == 0) // skip empty line
            continue;
        if (startsWith(line, ".macro"))
        {
            macro = (struct Macro*)malloc(sizeof(struct Macro));
            macro->args = createStringTable();
            macro->code = createStringTable();
            macro->lineNum = 0;
            memset(macro->name, 0, sizeof(macro->name));

            getMacroName(line, macro->name);
            getMacroArg(line, macro->args);
        }
        else if (startsWith(line, ".end_macro"))
        {
            addMacro(table, macro);
            free(macro);
            macro = NULL;
        }
        else
        {
            if (macro->code->nLine == 0)
                macro->lineNum = lineNum;
            if (isMacroCall(table, line))
            {
                expandMacroCall(table, line, macro->code);
            }
            else
            {
                appendMacroLine(line, macro->code);
            }
        }
    }
    fclose(f);
    return table;
}

int isMacroCall(struct MacroTable* table, const char* line)
{
    for (uint32_t i = 0; i < table->nMacro; ++i)
        if (startsWith(line, table->macros[i].name))
            return 1;
    return 0;
}

struct Macro* getMacroFromLine(struct MacroTable* table, const char* line)
{
    for (uint32_t i = 0; i < table->nMacro; ++i)
        if (startsWith(line, table->macros[i].name))
            return &table->macros[i];
    return NULL;
}

void printMacroTable(struct MacroTable* table)
{
    for (uint32_t i = 0; i < table->nMacro; ++i)
    {
        struct Macro* macro = &table->macros[i];
        printf("macro->name = %s\n", macro->name);
        printf("macro->nArgs = %d\n", macro->args->nLine);
        if (macro->args->nLine > 0)
        {
            printf("macro->args = ");
            for (uint32_t j = 0; j < macro->args->nLine; ++j)
                printf("%s|", macro->args->lines[j]);
            puts("");
        }
        
        for (uint32_t j = 0; j < macro->code->nLine; ++j)
            printf("-> %s\n", macro->code->lines[j]);
        puts("-------------------------------------");
    }
}
