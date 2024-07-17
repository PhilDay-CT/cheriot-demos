// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

// Contributed by Configured Things Ltd

#include "cdefs.h"
#include <compartment.h>
#include <debug.hh>
//#include <fail-simulator-on-error.h>
#include <thread.h>
#include <tick_macros.h>


// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Publisher">;


//
// Helper for the example to create delay in the publisher
//
static inline void sleep(const uint32_t mS)
{
	Timeout t1{MS_TO_TICKS(mS)};
	thread_sleep(&t1, ThreadSleepNoEarlyWake);
}

#include "logger/logger.h"

//
// This compartment can set config values config1 and config2
//
#include "config_broker.h"
#define CONFIG1 "config1"
DEFINE_WRITE_CONFIG_CAPABILITY(CONFIG1, sizeof(LoggerConfig))
#define CONFIG2 "config2"
DEFINE_WRITE_CONFIG_CAPABILITY(CONFIG2, sizeof(LoggerConfig))

//
// Callback invoked by the Broker when we request to publish a
// new value.
//
void __cheri_callback publish(void *dst, void *src) {
	memcpy(dst, src, sizeof(LoggerConfig));
}

LoggerConfig logger = {
	"100.102.103.104",
	666,
	LogLevel::Debug
};

// Helper to set some dummy config
void gen_config(SObj        sealedCap,
                const char *itemName,
                int         count,
                const char *token)
{
	Debug::log("thread {} Set {}", thread_id_get(), itemName);

	static uint8_t level = 0;	
	if (++level > 6) {
		level = 0;
	}

	logger.level = (LogLevel)level;

	// Create a read only capability to pass through the broker
	// as context to the publish callback, to ensure that it can't change
	// the data or capture the pointer.
	CHERI::Capability roData{&logger};
	roData.permissions() &= {CHERI::Permission::Load};

	// Call the broker to publish the new value
	if (set_config(sealedCap, sizeof(LoggerConfig), publish, static_cast<void *>(roData)) < 0)
	{
		Debug::log("Failed to set value for {}", sealedCap);
	};

	// Free the data value
	//free(data);
};

// Helper to generat invalid data
void gen_bad_config(SObj sealedCap, const char *itemName)
{
	Debug::log("thread {} Sending bad data for {}", thread_id_get(), itemName);
	void *d = malloc(4);
	set_config(sealedCap, sizeof(d), publish, d);
	free(d);
};

//
// Tread Entry point for the publisher
//
void __cheri_compartment("publisher") init()
{
	gen_config(WRITE_CONFIG_CAPABILITY(CONFIG1), CONFIG1, 0, "Wile-E");
	gen_config(WRITE_CONFIG_CAPABILITY(CONFIG2), CONFIG2, 0, "Coyote");

	int loop = 1;
	while (true)
	{
		sleep(1500);
		gen_config(WRITE_CONFIG_CAPABILITY(CONFIG1), CONFIG1, loop++, "Wile-E");

		sleep(1500);
		gen_config(WRITE_CONFIG_CAPABILITY(CONFIG2), CONFIG2, loop++, "Coyote");

		// Check we're not leaking data;
		// Debug::log("heap quota available: {}",
		//   heap_quota_remaining(MALLOC_CAPABILITY));
	}
};

void __cheri_compartment("publisher") bad_dog()
{
	while (true)
	{
		sleep(12000);
		gen_bad_config(WRITE_CONFIG_CAPABILITY(CONFIG1), CONFIG1);
	}
};


