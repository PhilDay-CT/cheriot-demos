// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include "cdefs.h"
#include <compartment.h>
#include <debug.hh>
//#include <fail-simulator-on-error.h>
#include <thread.h>
#include <tick_macros.h>

#include "../parser/parser.h"

// Debuging can be enable with "xmake --config --debug-publisher=true"
// using Debug = ConditionalDebug<DEBUG_PUBLISHER, "Publisher">;
using Debug = ConditionalDebug<true, "Publisher">;

//
// Helper for the example to create delay in the publisher
//
static inline void sleep(const uint32_t mS)
{
	Timeout t1{MS_TO_TICKS(mS)};
	thread_sleep(&t1, ThreadSleepNoEarlyWake);
}

#include "../logger/logger.h"

//
// This compartment can set config values "logger" and "rgb_led"
//
#include "../config_broker/config_broker.h"
#define LOGGER_CONFIG "logger"
DEFINE_WRITE_CONFIG_CAPABILITY(LOGGER_CONFIG, sizeof(LoggerConfig))
#define RGB_LED_CONFIG "rgb_led"
DEFINE_WRITE_CONFIG_CAPABILITY(RGB_LED_CONFIG, sizeof(LoggerConfig))

// Foward defignion of the hanlders
void update_logger_config(SObj, const char *);
void update_rgbled_config(SObj, const char *);

// Map of Config values to topics
struct Config
{
	const char *topic;
	SObj        cap;                     // Sealed Write Capability
	void (*handler)(SObj, const char *); // Handler to call
};

// Can't use the macros at the file level to statically
// initialise TopicMap, so do in via a function
Config TopicMap[2];
void   setUpTopicMap()
{
	static bool init = false;
	if (!init)
	{
		TopicMap[0].topic   = "logger";
		TopicMap[0].cap     = WRITE_CONFIG_CAPABILITY(LOGGER_CONFIG);
		TopicMap[0].handler = update_logger_config;

		TopicMap[1].topic   = "rgbled";
		TopicMap[1].cap     = WRITE_CONFIG_CAPABILITY(RGB_LED_CONFIG);
		TopicMap[1].handler = update_rgbled_config;

		init = true;
	}
}

//
// Callback invoked by the Broker when we request to publish a
// new value.  In all cases we have the data already parsed into
// an object on the stack, so this is a simple copy.  Other use
// cases might pull some data from globals and other sources at
// this point.
//
void __cheri_callback publish(void *dst, void *src)
{
	memcpy(dst, src, sizeof(LoggerConfig));
}

//
// Parse and publish a new Logger Config value
//
void update_logger_config(SObj sealedWriteCap, const char *data)
{
	// Do the parse in a sandbox.

	// We passing a pointer to an object on our stack, so it can't
	// be captured, but also stop the parser from reading
	LoggerConfig      config;
	CHERI::Capability woConfig{&config};
	woConfig.permissions() &= {CHERI::Permission::Store};

	if (parseLoggerConfig(data, woConfig) < 0)
	{
		Debug::log("thread {} parser failed", thread_id_get());
		return;
	}

	// Create a read only capability to pass through the broker
	// as context to the publish callback, to ensure that it can't change
	// the data or capture the pointer.
	CHERI::Capability roConfig{&config};
	roConfig.permissions() &= {CHERI::Permission::Load};

	// Call the broker to publish the new value
	if (set_config(sealedWriteCap,
	               sizeof(LoggerConfig),
	               publish,
	               static_cast<void *>(roConfig)) < 0)
	{
		Debug::log("thread {} Failed to set value for {}",
		           thread_id_get(),
		           sealedWriteCap);
	};
};

//
// Parse and publish LED config data
//
void update_rgbled_config(SObj sealedWriteCap, const char *data)
{
	Debug::log("thread {} publish RGB LED {}", thread_id_get(), data);
};

//
// Comparemnt Entry point for the publisher
//
void __cheri_compartment("publisher")
  updateConfig(const char *topic, const char *message)
{
	Debug::log("thread {} got {} on {}", thread_id_get(), message, topic);

	setUpTopicMap();

	bool found = false;
	for (auto t : TopicMap)
	{
		if (strcmp(t.topic, topic) == 0)
		{
			found = true;
			t.handler(t.cap, message);
		}
	}

	if (!found)
	{
		Debug::log(
		  "thread {} Unexpected Message topic {}", thread_id_get(), topic);
	}
};

//
// The following represents a compromised thread in the
// publisher which attempts to publish an invalid value
//
void __cheri_compartment("publisher") bad_dog()
{
	while (true)
	{
		// Oversized value which might expolit a buffer oveflow
		sleep(8000);
		Debug::log("-----------------------------");
		Debug::log(
		  "thread {} Sending oversize data for {}", thread_id_get(), LOGGER_CONFIG);
		void *d1 = malloc(100);
		set_config(WRITE_CONFIG_CAPABILITY(LOGGER_CONFIG), sizeof(d1), publish, d1);
		free(d1);

		// Undersized value which might break the consumer
		sleep(8000);
		Debug::log("-----------------------------");
		Debug::log(
		  "thread {} Sending undersize data for {}", thread_id_get(), LOGGER_CONFIG);
		void *d2 = malloc(4);
		set_config(WRITE_CONFIG_CAPABILITY(LOGGER_CONFIG), sizeof(d2), publish, d2);
		free(d2);
	}
};
