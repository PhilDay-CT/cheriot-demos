// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <debug.hh>

#include "rgb_led.h"
#include <platform-rgbctrl.hh>

// Expose debugging features unconditionally for this library.
using Debug = ConditionalDebug<false, "RGB LED">;


/**
 * Configure the LEDs
 * In this demo it just prints the config value
 */
void __cheri_libcall rgb_led_config(void *c)
{
	auto *config = (rgbLed::Config *)c;
	auto driver = MMIO_CAPABILITY(SonataRgbLedController, rgbled);

	Debug::log("LED 0 red: {} green: {} blue: {}",
	           config->led0.red,
	           config->led0.green,
	           config->led0.blue);
	Debug::log("LED 1 red: {} green: {} blue: {}",
	           config->led1.red,
	           config->led1.green,
	           config->led1.blue);

	driver->rgb(SonataRgbLed::Led0, config->led0.red, config->led0.green, config->led0.blue);
	driver->rgb(SonataRgbLed::Led1, config->led1.red, config->led1.green, config->led1.blue);
	driver->update();
}
