#ifndef __SERVER_H__
#define __SERVER_H__


/**
 * Esto es más que nada por no incluir cosas de más, dejarlo más legible y salvar un poco de
 * memoria, pero en lugar de incluir todo Arduino.h (por lo que ví solo usas String y Serial, a lo
 * mejor se me pasó algo), podés incluir específicamente las librerías que vayas a usar, como lo
 * serían WString.h (Para los String) y USBAPI.h (Para el Serial). Cuando usás, por ejemplo, uint8_t podés
 * definirlo vos mismo escribiendo typedef unsigned char uint8_t.
 */
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "util.h"

#ifndef N
#define N 5
#endif
#define FUERARANGO "Fuera de rango"
#define ERRORPET "Peticion erronea"
#define GUARDADO "Guardado"
#define FALLOSDHCP 10

class server: EthernetServer
{
    public:
        server ( DATA & );
        void setup ();
        int rutina ();
        void load ();
    private:
        DATA *data;
        int flagGuardado = 0;                               
        String peticion;
        unsigned long millisDHCP = 0;
        long periodoDHCP = 3000000; //Si le pongo int tiraba problemas de casteo, el ciclo es 30min
        int contErrorDHCP = 0;
        String bufferClave = "", bufferUser = "";

        void retorno ();
        void checkDHCP ();
        bool checkStr ( int, const char * );
        bool checkAlfaNum ( char );
        bool checkLogin ();

        String comandoServer ( int );
        String lecturaServer ( int );
        String encodeIp ( IPAddress & );
        String encodeTomas ( bool *, float * );
        String encodeMac ( byte * );

        int leerTemp ( String &, int );
        bool comprobarTempBoundaries ( String &, int, int );
};


#endif
