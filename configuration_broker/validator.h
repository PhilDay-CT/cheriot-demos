#include <atomic>
#include <compartment.h>

void __cheri_compartment("validator") ValidatorInit();

//bool __cheri_callback check(void * data);