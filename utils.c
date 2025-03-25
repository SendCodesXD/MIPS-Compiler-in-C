#include "utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

int startsWith(const char* s1, const char* s2)
{
    if (0 == memcmp(s1, s2, strlen(s2)))
        return 1;
    return 0;
}

char* stripInplace(char* s)
{
    char *p = s;
    char *q = s;
    while (isspace((unsigned char)*q))
        ++q; // Skip leading whitespace
    while (*q)
        *p++ = *q++; // Move rest of string to the start
    *p = '\0'; // Null terminate the string
    while (p > s && isspace((unsigned char)*(--p)))
        *p = '\0'; // Strip trailing whitespace
    return s;
}

char* stripDoubleQuotesInplace(char* s)
{
    char *p = s;
    char *q = s;
    int len = strlen(s);

    // If the string is empty or doesn't start/end with a quote, do nothing
    if (len == 0)
        return s;

    // Skip leading double quotes
    while (*q == '"')
        ++q;

    // Copy the rest of the string forward
    while (*q)
        *p++ = *q++;
    *p = '\0'; // Null-terminate after copying

    // Remove trailing double quotes
    if (p > s)
    {
        while (*--p == '"' && p >= s)
            *p = '\0';
    }
    return s;
}

struct StringTable* createStringTable()
{
    struct StringTable* table = (struct StringTable*)malloc(sizeof(struct StringTable));
    table->lines = NULL;
    table->nLine = 0;
    return table;
}

int addString(struct StringTable* table, const char* s)
{
    table->nLine++;
    table->lines = (char**)realloc(table->lines, table->nLine * sizeof(char*));
    table->lines[table->nLine - 1] = strdup(s);
    return 1;
}

int destroyStringTable(struct StringTable* table)
{
    if (table->lines)
    {
        for (uint32_t i = 0; i < table->nLine; ++i)
            free(table->lines[i]);
        free(table->lines);
    }
    free(table);
    return 1;
}

void printStringTable(struct StringTable* table)
{
    for (uint32_t i = 0; i < table->nLine; ++i)
    {
        printf("%s\n", table->lines[i]);
    }
    puts("---------------------------------");
}

char *stringReplace2(char *orig, char *rep, char *with)
{
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}