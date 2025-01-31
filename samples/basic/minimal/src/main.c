/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(minimal, LOG_LEVEL_DBG);


int main(void)
{
  while(1){
    LOG_INF("Hello World from minimal!\n");
    k_sleep(K_SECONDS(5));
  }

	return 0;
}
