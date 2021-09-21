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
#define PERIODODHCP long(9000000)
#define GET 0
#define POST 1

enum rutas
{
    HOME,
    CMD,
    LEC,
    ERROR=-1
};

class server: EthernetServer
{
    public:
        server(DATA &data, void (*guardarSD)());
        void setup();
        int rutina();
        void load();
    private:

        String req, message;
        void parseGET (), parsePOST ();
        void lectura();
        void devolucion();
        bool tipo=0;
        int ruta=0;
        String cmd="";
        String param="";
        String clave="";
        String user="";

        DATA *data;
        void (*guardarSD)();
        int retornoRutina = 0;       
        unsigned long millisDHCP = 0;
        int contErrorDHCP = 0;
        String bufferClave = "", bufferUser = "";

        void retorno(bool);
        void conversion();
        void checkDHCP();
        bool checkStr(int, const char *);
        bool checkAlfaNum(char);
        bool checkLogin();

        String comandoServerGET(int);
        String lecturaServer(int);
        String encodeIp(IPAddress &);
        String encodeTomas(bool *, float *);

        int leerTemp(String &, int);
        bool comprobarTempBoundaries(String &, int, int);

        

        
};


#endif
