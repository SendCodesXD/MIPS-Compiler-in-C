#include "data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct DataTable* createDataTable()
{
    struct DataTable* table = (struct DataTable*)malloc(sizeof(struct DataTable));
    table->nData = 0;
    table->data = NULL;
    return table;
}

int addData(struct DataTable* table, const char* name, uint32_t size, void* dataValue, uint32_t address)
{
    table->nData++;
    table->data = (struct Data*)realloc(table->data, table->nData * sizeof(struct Data));
    table->data[table->nData - 1].name = name ? strdup(name) : NULL;
    table->data[table->nData - 1].size = size;
    table->data[table->nData - 1].hasValue = (dataValue != NULL) ? 1 : 0;
    if (dataValue != NULL)
    {
        void* tmp = malloc(size);
        memcpy(tmp, dataValue, size);
        table->data[table->nData - 1].dataValue = tmp;
    }
    else
        table->data[table->nData - 1].dataValue = NULL;
    table->data[table->nData - 1].address = address;
    return 1;
}

int destroyDataTable(struct DataTable* table)
{
    return 1;
}

void printDataTable(struct DataTable* table)
{
    for (uint32_t i = 0; i < table->nData; ++i)
    {
        struct Data* pData = &table->data[i];
        printf("data: \"%s\", address: 0x%08x, hasValue=%d\n", pData->name ? pData->name : "<none>", pData->address,
            pData->hasValue
        );
    }
    puts("------------------------------------");
}