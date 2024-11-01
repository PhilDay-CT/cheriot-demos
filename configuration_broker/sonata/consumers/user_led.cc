// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdlib>
#include <debug.hh>
#include <thread.h>
#include <token.h>

// Define sealed capabilities that gives this compartment
// read access to configuration data "logger" and "user_led"
#include "common/config_broker/config_broker.h"

#define USER_LED_CONFIG "user_led"
DEFINE_READ_CONFIG_CAPABILITY(USER_LED_CONFIG)

#include <platform-gpio.hh>

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Consumer #2">;

#include "config/include/user_led.h"

#include "common/config_consumer/config_consumer.h"

namespace
{
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
	 * Handle updates to the User LED configuration
	 */
	int user_led_handler(void *newConfig)
	{
		// Note the consumer helper will have already made a 
		// fast claim on the new config value, and we only
		// need it for the duration of this call
		
		// Configure the controller
		auto config = static_cast<userLed::Config *>(newConfig);
		Debug::log("User LEDs: {} {} {} {} {} {} {} {}",
					config->led0,
					config->led1,
					config->led2,
					config->led3,
					config->led4,
					config->led5,
					config->led6,
					config->led7);

		setLED(0, config->led0);
		setLED(1, config->led1);
		setLED(2, config->led2);
		setLED(3, config->led3);
		setLED(4, config->led4);
		setLED(5, config->led5);
		setLED(6, config->led6);
		setLED(7, config->led7);
		return 0;
	}

} // namespace

/**
 * Thread entry point.  The waits for changes to one
 * or more configuration values and then calls the
 * appropriate handler.
 */
void __cheri_compartment("user_led") init()
{
	// List of configuration items we are tracking
	ConfigConsumer::ConfigItem configItems[] = {
	  {READ_CONFIG_CAPABILITY(USER_LED_CONFIG), user_led_handler, 0, nullptr},
	};

	size_t numOfItems = sizeof(configItems) / sizeof(configItems[0]);

	ConfigConsumer::run(configItems, numOfItems);
}
