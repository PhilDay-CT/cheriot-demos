// Copyright Configured Things and CHERIoT Contributors.
// SPDX-License-Identifier: MIT

#include <compartment.h>
#include <debug.hh>

/// Expose debugging features unconditionally for this compartment.
using Debug = ConditionalDebug<true, "Hydro test">;

#define CONTEXT "Examples"
#define MESSAGE "Arbitrary data to hash"
#define MESSAGE_LEN 22

#include "hydrogen.h"

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


void random() {
	Debug::log("----------- random ------------");

	for (auto i=0; i<5; i++)
	{
		uint32_t r_uint32 = hydro_random_u32();
		Debug::log("random uint32: {}", r_uint32);
	}

	for (auto i=0; i<5; i++)
	{
		uint32_t r_in_0_to_99 = hydro_random_uniform(100);
		Debug::log("random uniform: {}", r_in_0_to_99);
	}

	for (auto i=0; i<3; i++)
	{
		uint8_t buf[32];
		hydro_random_buf(buf, sizeof buf);
		printHexString("random buf", buf, sizeof buf);
	}

	for (auto i=0; i<3; i++)
	{
		uint8_t seed[hydro_random_SEEDBYTES] = { 0 };
		uint8_t buf[32];
		hydro_random_buf_deterministic(buf, sizeof buf, seed);
		printHexString("random buf deterministic", buf, sizeof buf);
	}
}

void hash() {
	Debug::log("----------- hash ------------");
	Debug::log("Creating hash of: {}", MESSAGE);
	
	uint8_t hash[hydro_hash_BYTES];
	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, NULL);
	printHexString("hash", hash, hydro_hash_BYTES);
}

void keyed_hash() {
	Debug::log("----------- keyed hash xxx ------------");
	Debug::log("Creating hash of: {}", MESSAGE);
	
	uint8_t hash[hydro_hash_BYTES];
	uint8_t key[hydro_hash_KEYBYTES];
	hydro_hash_keygen(key);
	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, key);
	
	printHexString("key", key, hydro_hash_KEYBYTES);
	printHexString("hash-1", hash, hydro_hash_BYTES);

	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, key);
	printHexString("hash-2", hash, hydro_hash_BYTES);
}

void sign() {
	Debug::log("----------- Sign and verify ------------");
	
	hydro_sign_keypair key_pair;
	hydro_sign_keygen(&key_pair);

	uint8_t signature[hydro_sign_BYTES];
	Debug::log("key pair created");
	printHexString("secret key", key_pair.sk, hydro_sign_SECRETKEYBYTES);
	printHexString("public key", key_pair.pk, hydro_sign_PUBLICKEYBYTES);

	/* Sign the message using the secret key */
	hydro_sign_create(signature, MESSAGE, MESSAGE_LEN, CONTEXT, key_pair.sk);
	Debug::log("signed");
	printHexString("signature", signature, hydro_sign_BYTES);
	
	/* Verify the signature using the public key */
	if (hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, key_pair.pk) == 0) {
		Debug::log("Verified");
	}
	else
	{
		Debug::log("Verify failed");
	}
}

void check_signature() {

	static uint8_t pubKey[] = {
		0x28, 0x6f, 0x47, 0x98, 0x84, 0x68, 0x49, 0xac, 0xd5, 0x9b, 0x93, 0x09, 0x7b, 0x01, 0x92, 0x95, 
		0xcf, 0x44, 0xfa, 0xad, 0xa0, 0x48, 0x08, 0xc9, 0x6a, 0x2a, 0x27, 0x0c, 0x09, 0x01, 0x0c, 0x6c, 
	};

	static uint8_t signature[] = {
		0x80, 0xe2, 0x09, 0x21, 0xaa, 0xd0, 0xcc, 0x84, 0x72, 0x9d, 0xdc, 0xd9, 0xf5, 0xe0, 0x51, 0x58, 
		0x5c, 0xe7, 0x9e, 0x56, 0xc9, 0xee, 0xd7, 0x14, 0xf3, 0x0a, 0x2e, 0xc7, 0xf1, 0x8b, 0xa1, 0x76, 
		0x4f, 0x88, 0x44, 0xe8, 0xda, 0x0a, 0x90, 0x6a, 0x81, 0x42, 0x83, 0xe0, 0xf8, 0x16, 0x6d, 0x94, 
		0x1a, 0x27, 0xee, 0xff, 0x27, 0xff, 0xaf, 0xfe, 0x70, 0x61, 0x31, 0x1f, 0x25, 0xa6, 0x0a, 0x07, 
	};

	Debug::log("----------- verify webm signature ------------");
	
	if (hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, pubKey) == 0) {
		Debug::log("Verified");
	}
	else
	{
		Debug::log("Verify failed");
	}

}

/// Thread entry point.
void __cheri_compartment("crypto") crypto_test()
{
	Debug::log("--- Crypto tests ---");
	hydro_init();
	
	random();
	hash();
	keyed_hash();
	sign();
	check_signature();
}
