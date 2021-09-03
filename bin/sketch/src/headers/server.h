#line 1 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\src\\headers\\server.h"
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
#define PERIODODHCP long(3600000)
#define GET 0
#define POST 1

class server: EthernetServer
{
    public:
        server ( DATA &data, void (*guardarSD) () );
        void setup ();
        int rutina ();
        void load ();
    private:
        DATA *data;
        void (*guardarSD) ();
        int retornoRutina = 0;                               
        String peticion, header;
        unsigned long millisDHCP = 0;
        int contErrorDHCP = 0;
        String bufferClave = "", bufferUser = "";

        void retorno ( bool );
        void checkDHCP ();
        bool checkStr ( int, const char * );
        bool checkAlfaNum ( char );
        bool checkLogin ( int );

        String comandoServerGET ( int );
        String lecturaServer ( int );
        String encodeIp ( IPAddress & );
        String encodeTomas ( bool *, float * );

        int leerTemp ( String &, int );
        bool comprobarTempBoundaries ( String &, int, int );

        String debugLog ();
};


#endif
