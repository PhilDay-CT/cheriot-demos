// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT
namespace systemConfig {

const auto IdLength = 16;

struct Config
{
	char id[IdLength];
	bool switches[8];
};

}

systemConfig::Config get_system_config();
