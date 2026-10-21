// Wii: TLS client for the firmware's MQTT module (MQTT.h enables TLS when
// <WiFiClientSecure.h> exists). wolfSSL over WiFiClient's libogc socket,
// configured by tls/user_settings.h. Only what MQTT uses: setInsecure()
// (MQTT never pins a certificate) and the Client interface.
#ifndef wificlientsecure_h
#define wificlientsecure_h
#include "WiFiClient.h"

struct WOLFSSL;

class WiFiClientSecure : public WiFiClient
{
  public:
    WiFiClientSecure() {}
    ~WiFiClientSecure();
    void setInsecure() { insecure = true; }

    virtual int connect(const char *host, uint16_t port) override;
    virtual int connect(IPAddress ip, uint16_t port) override;
    virtual size_t write(uint8_t b) override { return write(&b, 1); }
    virtual size_t write(const uint8_t *buf, size_t size) override;
    virtual int available() override;
    virtual int read() override;
    virtual int read(uint8_t *buf, size_t size) override;
    virtual int peek() override;
    virtual void stop() override;
    virtual uint8_t connected() override;

    // wolfSSL I/O callbacks (public for the C linkage glue in the .cpp)
    int ioRecv(char *buf, int sz);
    int ioSend(const char *buf, int sz);

  private:
    WOLFSSL *ssl = nullptr;
    bool insecure = false;
    bool handshaking = false;
};
#endif
