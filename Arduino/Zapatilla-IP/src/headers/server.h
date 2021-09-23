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

enum RUTAS
{
    HOME,
    CMD,
    LEC,
    ERROR=-1
};
enum PET
{
    GET,
    POST
};

class server: EthernetServer
{
    public:
        server(DATA &data, void (*guardarSD)());

        void setup();
        void load();

        int8_t rutina();
    private:
        DATA *data;
        void (*guardarSD)();    
        int8_t retornoRutina = 0;   
        
        void lectura();
        void conversion();
        void checkLogin();
        void ejecutarCmd();
        void devolucion();

        String retornoLecturas();
        bool parseGET(), parsePOST(), parseStr(String str);

        bool errorParse=0, errorCmd=0, errorLogin=0;
        uint8_t tipo=0;//Tipo de peticion HTTP 0:GET 1:POST
        uint8_t ruta=0;//Rutas de las peticiones HTTP 0:/ 1:/cmd 2:/lec -1:error
        String req, message;
        String user, clave, cmd, param;
};


#endif
