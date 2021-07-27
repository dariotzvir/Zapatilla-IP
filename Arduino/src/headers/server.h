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

        void retorno ();

        String comandoServer ( int );
        String lecturaServer ( int );
        String encodeIp ( IPAddress & );
        String encodeTomas ( bool *, float * );
        String encodeMac ( byte * );

        int leerTemp ( String &, int );
        bool comprobarTempBoundaries ( String &, int, int );
};


#endif
