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
#include "../config_broker/config_broker.h"

#define RGB_LED_CONFIG "rgb_led"
DEFINE_READ_CONFIG_CAPABILITY(RGB_LED_CONFIG)

#define LOGGER_CONFIG "logger"
DEFINE_READ_CONFIG_CAPABILITY(LOGGER_CONFIG)

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Consumer #1">;

#include "../include/logger.h"
#include "../include/rgb_led.h"

#include "../config_consumer/config_consumer.h"

namespace
{
	/**
	 * Handle updates to the logger configuration
	 */
	int logger_handler(void *newConfig)
	{
		static LoggerConfig *config;

		// Claim the config against our heap quota to ensure
		// it remains available, as the broker will free it
		// when it gets a new value.
		if (heap_claim(MALLOC_CAPABILITY, newConfig) == 0)
		{
			Debug::log("Failed to claim {}", newConfig);
			return -1;
		}

		auto oldConfig = config;
		config         = static_cast<LoggerConfig *>(newConfig);
		logger_config(config);
		if (oldConfig)
		{
			free(oldConfig);
		}
		return 0;
	}

	/**
	 * Handle updates to the RGB LED configuration
	 */
	int led_handler(void *newConfig)
	{
		// Claim the config against our heap quota to ensure
		// it remains available, as the broker will free it
		// when it gets a new value.
		//
		// The call to configure the led might be into another
		// compartment so we can't rely on the fast claim
		if (heap_claim(MALLOC_CAPABILITY, newConfig) == 0)
		{
			Debug::log("Failed to claim {}", newConfig);
			return -1;
		}

		// Configure the controller
		auto config = static_cast<rgbLed::Config *>(newConfig);
		rgb_led_config(config);

		// We can assume the controller has used these values
		// now so just release our claim on the config
		free(newConfig);

		return 0;
	}

} // namespace

/**
 * Thread entry point.  The waits for changes to one
 * or more configuration values and then calls the
 * appropriate handler.
 */
void __cheri_compartment("consumer1") init()
{
	/// List of configuration items we are tracking
	ConfigConsumer::ConfigItem configItems[] = {
	  {READ_CONFIG_CAPABILITY(LOGGER_CONFIG), logger_handler,  0, nullptr},
	  {READ_CONFIG_CAPABILITY(RGB_LED_CONFIG), led_handler, 0, nullptr},
	};

	size_t numOfItems = sizeof(configItems) / sizeof(configItems[0]);

	ConfigConsumer::run(configItems, numOfItems);
}
