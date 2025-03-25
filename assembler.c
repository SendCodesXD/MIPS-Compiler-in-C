#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "macro.h"
#include "data.h"
#include "symbol.h"
#include "constants.h"
#include "utils.h"
#include "instruction.h"
#include "vm.h"

#ifdef MYDEBUG
#define debug_print(...) printf(__VA_ARGS__)
#else
#define debug_print(...) do {} while(0)
#endif

#define DEC_FMT "%u"
#define HEX_FMT_4 "0x%04x"
#define HEX_FMT_8 "0x%08x"

enum Section
{
    sectionTypeUnknown,
    sectionTypeText,
    sectionTypeData,
};

struct StringTable* readCode()
{
    struct StringTable* codeTable = createStringTable();
    FILE* f = fopen("mips.txt", "rb");
    char line[LINE_SIZE];
    char* cur;
    fgets(line, sizeof(line), f); // skip first line, we don't need it
    while ((cur = fgets(line, sizeof(line), f)) != NULL)
    {
        stripInplace(line);
        if (line[0] == 0)
            continue;
        addString(codeTable, line);
    }
    fclose(f);
    return codeTable;
}

struct MacroTable* makeMacroTable(struct StringTable* codeTable)
{
    for (uint32_t i = 0; i < codeTable->nLine; ++i)
    {
        if (startsWith(codeTable->lines[i], ".include \""))
        {
            char* name = strdup(codeTable->lines[i] + 9);
            stripInplace(name);
            stripDoubleQuotesInplace(name);
            // debug_print("[+] Parsed macro from \"%s\"\n", name);
            struct MacroTable* ret = parseMacroFromFile(name);
            free(name);
            return ret;
        }
    }
    // debug_print("[+] Found no include in code\n");
    return NULL;
}

void printCodeTable(struct StringTable* codeTable)
{
    for (uint32_t i = 0; i < codeTable->nLine; ++i)
    {
        printf("0x%08x: %s\n", TEXT_BASE + WORD_SIZE * i, codeTable->lines[i]);
    }
    puts("-------------------------------------");
}

char* makeAssemblyLine(const char* m, const char* a1, const char* a2, const char* a3, char* line)
{
    if (!a1[0])
        sprintf(line, "%s", m);
    else if (!a2[0])
        sprintf(line, "%s %s", m, a1);
    else if (!a3[0])
        sprintf(line, "%s %s,%s", m, a1, a2);
    else
        sprintf(line, "%s %s,%s,%s", m, a1, a2, a3);
    return line;
}

int expandCodeAndMakeSymbolTable(
    struct StringTable* orgCodeTable,
    struct MacroTable* macroTable,
    struct StringTable* newCodeTable,
    struct SymbolTable* symbolTable,
    struct DataTable* dataTable)
{
    enum Section currentSection = sectionTypeUnknown;
    uint32_t dataOffset = 0;

    for (uint32_t i = 0; i < orgCodeTable->nLine; ++i)
    {
        const char* line = orgCodeTable->lines[i];
        if (startsWith(line, ".text"))
        {
            currentSection = sectionTypeText;
            continue;
        }
        else if (startsWith(line, ".data"))
        {
            currentSection = sectionTypeData;
            dataOffset = 0;
            continue;
        }
        else if (startsWith(line, ".include"))
        {
            continue;
        }
        
        if (currentSection == sectionTypeText)
        {
            char symbol[128];
            char lineWithoutSymbol[LINE_SIZE];
            int hasSymbol = getSymbol(line, symbol, lineWithoutSymbol);
            if (hasSymbol)
                addSymbol(symbolTable, symbol, TEXT_BASE + newCodeTable->nLine * WORD_SIZE, symbolTypeCode);
            if (macroTable && isMacroCall(macroTable, lineWithoutSymbol))
                expandMacroCall(macroTable, lineWithoutSymbol, newCodeTable);
            else
                addString(newCodeTable, lineWithoutSymbol);
        }
        else if (currentSection == symbolTypeData)
        {
            char symbol[128];
            char lineWithoutSymbol[LINE_SIZE];
            int hasSymbol = getSymbol(line, symbol, lineWithoutSymbol);
            if (hasSymbol)
                addSymbol(symbolTable, symbol, DATA_BASE + dataOffset, symbolTypeData);
            struct StringTable* tmpCodeTable = createStringTable();
            if (macroTable && isMacroCall(macroTable, lineWithoutSymbol))
                expandMacroCall(macroTable, lineWithoutSymbol, tmpCodeTable);
            else
                addString(tmpCodeTable, lineWithoutSymbol);
            for (uint32_t i = 0; i < tmpCodeTable->nLine; ++i)
            {
                const char* tmpLine = tmpCodeTable->lines[i];
                char tmpSymbol[128];
                char tmpLineWithoutSymbol[LINE_SIZE];
                int tmpHasSymbol = getSymbol(tmpLine, tmpSymbol, tmpLineWithoutSymbol);
                if (tmpHasSymbol)
                    addSymbol(symbolTable, tmpSymbol, DATA_BASE + dataOffset, sectionTypeData);
                if (startsWith(tmpLineWithoutSymbol, ".asciiz"))
                {
                    char* data = tmpLineWithoutSymbol + 8;
                    stripInplace(data);
                    stripDoubleQuotesInplace(data);
                    addData(dataTable, tmpHasSymbol ? tmpSymbol : NULL, strlen(data) + 1, data, DATA_BASE + dataOffset);
                    dataOffset += strlen(data) + 1;
                }
                else if (startsWith(tmpLineWithoutSymbol, ".word"))
                {
                    char* data = tmpLineWithoutSymbol + 6;
                    stripInplace(data);
                    uint32_t n = (uint32_t)atoi(data);
                    addData(dataTable, tmpHasSymbol ? tmpSymbol : NULL, WORD_SIZE, (void*)&n, DATA_BASE + dataOffset);
                    dataOffset += WORD_SIZE;
                }
                else if (startsWith(tmpLineWithoutSymbol, ".space"))
                {
                    char* data = tmpLineWithoutSymbol + 7;
                    stripInplace(data);
                    uint32_t n = (uint32_t)atoi(data);
                    addData(dataTable, tmpHasSymbol ? tmpSymbol : NULL, n, NULL, DATA_BASE + dataOffset);
                    dataOffset += n;
                }
                else
                {
                    printf("Warning: unregconize directive: %s\n", tmpLineWithoutSymbol);
                }
            }

            destroyStringTable(tmpCodeTable);
        }
        else
        {
            printf("Warning: unknown section, ignoring \"%s\"\n", line);
            continue;
        }
    }
    return 1;
}

int expandPseudoInstructionAndFinalize(struct StringTable* orgCodeTable, struct SymbolTable* symbolTable, struct StringTable* newCodeTable)
{
    // Fix symbol table first
    uint32_t nWordOffset = 0;
    for (uint32_t i = 0; i < orgCodeTable->nLine; ++i)
    {
        const char* line = orgCodeTable->lines[i];
        char m[128];
        char a1[128];
        char a2[128];
        char a3[128];
        parseLineSimple(line, m, a1, a2, a3);
        uint32_t currentAddress = TEXT_BASE + WORD_SIZE * (i + nWordOffset);
        if (0 == strcmp(m, "la"))
        {
            for (uint32_t j = 0; j < symbolTable->nSymbol; ++j)
            {
                if (symbolTable->symbols[j].type == symbolTypeCode && 
                    symbolTable->symbols[j].address > currentAddress)
                {
                    symbolTable->symbols[j].address += 1 * WORD_SIZE;
                }
            }
            nWordOffset += 1;
        }
        else if (0 == strcmp(m, "li"))
        {
            uint32_t immValue = getImmValue(a2);
            if (immValue > 0xFFFF)
            {
                for (uint32_t j = 0; j < symbolTable->nSymbol; ++j)
                {
                    if (symbolTable->symbols[j].type == symbolTypeCode && 
                        symbolTable->symbols[j].address > currentAddress)
                    {
                        symbolTable->symbols[j].address += 1 * WORD_SIZE;
                    }
                }
                nWordOffset += 1;
            }
        }
    }
    
    // Then compile to real code
    nWordOffset = 0;
    for (uint32_t i = 0; i < orgCodeTable->nLine; ++i)
    {
        const char* line = orgCodeTable->lines[i];
        char m[128];
        char a1[128];
        char a2[128];
        char a3[128];
        char tmp[LINE_SIZE];
        char tmp1[128];
        char tmp2[128];
        char tmp3[128];
        parseLineSimple(line, m, a1, a2, a3);
        uint32_t currentAddress = TEXT_BASE + WORD_SIZE * (i + nWordOffset);
        const char* numFmt = HEX_FMT_4;
        if (0 == strcmp(m, "la"))
        {
            // la r,label
            uint32_t labelAddress;
            findSymbolByName(symbolTable, a2, &labelAddress);
            
            sprintf(tmp2, numFmt, labelAddress >> 16);
            makeAssemblyLine("lui", "$at", tmp2, "", tmp);
            addString(newCodeTable, tmp);

            sprintf(tmp2, numFmt, labelAddress & 0xFFFF);
            makeAssemblyLine("ori", a1, "$at", tmp2, tmp);
            addString(newCodeTable, tmp);

            nWordOffset += 1;
        }
        else if (0 == strcmp(m, "li"))
        {
            // li r, number
            uint32_t number = getImmValue(a2);
            if (number > 0xFFFF)
            {
                sprintf(tmp2, numFmt, number >> 16);
                makeAssemblyLine("lui", "$at", tmp2, "", tmp);
                addString(newCodeTable, tmp);

                sprintf(tmp2, numFmt, number & 0xFFFF);
                makeAssemblyLine("ori", a1, "$at", tmp2, tmp);
                addString(newCodeTable, tmp);
                nWordOffset += 1;
            }
            else
            {
                makeAssemblyLine("addiu", a1, "$zero", a2, tmp);
                addString(newCodeTable, tmp);
            }
        }
        else if (0 == strcmp(m, "move"))
        {
            // move r1, r2
            makeAssemblyLine("addu", a1, "$0", a2, tmp);
            addString(newCodeTable, tmp);
        }
        else
        {
            uint32_t labelAddress;
            if (a1[0] && findSymbolByName(symbolTable, a1, &labelAddress))
                sprintf(a1, numFmt, labelAddress);
            if (a2[0] && findSymbolByName(symbolTable, a2, &labelAddress))
                sprintf(a2, numFmt, labelAddress);
            if (a3[0] && findSymbolByName(symbolTable, a3, &labelAddress))
                sprintf(a3, numFmt, labelAddress);
            makeAssemblyLine(m, a1, a2, a3, tmp);
            addString(newCodeTable, tmp);
        }
    }

    return 1;
}

int writeSymbolTableToFile(struct SymbolTable* symbolTable)
{
    char line[LINE_SIZE];
    FILE* f = fopen("symboltable.txt", "wb");
    for (uint32_t i = 0; i < symbolTable->nSymbol; ++i)
    {
        snprintf(line, sizeof(line), "%s\t0x%08x\n", symbolTable->symbols[i].symbol, symbolTable->symbols[i].address);
        fwrite(line, sizeof(char), strlen(line), f);
    }
    fclose(f);
    return 1;
}

int writeBinaryCodeToFile(struct StringTable* finalCodeTable, struct StringTable* newCodeTable, struct StringTable* orgCodeTable, struct MacroTable* macroTable)
{
    char tmp[32];
    FILE* f = fopen("execute.txt", "wb");
    uint32_t currentAddress = TEXT_BASE;
    uint32_t i = 0;
    struct Instruction ins;
    struct StringTable* onlyOrgCodeTable = createStringTable();
    for (i = 0; i < orgCodeTable->nLine; ++i)
    {
        const char* line = orgCodeTable->lines[i];
        if (0 == strcmp(line, ".text"))
            break;
    }
    i++;
    uint32_t lineNumber = i + 1;
    for (; i < orgCodeTable->nLine; ++i)
    {
        const char* line = orgCodeTable->lines[i];
        if (0 == strcmp(line, ".data"))
            break;
        addString(onlyOrgCodeTable, line);
    }
    //printCodeTable(onlyOrgCodeTable);
    uint32_t j = 0, jj = 0;
    for (i = 0; i < onlyOrgCodeTable->nLine; ++i)
    {
        char lineWithoutSymbol[LINE_SIZE];
        char symbol[128];
        char m[128], a1[128], a2[128], a3[128];
        struct Macro* macro;
        const char* line = onlyOrgCodeTable->lines[i];
        getSymbol(line, symbol, lineWithoutSymbol);
        if (macroTable && ((macro = getMacroFromLine(macroTable, lineWithoutSymbol)) != NULL))
        {
            for (uint32_t k = 0; k < macro->code->nLine; ++k)
            {
                parseLine(finalCodeTable->lines[jj], &ins, currentAddress);
                parseLineSimple(newCodeTable->lines[j], m, a1, a2, a3);
                if (k == 0)
                {
                    fprintf(f, "0x%08x\t0x%08x\t%-30s\t%3d: <%u> %s\n", 
                        currentAddress,
                        ins.asUint32,
                        finalCodeTable->lines[jj],
                        lineNumber,
                        macro->lineNum,
                        newCodeTable->lines[j]
                    );
                    if (0 == strcmp(m, "la"))
                    {
                        currentAddress += WORD_SIZE;
                        jj++;
                        parseLine(finalCodeTable->lines[jj], &ins, currentAddress);
                        fprintf(f, "0x%08x\t0x%08x\t%-30s\n", 
                            currentAddress,
                            ins.asUint32,
                            finalCodeTable->lines[jj]
                        );
                    }
                }
                else
                {
                    fprintf(f, "0x%08x\t0x%08x\t%-30s\n", 
                        currentAddress,
                        ins.asUint32,
                        finalCodeTable->lines[jj]
                    );
                    if (0 == strcmp(m, "la"))
                    {
                        currentAddress += WORD_SIZE;
                        jj++;
                        parseLine(finalCodeTable->lines[jj], &ins, currentAddress);
                        fprintf(f, "0x%08x\t0x%08x\t%-30s\n", 
                            currentAddress,
                            ins.asUint32,
                            finalCodeTable->lines[jj]
                        );
                    }
                }
                j++;
                jj++;
                currentAddress += WORD_SIZE;
            }
        }
        else
        {
            parseLine(finalCodeTable->lines[j], &ins, currentAddress);
            parseLineSimple(newCodeTable->lines[j], m, a1, a2, a3);
            fprintf(f, "0x%08x\t0x%08x\t%-30s\t%3d: %s\n", 
                currentAddress,
                ins.asUint32,
                finalCodeTable->lines[j],
                lineNumber,
                newCodeTable->lines[j]
            );
            if (0 == strcmp(m, "la"))
            {
                currentAddress += WORD_SIZE;
                jj++;
                parseLine(finalCodeTable->lines[jj], &ins, currentAddress);
                fprintf(f, "0x%08x\t0x%08x\t%-30s\n", 
                    currentAddress,
                    ins.asUint32,
                    finalCodeTable->lines[jj]
                );
            }
            j++;
            jj++;
            currentAddress += WORD_SIZE;
        }
        lineNumber++;
    }

    destroyStringTable(onlyOrgCodeTable);
    fclose(f);
    return 1;
}

int executeCode(struct StringTable* codeTable, struct DataTable* dataTable)
{
    struct VirtualMachine* vm = createVirtualMachine();
    initDataForVirtualMachine(vm, dataTable);
    uint32_t currentAddress = vm->pc;
    struct Instruction ins;
    while (vm->exit == 0)
    {
        uint32_t i = (currentAddress - TEXT_BASE) / WORD_SIZE;
        if (i >= codeTable->nLine)
        {
            vm->exit = 1;
            break;
        }
        parseLine(codeTable->lines[i], &ins, currentAddress);
        runOneInstruction(vm, ins.asUint32, &currentAddress);
    }
}

int main(int argc, char* argv[])
{
    struct StringTable* orgCodeTable = readCode();
    struct MacroTable* macroTable = makeMacroTable(orgCodeTable);
    struct StringTable* newCodeTable = createStringTable();
    struct SymbolTable* symbolTable = createSymbolTable();
    struct DataTable* dataTable = createDataTable();
    struct StringTable* finalCodeTable = createStringTable();
    expandCodeAndMakeSymbolTable(orgCodeTable, macroTable, newCodeTable, symbolTable, dataTable);
    expandPseudoInstructionAndFinalize(newCodeTable, symbolTable, finalCodeTable);
    writeSymbolTableToFile(symbolTable);
    writeBinaryCodeToFile(finalCodeTable, newCodeTable, orgCodeTable, macroTable);
    puts("Assemble: operation completed successfully.");
    executeCode(finalCodeTable, dataTable);
    return 0;
}