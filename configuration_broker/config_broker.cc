// Copyright Microsoft and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

// Contributed by Configured Things Ltd

#include "cdefs.h"
#include "cheri.hh"
#include <compartment.h>
#include <cstdlib>
#include <debug.hh>
#include <fail-simulator-on-error.h>
#include <futex.h>
#include <locks.hh>
#include <string.h>
#include <thread.h>
#include <vector>

#include "config_broker.h"
#include "validator/validator.h"

// Import some useful things from the CHERI namespace.
using namespace CHERI;

// Debuging can be enable with "xmake --config --debug-config_broker=true"
using Debug = ConditionalDebug<DEBUG_CONFIG_BROKER, "Config Broker">;

// Internal view of a Config Item
struct InternalConfigitem
{
	std::atomic<uint32_t> version; // version - used as a futex
	void                 *data;    // value
	const char           *id;      // id
	FlagLock             lock;     // lock to prevent concurrent changes
	bool                 __cheri_callback (*validator)(void *); // validator method
};

//
// Set of config data items.
//
std::vector<struct InternalConfigitem *> configData;

//
// Sealing Types
//
#define CONFIG_WRITE STATIC_SEALING_TYPE(WriteConfigKey)
#define CONFIG_READ STATIC_SEALING_TYPE(ReadConfigKey)
#define CONFIG_VALIDATE STATIC_SEALING_TYPE(ValidateConfigKey)



//
// unseal a config capability.
//
ConfigToken *config_capability_unseal(SObj sealedCap, SKey key)
{
	ConfigToken *token = token_unseal(key, Sealed<ConfigToken>{sealedCap});

	if (token == nullptr)
	{
		Debug::log("invalid config capability {}", sealedCap);
		return nullptr;
	}

	Debug::log("Unsealed id: {} size:{} item: {}",
	           token->id,
	           token->maxSize,
	           token->ConfigId);

	if (token->id == 0)
	{
		// Assign an ID so we can track the callbacks added
		// from this capability
		static uint16_t nextId = 1;
		token->id              = nextId++;
	}

	return token;
}

//
// Find a Config by name.  If it doesn't already exist
// create one.  Use a LockGuard to protect aginst two
// threads trying to create the same item.
//
FlagLock lockFindOrCreate;

InternalConfigitem *find_or_create_config(ConfigToken *token)
{
	LockGuard g{lockFindOrCreate};

	for (auto &c : configData)
	{
		if (strcmp(c->id, token->ConfigId) == 0)
		{
			return c;
		}
	}

	// Allocate a Config object
	InternalConfigitem *c = static_cast<InternalConfigitem *>(malloc(sizeof(InternalConfigitem)));

	// Use the ConfigId from the token that triggered the creation
	// as the name value, since sealed objects are guaranteed not
	// to be deallocated.
	c->id      = token->ConfigId;
	c->version = 0;
	c->data    = nullptr;

	// Add it to the vector
	configData.push_back(c);

	return c;
};

//
// Set a new value for the configuration item described by
// the capability.  Use a LockGuard to protect against multiple
// threads trying to set the same item.
//
FlagLock lockSetConfig;

int __cheri_compartment("config_broker")
  set_config(SObj sealedCap, size_t size, __cheri_callback void cb(void* buffer, void *context), void *context)
{
	Debug::log("thread {} Set config called with {} {} {}",
	           thread_id_get(),
	           sealedCap,
	           size, 
			   context);
	
	//
	// Check that we've been given a valid capability 
	//
	ConfigToken *token = config_capability_unseal(sealedCap, CONFIG_WRITE);
	if (token == nullptr)
	{
		Debug::log("Invalid capability: {}", sealedCap);
		return -1;
	}

	// Check the size is within the allowed range
	if (size > token->maxSize)
	{
		Debug::log("invalid size {} for capability: {}", size, sealedCap);
		return -1;
	}

	//
	// We need to load all the validators before we can
	// accept any updates. Rather than have a thread in
	// the validator compartment that runs to do that and
	// then has to hang around (as thread can't exit) we
	// use the first thread that tries to load a value to
	// trigger that.
	//
	static bool ValidatorsLoaded = false;
	if (!ValidatorsLoaded) {
		ValidatorInit();
		ValidatorsLoaded = true;
	}

	// Find or create a config structure
	InternalConfigitem *c = find_or_create_config(token);

	//
	// Guard against concurrent updates
	//
	LockGuard g{c->lock};
	//LockGuard g{lockSetConfig};
	
	

	/*
	if (size > static_cast<size_t>(Capability{data}.bounds()))
	{
		Debug::log("size {} > data.bounds() {}", size, data);
		return -1;
	}
	*/

	// Allocate heap space for the new value
	void *newData = malloc(size);
	if (newData == nullptr)
	{
		Debug::log("Failed to allocate space for {}", token->ConfigId);
		return -1;
	}

	// Create a write only Capability to pass to the callback
	CHERI::Capability woNewData{newData};
	woNewData.permissions() &= {CHERI::Permission::Store};
	
	// Invoke the callback to copy the data
	cb(woNewData, context);

	// Call the validator
	if (!c->validator(newData)) {
		Debug::log("Validator failed for {}", token->ConfigId);
		return -1;
	}

	// Free the old data value.  Any subscribers that received it should
	// have thier own claim on it if needed
	if (c->data)
	{
		free(c->data);
	}

	// Neither we nor the subscribers need to be able to update the
	// value, so just track through a readOnly capabaility
	CHERI::Capability roData{newData};
	roData.permissions() &=
	  roData.permissions().without(CHERI::Permission::Store);
	c->data = roData;
	Debug::log("Data {}", c->data);

	// Mark it as having been updated
	c->version++;

	// Notify anyone waiting for the version to change
	Debug::log("Waking subscribers {}", c->version.load());
	c->version.notify_all();

	return 0;
}

//
// Get the current value of a Configuration item.  The data
// member will be nullptr if the item has not yet been set.
//
ConfigItem __cheri_compartment("config_broker") get_config(SObj sealedCap)
{
	// Object to return.  Stack is initialised to zeros
	ConfigItem result;

	Debug::log(
	  "thread {} get_config called with {}", thread_id_get(), sealedCap);

	// Get the calling compartments name from
	// its sealed capability
	ConfigToken *token = config_capability_unseal(sealedCap, CONFIG_READ);

	if (token == nullptr)
	{	
		// Didn't get passed a vaild Read Capability
	 	Debug::log("Invalid capability {}", sealedCap);
	 	return result;
	}

	auto c = find_or_create_config(token);

	//
	// lock to prevent concurrent set/get on the same 
	// item
	LockGuard g{c->lock};
	
	//
	// Populate the return object.
	//
	
	// id is a read only pointer as its static
	result.id = c->id; 
	
	// Provide the version value at this point in time 	
	result.version = c->version.load();

	// Data is already a read only pointer
	result.data = c->data;

	// Create a readonly pointer to the version futex
	CHERI::Capability roFutex{&c->version};
	roFutex.permissions() &=
	  roFutex.permissions().without(CHERI::Permission::Store);
	result.versionFutex = roFutex;

	return result;
}


void __cheri_compartment("config_broker") set_validator(SObj sealedCap, __cheri_callback bool cb(void* buffer)) {
	Debug::log(
	  "thread {} set validator called with {}", thread_id_get(), sealedCap);

	// Get the calling compartments name from
	// its sealed capability
	
	ConfigToken *token = config_capability_unseal(sealedCap, CONFIG_VALIDATE);
	Debug::log(
	  "thread {} unseal gave {}", thread_id_get(), token);
	
	if (token == nullptr)
	{	
		// Didn't get passed a valid Read Capability
	 	Debug::log("Invalid capability {}", sealedCap);
	 	return;
	}

	auto c = find_or_create_config(token);
	c->validator = cb;
	
	return;
}