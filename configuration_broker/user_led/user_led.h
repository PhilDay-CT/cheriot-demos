// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <stdlib.h>

/**
 * Mocked example of configuration data for a controller
 * with a set of eight LEDs that can be turned on and off
 * (such as the user LEDs on a Sonata Board)
 */

enum class User_LED
{
	Off = 0,
	On  = 1,
};

struct User_LED_Config
{
	User_LED led0;
	User_LED led1;
	User_LED led2;
	User_LED led3;
	User_LED led4;
	User_LED led5;
	User_LED led6;
	User_LED led7;
};

/**
 * Function which nominally configures the user LEDs
 * In this demo it just prints the config value
 */
void __cheri_libcall user_led_config(void *config);
