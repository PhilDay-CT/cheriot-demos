// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <stddef.h>

void __cheri_compartment("crypto") crypto_init();

CHERI::Capability<void> __cheri_compartment("crypto") verify_signature(const void *payload, size_t payloadLength);

