// Wii: TLS (wolfSSL) over WiFiClient's libogc socket; see WiFiClientSecure.h.
#include "WiFiClientSecure.h"
#include "ogc_posix_net.h"

#include <errno.h>
#include <reent.h>
#include <stdio.h>
#include <string.h>

#include <wolfssl/wolfcrypt/settings.h> // first: pulls in tls/user_settings.h
#include <wolfssl/ssl.h>

#include <gccore.h>
#include <ogc/lwp_watchdog.h>

// How long a TLS step may wait on the peer. Handshakes run inside the
// firmware's single-threaded loop, so a dead broker must fail, not hang.
#define TLS_HANDSHAKE_WAIT_MS 8000
#define TLS_SEND_WAIT_MS 2000

extern "C" int _getentropy_r(struct _reent *r, void *buf, size_t len); // portduino_glue_wii.cpp

extern "C" int wii_tls_seed(unsigned char *out, unsigned int sz)
{
    return _getentropy_r(_REENT, out, sz) == 0 ? 0 : -1;
}

static WOLFSSL_CTX *tlsContext()
{
    static WOLFSSL_CTX *ctx;
    if (!ctx) {
        wolfSSL_Init();
        ctx = wolfSSL_CTX_new(wolfSSLv23_client_method()); // TLS 1.2 or 1.3, whichever the broker offers
        if (!ctx)
            return nullptr;
        wolfSSL_SetIORecv(ctx, [](WOLFSSL *, char *buf, int sz, void *c) {
            return static_cast<WiFiClientSecure *>(c)->ioRecv(buf, sz);
        });
        wolfSSL_SetIOSend(ctx, [](WOLFSSL *, char *buf, int sz, void *c) {
            return static_cast<WiFiClientSecure *>(c)->ioSend(buf, sz);
        });
    }
    return ctx;
}

int WiFiClientSecure::ioRecv(char *buf, int sz)
{
    unsigned ready = handshaking ? wii_poll_wait(psock, WII_POLLIN, TLS_HANDSHAKE_WAIT_MS) : wii_poll(psock, WII_POLLIN);
    if (!(ready & (WII_POLLIN | WII_POLLERR | WII_POLLHUP)))
        return handshaking ? WOLFSSL_CBIO_ERR_TIMEOUT : WOLFSSL_CBIO_ERR_WANT_READ;
    long n = wii_read(psock, buf, sz);
    if (n > 0)
        return (int)n;
    if (n == 0)
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return WOLFSSL_CBIO_ERR_WANT_READ;
    return errno == ECONNRESET ? WOLFSSL_CBIO_ERR_CONN_RST : WOLFSSL_CBIO_ERR_GENERAL;
}

int WiFiClientSecure::ioSend(const char *buf, int sz)
{
    int wait = handshaking ? TLS_HANDSHAKE_WAIT_MS : TLS_SEND_WAIT_MS;
    if (!(wii_poll_wait(psock, WII_POLLOUT, wait) & (WII_POLLOUT | WII_POLLERR | WII_POLLHUP)))
        return WOLFSSL_CBIO_ERR_TIMEOUT;
    long n = wii_write(psock, buf, sz); // blocking socket: the count is exact
    if (n > 0)
        return (int)n;
    return errno == ECONNRESET || errno == EPIPE ? WOLFSSL_CBIO_ERR_CONN_RST : WOLFSSL_CBIO_ERR_GENERAL;
}

int WiFiClientSecure::connect(const char *host, uint16_t port)
{
    if (!WiFiClient::connect(host, port)) // also drops any previous session (stop())
        return 0;
    WOLFSSL_CTX *ctx = tlsContext();
    if (!ctx || !(ssl = wolfSSL_new(ctx))) {
        ::printf("WiFiClientSecure: no TLS session (out of memory?)\n");
        WiFiClient::stop();
        return 0;
    }
    // MQTT only ever calls setInsecure(): the session is encrypted, the
    // broker's certificate is not checked (as on the other platforms).
    wolfSSL_set_verify(ssl, insecure ? WOLFSSL_VERIFY_NONE : WOLFSSL_VERIFY_PEER, nullptr);
    wolfSSL_SetIOReadCtx(ssl, this);
    wolfSSL_SetIOWriteCtx(ssl, this);
    struct in_addr literal;
    if (!inet_aton(host, &literal)) // SNI for hostnames: shared brokers pick the certificate by it
        wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host, (unsigned short)strlen(host));

    uint64_t started = gettime();
    handshaking = true;
    int ret = wolfSSL_connect(ssl);
    handshaking = false;
    unsigned ms = (unsigned)ticks_to_millisecs(gettime() - started);
    if (ret != WOLFSSL_SUCCESS) {
        char reason[80];
        wolfSSL_ERR_error_string_n(wolfSSL_get_error(ssl, ret), reason, sizeof(reason));
        ::printf("WiFiClientSecure: TLS handshake with %s:%u failed after %u ms: %s\n", host, port, ms, reason);
        stop();
        return 0;
    }
    ::printf("WiFiClientSecure: %s with %s:%u (%s) in %u ms\n", wolfSSL_get_version(ssl), host, port,
             wolfSSL_get_cipher(ssl), ms);
    return 1;
}

int WiFiClientSecure::connect(IPAddress ip, uint16_t port)
{
    char host[16];
    snprintf(host, sizeof(host), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return connect(host, port);
}

size_t WiFiClientSecure::write(const uint8_t *buf, size_t size)
{
    if (!ssl || size == 0)
        return 0;
    int n = wolfSSL_write(ssl, buf, (int)size);
    if (n <= 0) {
        errorCode = ECONNRESET;
        return 0;
    }
    return (size_t)n;
}

int WiFiClientSecure::available()
{
    if (!ssl)
        return 0;
    int pending = wolfSSL_pending(ssl);
    if (pending > 0)
        return pending;
    if (errorCode != 0)
        return 0;
    char c;
    int n = wolfSSL_peek(ssl, &c, 1); // pulls in the next record if one has arrived
    if (n > 0)
        return wolfSSL_pending(ssl);
    int err = wolfSSL_get_error(ssl, n);
    if (err != WOLFSSL_ERROR_WANT_READ && err != WOLFSSL_ERROR_WANT_WRITE)
        errorCode = ECONNRESET; // close_notify, reset or a broken record
    return 0;
}

int WiFiClientSecure::read()
{
    uint8_t b;
    return read(&b, 1) == 1 ? b : -1;
}

int WiFiClientSecure::read(uint8_t *buf, size_t size)
{
    int n = available();
    if (n <= 0)
        return -1;
    if ((size_t)n > size)
        n = (int)size;
    n = wolfSSL_read(ssl, buf, n);
    return n > 0 ? n : -1;
}

int WiFiClientSecure::peek()
{
    char c;
    if (available() <= 0 || wolfSSL_peek(ssl, &c, 1) != 1)
        return -1;
    return (uint8_t)c;
}

uint8_t WiFiClientSecure::connected()
{
    if (!psock || !ssl)
        return 0;
    return errorCode == 0 || wolfSSL_pending(ssl) > 0;
}

void WiFiClientSecure::stop()
{
    if (ssl) {
        wolfSSL_free(ssl); // no close_notify: the socket may already be dead
        ssl = nullptr;
    }
    WiFiClient::stop();
}

WiFiClientSecure::~WiFiClientSecure()
{
    stop();
}
