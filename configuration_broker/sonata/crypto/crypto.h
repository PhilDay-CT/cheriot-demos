// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <stddef.h>
#include <token.h>

void __cheri_compartment("crypto") crypto_init();

// Verify a signture and return a capabilty to the message 
CHERI::Capability<void> __cheri_compartment("crypto") verify_signature(const void *payload, size_t payloadLength);

// Sign a message and return a capabilty to a heap allocated buffer
// containing the message and the signature
CHERI::Capability<void>  __cheri_compartment("crypto") sign(SObj allocator, const char* context,
               const char *message, size_t messageLength);