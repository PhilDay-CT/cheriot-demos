// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

// Contributed by Configured Things Ltd

#include "cdefs.h"
#include "compartment-macros.h"
#include "token.h"
#include <compartment.h>

// Interal representaion of a configuration token
struct ConfigToken
{
	uint16_t   id;         // id for the capability, assigned on first use
	size_t     maxSize;    // Max size of the item
	const char ConfigId[]; // Name of the configuration item
};

//
// SEALED CAPABILITY TO READ CONFIG
//
#define DEFINE_READ_CONFIG_CAPABILITY(name)                                    \
                                                                               \
	DECLARE_AND_DEFINE_STATIC_SEALED_VALUE(                                    \
	  struct {                                                                 \
		  uint16_t   id;                                                       \
		  size_t     maxSize;                                                  \
		  const char ConfigId[sizeof(name)];                                   \
	  },                                                                       \
	  config_broker,                                                           \
	  ReadConfigKey,                                                           \
	  __read_config_capability_##name,                                         \
	  0,                                                                       \
	  0,                                                                       \
	  name);

#define READ_CONFIG_CAPABILITY(name)                                           \
	STATIC_SEALED_VALUE(__read_config_capability_##name)

//
// SEALED CAPABILITY TO WRITE CONFIG
//
#define DEFINE_WRITE_CONFIG_CAPABILITY(name, size)                             \
                                                                               \
	DECLARE_AND_DEFINE_STATIC_SEALED_VALUE(                                    \
	  struct {                                                                 \
		  uint16_t   id;                                                       \
		  size_t     maxSize;                                                  \
		  const char ConfigId[sizeof(name)];                                   \
	  },                                                                       \
	  config_broker,                                                           \
	  WriteConfigKey,                                                          \
	  __write_config_capability_##name,                                        \
	  0,                                                                       \
	  size,                                                                    \
	  name);

#define WRITE_CONFIG_CAPABILITY(name)                                          \
	STATIC_SEALED_VALUE(__write_config_capability_##name)

//
// Data type for a configuration item.  The version is used
// as a futex when waiting for updates
//
struct ConfigItem
{
	uint32_t version; // version - used as a futex
	void    *data;    // value
};

/**
 * Set configuration data
 */
int __cheri_compartment("config_broker")
  set_config(SObj configWriteCapability, void *data, size_t size);

/**
 * Read a configuration value.
 */
ConfigItem *__cheri_compartment("config_broker")
  get_config(SObj configReadCapability);
