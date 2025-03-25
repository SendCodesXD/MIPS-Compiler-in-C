#ifndef __DATA_H__
#define __DATA_H__
#include <stdint.h>

// enum DataType
// {
//     dataTypeUnknown,
//     dataTypeUninitialized,
//     dataTypeInitialized
// };

struct Data
{
    char* name;
    uint32_t size;
    uint8_t hasValue;
    uint8_t* dataValue;
    uint32_t address;
};

struct DataTable
{
    uint32_t nData;
    struct Data* data;
};

struct DataTable* createDataTable();
int addData(struct DataTable* table, const char* name, uint32_t size, void* dataValue, uint32_t address);
int destroyDataTable(struct DataTable* table);
void printDataTable(struct DataTable* table);

#endif