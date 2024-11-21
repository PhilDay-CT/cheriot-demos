// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <debug.hh>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Hydro test">;

#define CONTEXT "Examples"
#define MESSAGE "X"
#define MESSAGE_LEN 1

#include "hydrogen.h"

#include "./config_pub_key.h"
#include "./config_pri_key.h"

void hexToString(char *buf, int s)
{
	const char Hexdigits[] = "0123456789abcdef";
	buf[1] = Hexdigits[s & 0xf];
	s >>= 4;
	buf[0] = Hexdigits[s & 0xf];
}

void printHexString(const char *label, uint8_t *data, size_t dataLength) {
	char buffer[2*dataLength+1];
	for (auto i=0; i < dataLength; i++)
	{
		hexToString(buffer+(2*i), *(data+i));
	}
	buffer[2*dataLength] = 0;
	Debug::log("{}: {}", label, (const char*)buffer);
}


/// Local implementation of hydeo_sign_verify
int
local_sign_verify(const uint8_t csig[hydro_sign_BYTES], const void *m_, size_t mlen,
                  const char    ctx[hydro_sign_CONTEXTBYTES],
                  const uint8_t pk[hydro_sign_PUBLICKEYBYTES])
{
    hydro_sign_state st;

	int res;
	res = hydro_sign_init(&st, ctx);
	Debug::log("sign init gave {}", res);

    res =  hydro_sign_update(&st, m_, mlen);
	Debug::log("sign update gave {}", res);

	res = hydro_sign_final_verify(&st, csig, pk);
	Debug::log("sign final verify gave {}", res);

    return 0;
}


void hash() {
	Debug::log("----------- hash ------------");
	Debug::log("Creating hash of: {} with {}", MESSAGE, CONTEXT);
	
	uint8_t hash[hydro_hash_BYTES];
	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, NULL);
	printHexString("hash         ", hash, hydro_hash_BYTES);

	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	printHexString("hash with key", hash, hydro_hash_BYTES);
}


void sign() {
	Debug::log("----------- Sign and verify ------------");
	
	printHexString("message", (uint8_t *)MESSAGE, MESSAGE_LEN);
	printHexString("context", (uint8_t *)CONTEXT, 8);
	
	uint8_t signature[hydro_sign_BYTES];
	for (int i=0; i<hydro_sign_BYTES; i++) {
		signature[i] = 0;	
	}
	Debug::log("*** SIGN");
	hydro_sign_create(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pri_key);
	Debug::log("signed");
	printHexString("Priv key ", config_pri_key, hydro_sign_SECRETKEYBYTES);
	printHexString("signature", signature, hydro_sign_BYTES);
	
	Debug::log("\n");
	for (int i=0; i<hydro_sign_BYTES; i++) {
		signature[i] = 0;	
	}
	Debug::log("*** SIGN");
	hydro_sign_create(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pri_key);
	Debug::log("signed");
	printHexString("Priv key ", config_pri_key, hydro_sign_SECRETKEYBYTES);
	printHexString("signature", signature, hydro_sign_BYTES);
	
	/* Verify the signature using the public key */
	int res = hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	Debug::log("");
	printHexString("Pub  key ", config_pub_key, hydro_sign_PUBLICKEYBYTES);
	Debug::log("sign verify for good signature gives {}", res);

	uint8_t x = signature[0];
    signature[0] = 0;
    res = hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	Debug::log("sign verify for bad signature gives {}", res);

    signature[0] = x;
    res = hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	Debug::log("sign verify for restored signature gives {}", res);
}


/// Thread entry point.
void __cheri_compartment("crypto") crypto_test()
{
	Debug::log("--- libhydrogen tests ---");
	
	hydro_init();
	hash();
	sign();
}
