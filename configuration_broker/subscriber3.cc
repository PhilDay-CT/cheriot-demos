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

#include "data.h"
#include "sandbox.h"

// Keep track of the items and their last version
struct Config
{
	SObj        capability; // Sealed Read Capability
	ConfigItem *item;       // item from the broker
	uint32_t    version;    // last version from broker
	Data       *data;       // last valid config data
};

//
// Process a change in a configurarion value
//   configData is a pointer to the current value
//   item is the new, untrusted and possibily invaild new version
//
void process_update(Data **configData, ConfigItem *item)
{
	if (item->data != nullptr)
	{
		if (sandbox_validate(item->data) < 0)
		{
			Debug::log("thread {} Validation failed for {}",
			           thread_id_get(),
			           item->data);
		}
		else
		{
			// New value is valid - release our claim on the old value
			if (*configData != nullptr)
			{
				free(*configData);
			}
			*configData = static_cast<Data *>(item->data);

			// Claim the new value so we keep access to it even if
			// the next value from the broker is invalid
			heap_claim(MALLOC_CAPABILITY, *configData);

			// Act on the new value
			;
		}
	}

	// Print the current value
	print_config(item->id, *configData);
}

//
// Thread entry point.
//
void __cheri_compartment("subscriber3") init()
{
	// List of configuration items we are tracking
	Config configItems[] = {
	  {READ_CONFIG_CAPABILITY(CONFIG1), nullptr, 0, nullptr},
	  {READ_CONFIG_CAPABILITY(CONFIG2), nullptr, 0, nullptr},
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
		c.item = get_config(c.capability);
		if (c.item == nullptr)
		{
			Debug::log(
			  "thread {} failed to get {}", thread_id_get(), c.item->id);
			return;
		}

		c.version = c.item->version.load();
		Debug::log("thread {} got version:{} of {}",
		           thread_id_get(),
		           c.version,
		           c.item->id);
		process_update(&c.data, c.item);
	}

	// Loop waiting for config changes
	while (true)
	{
		// Create a set of wait events
		struct EventWaiterSource events[numOfItems];

		for (auto i = 0; i < numOfItems; i++)
		{
			events[i] = {&(configItems[i].item->version),
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
					c->item    = get_config(c->capability);
					c->version = c->item->version.load();
					Debug::log("thread {} got version:{} of {}",
					           thread_id_get(),
					           c->version,
					           c->item->id);

					process_update(&c->data, c->item);
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
