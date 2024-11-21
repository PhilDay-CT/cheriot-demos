// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdlib>
#include <debug.hh>
#include <mqtt.h>
#include <thread.h>
#include <token.h>

#include "config/include/system_config.h"

#include "../crypto/crypto.h"

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Status">;

// Publish a string to the status topic
void publish(SObj mqtt, std::string topic, void *status)
{
	// Use a capability with only Load
	// permission so we can be sure the MQTT stack
	// doesn't capture a pointer to our buffer
	Timeout           t{MS_TO_TICKS(5000)};
	CHERI::Capability roJSON{status};
	roJSON.permissions() &= {CHERI::Permission::Load};

	auto ret = mqtt_publish(&t,
	                        mqtt,
	                        1, // QoS 1 = delivered at least once
	                        topic.data(),
	                        topic.size(),
	                        roJSON,
	                        roJSON.bounds());

	if (ret < 0)
	{
		Debug::log("Failed to send status: error {}", ret);
	}
}

void send_status(SObj mqtt, std::string topic, systemConfig::Config *config)
{
	Debug::log("Sending Status");
	char status[100];

	static uint8_t prev = 0;
	static bool    init = false;

	// Convert the switches to a value
	uint8_t switchesValue = 0;
	for (auto i = 0; i < 8; i++)
	{
		if (config->switches[i])
		{
			switchesValue += 1 << i;
		}
	}

	// Create the JSON representation
	snprintf(
	  status,
	  sizeof(status) / sizeof(status[0]),
	  "{\"Status\":\"On\",\"switches\": [%d, %d, %d, %d, %d, %d, %d, %d]}",
	  config->switches[0] ? 1 : 0,
	  config->switches[1] ? 1 : 0,
	  config->switches[2] ? 1 : 0,
	  config->switches[3] ? 1 : 0,
	  config->switches[4] ? 1 : 0,
	  config->switches[5] ? 1 : 0,
	  config->switches[6] ? 1 : 0,
	  config->switches[7] ? 1 : 0);

	// Get a signed version
	CHERI::Capability<void> signed_message = sign(MALLOC_CAPABILITY, "StatusCX", status, strlen(status));  
	if (signed_message.is_valid()) {
		Debug::log("Publishing signed message {}", signed_message);
		publish(mqtt, topic, signed_message);
		heap_free(MALLOC_CAPABILITY, signed_message);
	}
	else {
		Debug::log("Failed to sign message");
	}
}

void clear_status(SObj mqtt, std::string topic)
{
	// Clear the peristent status message by
	// pubishing a zero length message   
	char status;
	CHERI::Capability statusCap = {&status};
	statusCap.bounds() = 0;
	Debug::log("clear status with {}", statusCap);
	publish(mqtt, topic, statusCap);
}
