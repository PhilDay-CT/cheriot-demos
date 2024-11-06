// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdint>
#include <cstdlib>
#include <debug.hh>
#include <thread.h>
#include <token.h>
#include <platform-gpio.hh>

// Define a sealed capability that gives this compartment
// read access to configuration data "logger" and "rgb_led"
#include "common/config_broker/config_broker.h"

#define SYSTEM_CONFIG "system"
DEFINE_WRITE_CONFIG_CAPABILITY(SYSTEM_CONFIG)


// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "System Config">;

#include "config/include/system_config.h"


namespace
{
	static systemConfig::Config *currentConfig;

	// Helpers to read the swicth values
	auto switches()
	{
		return MMIO_CAPABILITY(SonataGPIO, gpio);
	}

	// ID comes from the first two switches
	auto read_id() {
		uint8_t res = 0;
		for (auto i=0; i<2; i++) {
			auto set = switches()->read_switch(i);
			if (set) {
				res += 1<<i;
			}
		}
		return res;
	}

	auto read_switches() {
		uint8_t res = 0;
		for (auto i=0; i<8; i++) {
			auto set = switches()->read_switch(i);
			if (set) {
				res += 1<<i;
			}
		}
		return res;
	}

	
} // namespace

/**
 * Update the System configuration by reading
 * the switches
 */
systemConfig::Config get_system_config()
{
	systemConfig::Config config;

	// Generate a system name
	char name[10] = "Sonata";

	// Read the switches and update config if they change
	for (auto i=0; i<8; i++) {
		config.switches[i] = switches()->read_switch(i);
	}

	auto newId = read_id();
	snprintf(config.id, sizeof(config.id), "%s-%d", name, newId);
	
	return config;
}
