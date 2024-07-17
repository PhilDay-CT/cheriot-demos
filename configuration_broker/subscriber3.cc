// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

// Contributed by Configured Things Ltd

#include <compartment.h>
#include <debug.hh>
#include <fail-simulator-on-error.h>
#include <multiwaiter.h>
#include <thread.h>

// Define a sealed capability that gives this compartment
// read access to configuration data "config1" and "config2"
#include "config_broker.h"
#include "token.h"
#define CONFIG1 "config1"
DEFINE_READ_CONFIG_CAPABILITY(CONFIG1)
#define CONFIG2 "config2"
DEFINE_READ_CONFIG_CAPABILITY(CONFIG2)

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Subscriber #3">;

#include "logger/logger.h"

#include "data.h"

// Keep track of the items and their last version
struct Config
{
	SObj        capability;      // Sealed Read Capability
	uint32_t    version;
	std::atomic<uint32_t> *versionFutex;  
	void        (*handler)(void *); // Handler to call
};

void logger_handler(void *d) {
	Debug::log("logger_handler called");
	logger_config((LoggerConfig *)d);
}

void led_handler(void *d) {
	Debug::log("led_handler called");
}

//
// Thread entry point.
//
void __cheri_compartment("subscriber3") init()
{
	// List of configuration items we are tracking
	Config configItems[] = {
	  {READ_CONFIG_CAPABILITY(CONFIG1), 0, nullptr, logger_handler},
	  {READ_CONFIG_CAPABILITY(CONFIG2), 0, nullptr, led_handler},
	};

	auto numOfItems = sizeof(configItems) / sizeof(configItems[0]);

	// Create the multi waiter
	struct MultiWaiter *mw = nullptr;
	Timeout             t1{MS_TO_TICKS(1000)};
	multiwaiter_create(&t1, MALLOC_CAPABILITY, &mw, 2);
	if (mw == nullptr)
	{
		Debug::log("thread {} failed to create multiwaiter", thread_id_get());
		return;
	}

	// Initial read of the config item to get the current value (if any)
	// and the version & futex to wait on
	for (auto &c : configItems)
	{
		auto item = get_config(c.capability);
		if (item.versionFutex == nullptr)
		{
			Debug::log(
			  "thread {} failed to get {}", thread_id_get(), c.capability);
			return;
		}

		c.version = item.version;
		c.versionFutex = item.versionFutex;

		Debug::log("thread {} got version:{} of {}",
		           thread_id_get(),
		           c.version,
		           item.id);
		if (item.data != nullptr) {
			c.handler(item.data);
		}
		else
		{
			Debug::log("No data yet for {}", item.id);
		}	
	}

	// Loop waiting for config changes
	while (true)
	{
		// Create a set of wait events
		struct EventWaiterSource events[numOfItems];

		for (auto i = 0; i < numOfItems; i++)
		{
			events[i] = {configItems[i].versionFutex,
			             EventWaiterFutex,
			             configItems[i].version};
		}

		Timeout t{MS_TO_TICKS(10000)};
		if (multiwaiter_wait(&t, mw, events, 2) == 0)
		{
			// find out which value changed
			for (auto i = 0; i < numOfItems; i++)
			{
				if (events[i].value == 1)
				{
					auto c     = &configItems[i];
					auto item  = get_config(c->capability);
					Debug::log("thread {} got version:{} of {}",
					           thread_id_get(),
					           item.version,
					           item.id);

					c->version = item.version;
					c->handler(item.data);
				}
			}
		}
		else
		{
			Debug::log("thread {} wait timeout", thread_id_get());
		}

		// Check we're not leaking data;
		// Debug::log("heap quota available: {}",
		//   heap_quota_remaining(MALLOC_CAPABILITY));
	}
}
