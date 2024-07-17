// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

// Contributed by Configured Things Ltd

#include <debug.hh>
#include <thread.h>
#include <cheri.hh>

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
		           (const char *)config->ipv4,
		           config->port,
		           config->level);
}

//
// Validates a logger configuration change.  The incomming data
// is not trusted, so this should be run in sandbox compartment
//
bool __cheri_libcall validate_logger_config(void *untrusted) {

	// Check the bounds and that we can read
	if (!check_pointer<PermissionSet{Permission::Load}>(untrusted, sizeof(LoggerConfig))) {
		Debug::log("Invalid pointer");
		return false;
	}

	LoggerConfig *config = (LoggerConfig *)untrusted;
	
	// Regex on the ip address
	// **TBD **

	// Debug level
	switch (config->level) {
		case LogLevel::Debug:
		case LogLevel::Info:
		case LogLevel::Warn:
		case LogLevel::Error:
			break;

		default:
			Debug::log("Invalid Loglevel");
			return false;
	}
	return true;
}
