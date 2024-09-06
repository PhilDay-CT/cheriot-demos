// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <stdlib.h>

// Mocked example of configuration data for a controller
// with two RGB LEDs (such as on the Sonata Board)

namespace LCD {

enum class Logo
{
	lowRISC = 0,
	ConfiguredThings  = 1,
};

struct Config
{
	Logo logo;
};

} // namespace lcd

/**
 * Configure the LCD to show a particular logo 
 */
void __cheri_libcall lcd_config(void *config);
