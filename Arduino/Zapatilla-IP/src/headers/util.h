#ifndef __UTIL_H__
#define __UTIL_H__

//Librerías:
#include <Arduino.h>
#include <string.h>
#include <IPAddress.h>

#ifndef N
#define N 5
#endif
#define BAUD1 9600
#define BAUD2 115200
#define TEMP_MAX 80
#define TEMP_MIN -40
#define PUERTO_DEFAULT 80

//#define DEBUGSD
//#define DEBUGMAC
//#define DEBUGDHCP
//#define DEBUGPET
//#define DEBUGPUL
//#define DEBUGANALOG

enum pantallas
{
    APAGADA = 0,
    PRINCIPAL,
    TMIN,
    TMAX,
    DHCP,
    IPHOST,
    IPDEF,
    MAC,
    ESTSD,
    RESET
};

enum botones
{
    ONOFF = 0,
    ENTER,
    DER,
    IZQ, 
};

struct DATA 
{
    bool estTomas[N] = {1, 1, 1, 1, 1};

    float corriente[N] = {0, 0, 0, 0, 0};
    float tension = 220;
    float temp, hum;

    float factorZMPT = 1;
    float factorACS = 1;
    float midPointZMPT = 0, midPointACS = 0; 

    int8_t tempMax = 80, tempMin = -40;
    String usuario = "admin", clave = "12345";

    bool dhcp = 0;
    uint16_t puerto = 80;
    byte mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    IPAddress ipDef;
    /**
     * Se guarda la MAC e IP en un string ya que luego se usan en esta forma
     */
    char macString[30];  
    char ipString[16];

    void actMacString()
    {
        char buf[30];
        char hexDigit;
        for(uint8_t i=0; i<6; i++)
        {
            buf[i*5] = '0'; 
            buf[i*5+1] = 'x';
            
            hexDigit = mac[i]/16;
            buf[i*5+2] = (hexDigit < 10 ? '0' : 'A'-10) + hexDigit;
            
            hexDigit = mac[i] - 16*hexDigit;
            buf[i*5+3] = (hexDigit < 10 ? '0' : 'A'-10 ) + hexDigit;
            buf[i*5+4] = (i!=5 ? ':' : '\0' );
       }
        #ifdef DEBUGMAC
        for(char i : buf) Serial.print(i);
        Serial.println("");
        #endif
        strcpy(macString, buf);
   }
    void actIpString()
    {
        char buf[16];
        char cen = 0, dec = 0, uni = 0;
        for(int i=0; i<4; i++)
        {
            cen = ipDef[i]/100;
            buf[i*4] = '0' + cen; 

            dec = (ipDef[i] - 100*cen)/10;
            buf[i*4+1] = '0' + dec; 

            uni = ipDef[i] - 100*cen - 10*dec;
            buf[i*4+2] = '0' + uni; 
            buf[i*4+3] = (i!=3 ? '.' : '\0'); 
       }        
        #ifdef DEBUGMAC
        for(char i : buf) Serial.print(i);
        Serial.println("");
        #endif
        strcpy(ipString, buf);
   }
};

struct PINES
{
    const uint8_t tomas[N] = {28, 29, 30, 31, 32};
    const uint8_t leds[N] = {34, 35, 36, 37, 38};
    const uint8_t pulTomas[N] = {22, 23, 24, 25, 26};
    const uint8_t ACS[N] = {A8, A9, A10, A11, A12};

    const uint8_t pulMenu[4] = {40,42,41,43}; //ONOFF ENT DER IZQ
    
    const uint8_t pinZmpt = A13;
    const uint8_t pinRst = 18;
    const uint8_t pinSD = 4;
    const uint8_t pinDHT = 19;
};

#endif