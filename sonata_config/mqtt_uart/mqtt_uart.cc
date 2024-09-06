// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <cheriot-atomic.hh>
#include <compartment.h>
#include <cstdint>
#include <debug.hh>
#include <errno.h>
#include <interrupt.h>
#include <platform-gpio.hh>
#include <platform-entropy.hh>
#include <riscvreg.h>
#include <thread.h>
#include <tick_macros.h>
#include <type_traits>
#include <vector>

#include "../provider/provider.h"
#include "../console/console.h"

using CHERI::Capability;

//
// Helper to generate a hex string for debugging
// what we actually receive
//
void hex(char *buf, int s)
{
	const char Hexdigits[] = "0123456789abcdef";
	buf[1] = Hexdigits[s & 0xf];
	s >>= 4;
	buf[0] = Hexdigits[s & 0xf];
	buf[2] = 0;
}

char name[10] = "Sonata-xx";

/// Thread entry point.
void __cheri_compartment("mqtt_uart")
init()
{
	// Allocate a name
	{
		EntropySource  entropy;
		
		const char Hexdigits[] = "0123456789abcdef";
		auto id = entropy();
		
		name[8] = Hexdigits[id & 0xf];
		id >>= 4;
		name[7] = Hexdigits[id & 0xf];
	}
	

	console::print("Starting ...");
				
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
				console::print("Ready ...");

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
				console::print("---");
				console::print(topic);
				console::print(message);
				updateConfig(topic, message);

				// Reset the buffers
				ti=topic;
				mi=message;
			}
//		});	
	}
}
