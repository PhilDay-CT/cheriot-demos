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
#include <sntp.h>
#include <tick_macros.h>

#include "mosquitto.org.h"

#include "../console/console.h"
#include "../provider/provider.h"

using CHERI::Capability;

using Debug            = ConditionalDebug<true, "MQTT">;
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
	const char *conf_id = topic + ::topic.size()-1;
	Debug::log("topic: {} frag: {}", topic, conf_id);

	// we need a null terminates string to parse
	auto json = (char*)malloc(payloadLength+1);
	std::memcpy((void *)json, payload, payloadLength);
	json[payloadLength] = 0;
	
	Debug::log("Got topic: {} json {}", topic, (const char*)json);
	console::print("---");
	console::print(conf_id);
	console::print(json);
	updateConfig(conf_id, json);

	free(json);
}

void __cheri_compartment("mqtt") init()
{
	int     ret;
	Timeout t{MS_TO_TICKS(5000)};

	network_start();

	// SNTP must be run for the TLS stack to be able to check certificate dates.
	while (sntp_update(&t) != 0)
	{
		Debug::log("Failed to update NTP time");
		t = Timeout{MS_TO_TICKS(5000)};
	}

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

	constexpr std::string_view TopicPrefix{"CT-Sonata/"};
	topic.reserve(TopicPrefix.size() + 8 + 2);
	topic.append(TopicPrefix.data(), TopicPrefix.size());
#if 0
	topic.append("testing");
#else

	char id[9];
	{
		EntropySource entropySource;
		for (int i = 0; i < 8; i++)
		{
			id[i] = ('a' + entropySource() % 26);
		}
	}
#endif
	topic.append(id);
	topic.append("/#");
	
	id[8] = 0;
	console::header(id);
	while (true)
	{

		// Prefix with something recognizable, for convenience.
		memcpy(clientID.data(), clientIDPrefix.data(), clientIDPrefix.size());
		// Suffix with random character chain.
		mqtt_generate_client_id(clientID.data() + clientIDPrefix.size(),
		                        clientID.size() - clientIDPrefix.size());

		Debug::log("Connecting to MQTT broker... {} ");

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

		if (!Capability{handle}.is_valid())
		{
			Debug::log("Failed to connect.");
			continue;
		}

		Debug::log("Connected to MQTT broker!");

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

		while (true)
		{
			size_t  heapFree = heap_available();
			ret = mqtt_run(&t, handle);

			if (ret < 0)
			{
				Debug::log("Failed to wait for the SUBACK, error {}.", ret);
				mqtt_disconnect(
				  &t, STATIC_SEALED_VALUE(mqttTestMalloc), handle);
				break;
			}
		}
	}
}

