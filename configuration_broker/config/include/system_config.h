// Copyright Configured Things Ltd and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <stdlib.h>

// Mocked example of configuration data for a controller
// with two RGB LEDs (such as on the Sonata Board)

namespace systemConfig {

enum class Kind
{
	lowRISC = 0,
	ConfiguredThings  = 1,
};

const auto IdLength = 16;

struct Config
{
	Kind kind;
	char id[IdLength];
};

} // systemConfig

/**
 * Configure the systemConfig
 */
void __cheri_libcall system_config(void *config);
