/* wolfSSL settings for the Wii build: a TLS 1.2/1.3 client for MQTT
 * (WiFiClientSecure in overlay/lib/WiFi). build.sh compiles wolfSSL with
 * this file into build/libwolfssl.a. Based on the config of the Sonoran Wii
 * app, which runs wolfSSL on the same hardware. */
#ifndef WII_MESHTASTIC_USER_SETTINGS_H
#define WII_MESHTASTIC_USER_SETTINGS_H

#ifdef __BIG_ENDIAN__
#define BIG_ENDIAN_ORDER
#endif

/* Platform: one TLS user (the main loop), our own socket I/O callbacks */
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_DEV_RANDOM
#define WOLFSSL_USER_IO
#define WOLFSSL_NO_SOCK
#define WOLFSSL_IGNORE_FILE_WARN
#define NO_MAIN_DRIVER

/* Seed from the console's entropy source (WiFiClientSecure.cpp) */
#ifdef __cplusplus
extern "C" {
#endif
int wii_tls_seed(unsigned char *out, unsigned int sz);
#ifdef __cplusplus
}
#endif
#define CUSTOM_RAND_GENERATE_SEED wii_tls_seed

/* Protocol: TLS 1.2 and 1.3 client */
#define WOLFSSL_TLS13
#define NO_OLD_TLS
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_EXTENDED_MASTER
#define HAVE_ENCRYPT_THEN_MAC
#define HAVE_SNI
#define HAVE_HKDF
#define HAVE_AEAD
#define NO_DH
#define NO_PSK
#define NO_SESSION_CACHE
#define NO_WOLFSSL_SERVER

/* Cipher suites: AES-GCM and ChaCha20-Poly1305 */
#define HAVE_AESGCM
#define GCM_TABLE_4BIT
#define WOLFSSL_AES_128
#define WOLFSSL_AES_256
#define HAVE_CHACHA
#define HAVE_POLY1305
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512

/* Key exchange and signatures (brokers use RSA or ECDSA certificates) */
#define HAVE_ECC
#define ECC_TIMING_RESISTANT
#define HAVE_ECC384
#define ECC_SHAMIR
#define HAVE_CURVE25519
#define WC_RSA_PSS
#define WC_RSA_BLINDING
#define WOLFSSL_SP_MATH_ALL
#define WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_HAVE_SP_ECC
#define SP_INT_BITS 4096
#define TFM_TIMING_RESISTANT

/* Certificates */
#define WOLFSSL_BASE64_ENCODE
#define WOLFSSL_ALT_CERT_CHAINS

/* Off */
#define NO_DSA
#define NO_RC4
#define NO_MD4
#define NO_DES3
#define NO_PWDBASED
#define NO_OLD_RNGNAME
#define WOLFSSL_NO_SHAKE128
#define WOLFSSL_NO_SHAKE256

#endif
