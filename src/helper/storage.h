#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool Storage_Init(const char *name, size_t item_size, uint16_t max_items);
bool Storage_Load(const char *name, uint16_t num, void *item, size_t item_size);
bool Storage_Save(const char *name, uint16_t num, const void *item, size_t item_size);
bool Storage_Exists(const char *name);

#endif
