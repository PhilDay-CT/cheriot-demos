// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

/**
 * Code to run inside a sandbox compartment to parse
 * various configuration items from serialised JSON into
 * the corresponing struct.
 *
 * For each configuration item type there must be
 *   * A sealed capability granting permission to register
 *     the parser, and defining some key charateristics.
 *   * A callback which will perform the parse, typically using
 *     the collection of helper functions in parser_helper.h
 */

/**
 * Normally for code designed to run in a sandbox compartment
 * we would block heap operations by including the following
 * definitions:
 *
 * #define CHERIOT_NO_AMBIENT_MALLOC
 * #define CHERIOT_NO_NEW_DELETE
 *
 * However our parser (and specifically magic_emum) relies on new()
 * so instead we give it a small heap quota and call heap_free_all()
 * at the end of each parser callback to clean up if there is any leakage,
 * We also log if it actually had to free anything.
 * That still leaves a small residual risk that a poorly written parser
 * could affect subsequent calls.  If you want to protect against interaction
 *
 */
#define MALLOC_QUOTA 200

#include <compartment.h>
#include <cstdlib>
#include <debug.hh>
#include <string.h>
#include <thread.h>

#include <magic_enum/magic_enum.hpp>

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Parser">;

#include "parser.h"
#include "parser_helper.h"

// Set for Items we are allowed to register a parser for
#include "../config_broker/config_broker.h"

#include "../user_led/user_led.h"
#define USER_LED_CONFIG "user_led"
DEFINE_PARSER_CONFIG_CAPABILITY(USER_LED_CONFIG, sizeof(User_LED_Config), 1800);

/**
 * Parse a json string into an User LED Config struct.
 */
int __cheri_callback parse_User_LED_config(const char *json, void *dst)
{
	auto        *config = static_cast<User_LED_Config *>(dst);
	JSONStatus_t result;

	// Check we have valid JSON
	result = json_parser::validate(json, strlen(json));
	if (result != JSONSuccess)
	{
		Debug::log("thread {} Invalid JSON {}", thread_id_get(), json);
		return -1;
	}

	// query the individual values and populate the config struct
	bool parsed = true;
	parsed      = parsed && get_enum<User_LED>(json, "led0", &config->led0);
	parsed      = parsed && get_enum<User_LED>(json, "led1", &config->led1);
	parsed      = parsed && get_enum<User_LED>(json, "led2", &config->led2);
	parsed      = parsed && get_enum<User_LED>(json, "led3", &config->led3);
	parsed      = parsed && get_enum<User_LED>(json, "led4", &config->led4);
	parsed      = parsed && get_enum<User_LED>(json, "led5", &config->led5);
	parsed      = parsed && get_enum<User_LED>(json, "led6", &config->led6);
	parsed      = parsed && get_enum<User_LED>(json, "led7", &config->led7);

	// Free any heap the parser might have left allocated
	auto heap_freed = heap_free_all(MALLOC_CAPABILITY);
	if (heap_freed > 0)
	{
		Debug::log("Freed {} from heap", heap_freed);
	}

	return (parsed) ? 0 : -1;
}

/**
 * Register the parser with the Broker. This needs to be
 * run before any values can be accepted.
 *
 * There is no init mechanism in cheriot and threads are not
 * expected to terminate, so rather than have a separate thread
 * just to run this which then blocks we expose it as method for
 * the Broker to call when the first item is published.
 */
int __cheri_compartment("parser_user_led") parse_user_led_init()
{
	// USER LED Config Parser
	auto res = set_parser(PARSER_CONFIG_CAPABILITY(USER_LED_CONFIG),
	                      parse_User_LED_config);

	if (res < 0)
	{
		Debug::log("Failed to register parser for user led");
	}

	return res;
}
