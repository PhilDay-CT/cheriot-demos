// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <cheri.hh>
#include <debug.hh>
#include <thread.h>

#include "logger.h"

// Expose debugging features unconditionally for this library.
using Debug = ConditionalDebug<true, "Logger">;

// Import some useful things from the CHERI namespace.
using namespace CHERI;

//
// Function which nominally configures the logger
// In this demo it just prints the config value
//
void __cheri_libcall logger_config(void *c)
{
	LoggerConfig *config = (LoggerConfig *)c;
	Debug::log("thread {} host: {} port: {} level: {}",
	           thread_id_get(),
	           (const char *)config->host.address,
	           (int16_t)config->host.port,
	           config->level);
}

//
// Validates a logger configuration change.  The incomming data
// is not trusted, so this should be run in sandbox compartment
//
bool __cheri_libcall validate_logger_config(void *untrusted)
{
	// Check the bounds and that we can read
	if (!check_pointer<PermissionSet{Permission::Load}>(untrusted,
	                                                    sizeof(LoggerConfig)))
	{
		Debug::log("thread: {} Invalid pointer", thread_id_get());
		return false;
	}

	LoggerConfig *config = (LoggerConfig *)untrusted;

	// Regex on the ip address
	// **TBD **

	// Debug level
	switch (config->level)
	{
		case LogLevel::Debug:
		case LogLevel::Info:
		case LogLevel::Warn:
		case LogLevel::Error:
			break;

		default:
			Debug::log("thread {} Invalid Loglevel {}",
			           thread_id_get(),
			           (int32_t)config->level);
			return false;
	}
	return true;
}
