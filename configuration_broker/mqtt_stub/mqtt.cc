// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

//
// Fake MQTT server that periodically "publishes"
// messages where the data is a json string
//

#include "cdefs.h"
#include <compartment.h>
#include <debug.hh>
#include <thread.h>
#include <tick_macros.h>

#include "../publisher/publisher.h"

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "MQTT">;

struct Message
{
	const char *topic;
	const char *json;
};

// Set of messages to publish in a continual loop
const Message Messages[] = {
  {"logger",
   "{\"host\": {\"address\":\"100.101.102.103\",\"port\":666},\"level\":1}"},

  {"logger",
   "{\"host\": {\"address\":\"100.101.102.103\",\"port\":666},\"level\":2}"},

  {"rgbled", "{\"1\": {\"red\": 200}"},

  {"logger",
   "{\"host\": {\"address\":\"100.101.102.103\",\"port\":666},\"level\":3}"},

  {"logger",
   "{\"host\": {\"address\":\"100.101.102.103\",\"port\":666},\"level\":4}"},

  {"logger",
   "{\"host\": {\"address\":\"100.101.102.103\",\"port\":666},\"level\":5}"},

  {"logger", "{\"x\":"},
};

//
// Helper for the example to create delay in the publisher
//
static inline void sleep(const uint32_t mS)
{
	Timeout t1{MS_TO_TICKS(mS)};
	thread_sleep(&t1, ThreadSleepNoEarlyWake);
}

//
// Thread Entry point for the MQTT stub
//
void __cheri_compartment("mqtt") init()
{
	int i = 0;
	while (true)
	{
		Debug::log("-----------------------------");
		Debug::log("thread {} Send {} {}",
		           thread_id_get(),
		           Messages[i].topic,
		           Messages[i].json);
		updateConfig(Messages[i].topic, Messages[i].json);
		sleep(1000);

		if (++i >= (sizeof(Messages) / sizeof(Messages[0])))
		{
			i = 0;
		}
	}
};
