// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <debug.hh>

void __cheri_compartment("provider") provider_run();

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Crypto">;
using Error = ConditionalDebug<true, "Crypto">;

using CHERI::Capability;

#include "hydrogen.h"

#include "crypto.h"
#include "./config_pub_key.h"



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
		result.bounds() = messageLength;
		return result;  
	}
	else {
		Error::log("Signature validation failed");
		return nullptr;
	}
}

