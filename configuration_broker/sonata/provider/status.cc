// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdlib>
#include <debug.hh>
#include <mqtt.h>
#include <thread.h>
#include <token.h>
#include <platform-gpio.hh>


// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Status">;

// Helpers to read the switch values
auto switches()
{
	return MMIO_CAPABILITY(SonataGPIO, gpio);
}

#define STATUS_SIZE 100
char status[STATUS_SIZE];

// Create a JSON view of the sonata status
auto create_status(uint8_t switches) {
	snprintf(status, STATUS_SIZE, "{\"Status\":\"On\",\"switches\":%d}", switches);
	Debug::log("Status: {} {}", (const char *)status, strlen(status));
}

void send_status(SObj mqtt, std::string topic)
{
	static uint8_t prev = 0;
	static bool init = false;
	
	uint8_t current = 0;
	for (auto i=0; i<8; i++)
	{
		auto set = switches()->read_switch(i);
		if (set) {
			current += 1<<i;
		}
	}
	
	if (!init || (current != prev))
	{
		Debug::log("Status changed - {} => {}", prev, current);
		init = true;
		prev = current;
		create_status(current);
	
		Timeout t{MS_TO_TICKS(5000)};
		CHERI::Capability roJSON{status};
		roJSON.permissions() &= {CHERI::Permission::Load};

		auto ret = mqtt_publish(&t,
								mqtt,
								1, // QoS 1 = delivered at least once
								topic.data(),
								topic.size(),
								roJSON,
								strlen(status));

		if (ret < 0)
		{
			Debug::log("Failed to send status: error {}", ret);
		}

	}

}