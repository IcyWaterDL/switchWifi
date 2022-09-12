/*
 * led.h
 *
 *  Created on: Jun 29, 2022
 *      Author: Thanh Vu
 */
#include "../common.h"

#ifndef MAIN_LED_LED_H_
#define MAIN_LED_LED_H_

#define LED_STATUS  2
#define LED_BLINK	4

#define LED_PIN_SEL  		((1ULL << LED_STATUS) | (1ULL << LED_BLINK))

#define LED_ON				1
#define LED_OFF				0

void led_init(void);
void led_status_task(void *arg);

#endif /* MAIN_LED_LED_H_ */
