#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "led.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led, LOG_LEVEL_DBG);

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);
static led_mode_t s_prev_mode = LED_OFF;
static led_mode_t s_curr_mode = LED_OFF;
static int s_timer_event_count = 0;

static void turn_on_led(){
   gpio_pin_set_dt(&led, 1);
}

static void turn_off_led(){
   gpio_pin_set_dt(&led, 0);
}


static void timer_handler(struct k_timer *dummy)
{
  s_timer_event_count++;
  if (s_curr_mode==LED_SLOW_FLASH){
    int v = s_timer_event_count % 5;
    if (v==1) {
      turn_off_led();
    } else if (v==0) {
      turn_on_led();
    }
  }else{
    gpio_pin_toggle_dt(&led);
  }
}

K_TIMER_DEFINE(s_timer, timer_handler, NULL);
static bool s_is_timer_on = false;

static void start_timer(bool is_slow){
  if (s_is_timer_on) {
     LOG_ERR("Led timer has already turned on");
     k_timer_stop(&s_timer);
  }

  if (is_slow) {
    k_timer_start(&s_timer, K_MSEC(1000), K_MSEC(1000));
  } else {
    k_timer_start(&s_timer, K_MSEC(200), K_MSEC(200));
  }

  s_is_timer_on = true;
}

static void stop_timer(void){
  if (!s_is_timer_on) {
     LOG_ERR("Led timer has already turned off");
  } else {
   k_timer_stop(&s_timer);
  }

  s_is_timer_on = false;
}

void led_init(void) {
  	int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			LOG_ERR("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
		}
}


static void enable_mode(led_mode_t mode) {
  switch(mode){
    case LED_ON:
      turn_on_led();
      break;

    case LED_QUICK_FLASH:
      turn_on_led();
      start_timer(false);
      break;

    case LED_SLOW_FLASH:
      turn_on_led();
      start_timer(true);
      break;

    case LED_OFF:
      /* passthrough */
    default:
      /* all invalid value will be treated as off */
      turn_off_led();
  }
}

static void disable_mode(led_mode_t mode) {
  switch(mode){
    case LED_QUICK_FLASH:
      /* passthrough */
    case LED_SLOW_FLASH:
      stop_timer();
      turn_off_led();
      break;

    default:
      /*do nothing*/
      NULL;
  }
}

void led_set(led_mode_t mode) {
  s_prev_mode = s_curr_mode;
  disable_mode(s_curr_mode);
  s_curr_mode = mode;
  enable_mode(mode);
}

void led_restore(void) {
  disable_mode(s_curr_mode);
  s_curr_mode = s_prev_mode;
  enable_mode(s_curr_mode);
  s_prev_mode = LED_OFF;
}
