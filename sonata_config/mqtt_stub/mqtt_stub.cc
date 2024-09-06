// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

/**
 * Fake MQTT server that periodically "publishes"
 * messages where the data is a json string
 */

/*
 * Examples of input in JSON
 *
 * topic: logger
 * -------------
 * {
 *   host: {
 *     address: "x.x.x.x",
 *     port: 400
 *   },
 *   level: "info"
 * }
 *
 * topic: rgbled
 * -------------
 * {
 *   led0: {red: 100, green: 100, blue: 100},
 *   led1: {red: 200, green: 200, blue: 200},
 * }
 *
 * topic: userled
 * --------------
 * {
 *   led0: 'on',
 *   led1: 'off',
 *   led2: 'ON',
 *   led3: 'OFF',
 *   led4: 'Off',
 *   led5: 'On',
 *   led6: 'off',
 *   led7: 'On'
 * }
 *
 */

#include "cdefs.h"
#include <compartment.h>
#include <debug.hh>
#include <errno.h>
#include <fail-simulator-on-error.h>
#include <thread.h>
#include <platform-entropy.hh>

#include "../provider/provider.h"

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "MQTT">;
#include "../console/console.h"

namespace
{

	/**
	 * Define a test message
	 */
	struct Message
	{
		const char *description;
		int         expected;
		const char *topic;
		const char *json;
	};

	// Set of messages to publish.
	const Message Messages[] = {

	  {"Console config",
	   0,
	   "console",
	   "{\"header\": \"Test 123\"}"},

	  // Valid RGB LED config
	  {"Valid RGB LED config",
	   0,
	   "rgbled",
	   "{\"led0\":{\"red\":0,   \"green\":86,  \"blue\":164},"
	   " \"led1\":{\"red\":155, \"green\":100, \"blue\":100}}"},

	  // Valid User LED config
	  {"Valid User LED config",
	   0,
	   "userled",
	   "{\"led0\":\"on\",\"led1\":\"off\",\"led2\":\"ON\",\"led3\":\"OFF\","
	   " \"led4\":\"On\",\"led5\":\"Off\",\"led6\":\"on\",\"led7\":\"off\"}"},

	  // Invalid RBG LED config - invalid Json
	  {"Invalid Logger config (bad JSON)", -EINVAL, "rgbled", "{\"x\":"},

	  {"Console config",
	   -EINVAL,
	   "console",
	   "{\"header\": \"Test very very very very very long\"}"},

	  // Valid RGB LED config
	  {"Valid RGB LED config",
	   0,
	   "rgbled",
	   "{\"led1\":{\"red\":0,  \"green\":86, \"blue\":164},"
	   " \"led0\":{\"red\":155,\"green\":100,\"blue\":100}}"},

	  // Invalid User LED config
	  {"Valid User LED config",
	   -EINVAL,
	   "userled",
	   "{\"led0\":\"OFF\",\"led1\":\"ON\",\"led2\":\"off\",\"led3\":\"on\"}"},

	  {"Console config",
	   0,
	   "console",
	   "{\"header\": \"Still here\"}"},

	  // Valid User LED config
	  {"Valid User LED config",
	   0,
	   "userled",
	   "{\"led0\":\"OFF\",\"led1\":\"ON\",\"led2\":\"off\",\"led3\":\"on\","
	   " \"led4\":\"Off\",\"led5\":\"On\",\"led6\":\"off\",\"led7\":\"on\"}"},

	   // Invalid RGB LED config
	  {"Invalid RGB LED config",
	   -EINVAL,
	   "rgbled",
	   "{\"led0\":{\"red\":0,  \"green\":286,\"blue\":400},"
	   " \"led1\":{\"red\":255,\"green\":200,\"blue\":200}}"},

	  
	};

} // namespace

char name[10] = "Sonata-xx";

/**
 * Thread Entry point for the MQTT stub.  The thread "publishes" each message by
 * calling the Provider's UpdateConfig() method as if the Provider has
 * subscribed to the topics.  It then waits a short time before publishing the
 * next message. After all the messages has been sent it sends two further
 * messages in quick succession to show the rate limiting in operation.
 */
void __cheri_compartment("mqtt_stub") init()
{
	Debug::log("*** MQTT Started *** thread {} ", thread_id_get());

	// Allocate a name
	{
		EntropySource  entropy;
		
		const char Hexdigits[] = "0123456789abcdef";
		auto id = entropy();
		Debug::log("ID: {}", id);

		name[8] = Hexdigits[id & 0xf];
		id >>= 4;
		name[7] = Hexdigits[id & 0xf];
	}
	Debug::log("Name: {}", (const char *)name);

	console::print("Starting ...");
	while (true) { 
		for (auto &m : Messages)
		{
			Debug::log("-------- {} --------", m.description);
			Debug::log("thread {} Send {} {}", thread_id_get(), m.topic, m.json);
			console::print(m.topic);
			console::print(m.json);
			auto res = updateConfig(m.topic, m.json);
			Debug::Assert(res == m.expected, "Unexpected result {}", res);

			// Give the consumers a chance to run
			Timeout t1{MS_TO_TICKS(500)};
			thread_sleep(&t1, ThreadSleepNoEarlyWake);
		}

		Debug::log("\n---- Finished ----");
		
		Timeout t1{MS_TO_TICKS(1000)};
		thread_sleep(&t1, ThreadSleepNoEarlyWake);
	}
};
