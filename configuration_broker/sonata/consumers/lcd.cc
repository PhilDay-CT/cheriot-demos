// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdint>
#include <cstdlib>
#include <debug.hh>
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
	/**
	 * Handle updates to the System configuration
	 */
	int system_config_handler(void *newConfig)
	{
		static auto init = false;
		static auto lcd  = SonataLcd();

		// Process the configuration
		auto config= static_cast<systemConfig::Config *>(newConfig);
		
		Debug::log("System Config: {}",
					(const char *)config->id);

		CHERI::with_interrupts_disabled([&]() {	

			auto screen   = Rect::from_point_and_size(Point::ORIGIN, lcd.resolution());
			
			if (!init) {
				auto logoRect = screen.centered_subrect({105, 80});
				lcd.draw_image_rgb565(logoRect, CTLogo105x80);
				init = true;
			}
		
			auto idRect = Rect::from_point_and_size({0, 0}, {screen.right, 17});
			lcd.fill_rect(idRect, Color::White);
			lcd.draw_str({10, 2}, config->id, Color::White, Color::Black);
			
			uint32_t x = 5;
			uint32_t y = screen.bottom-17;
			char switchNr[2];
			for (auto i=0; i<8; i++) {
				Debug::log("Switch {}: {} {} {}", i, config->switches[i], x, y);
				snprintf(switchNr, 2, "%d", i);
				if (config->switches[i]) {
					lcd.draw_str({x, y}, switchNr, Color::Red, Color::White);
				} else {
					lcd.draw_str({x, y}, switchNr, Color::Black, Color::White);
				}
				x += 12;
			}
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
