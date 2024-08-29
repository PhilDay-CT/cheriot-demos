// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <cheriot-atomic.hh>
#include <compartment.h>
#include <cstdint>
#include <debug.hh>
#include <errno.h>
#include <interrupt.h>
#include <platform-gpio.hh>
#include <riscvreg.h>
#include <thread.h>
#include <tick_macros.h>
#include <type_traits>
#include <vector>

#include "../sonata_lcd/lcd.hh"
#include "../provider/provider.h"


/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Reader">;
using CHERI::Capability;

#define debug(s) lcd.draw_str({x, y+=10}, s, Color::White,Color::Black)

void hex(char *buf, int s)
{
	const char          Hexdigits[] = "0123456789abcdef";
	buf[1] = Hexdigits[s & 0xf];
	s >>= 4;
	buf[0] = Hexdigits[s & 0xf];
	buf[2] = 0;
}

/// Thread entry point.
void __cheri_compartment("reader")
init()
{

	using namespace sonata::lcd;

	auto lcd  = SonataLcd();
	
	// Used to control where debug goes in the lcd 
	uint32_t x=10;
	uint32_t y=0;
	debug("Starting ...");
				
	auto uart = MMIO_CAPABILITY(Uart, uart);

	// Get a capability to the UART.
	auto uart_blocking_read = [&]() {
		return uart->blocking_read();
	};
	
	while (true)
	{
		char *topic = (char*)malloc(20);
		char *message = (char*)malloc(1024);
			
		//CHERI::with_interrupts_disabled([&]() {
			while (true)
			{
				char *ti=topic;
				char *mi=message;
				
				// Get the topic
				debug("Ready ...");

				while (true) {
					char c;
					c = uart->blocking_read();
					if (c == ':') {
						*ti++ = 0;
						break;
					}
					*ti++ = c;
				}

				// Get the message
				while (true) {
					char c;
					c = uart->blocking_read();
					if (c == 10) {
						*mi++ = 0;
						break;
					}
					*mi++ = c;
				}
				y=30;
				debug("---");
				debug(topic);
				debug(message);
				updateConfig(topic, message);

				// Reset the buffers
				ti=topic;
				mi=message;
			}
//		});	
	}
}
