#ifndef __SERVER_H__
#define __SERVER_H__

//Librerías:
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

//Librerías locales:
#include "util.h"

#ifndef N
#define N 5
#endif
#define FUERARANGO "Fuera de rango"
#define ERRORPET "Peticion erronea"
#define GUARDADO "Guardado"
#define FALLOSDHCP 10
#define PERIODODHCP long(9000000)

enum RUTAS
{
    HOME,
    CMD,
    LEC,
    ERROR=-1
};
enum PET
{
    NOPET=-1,
    GET,
    POST
};

class Servidor: EthernetServer
{
    public:
        Servidor(DATA &data);

        void setup();
        void load();

        int8_t rutina();
    private:
        DATA *data;
        int8_t retornoRutina = 0;   
        
        int8_t lectura();
        int8_t parsePet();
        bool checkLogin();
        int8_t ejecutarCmd();
        void devolucion();

        void checkDHCP();

        bool cambioMac();
        bool cambioTemp(bool flag);
        bool cambioTempMax();
        bool cambioTempMin();
        bool cambioTomas();
        bool cambioIp();
        bool cambioDhcp();
        bool cambioPuerto();
        bool cambioUser();
        bool cambioClave();
        bool verificarCambio();

        String retornoLecturas();
        bool parseGET(), parsePOST(), parseStr(String str);

        bool errorParse=0, errorCmd=0, errorLogin=0, errorParam=0;
        int8_t tipoPet=-1;//Tipo de peticion HTTP 0:GET 1:POST
        int8_t ruta=0;//Rutas de las peticiones HTTP 0:/ 1:/cmd 2:/lec -1:error
        String req, message;
        String user, clave, cmd, param;

        String bufferClave, bufferUser;

        const char *str[10] = 
        { 
            "mac", "tempmax", "tempmin", 
            "tomas", "ipdef", "dhcp", 
            "puerto", "usuario", "clave",
            "verificar" 
        };
        bool (Servidor::*fun[10]) () = 
        { 
            &Servidor::cambioMac, &Servidor::cambioTempMax, &Servidor::cambioTempMin, 
            &Servidor::cambioTomas, &Servidor::cambioIp, &Servidor::cambioDhcp, 
            &Servidor::cambioPuerto, &Servidor::cambioUser, &Servidor::cambioClave, 
            &Servidor::verificarCambio 
        };
        unsigned long millisDHCP=0;
};


#endif
