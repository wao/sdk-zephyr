#ifndef __NVSFS_H 
#define __NVSFS_H

#include <zephyr/kernel.h>

int nvsfs_init(void);
ssize_t nvsfs_read(uint16_t id, void *data, size_t len);
ssize_t nvsfs_write(uint16_t id, const void *data, size_t len);
int nvsfs_delete(uint16_t id);

#endif
