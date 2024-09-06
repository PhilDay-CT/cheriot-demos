// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <debug.hh>

#include "user_led.h"
#include <platform-gpio.hh>

// Expose debugging features unconditionally for this library.
using Debug = ConditionalDebug<false, "USER LED">;


void setLED(int id, userLed::State state)
{
	auto driver = MMIO_CAPABILITY(SonataGPIO, gpio);
	if (state == userLed::State::On) {
		Debug::log("Set LED {} on", id);
		driver->led_on(id);
	}
	else {
		Debug::log("Set LED {} off", id);
		driver->led_off(id);	
	}
}

/**
 * Function which nominally configures the user LEDs
 * In this demo it just prints the config value
 */
void __cheri_libcall user_led_config(void *c)
{
	//auto driver = MMIO_CAPABILITY(SonataGPIO, gpio);

	auto *config = (userLed::Config *)c;
	Debug::log("User LEDs: {} {} {} {} {} {} {} {}",
	           config->led0,
	           config->led1,
	           config->led2,
	           config->led3,
	           config->led4,
	           config->led5,
	           config->led6,
	           config->led7);
	
	int count = 0;
	setLED(0, config->led0);
	setLED(1, config->led1);
	setLED(2, config->led2);
	setLED(3, config->led3);
	setLED(4, config->led4);
	setLED(5, config->led5);
	setLED(6, config->led6);
	setLED(7, config->led7);
}
