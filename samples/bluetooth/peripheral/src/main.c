/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

//#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
//#include <zephyr/bluetooth/services/bas.h>
//#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/drivers/gpio.h>

#include "cts.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blekey, LOG_LEVEL_DBG);

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);

/* Custom Service Variables */
#define BT_UUID_CUSTOM_SERVICE_VAL 0xFFE0

//static const struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
//	BT_UUID_CUSTOM_SERVICE_VAL);
static const struct bt_uuid_16 vnd_uuid = BT_UUID_INIT_16(BT_UUID_CUSTOM_SERVICE_VAL);

static const struct bt_uuid_16 vnd_enc_uuid = BT_UUID_INIT_16(0xFFE1);

#define VND_MAX_LEN 1

static uint8_t vnd_value[1] = {0};

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

  LOG_INF("read_vnd");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 1);
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;
  LOG_INF("write_vnd");

	if (offset + len > VND_MAX_LEN) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	value[offset + len] = 0;

	return len;
}

static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
  LOG_INF("ccc changed %d", value);
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(vnd_svc,
	BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | 
			       BT_GATT_CHRC_WRITE | 
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ |
			       BT_GATT_PERM_WRITE,
			       read_vnd, write_vnd, vnd_value),
	BT_GATT_CCC(vnd_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_CUSTOM_SERVICE_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_INF("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

static uint8_t gatt_connected = 1;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err 0x%02x)\n", err);
	} else {
    gatt_connected = 0;
    gpio_pin_set_dt(&led, gatt_connected);
		LOG_INF("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
  gatt_connected = 1;
  gpio_pin_set_dt(&led, gatt_connected);
	LOG_INF("Disconnected (reason 0x%02x)\n", reason);
}

static void alert_stop(void)
{
	LOG_INF("Alert stopped\n");
}

static void alert_start(void)
{
	LOG_INF("Mild alert started\n");
}

static void alert_high_start(void)
{
	LOG_INF("High alert started\n");
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.mild_alert = alert_start,
	.high_alert = alert_high_start,
};

static void bt_ready(void)
{
	int err;

	LOG_INF("Bluetooth initialized\n");

	//cts_init();

	//if (IS_ENABLED(CONFIG_SETTINGS)) {
//		settings_load();
	//}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_INF("Advertising failed to start (err %d)\n", err);
		return;
	}

	LOG_INF("Advertising successfully started\n");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

#if 0
static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}
#endif


static void config_led(void)
{
  	int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			LOG_ERR("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
		}
}

static struct gpio_dt_spec key1 = GPIO_DT_SPEC_GET(DT_NODELABEL(key1), gpios);
static struct gpio_callback key1_cb_data;

K_SEM_DEFINE(key1_sem, 1, 1);

void key1_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	LOG_INF("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
  k_sem_give(&key1_sem);
}

void config_key1(void)
{
  int ret = gpio_pin_configure_dt(&key1, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, key1.port->name, key1.pin);
    return;
	}

  ret = gpio_pin_interrupt_configure_dt(&key1,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, key1.port->name, key1.pin);
    return;
	}

	gpio_init_callback(&key1_cb_data, key1_pressed, BIT(key1.pin));
	gpio_add_callback(key1.port, &key1_cb_data);
	LOG_INF("Set up button at %s pin %d\n", key1.port->name, key1.pin);
}

int main(void)
{
	struct bt_gatt_attr *vnd_ind_attr;
	char str[BT_UUID_STR_LEN];
	int err;

  config_led();
  config_key1();

  gpio_pin_set_dt(&led, gatt_connected);

  int i = 0;
  LOG_INF("Bluetooth init %d", i++);
  k_sleep(K_SECONDS(1));

	err = bt_enable(NULL);
	if (err) {
		LOG_INF("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_ready();

	bt_gatt_cb_register(&gatt_callbacks);
	//bt_conn_auth_cb_register(&auth_cb_display);

	vnd_ind_attr = bt_gatt_find_by_uuid(vnd_svc.attrs, vnd_svc.attr_count,
					    &vnd_enc_uuid.uuid);
	bt_uuid_to_str(&vnd_enc_uuid.uuid, str, sizeof(str));
	LOG_INF("Indicate VND attr %p (UUID %s)\n", vnd_ind_attr, str);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
    k_sem_take(&key1_sem, K_FOREVER);

		/* Current Time Service updates only when time is changed */
		//cts_notify();

		/* Heartrate measurements simulation */
		//hrs_notify();

		/* Battery level simulation */
		//bas_notify();

    vnd_value[0] = gpio_pin_get_dt(&key1);
    LOG_INF("Bluetooth run %d %d", i++, vnd_value[0]);
    bt_gatt_notify(NULL, &vnd_svc.attrs[1], vnd_value, 1);
    gpio_pin_set_dt(&led, vnd_value[0] );
	}
	return 0;
}
