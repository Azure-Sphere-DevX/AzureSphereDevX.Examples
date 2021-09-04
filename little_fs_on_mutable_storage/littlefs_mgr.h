#pragma once

#include "lfs.h"
#include "lfs_util.h"
#include "unistd.h"



extern int mutableStorageFd;

// Forward declarations

int storage_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int storage_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int storage_erase(const struct lfs_config *c, lfs_block_t block);
int storage_sync(const struct lfs_config *c);

