// libogc's <netinet/in.h> without ntohl/htonl, which glibc's provides and
// src/mqtt/MQTT.cpp relies on: add them from libogc's <arpa/inet.h>.
#pragma once
#include_next <netinet/in.h>
#include <arpa/inet.h>
