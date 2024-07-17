// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#define CHERIOT_NO_AMBIENT_MALLOC
#define CHERIOT_NO_NEW_DELETE

#include <compartment.h>
#include <cstdlib>
#include <debug.hh>
#include <fail-simulator-on-error.h>
#include <string.h>

#include "./core_json.h"

#include "../logger/logger.h"

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Parser">;

int atoi(const char *str, size_t len)
{
	int acc = 0;
	for (int i = 0; i < len && str && isdigit(*str); ++str)
	{
		acc *= 10;
		acc += *str - 0x30;
	}
	return acc;
}
//
// Helper function to extract a string value
//
bool getString(const char *json, const char *key, char *dst)
{
	char  *value;
	size_t valueLength;

	auto result = JSON_Search((char *)json,
	                          strlen(json),
	                          (char *)key,
	                          strlen(key),
	                          &value,
	                          &valueLength);

	if (result == JSONSuccess)
	{
		strncpy(dst, value, valueLength);
		dst[valueLength] = '\0';
		return true;
	}
	else
	{
		Debug::log("Missing key {} in {}", key, json);
		return false;
	}
}

//
// Helper function to extract a string value
//
bool getNumber(const char *json, const char *key, uint16_t *dst)
{
	char  *value;
	size_t valueLength;

	auto result = JSON_Search((char *)json,
	                          strlen(json),
	                          (char *)key,
	                          strlen(key),
	                          &value,
	                          &valueLength);

	if (result == JSONSuccess)
	{
		*dst = atoi(value, valueLength);
		return true;
	}
	else
	{
		Debug::log("Missing key {} in {}", key, json);
		return false;
	}
}

//
// Parse a json string into a LoggerConfig object.
// Note that we run in a sandbox compartment to contain the blast
// radius of any parsing errors, so we don't have to do any validation
// on the parameters we're given
//
int __cheri_compartment("parser")
  parseLoggerConfig(const char *json, LoggerConfig *config)
{
	JSONStatus_t result;

	// Check we have valid JSON
	result = JSON_Validate(json, strlen(json));
	if (result != JSONSuccess)
	{
		Debug::log("Invalid JSON {}", json);
		return -1;
	}

	// query the invidual values and populate the logger config object
	bool parsed = true;
	parsed = parsed && getString(json, "host.address", config->host.address);
	parsed = parsed && getNumber(json, "host.port", &config->host.port);
	uint16_t level;
	parsed        = parsed && getNumber(json, "level", &level);
	config->level = (LogLevel)level;

	if (parsed)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}