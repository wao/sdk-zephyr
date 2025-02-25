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
#include <zephyr/debug/thread_analyzer.h>
#include "led.h"
#include "bat.h"
#include "zephyr/bluetooth/hci_types.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(blekey, LOG_LEVEL_DBG);


/* Custom Service Variables */
#define BT_UUID_CUSTOM_SERVICE_VAL 0xFFE0

//static const struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
//	BT_UUID_CUSTOM_SERVICE_VAL);
static const struct bt_uuid_16 vnd_uuid = BT_UUID_INIT_16(BT_UUID_CUSTOM_SERVICE_VAL);

static const struct bt_uuid_16 vnd_enc_uuid = BT_UUID_INIT_16(0xFFE1);
static const struct bt_uuid_16 vnd_enc2_uuid = BT_UUID_INIT_16(0xFFE3);

#define VND_MAX_LEN 1

static uint8_t vnd_value[1] = {0};
static uint8_t vnd2_value[1] = {0};

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

static void vnd2_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
  LOG_INF("ccc2 changed %d", value);
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(vnd_svc,
	BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(&vnd_enc2_uuid.uuid,
			       BT_GATT_CHRC_READ | 
			       //BT_GATT_CHRC_WRITE | 
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
             //|
			       //BT_GATT_PERM_WRITE,
			       read_vnd, write_vnd, vnd2_value),
	BT_GATT_CCC(vnd2_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
  //For a defact in Y6000 code, this character must be in last one in services list
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | 
			       //BT_GATT_CHRC_WRITE | 
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
             //|
			       //BT_GATT_PERM_WRITE,
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


struct bt_conn_info info;
static bool s_connected = false;
static bt_addr_le_t addr = { 0, { 0xfc, 0xdb, 0x20, 0x6a, 0x70, 0x20 }};

static void dump_conn_info(){
  if(s_connected){
      char le_addr[BT_ADDR_LE_STR_LEN];
      bt_addr_le_to_str(info.le.remote, le_addr, sizeof(le_addr));
      uint8_t * ap = &(info.le.remote->a.val);
      LOG_INF("Connected to %s 0x%x - 0x%0x%0x%0x%0x%0x%0x\n", le_addr, info.le.remote->type, *ap, *(ap+1), *(ap+2), *(ap+3), *(ap+4), *(ap+5));
  }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err 0x%02x)\n", err);
	} else {
    led_set(LED_SLOW_FLASH);

    bt_conn_get_info(conn, &info);
    s_connected = true;
    dump_conn_info();

#if 0
    if (!bt_addr_le_eq(&addr, info.le.remote)) {
      LOG_ERR("Not expected device, reject connection");
      int ret = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN );
      if (ret) {
        LOG_INF("Disconnecting failed to (err %d)\n", ret);
        return;
      }
    }
#endif
  } 
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
  s_connected = false;
  led_set(LED_OFF);
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

#if 0
static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

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


typedef struct _gpio_key_control {
  struct gpio_dt_spec key;
  struct gpio_callback cb;
} gpio_key_control;

gpio_key_control key1 =  { .key = GPIO_DT_SPEC_GET(DT_NODELABEL(key1), gpios) };

gpio_key_control sw_keys[] = {
  { .key = GPIO_DT_SPEC_GET(DT_NODELABEL(sw1), gpios) },
  { .key = GPIO_DT_SPEC_GET(DT_NODELABEL(sw2), gpios) },
  { .key = GPIO_DT_SPEC_GET(DT_NODELABEL(sw3), gpios) },
  { .key = GPIO_DT_SPEC_GET(DT_NODELABEL(key2), gpios) },
};

const struct gpio_dt_spec sw_output_gpio = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), sw_output_gpios);

K_SEM_DEFINE(key1_sem, 0, 4);

void key1_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	LOG_INF("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
  k_sem_give(&key1_sem);
}


void config_key_control(gpio_key_control * key)
{
  int ret = gpio_pin_configure_dt(&key->key, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, key->key.port->name, key->key.pin);
    return;
	}

  ret = gpio_pin_interrupt_configure_dt(&key->key,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, key->key.port->name, key->key.pin);
    return;
	}

	gpio_init_callback(&key->cb, key1_pressed, BIT(key->key.pin));
	gpio_add_callback(key->key.port, &key->cb);
	LOG_INF("Set up button at %s pin %d\n", key->key.port->name, key->key.pin);
}

#define SW_KEY_NUM 4

void config_sw_keys(void){
  int ret = gpio_pin_configure_dt(&sw_output_gpio, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, sw_output_gpio.port->name, sw_output_gpio.pin);
    return;
	}

  gpio_pin_set_dt(&sw_output_gpio, 1);

  for( int i = 0; i < SW_KEY_NUM; ++i ){
    config_key_control(&sw_keys[i]);
  }
}

static int update_count = 0;


void update_key1_value(uint8_t value) {
    vnd_value[0] = value;
    LOG_INF("Bluetooth run %d %d", update_count, vnd_value[0]);
    bt_gatt_notify(NULL, &vnd_svc.attrs[5], vnd_value, 1);
    if (value == 0) {
      led_restore();
    } else {
      led_set(LED_ON);
    }
}

void update_sw_value(uint8_t value) {
    vnd2_value[0] = value;
    LOG_INF("Bluetooth run %d %d", update_count, vnd2_value[0]);
    bt_gatt_notify(NULL, &vnd_svc.attrs[1], vnd2_value, 1);
}

static uint8_t key1_value = 0;
static uint8_t sw_value[] = { 0, 0, 0, 0 };

uint8_t cacluate_sw_value(void) {
  uint8_t ret = 0;
  for(int i = 0; i < SW_KEY_NUM; ++i){
    ret = 2 * ret + sw_value[SW_KEY_NUM-1-i];
  }
  return ret;
}

uint8_t read_sw_value(void){
  for( int i = 0; i < SW_KEY_NUM; ++i ){
    sw_value[i] = gpio_pin_get_dt(&sw_keys[i].key);
  }

  return cacluate_sw_value();
}


int main(void)
{
	struct bt_gatt_attr *vnd_ind_attr;
	char str[BT_UUID_STR_LEN];
	int err;

  led_init();
  config_key_control(&key1);
  config_sw_keys();

  led_set(LED_OFF);
  k_sem_take(&key1_sem, K_FOREVER);

  bat_init();

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

  bat_read();

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */

  update_key1_value(key1_value);
  update_sw_value(read_sw_value());
  led_set(LED_QUICK_FLASH);
  
	while (1) {
    k_sem_take(&key1_sem, K_FOREVER);
    update_count ++;
    dump_conn_info();
    bat_read();

		/* Current Time Service updates only when time is changed */
		//cts_notify();

		/* Heartrate measurements simulation */
		//hrs_notify();

		/* Battery level simulation */
		//bas_notify();

    key1_value = gpio_pin_get_dt(&key1.key);
    if( key1_value != vnd_value[0] ){
      update_key1_value(key1_value);
    }
    {
      uint8_t sw_value = read_sw_value();
      if (sw_value != vnd2_value[0]) {
        update_sw_value(sw_value);
      }
    }
    thread_analyzer_print();
	}
	return 0;
}
