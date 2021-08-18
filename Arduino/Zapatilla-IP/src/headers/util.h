#ifndef __UTIL_H__
#define __UTIL_H__

#include "IPAddress.h"
#include <Arduino.h>

#ifndef N
#define N 5
#endif

enum pantallas 
{
    APAGADA,
    PRINCIPAL,
    TMIN,
    TMAX,
    DHCP,
    IPHOST,
    IPDEF,
    ESTSD,
    RESET
};

enum botones
{
    ONOFF = 5,
    ENTER,
    DER,
    IZQ, 
    RST
};

struct DATA 
{
    bool estTomas [N] = {1, 1, 1, 1, 1};

    float corriente [N] = {0, 0, 0, 0, 0};
    int tension = 220;
    float temp, hum;

    int tempMax = 125, tempMin = -40;
    String usuario = "admin", clave = "12345";

    bool dhcp = 0;
    int puerto = 80;
    byte mac [6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    IPAddress ipDef;

    const float yCalibACS = 0.6;
    const float sensACS = 0.0586;

    const float yCalibZMPT = 0;
    const float sensZMPT = 1.31;
};

struct PINES
{
    const int tomas [N] = { 28, 29, 30, 31, 32 };
    const int leds [N] = { 34, 35, 36, 37, 38 };
    const int pulTomas [N] = { 22, 23, 24, 25, 26 };
    const int ACS [N] = { A8, A9, A10, A11, A12 };

    const int pulMenu [4] = { 43, 41, 42, 40 }; //ONOFF ENT DER IZQ
    
    const int pinZmpt = A13;
    const int pinRst = 18;
    int pinSD = 4;
    const int pinDHT = 19;
};

#endif