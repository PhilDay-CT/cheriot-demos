// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

// Contributed by Configured Things Ltd

#include "cdefs.h"
#include "compartment-macros.h"
#include "token.h"
#include <atomic>
#include <compartment.h>
#include <locks.hh>


// Internal representaion of a configuration token
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
		  size_t     notUsed;                                                  \
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
// SEALED CAPABILITY TO ADD A Validator
//
#define DEFINE_VALIDATE_CONFIG_CAPABILITY(name)                      \
                                                                               \
	DECLARE_AND_DEFINE_STATIC_SEALED_VALUE(                                    \
	  struct {                                                                 \
		  uint16_t   id;                                                       \
		  size_t     notUsed;                                                  \
		  const char ConfigId[sizeof(name)];                                   \
	  },                                                                       \
	  config_broker,                                                           \
	  ValidateConfigKey,                                                          \
	  __validate_config_capability_##name,                                        \
	  0,                                                                       \
	  0,                                                                    \
	  name);

#define VALIDATE_CONFIG_CAPABILITY(name)                                          \
	STATIC_SEALED_VALUE(__validate_config_capability_##name)


//
// Data type for a configuration item.
//
struct ConfigItem
{
	const char   *id;                    // id
	uint32_t     version;                // version
	void         *data;                  // value
	std::atomic<uint32_t> *versionFutex; // Futex to wait for version change
};

/**
 * Set configuration data
 */
int __cheri_compartment("config_broker")
  set_config(SObj configWriteCapability, size_t size, __cheri_callback void cb(void  *buffer, void *context), void *context);

/**
 * Read a configuration value.
 */
ConfigItem __cheri_compartment("config_broker")
  get_config(SObj configReadCapability);


void __cheri_compartment("config_broker")
  set_validator(SObj configValidateCapability,__cheri_callback bool cb(void* buffer));

