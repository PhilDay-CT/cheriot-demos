// Copyright SCI Semiconductor and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <NetAPI.h>
#include <cstdlib>
#include <debug.hh>
#include <errno.h>
#include <fail-simulator-on-error.h>
#include <locks.hh>
#include <mqtt.h>
#include <platform-entropy.hh>
#include <platform-rgbctrl.hh>
#include <sntp.h>
#include <tick_macros.h>

//#include "third_party/display_drivers/lcd.hh"
#include <platform-gpio.hh>

#include "mosquitto.org.h"

using CHERI::Capability;

using Debug            = ConditionalDebug<true, "Hugh the lightbulb">;
constexpr bool UseIPv6 = CHERIOT_RTOS_OPTION_IPv6;

/// Maximum permitted MQTT client identifier length (from the MQTT
/// specification)
constexpr size_t MQTTMaximumClientLength = 23;
/// Prefix for MQTT client identifier
constexpr std::string_view clientIDPrefix{"cheriotMQTT"};
/// Space for the random client ID.
std::array<char, MQTTMaximumClientLength> clientID;
static_assert(clientIDPrefix.size() < clientID.size());

// MQTT network buffer sizes
constexpr const size_t networkBufferSize    = 1024;
constexpr const size_t incomingPublishCount = 20;
constexpr const size_t outgoingPublishCount = 20;

// MQTT test broker: https://test.mosquitto.org/
// Note: port 8883 is encrypted and unautenticated
DECLARE_AND_DEFINE_CONNECTION_CAPABILITY(MosquittoOrgMQTT,
                                         "test.mosquitto.org",
                                         8883,
                                         ConnectionTypeTCP);

DECLARE_AND_DEFINE_ALLOCATOR_CAPABILITY(mqttTestMalloc, 32 * 1024);

std::string           topic;
std::atomic<uint32_t> barrier;

void __cheri_callback publishCallback(const char *topic,
                                      size_t      topicLength,
                                      const void *payload,
                                      size_t      payloadLength)
{
	// FIXME: Work out why != doesn't work (non-mangled memcmp being inserted)
	if (memcmp(topic, ::topic.data(), std::min(topicLength, ::topic.size())) !=
	    0)
	{
		Debug::log(
		  "Received a publish message on topic '{}', but expected '{}'",
		  std::string_view{topic, topicLength},
		  ::topic);
		return;
	}
	if (payloadLength < 3)
	{
		Debug::log("Payload is too short to be a colour");
		return;
	}
	auto *colours = static_cast<const uint8_t *>(payload);
	auto  leds    = MMIO_CAPABILITY(SonataRgbLedController, rgbled);
	leds->rgb(SonataRgbLed::Led0, colours[0], colours[1], colours[2]);
	leds->update();
}

auto status_leds()
{
	return MMIO_CAPABILITY(SonataGPIO, gpio);
}

void __cheri_compartment("provider") provider_init()
{
	int     ret;
	Timeout t{MS_TO_TICKS(5000)};

	status_leds()->led_on(0);
	network_start();
	status_leds()->led_on(1);

	// SNTP must be run for the TLS stack to be able to check certificate dates.
	while (sntp_update(&t) != 0)
	{
		Debug::log("Failed to update NTP time");
		t = Timeout{MS_TO_TICKS(5000)};
	}
	status_leds()->led_on(2);

	{
		timeval tv;
		int     ret = gettimeofday(&tv, nullptr);
		if (ret != 0)
		{
			Debug::log("Failed to get time of day: {}", ret);
		}
		else
		{
			// Truncate the epoch time to 32 bits for printing.
			Debug::log("Current UNIX epoch time: {}", int32_t(tv.tv_sec));
		}
	}

	while (true)
	{
		status_leds()->led_off(3);
		status_leds()->led_off(4);

		// Prefix with something recognizable, for convenience.
		memcpy(clientID.data(), clientIDPrefix.data(), clientIDPrefix.size());
		// Suffix with random character chain.
		mqtt_generate_client_id(clientID.data() + clientIDPrefix.size(),
		                        clientID.size() - clientIDPrefix.size());

		Debug::log("Connecting to MQTT broker...");

		t           = UnlimitedTimeout;
		SObj handle = mqtt_connect(&t,
		                           STATIC_SEALED_VALUE(mqttTestMalloc),
		                           STATIC_SEALED_VALUE(MosquittoOrgMQTT),
		                           publishCallback,
		                           nullptr,
		                           TAs,
		                           TAs_NUM,
		                           networkBufferSize,
		                           incomingPublishCount,
		                           outgoingPublishCount,
		                           clientID.data(),
		                           clientID.size());
		status_leds()->led_on(3);

		if (!Capability{handle}.is_valid())
		{
			Debug::log("Failed to connect.");
			continue;
		}

		Debug::log("Connected to MQTT broker!");

/*
		Debug::log(
		  "Subscribing to topic '{}' ({} bytes)", topic.c_str(), topic.size());
		ret = mqtt_subscribe(&t,
		                     handle,
		                     1, // QoS 1 = delivered at least once
		                     topic.data(),
		                     topic.size());

		if (ret < 0)
		{
			Debug::log("Failed to subscribe, error {}.", ret);
			mqtt_disconnect(&t, STATIC_SEALED_VALUE(mqttTestMalloc), handle);
			continue;
		}
*/
		status_leds()->led_on(4);

		while (true)
		{
			size_t  heapFree = heap_available();
			ret = mqtt_run(&t, handle);

			if (ret < 0)
			{
				Debug::log("Failed to wait for the SUBACK, error {}.", ret);
				status_leds()->led_off(3);
				status_leds()->led_off(4);
				mqtt_disconnect(
				  &t, STATIC_SEALED_VALUE(mqttTestMalloc), handle);
				break;
			}
		}
	}
}


