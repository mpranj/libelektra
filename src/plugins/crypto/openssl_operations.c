/**
 * @file
 *
 * @brief cryptographic interface using the gcrypt library
 *
 * @copyright BSD License (see doc/COPYING or http://www.libelektra.org)
 *
 */

#include "crypto.h"
#include "openssl_operations.h"
#include <kdberrors.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
#include <openssl/err.h>

static pthread_mutex_t *lockCs;
static int cryptoNumLocks;
static unsigned char cryptoSetup = 0;

void internalLockingCallback(int mode, int type, const char *file ELEKTRA_UNUSED, int line ELEKTRA_UNUSED)
{
    if (mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&(lockCs[type]));
    }
    else
    {
        pthread_mutex_unlock(&(lockCs[type]));
    }
}

void internalThreadId(CRYPTO_THREADID *tid)
{
    CRYPTO_THREADID_set_numeric(tid, (unsigned long)pthread_self());
}

int elektraCryptoOpenSSLInit(Key *errorKey)
{
	// check if libcrypto has already been initialized (possibly by the application)
	if (CRYPTO_get_locking_callback())
	{
		return 1;
	}

	// initialize the internal locking system based on the suggestions by the OpenSSL demos.
	// see demos/threads/mttest.c in the OpenSSL repository for further information
	cryptoNumLocks = CRYPTO_num_locks();
	lockCs = elektraMalloc(cryptoNumLocks * sizeof(pthread_mutex_t));
	for (int i = 0; i < cryptoNumLocks; i++)
	{
		pthread_mutex_init(&(lockCs[i]), NULL);
	}
	CRYPTO_THREADID_set_callback(internalThreadId);
	CRYPTO_set_locking_callback(internalLockingCallback);

	if (ERR_peek_error())
	{
		ELEKTRA_SET_ERRORF(125, errorKey, "libcrypto initialization failed. error code was: %lu", ERR_get_error());
		return (-1);
	}

	cryptoSetup = 1;
	return 1;
}

void elektraCryptoOpenSSLTeardown(void)
{
	if (!cryptoSetup)
	{
		return;
	}

	CRYPTO_set_locking_callback(NULL);
	for (int i = 0; i < cryptoNumLocks; i++)
	{
		pthread_mutex_destroy(&(lockCs[i]));
	}
	elektraFree(lockCs);
}

int elektraCryptoOpenSSLHandleCreate(elektraCryptoHandle **handle, KeySet *config, Key *errorKey)
{
	unsigned char keyBuffer[64], ivBuffer[64];

	*handle = NULL;

	// retrieve keys from configuration
	Key *key = elektraCryptoReadParamKey(config, errorKey);
	Key *iv = elektraCryptoReadParamIv(config, errorKey);
	if (key == NULL || iv == NULL)
	{
		return (-1);
	}

	keyGetBinary(key, keyBuffer, sizeof(keyBuffer));
	keyGetBinary(iv, ivBuffer, sizeof(ivBuffer));

	*handle = elektraMalloc(sizeof(elektraCryptoHandle));
	if (!(*handle))
	{
		memset(keyBuffer, 0, sizeof(keyBuffer));
		memset(ivBuffer, 0, sizeof(ivBuffer));
		ELEKTRA_SET_ERROR(87, errorKey, "Memory allocation failed");
		return (-1);
	}

	EVP_EncryptInit(&((*handle)->encrypt), EVP_aes_256_cbc(), keyBuffer, ivBuffer);
	EVP_DecryptInit(&((*handle)->decrypt), EVP_aes_256_cbc(), keyBuffer, ivBuffer);

	memset(keyBuffer, 0, sizeof(keyBuffer));
	memset(ivBuffer, 0, sizeof(ivBuffer));

	if (ERR_peek_error())
	{
		ELEKTRA_SET_ERRORF(130, errorKey, "Failed to create handle! libcrypto error code was: %lu", ERR_get_error());
		return (-1);
	}

	return 1;
}

void elektraCryptoOpenSSLHandleDestroy(elektraCryptoHandle *handle)
{
	if (handle)
	{
		EVP_CIPHER_CTX_cleanup(&(handle->encrypt));
		EVP_CIPHER_CTX_cleanup(&(handle->decrypt));
		elektraFree(handle);
	}
}

int elektraCryptoOpenSSLEncrypt(elektraCryptoHandle *handle, Key *k, Key *errorKey)
{
	struct ElektraCryptoHeader header;
	// NOTE to prevent memory overflows in libcrypto the buffer holding the encrypted content
	//      is one block bigger than the inupt buffer.
	unsigned char cipherBuffer[2*ELEKTRA_CRYPTO_SSL_BLOCKSIZE];
	unsigned char contentBuffer[ELEKTRA_CRYPTO_SSL_BLOCKSIZE];
	unsigned char *output;
	int written = 0;
	size_t outputLen;
	BIO *encrypted;
	const unsigned char *value = (unsigned char*)keyValue(k);

	// check if key has been marked for encryption
	const Key *metaEncrypt = keyGetMeta(k, ELEKTRA_CRYPTO_META_ENCRYPT);
	if (metaEncrypt == NULL || strlen(keyValue(metaEncrypt)) == 0)
	{
		// nothing to do
		return 1;
	}

	// prepare the crypto header
	header.contentLen = keyGetValueSize(k);
	header.flags = ELEKTRA_CRYPTO_FLAG_NONE;

	switch (keyIsString(k))
	{
	case 1: // string
		header.flags = ELEKTRA_CRYPTO_FLAG_STRING;
		break;
	case 0: // binary
		break;
	case -1: // NULL pointer
		header.flags = ELEKTRA_CRYPTO_FLAG_NULL;
		break;
	}

	encrypted = BIO_new(BIO_s_mem());
	if (!encrypted)
	{
		ELEKTRA_SET_ERROR(87, errorKey, "Memory allocation failed");
		return (-1);
	}

	// encrypt the header
	memcpy(contentBuffer, &header, sizeof(struct ElektraCryptoHeader));
	EVP_EncryptUpdate(&(handle->encrypt), cipherBuffer, &written, contentBuffer, sizeof(struct ElektraCryptoHeader));
	if (written > 0)
	{
		BIO_write(encrypted, cipherBuffer, written);
	}

	if (ERR_peek_error())
	{
		goto error;
	}

	// encrypt content block by block (i = start of the current block)
	for (unsigned int i = 0; i < header.contentLen; i += ELEKTRA_CRYPTO_SSL_BLOCKSIZE)
	{
		// load content partition into the content buffer
		long contentLen = ELEKTRA_CRYPTO_SSL_BLOCKSIZE;
		if ((i + 1) * ELEKTRA_CRYPTO_SSL_BLOCKSIZE > header.contentLen)
		{
			contentLen = header.contentLen - (i * ELEKTRA_CRYPTO_SSL_BLOCKSIZE);
		}
		memcpy(contentBuffer, (value + i), contentLen);

		EVP_EncryptUpdate(&(handle->encrypt), cipherBuffer, &written, contentBuffer, contentLen);
		if (written > 0)
		{
			BIO_write(encrypted, cipherBuffer, written);
		}

		if (ERR_peek_error())
		{
			goto error;
		}
	}

	EVP_EncryptFinal(&(handle->encrypt), cipherBuffer, &written);
	if (written > 0)
	{
		BIO_write(encrypted, cipherBuffer, written);
	}

	if (ERR_peek_error())
	{
		goto error;
	}

	// write back the cipher text to the key
	outputLen = BIO_get_mem_data(encrypted, &output);
	if (outputLen > 0)
	{
		keySetBinary(k, output, outputLen);
		keySetMeta(k, ELEKTRA_CRYPTO_META_ENCRYPTED, "X");
	}
	BIO_free_all(encrypted);

	return 1;

error:
	ELEKTRA_SET_ERRORF(127, errorKey, "Encryption error! libcrypto error code was: %lu", ERR_get_error());
	if (encrypted)
	{
		BIO_free_all(encrypted);
	}
	return (-1);
}

int elektraCryptoOpenSSLDecrypt(elektraCryptoHandle *handle, Key *k, Key *errorKey)
{
	return 1;
}
