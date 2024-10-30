// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdint>
#include <cstdlib>
#include <debug.hh>
#include <fail-simulator-on-error.h>
#include <thread.h>
#include <token.h>

// Define a sealed capability that gives this compartment
// read access to configuration data "logger" and "rgb_led"
#include "common/config_broker/config_broker.h"

#define SYSTEM_CONFIG "system"
DEFINE_READ_CONFIG_CAPABILITY(SYSTEM_CONFIG)


// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "LCD Config">;

#include "config/include/system_config.h"
#include "common/config_consumer/config_consumer.h"

#include "../third_party/display_drivers/lcd.hh"
#include "../logos/lowrisc_logo.h"
#include "../logos/CT_logo.h"

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

	/**
	 * Handle updates to the System configuration
	 */
	int system_config_handler(void *newConfig)
	{

		static auto lcd  = SonataLcd();
	
		// Note the consumer helper will have already made a 
		// fast claim on the new config value, but we want to
		// keep access to it so we can compare changes
		if (heap_claim(MALLOC_CAPABILITY, newConfig) == 0)
		{
			Debug::log("Failed to claim {}", newConfig);
			return -1;
		}

		auto oldConfig = currentConfig;
	
		// Process the configuration
		currentConfig = static_cast<systemConfig::Config *>(newConfig);
		
		Debug::log("System Config: {} {}",
					(const char *)currentConfig->group,
					currentConfig->kind);

		CHERI::with_interrupts_disabled([&]() {	
			
			auto screen   = Rect::from_point_and_size(Point::ORIGIN, lcd.resolution());

			if ((oldConfig == nullptr) || (currentConfig->kind != oldConfig->kind)) {
				Debug::log ("Kind changed !");

				auto logoRect = screen.centered_subrect({105, 80});

				switch (currentConfig->kind) {
					case systemConfig::Kind::ConfiguredThings:
						lcd.draw_image_rgb565(logoRect, CTLogo105x80);
						break;
			
					case systemConfig::Kind::lowRISC:
						lcd.draw_image_rgb565(logoRect, lowriscLogo105x80);
						break;
				}
			}

			Debug::log("id: {} switches: {}", read_id(), read_switches());
			
			// Always update the ID
			auto idRect = Rect::from_point_and_size({0, 0}, {screen.right, 17});
			lcd.fill_rect(idRect, Color::White);
			lcd.draw_str({10, 2}, currentConfig->group, Color::White, Color::Black);
			
		});
	
		return 0;
	}

} // namespace

/**
 * Thread entry point.  The waits for changes to one
 * or more configuration values and then calls the
 * appropriate handler.
 */
void __cheri_compartment("lcd") init()
{
	/// List of configuration items we are tracking
	ConfigConsumer::ConfigItem configItems[] = {
	  {READ_CONFIG_CAPABILITY(SYSTEM_CONFIG), system_config_handler, 0, nullptr},
	};

	size_t numOfItems = sizeof(configItems) / sizeof(configItems[0]);

	ConfigConsumer::run(configItems, numOfItems);
}
