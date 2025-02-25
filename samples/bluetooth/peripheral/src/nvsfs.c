#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include "nvsfs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nvs, LOG_LEVEL_DBG);

static struct nvs_fs fs;

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

int nvsfs_init(void) {
  int rc;
  struct flash_pages_info info;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors
	 *	starting at NVS_PARTITION_OFFSET
	 */
	fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(fs.flash_device)) {
		LOG_ERR("Flash device %s is not ready\n", fs.flash_device->name);
		return 0;
	}
	fs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);
	if (rc) {
		LOG_ERR("Unable to get page info, rc=%d\n", rc);
		return rc;
	}
	fs.sector_size = info.size;
	fs.sector_count = 3U;

	rc = nvs_mount(&fs);
	if (rc) {
		LOG_ERR("Flash Init failed, rc=%d\n", rc);
		return rc;
	}

  return 0;
}

ssize_t nvsfs_read(uint16_t id, void *data, size_t len) {
  int err = nvs_read(&fs, id, data, len);
  if (err<=0) {
    LOG_ERR("read key %d error code %d", id, err);
  }

  return err;
}

ssize_t nvsfs_write(uint16_t id, const void *data, size_t len) {
  int err = nvs_write(&fs, id, data, len);
  if (err<=0) {
    LOG_ERR("write key %d error code %d", id, err);
  }

  return err;
}

int nvsfs_delete(uint16_t id) {
  int err = nvs_delete(&fs, id);
  if (err<=0) {
    LOG_ERR("Delete key %d error code %d", id, err);
  }

  return err;
}
