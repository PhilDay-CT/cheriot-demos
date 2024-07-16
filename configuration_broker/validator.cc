#include <atomic>
#include <compartment.h>
#include <debug.hh>
#include <thread.h>

// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Validator">;

#include "validator.h"

#include "config_broker.h"
#define CONFIG1 "config1"
DEFINE_VALIDATE_CONFIG_CAPABILITY(CONFIG1);

bool __cheri_callback check(void * data) {
    Debug::log( "thread {} called validator",thread_id_get());
    return true;
}

void __cheri_compartment("validator") ValidatorInit()
{
    Debug::log("Validator init {}", thread_id_get());
    set_validator(VALIDATE_CONFIG_CAPABILITY(CONFIG1), check);
    Debug::log("******** Validator Done {}", thread_id_get());    
}