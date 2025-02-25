#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include "bat.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bat, LOG_LEVEL_DBG);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

void bat_init(void) {
  int err;

  if (!adc_is_ready_dt(&adc_channels[0])) {
    LOG_ERR("Battery adc %s is not ready", adc_channels[0].dev->name); 
    return;
  }

  err = adc_channel_setup_dt(&adc_channels[0]);
  if (err<0) {
    LOG_ERR("Set up battery adc channel error %d", err);
  }
}

int bat_read(void) {
  int err;
  int32_t val_mv;
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

		(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

			err = adc_read_dt(&adc_channels[0], &sequence);
			if (err < 0) {
				LOG_ERR("Could not read (%d)\n", err);
        return err;
			}

			/*
			 * If using differential mode, the 16 bit value
			 * in the ADC sample buffer should be a signed 2's
			 * complement value.
			 */
			if (adc_channels[0].channel_cfg.differential) {
				val_mv = (int32_t)((int16_t)buf);
			} else {
				val_mv = (int32_t)buf;
			}
      LOG_INF("%"PRId32, val_mv);
			err = adc_raw_to_millivolts_dt(&adc_channels[0],
						       &val_mv);
			/* conversion to mV may not be supported, skip if not */
			if (err < 0) {
				LOG_ERR(" (value in mV not available)\n");
			} else {
				LOG_INF(" = %"PRId32" mV\n", val_mv);
			}
      return val_mv;
}

