#include "littlefs_mgr.h"

/// <summary>
/// Littlefs callback function to handle reads from storage
/// </summary>
int storage_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    if (pread(mutableStorageFd, buffer, size, block * size) < 0) {
        return LFS_ERR_IO;
    } else {
        return LFS_ERR_OK;
    }
}

/// <summary>
///  Littlefs callback function to handle writes to storage
/// </summary>
int storage_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    if (pwrite(mutableStorageFd, buffer, size, block * size) == size) {
        return LFS_ERR_OK;
    } else {
        return LFS_ERR_IO;
    }
}

/// <summary>
/// Littlefs callback function to erase a storage block
/// </summary>
int storage_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint8_t block_data[c->block_size];
    memset(&block_data, 0x00, c->block_size);
    return storage_write(c, block, 0x00, &block_data, c->block_size);
}

/// <summary>
/// Littlefs callback function to sync storage (not used)
/// </summary>
int storage_sync(const struct lfs_config *c)
{
    fsync(mutableStorageFd);
    return LFS_ERR_OK;
}
