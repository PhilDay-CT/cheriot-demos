// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <cstdint>
#include <cstdlib>
#include <debug.hh>

void __cheri_compartment("provider") provider_run();

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Crypto">;
using Error = ConditionalDebug<true, "Crypto">;

using CHERI::Capability;

#include "hydrogen.h"

#include "crypto.h"
#include "./config_pub_key.h"
#include "./status_pri_key.h"
#include "./status_pub_key.h"


/**
 * Initaliser for libHyrogen 
 */
void __cheri_compartment("crypto") crypto_init()
{
	hydro_init();

	// Call the next step in the init chain
	provider_run();
}

/** 
 * Verify a signature in a payload.
 *
 * Packed as Context[8] + Signature[64] + Message
 */
Capability<void> __cheri_compartment("crypto") verify_signature(const void *payload, size_t payloadLength)
{
	size_t messageOffset = hydro_sign_CONTEXTBYTES + hydro_sign_BYTES;
	if (payloadLength <= messageOffset)
	{
		Error::log("Message too short to verify");
		return nullptr;
	}
	const char *context = (char *)payload;
	const uint8_t *signature = (uint8_t *)(context + hydro_sign_CONTEXTBYTES);
	void *message = (void *)(context + messageOffset);
	size_t messageLength = payloadLength - messageOffset;
	
	//
	// Stuff to look up key from context goes here
	//
	uint8_t *key = config_pub_key;
	
	if (hydro_sign_verify(signature, message, messageLength, context, key) == 0)
	{
		Debug::log("Signature Verified");
		Capability result {message};
		result.permissions() &= {CHERI::Permission::Load};
		result.bounds() = messageLength;
		return result;  
	}
	else {
		Error::log("Signature validation failed");
		return nullptr;
	}
}

/**
 * Create a signed message.
 *
 * Packed as Context[8] + Signature[64] + Message
 */
CHERI::Capability<void>  __cheri_compartment("crypto") sign(SObj allocator, const char* context,
               const char *message, size_t messageLength) {

	Timeout t{100};
	size_t s_size = hydro_sign_CONTEXTBYTES + hydro_sign_BYTES + messageLength;
	void *signed_message = heap_allocate(&t, allocator, s_size);
	if (!Capability{signed_message}.is_valid())
	{
		Debug::log("Failed to allocate {} of heap for signed message", s_size);
		return signed_message;
	}

	char *s_context = (char *)signed_message;
	uint8_t *s_signature = (uint8_t *)s_context + hydro_sign_CONTEXTBYTES;
	char *s_message = (char *)s_signature + hydro_sign_BYTES;

	strncpy(s_context, context, hydro_sign_CONTEXTBYTES);
	strncpy(s_message, message, messageLength);
	
	// Stuff to look up key from context goes here
	//
	uint8_t *key = status_pri_key;
	
	hydro_sign_create(s_signature, message, messageLength, context, status_pri_key);
	Debug::log("Signature generated");
	
	// Create a read only capabilty that is bound to the size of the message
	Capability<void>s_cap = {signed_message};
	s_cap.permissions() &= {CHERI::Permission::Load};
	s_cap.bounds() = s_size;

	return s_cap;
}
 