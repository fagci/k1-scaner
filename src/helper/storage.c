#include "storage.h"
#include "../driver/lfs.h"
#include "../external/printf/printf.h"
#include <string.h>

static uint8_t file_buffer[256];
static uint8_t temp_buf[32];

bool Storage_Init(const char *name, size_t item_size, uint16_t max_items) {
    lfs_file_t file;
    struct lfs_file_config config = {.buffer = file_buffer, .attr_count = 0};
    if (lfs_file_exists(name)) return false;
    int err = lfs_file_opencfg(&gLfs, &file, name, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC, &config);
    if (err < 0) return false;
    uint32_t total_size = max_items * item_size;
    uint32_t written = 0;
    memset(temp_buf, 0, sizeof(temp_buf));
    while (written < total_size) {
        size_t to_write = total_size - written;
        if (to_write > sizeof(temp_buf)) to_write = sizeof(temp_buf);
        lfs_ssize_t result = lfs_file_write(&gLfs, &file, temp_buf, to_write);
        if (result != (lfs_ssize_t)to_write) { lfs_file_close(&gLfs, &file); return false; }
        written += to_write;
    }
    lfs_file_close(&gLfs, &file);
    return true;
}

bool Storage_Save(const char *name, uint16_t num, const void *item, size_t item_size) {
    lfs_file_t file;
    struct lfs_file_config config = {.buffer = file_buffer, .attr_count = 0};
    int err = lfs_file_opencfg(&gLfs, &file, name, LFS_O_RDWR | LFS_O_CREAT, &config);
    if (err < 0) return false;
    lfs_soff_t file_size = lfs_file_size(&gLfs, &file);
    uint32_t offset = num * item_size;
    uint32_t required_size = offset + item_size;
    if (required_size > (uint32_t)file_size) {
        lfs_file_seek(&gLfs, &file, 0, LFS_SEEK_END);
        uint32_t to_extend = required_size - file_size;
        memset(temp_buf, 0, sizeof(temp_buf));
        while (to_extend > 0) {
            size_t chunk = to_extend; if (chunk > sizeof(temp_buf)) chunk = sizeof(temp_buf);
            lfs_ssize_t w = lfs_file_write(&gLfs, &file, temp_buf, chunk);
            if (w != (lfs_ssize_t)chunk) { lfs_file_close(&gLfs, &file); return false; }
            to_extend -= chunk;
        }
    }
    lfs_file_seek(&gLfs, &file, offset, LFS_SEEK_SET);
    lfs_ssize_t written = lfs_file_write(&gLfs, &file, item, item_size);
    lfs_file_close(&gLfs, &file);
    return written == (lfs_ssize_t)item_size;
}

bool Storage_Load(const char *name, uint16_t num, void *item, size_t item_size) {
    lfs_file_t file;
    struct lfs_file_config config = {.buffer = file_buffer, .attr_count = 0};
    int err = lfs_file_opencfg(&gLfs, &file, name, LFS_O_RDONLY, &config);
    if (err < 0) return false;
    lfs_soff_t file_size = lfs_file_size(&gLfs, &file);
    uint32_t offset = num * item_size;
    if (offset + item_size > (uint32_t)file_size) { lfs_file_close(&gLfs, &file); return false; }
    lfs_file_seek(&gLfs, &file, offset, LFS_SEEK_SET);
    lfs_ssize_t read = lfs_file_read(&gLfs, &file, item, item_size);
    lfs_file_close(&gLfs, &file);
    return read == (lfs_ssize_t)item_size;
}

bool Storage_Exists(const char *name) {
    struct lfs_info info;
    return lfs_stat(&gLfs, name, &info) == 0;
}
