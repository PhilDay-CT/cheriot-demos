// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#define CHERIOT_NO_AMBIENT_MALLOC
#define CHERIOT_NO_NEW_DELETE

#include <compartment.h>
#include <debug.hh>

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Validator">;


// Set for Items we are allowed to register a validator for 
#include "../config_broker.h"
#define CONFIG1 "config1"
DEFINE_VALIDATE_CONFIG_CAPABILITY(CONFIG1);

//
// LoggerConfig Validator
//
// Even through this is a library call we wrap it in
// out own callback so that it runs in this sandbox 
//
#include "../logger/logger.h"
bool __cheri_callback validateLogger(void * data) {
    bool valid = validate_logger_config(data);
    if (!valid) {
        Debug::log("thread {} Logger Config validation failed", thread_id_get());
    }
    return valid;
}

//
// Register all of our validators. This needs to be run before
// any values can be accepted.  
//
// There is no init mechanism in cheriot and threads are not
// expected to terminate, so rather than have a separate thread
// just to run this which then blocks we expose it as method for
// the Broker to call when the first item is published.
//
void __cheri_compartment("validator") ValidatorInit()
{
    // Logger Config Validator 
    set_validator(VALIDATE_CONFIG_CAPABILITY(CONFIG1), validateLogger);    
}