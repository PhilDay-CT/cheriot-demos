// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <stdlib.h>

// Mocked example of configuration data for a remote
// logging service  

enum class LogLevel
{
	Debug = 0,
	Info = 1,
    Warn = 2,
    Error = 3
};

struct LoggerConfig
{
	char     ipv4[16];  // ipv4 address of host
	uint16_t port;      // port on host
	LogLevel level;     // required logging level
};

// Function which nominally configures the logger
// In this demo it just prints the config value 
void __cheri_libcall logger_config(void *config);

// Function which validates a logger configuration change
bool __cheri_libcall validate_logger_config(void *c);

