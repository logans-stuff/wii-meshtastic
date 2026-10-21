/*
  WiFiClient.cpp - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef wificlient_h
#define wificlient_h
#include "Arduino.h"	
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"

class WiFiClient : public Client {

public:
  WiFiClient(int psock = 0, uint32_t peerAddr = 0, uint16_t peerPort = 0);

  uint8_t status();
  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual int available();
  virtual int read();
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek();
  virtual void flush();
  virtual void stop();
  virtual uint8_t connected();
  virtual operator bool();
  virtual IPAddress remoteIP() const;
  virtual uint16_t remotePort() const;
  virtual IPAddress localIP() const;
  virtual uint16_t localPort() const;

  friend class WiFiServer;

  using Print::write;

protected: // Wii: WiFiClientSecure runs TLS over this socket
  int psock;
  /// Wii: libogc has no getpeername, so the peer is remembered at connect/accept
  uint32_t peerAddr = 0; // network byte order
  uint16_t peerPort = 0;

  /// Wii: bytes already taken from the socket. One recv() drains what IOS
  /// holds, so data that arrived before a reset (a client that sends and
  /// closes at once) is still read; IOS drops unread data on a reset.
  uint8_t rxbuf[256];
  int rxpos = 0, rxlen = 0;

  int errorCode = -1;
};

#endif
