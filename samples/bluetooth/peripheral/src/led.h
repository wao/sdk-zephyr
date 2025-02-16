#ifndef __LED_H 
#define __LED_H

#include <stdbool.h>

typedef enum tag_led_mode {
  LED_OFF,
  LED_ON,
  LED_SLOW_FLASH,
  LED_QUICK_FLASH
} led_mode_t;

void led_init();
void led_set(led_mode_t mode);
void led_restore();

#endif
