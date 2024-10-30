// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdint>
#include <cstdlib>
#include <debug.hh>
#include <fail-simulator-on-error.h>
#include <thread.h>
#include <token.h>

// Define a sealed capability that gives this compartment
// read access to configuration data "logger" and "rgb_led"
#include "common/config_broker/config_broker.h"

#define RGB_LED_CONFIG "rgb_led"
DEFINE_READ_CONFIG_CAPABILITY(RGB_LED_CONFIG)


// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Consumer #1">;

#include "config/include/rgb_led.h"
#include <platform-rgbctrl.hh>

#include "common/config_consumer/config_consumer.h"

namespace
{
	/**
	 * Handle updates to the RGB LED configuration
	 */
	int led_handler(void *newConfig)
	{
		// Note the consumer helper will have already made a 
		// fast claim on the new config value, and we only
		// need it for the duration of this call
		
		// Process the configuration
		auto config = static_cast<rgbLed::Config *>(newConfig);
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

		return 0;
	}

} // namespace

/**
 * Thread entry point.  The waits for changes to one
 * or more configuration values and then calls the
 * appropriate handler.
 */
void __cheri_compartment("rgb_led") init()
{
	/// List of configuration items we are tracking
	ConfigConsumer::ConfigItem configItems[] = {
	  {READ_CONFIG_CAPABILITY(RGB_LED_CONFIG), led_handler, 0, nullptr},
	};

	size_t numOfItems = sizeof(configItems) / sizeof(configItems[0]);

	ConfigConsumer::run(configItems, numOfItems);
}
