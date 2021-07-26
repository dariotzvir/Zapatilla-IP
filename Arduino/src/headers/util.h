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
    float tension = 220;
    float temp, hum;

    int tempMax = 125, tempMin = -40;
    String usuario = "admin", contra = "12345";

    bool dhcp = 0;
    int puerto = 80;
    byte mac [6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    IPAddress ipDef;
};

struct PINES
{
    const int tomas [N] = { 33, 34, 37, 38, 41 };
    const int pulTomas [N] = { 22, 25, 26, 29, 30 };
    const int ACS [N] = { A8, A9, A10, A11, A12 };

    const int pulMenu [5] = { 49, 45, 46, 42, 18 }; //ONOFF ENT DER IZQ RST
    
    const int pinTension = A13;
    const int pinSD = 4;
    const int pinDHT = 19;
};

#endif