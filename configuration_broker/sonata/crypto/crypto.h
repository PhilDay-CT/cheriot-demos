// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <stddef.h>
#include <token.h>

// Helper functions
namespace CRYPTO {

struct Message {
    CHERI::Capability<void> data;
    size_t length;
};

void __cheri_compartment("crypto") crypto_init();

// Verify a signture and return the embedded message 
Message __cheri_compartment("crypto") verify_signature(const void *payload, size_t payloadLength);

// Sign a message and return new message which includes a capabilty to a heap allocated buffer
// containing the message and the signature
Message __cheri_compartment("crypto") sign(SObj allocator, const char* context,
               const char *message, size_t messageLength);

} // namespace CRYPTO

