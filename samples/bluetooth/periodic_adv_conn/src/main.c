/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>

#define NUM_RSP_SLOTS	 1 
#define NUM_SUBEVENTS	 1 
#define SUBEVENT_INTERVAL 30

/*
static const struct bt_le_per_adv_param per_adv_params = {
	.interval_min = 32,
	.interval_max = 32,
	.options = 0,
	.num_subevents = NUM_SUBEVENTS,
	.subevent_interval = 28,
	.response_slot_delay = 16,
	.response_slot_spacing = 50, 
	.num_response_slots = NUM_RSP_SLOTS,
};
*/

static const struct bt_le_per_adv_param per_adv_params = {
	.interval_min = 32,
	.interval_max = 32,
	.options = 0,
	.num_subevents = 1,
	.subevent_interval = 28,
	.response_slot_delay = 16,
	.response_slot_spacing = 50,
	.num_response_slots = 2,
};

static struct bt_le_per_adv_subevent_data_params subevent_data_param;
static struct net_buf_simple data_buf;
static uint32_t pkt_count = 0;


static void request_cb(struct bt_le_ext_adv *adv, const struct bt_le_per_adv_data_request *request)
{
	int err;

	/* Continuously send the same dummy data and listen to all response slots */

  pkt_count += 1;

  printk("request %d to %d at %d\n", request->start, request->count, pkt_count);



		subevent_data_param.subevent = 0;
		subevent_data_param.response_slot_start = 0;
		subevent_data_param.response_slot_count = 1;
		subevent_data_param.data = &data_buf;

	err = bt_le_per_adv_set_subevent_data(adv, 1, &subevent_data_param);
	if (err) {
		printk("Failed to set subevent data (err %d)\n", err);
	}
}

static bool get_address(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;

	if (data->type == BT_DATA_LE_BT_DEVICE_ADDRESS) {
		memcpy(addr->a.val, data->data, sizeof(addr->a.val));
		addr->type = data->data[sizeof(addr->a)];

		return false;
	}

	return true;
}

static struct bt_conn *default_conn;

static void response_cb(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
			struct net_buf_simple *buf)
{
	bt_addr_le_t peer;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (!buf) {
		return;
	}

	bt_addr_le_copy(&peer, &bt_addr_le_none);
	bt_data_parse(buf, get_address, &peer);
	if (bt_addr_le_eq(&peer, &bt_addr_le_none)) {
		/* No address found */
		return;
	}

	bt_addr_le_to_str(&peer, addr_str, sizeof(addr_str));
	printk("Address %s in subevent %d\n", addr_str, info->subevent);
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.pawr_data_request = request_cb,
	.pawr_response = response_cb,
};

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	printk("Connected (err 0x%02X)\n", err);

	__ASSERT(conn == default_conn, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);

	__ASSERT(conn == default_conn, "Unexpected disconnected callback");

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static void init_bufs(void)
{
		net_buf_simple_init_with_data(&data_buf, &pkt_count, sizeof(pkt_count));
}

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/*
struct bt_le_adv_param adv_param = { BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CODED, 
    BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL) };
    */
/*
struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0U,
		.secondary_max_skip = 0U,
		.options = (BT_LE_ADV_OPT_EXT_ADV |
			    BT_LE_ADV_OPT_CONNECTABLE |
			    BT_LE_ADV_OPT_CODED),
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
		.peer = NULL,
	};
  */

int main(void)
{
	int err;
	struct bt_le_ext_adv *pawr_adv;

	init_bufs();

	printk("Starting Periodic Advertising Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

  //adv_param.options = adv_param.options | BT_LE_ADV_OPT_CODED;

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CODED_NCONN, &adv_cb, &pawr_adv);
	//err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, &adv_cb, &pawr_adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(pawr_adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(pawr_adv, &per_adv_params);
	if (err) {
		printk("Failed to set periodic advertising parameters (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(pawr_adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	printk("Start Periodic Advertising\n");
	err = bt_le_ext_adv_start(pawr_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	while (true) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
