// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <cheriot-atomic.hh>
#include <compartment.h>
#include <debug.hh>
#include <errno.h>
#include <interrupt.h>
#include <platform-gpio.hh>
#include <riscvreg.h>
#include <thread.h>
#include <tick_macros.h>
#include <type_traits>
#include <vector>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Reader">;
using CHERI::Capability;


/// Thread entry point.
void __cheri_compartment("reader")
init()
{
	using namespace sonata::lcd;

	auto lcd      = SonataLCD();
	
	// Set the secret value on startup.
	Debug::log("Starting...");
	
	auto uart = MMIO_CAPABILITY(Uart, uart);

	// Get a capability to the UART.
	auto uart_blocking_read = [&]() {
		return uart->blocking_read();
	};

	while (true)
	{
		char topic[10];
		char message[200];
		
		//CHERI::with_interrupts_disabled([&]() {
			while (true)
			{
				int topic_index=0;
				int message_index=0;
		
				// Get the topic
				//Debug::log("get Topic ..");
				while (true) {
					char c;
					c = uart->blocking_read();
					if (c == 13) {
						topic[topic_index] = 0;	
						break;
					}
					topic[topic_index++] = c;
				}

				// Get the message
				while (true) {
					char c;
					c = uart->blocking_read();
					if (c == 13) {
						message[message_index] = 0;
						break;
					}
					message[message_index++] = c;
				}
				Debug::log("Got topic: +{}+ message: +{}+", (const char *)topic, (const char*)message);
			}
		//});	
	}
}
