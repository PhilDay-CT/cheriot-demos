#include <stdio.h>

#include <hydrogen.h>

#define CONTEXT "Examples"
#define MESSAGE "Arbitrary data to hash"
#define MESSAGE_LEN 22

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
	for (int i=0; i < dataLength; i++)
	{
		hexToString(buffer+(2*i), *(data+i));
	}
	buffer[2*dataLength] = 0;
	printf("%s: %s\n", label, (const char*)buffer);
}

void hash() {
	printf("----------- hash ------------\n");
	printf("Creating hash of: %s with %s\n", MESSAGE, CONTEXT);
	
	uint8_t hash[hydro_hash_BYTES];
	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, NULL);
	printHexString("hash         ", hash, hydro_hash_BYTES);

	hydro_hash_hash(hash, sizeof hash, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	printHexString("hash with key", hash, hydro_hash_BYTES);
}

void sign() {
   printf("------------ sign -------------\n");

	printHexString("message", (uint8_t *)MESSAGE, MESSAGE_LEN);
	printHexString("context", (uint8_t *)CONTEXT, 8);
	
   uint8_t signature[hydro_sign_BYTES];

   hydro_sign_create(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pri_key);
	printf("signed\n");
	printHexString("Priv key ", config_pri_key, hydro_sign_SECRETKEYBYTES);
	printHexString("signature", signature, hydro_sign_BYTES);
	
	/* Verify the signature using the public key */
	int res = hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	printf("\n");
	printHexString("Pub  key ", config_pub_key, hydro_sign_PUBLICKEYBYTES);
	printf("sign verify for good signature gives %d\n", res);

    uint8_t x = signature[0];
    signature[0] = 0;
    res = hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	printf("sign verify for bad signature gives %d\n", res);

    signature[0] = x;
    res = hydro_sign_verify(signature, MESSAGE, MESSAGE_LEN, CONTEXT, config_pub_key);
	printf("sign verify for restored signature gives %d\n", res);

}

int main() {
   printf("--- libhydrogen test ---\n");
   
   hydro_init();
   hash();
   sign();
}
