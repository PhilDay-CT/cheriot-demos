// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdlib>
#include <debug.hh>
#include <fail-simulator-on-error.h>
#include <multiwaiter.h>
#include <thread.h>
#include <token.h>

// Define sealed capabilities that gives this compartment
// read access to configuration data "logger" and "user_led"
#include "common/config_broker/config_broker.h"

#define USER_LED_CONFIG "user_led"
DEFINE_READ_CONFIG_CAPABILITY(USER_LED_CONFIG)

#define LOGGER_CONFIG "logger"
DEFINE_READ_CONFIG_CAPABILITY(LOGGER_CONFIG)

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Consumer #2">;

#include "config/include/logger.h"
#include "config/include/user_led.h"

#include "common/config_consumer/config_consumer.h"

namespace
{
	
	static LoggerConfig *config;

	/**
	 * Handle updates to the logger configuration
	 */
	int logger_handler(void *newConfig)
	{
		
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

		// Process the configuration change
		Debug::log("Configured with host: {} port: {} level: {}",
	           (const char *)config->host.address,
	           (int16_t)config->host.port,
	           config->level);

		if (oldConfig)
		{
			free(oldConfig);
		}
		return 0;
	}

	/**
	 * Handle updates to the User LED configuration
	 */
	int user_led_handler(void *newConfig)
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
void __cheri_compartment("consumer2") init()
{
	// List of configuration items we are tracking
	ConfigConsumer::ConfigItem configItems[] = {
	  {READ_CONFIG_CAPABILITY(LOGGER_CONFIG), logger_handler, 0, nullptr,},
	  {READ_CONFIG_CAPABILITY(USER_LED_CONFIG), user_led_handler, 0, nullptr},
	};

	size_t numOfItems = sizeof(configItems) / sizeof(configItems[0]);

	ConfigConsumer::run(configItems, numOfItems);
}
