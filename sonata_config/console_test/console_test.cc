// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdint>
#include <cstring>
#include <debug.hh>
#include <thread.h>

#include "../console/console.h"

void __cheri_compartment("console_test")
init()
{
	console::header("Mutley");
	console::header("Foo");

	for (int i=0;i<30;i++) {
		console::print("Hello Phil");
		Timeout t1{MS_TO_TICKS(500)};
		thread_sleep(&t1, ThreadSleepNoEarlyWake);
		console::error("Err", ".........1.........2.........3.........4.........5.........6.........7.........8");
		Timeout t2{MS_TO_TICKS(500)};
		thread_sleep(&t2, ThreadSleepNoEarlyWake);
		console::print(".........1.........2.........3.........4.........5.........6.........7.........8");
		Timeout t3{MS_TO_TICKS(500)};
		thread_sleep(&t3, ThreadSleepNoEarlyWake);
	}

}
