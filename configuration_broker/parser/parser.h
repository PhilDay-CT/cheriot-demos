#include <compartment.h>

#include "../logger/logger.h"

int __cheri_compartment("parser") parseLoggerConfig(const char* json, LoggerConfig * config);