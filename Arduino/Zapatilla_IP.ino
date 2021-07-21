/*
    dariotzvir@gmail.com
*/
//Librerías:

#include <SD.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>

#define N 5

#include "src/tomacorrientes/tomacorrientes.h"
#include "src/pulsadores/pulsadores.h"
#include "src/DHT22/DHT.h"
#include "src/pantallaOLED/pantallaOLED.h"
#include "src/server/server.h"
#include "src/ACS712/ACS712.h"

// Schipe: No hubiese sido más práctico crear estos como un array??
//Declaraciones de la corriente
ACS712 _ACS [N] = { ACS712 (A8, 5.0, 1023, 100), ACS712 (A9, 5.0, 1023, 100), ACS712 (A10, 5.0, 1023, 100), ACS712 (A11, 5.0, 1023, 100), ACS712 (A12, 5.0, 1023, 100) };
/*ACS712 toma0 (A0, 5.0, 1023, 100);
ACS712 toma1 (A1, 5.0, 1023, 100);
ACS712 toma2 (A2, 5.0, 1023, 100);
ACS712 toma3 (A3, 5.0, 1023, 100);
ACS712 toma4 (A4, 5.0, 1023, 100);*/

float corriente [N] = {0,0,0,0,0};

//asdwqesafsaf

// Schipe: Hacelo const si es una constante (Imagino que sí) o un #define.
//Declaraciones de la tension
float tension = 220;

//Declaraciones DHT
#define pinDHT 19
#define tipoDHT DHT22

DHT dht ( pinDHT , tipoDHT );

unsigned long timerDHT = 0;
float humAct, tempAct;

int tempMax = 125, tempMin = -40;
int tempMaxAux, tempMinAux;
bool flagDhcpAux;

//Declaraciones tomas
bool estTomas [N] = {1,1,1,1,1};
tomacorrientes objTomas ( estTomas );

//Declaraciones pulsadores
pulsadores objPul;

//Declaraciones pantalla 
pantallaOLED objPantalla ( corriente, estTomas );

int pantalla = 1;
bool selec = 0;
unsigned long timerPantalla = 0;
int periodoPantalla = (int)30000;

//Declaraciones Ethernet
IPAddress ipFija ( 192, 168, 100, 150 );
IPAddress ipDef ( 192, 168, 100, 150 );     // Schipe: Podrías poner IPAddress ipDef = ipFija; Supongo que tiene un operador asignación.
IPAddress ipStored ( 192, 168, 100, 150 ); //IP hardcodeada para cuando se resetea de fábrica

bool flagDhcp = 0;

int puerto = 80;        // Schipe: Lo mismo que antes, imagino que el puerto es constante así que declaralo const.
byte mac [] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Schipe: idem línea anterior.

server servidor ( mac, puerto, ipFija, corriente, estTomas, 
                tempAct, humAct, tension, tempMax, tempMin, flagDhcp );

//Declaraciones MEM

int pinSD = 4;                              // Schipe: const.
bool flagErrorSD = 0;
StaticJsonDocument <300> configJson;
String usuario = "admin", clave = "12345";  // Schipe: Salvo que vayas a dar la opción de cambiarse (Imagino que sí), declaralas const.

void setup() 
{   
    objPantalla.setup (); 
    objPantalla.pantallaBoot ();
    Serial.begin ( 9600 );
    //objmem.begin ();
    if ( !SD.begin ( pinSD ) ) 
    {
        Serial.println ( "SD falla" );
        flagErrorSD = 1;
    }
    else cargarSD ();

    tempMaxAux = tempMax;
    tempMinAux = tempMin;
    flagDhcpAux = flagDhcp;


    pinMode ( 13, OUTPUT );
    digitalWrite ( 13, HIGH );
    servidor.setup ();
    digitalWrite ( 13, LOW );

    objTomas.begin ();
    objPul.begin ();
    dht.begin ();
    //Calibra las mediciones tomando muestras
    /*toma0.autoMidPoint ();
    toma1.autoMidPoint ();
    toma2.autoMidPoint ();
    toma3.autoMidPoint ();
    toma4.autoMidPoint ();*/

    for ( int i = 0 ; i < 5 ; i++ ) _ACS [ i ].autoMidPoint ();

    objPantalla.pantallaPrincipal ( tempAct, humAct, tension );
}

/**
 * Schipe:
 *
 *   Ya que veo que estás manejandolo como state machine, utilizá un enum.
 *     Ej.: enum States{TMAX=1, TMin, TOMAS_CONM, SAVE, DONOTHING} mystates;  // O algo así.
 *
 *   Y lo que hago yo en estos casos es crear un array con los punteros a funciones
 * para cada estado, algo así:
 *
 * void (*stateFunc[])(void) = {statef0, statef1, statef2, ..., statefx};
 *
 *   Entonces acorde al valor actual del state (O directamente el retornado por servidor.rutina())
 * realiza la llamada a la función ubicada en dicho índice del array.
 *
 */
void loop() 
{
    funDHT ();
    funPul ();
    funACS ();
    funPantalla ();

    switch ( servidor.rutina () ) //Retorna un flag que indica si se cambió alguna variable
    {
        case 1:
            tempMaxAux = tempMax;
            guardarSD ();
        break;
        case 2:
            tempMinAux = tempMin;
            guardarSD ();
        case 3:
            for ( int i = 0 ; i < N ; i++ ) objTomas.conm ( i, estTomas [i] );
            guardarSD ();
        break;
        case 4:
            guardarSD ();
        break;
    }
}

void funDHT ()
{
  //Comprueba el tiempo entre cada muestra, las actualiza cada 2 segundos y luego comprueba si la temperatura está en los rangos de tempMax y tempMin.
  if ( millis () - timerDHT >= 2000 )
  {
    timerDHT = millis ();
    tempAct = dht.readTemperature ();
    humAct = dht.readHumidity ();
    //Si la temperatura está por arriba del límite y el toma está apagado lo prende, y viceversa.
    if ( tempAct > tempMax && !estTomas[4] ) objTomas.conm (4);
    if ( tempAct < tempMin && estTomas[4] ) objTomas.conm (4);
    
  }
}

void funPul ()
{
    for ( int i = 0 ; i < 10 ; i++ ) //Comprueba los diez pulsadores que hay, 0 al 4 son los de los tomas el resto son del meu y reset
    {
        if ( objPul.check (i) )
        {
            if ( i >= 5 ) timerPantalla = millis ();//Reinicia el timer de la pantalla así se puede ver en la pantalla lo que hacés
            switch ( i )
            {
                case 5:
                    //ON/OFF
                    if ( pantalla != 1 ) pantalla = 1; //Si está en una pantalla de menú o apagada lleva a la pantalla principal
                    else if ( pantalla == 1 ) pantalla = 0; //Si está en la pantalla principal la apaga
                    resetEstPantalla ();
                break;
                case 6:
                    //Ent
                    logicaEnter ();
                break;
                case 7:
                    //Der
                    logicaDer ();
                break;
                case 8:
                    //Izq
                    logicaIzq ();
                break;
                case 9:
                    //Reset
                    resetEstPantalla (); 
                    pantalla = 8; //Selecciona la pantalla de reset especial
                    funPantalla ();
                
                    reset ();
                break;
                default:
                    //Pulsadores tomas
                    objTomas.conm (i);
                    guardarSD ();
                break;
            }
        }
    }
}

void logicaEnter ()
{
    if ( pantalla < 2 ) pantalla = 2; //Si está en una pantalla que no sea de menú cambia a una que lo es
    else if ( pantalla == 2 || pantalla == 3 || pantalla == 4 ) //Pantallas con varaiables a modificar
    {
        if ( selec ) //Si ya se estaba modificando una variable se guarda el valor deseado desde la variable buffer a la original y se guarda en la SD
        {
            if ( pantalla == 2 ) tempMin = tempMinAux;
            if ( pantalla == 3 ) tempMax = tempMaxAux;
            if ( pantalla == 4 ) flagDhcp = flagDhcpAux;
            
            guardarSD ();
            objPul.flagTimer = 0; //Desactiva el que se pueda dejar el botón apretado para que el valor suba solo en los botones de la der e izq
        }
        else //Si no se estaba modificando una variable se reinician las variables para actualizarlas
        {
            tempMaxAux = tempMax; //Reinicia las variables de buffer
            tempMinAux = tempMin;
            flagDhcpAux = flagDhcp;
            
            objPul.flagTimer = 1; //Activa el que se pueda dejar el botón apretado para que el valor suba solo en los botones de la der e izq
        }
        selec = !selec; 
    }
}

void logicaDer ()
{
    if ( pantalla < 2 ) pantalla = 2; //Si está en una pantalla que no sea de menú cambia a una que lo es
    if ( selec ) //Si se está modificando una variable se aumenta la misma hasta su límite
    {
        if ( pantalla == 2 && tempMinAux < tempMaxAux ) tempMinAux++;
        if ( pantalla == 3 && tempMaxAux < 125 ) tempMaxAux++;
        if ( pantalla == 4 ) flagDhcpAux = !flagDhcpAux;
    }
    else //Si no se está modificando una variable se va a la siguiente o a la primera si se estaba en la última
    {
        pantalla++;
        if ( pantalla == 8 ) pantalla = 2;
    }
}

void logicaIzq ()
{
    if ( pantalla < 2 ) pantalla = 2; //Si está en una pantalla que no sea de menú cambia a una que lo es
    if ( selec ) //Si se está modificando una variable se disminuye la misma hasta su límite
    {
        if ( pantalla == 2 && tempMinAux > -40 ) tempMinAux--;
        if ( pantalla == 3 && tempMaxAux > tempMinAux ) tempMaxAux--;
        if ( pantalla == 4 ) flagDhcpAux = !flagDhcpAux;
    }
    else //Si no se está modificando una variable se va a la anterior o a la útlima si se estaba en la primera
    {
        pantalla--;
        if ( pantalla == 1 ) pantalla = 7;
    }
}

void funPantalla ()
{
    /*
        Pantallas:
        0 -- Apagada
        1 -- Principal
        2 -- Menú de temeperatura máxima
        3 -- Menú de temperatura mínima
        4 -- Menú de DHCP
        5 -- Menú de IP actual del server
        6 -- Menú de la IP éstatica que puede tener
        7 -- Menú de estado de la tarjeta SD
        8 -- Panatalla de reset
    */
    if ( millis () - timerPantalla >= periodoPantalla && selec == 0 ) pantalla = 0; //Si se supera el tiempo del timer se apaga la pantalla
    switch ( pantalla )
    {
        case 0: 
            objPantalla.pantallaApagada (); //Pantalla apagada
        break;
        case 1:
            objPantalla.pantallaPrincipal ( tempAct, humAct, tension ); //Pantalla del menu principal, los vectores se pasan con el constructor
        break;
        case 8:
            objPantalla.pantallaReset (); //Pantalla de reset, no necesita otra cosa
        break;
        default:
            objPantalla.menu ( pantalla, selec, tempMaxAux, tempMinAux, ipFija, Ethernet.localIP (), flagErrorSD, flagDhcpAux ); //Todaas las pantallas del menu en funcion de que pantalla se pasa
        break;
    }
}

void resetEstPantalla ()
{
    //Resetea variables relacionadas a la pantalla si se salió del menu sin guardar algo se usa solo para cuando apagás la pantalla
    //Mientras está en un menú
    selec = 0;
    objPul.flagTimer = 0;
    tempMaxAux = tempMax;
    tempMinAux = tempMin;
    flagDhcpAux = flagDhcp;
}

void reset ()
{
    if ( flagErrorSD == 0 ) crearSDdefecto (); //Si se levantó una SD durante el boot se crea el archivo por defecto que se hubiera creado de no tener un archivo de configuración
    
    wdt_enable( WDTO_60MS ); //Llama al watchdog
    while (1);
}

void guardarSD ()
{
    if ( flagErrorSD == 0 )
    {
        for ( int i = 0 ; i < 4 ; i++ )
        {
            configJson ["ipDef"][i] = ipDef [i];
            configJson ["ipFija"][i] = ipFija [i];
        }
        configJson ["tempMax"] = tempMax;
        configJson ["tempMin"] = tempMin;

        configJson ["dhcp"] = flagDhcp;
        configJson ["usuario"] = usuario;
        configJson ["clave"] = clave;

        for ( int i = 0 ; i < N ; i++ ) configJson ["estado"][i] = estTomas [i];

        if ( SD.exists ( "config.txt" ) ) SD.remove ( "config.txt" );
        File config = SD.open ( "config.txt", FILE_WRITE );
        serializeJsonPretty ( configJson, config );
        config.close ();
    }  
}

void cargarSD ()
{   
    if ( SD.exists ( "config.txt" ) )
    {
        Serial.println ( "Si hay archivo" );
        File config = SD.open ( "config.txt" );
        deserializeJson ( configJson, config );
        config.close ();

        byte aux [4];

        if ( configJson.containsKey ( "ipDef" ) )
        {
            for ( int i = 0 ; i < 4 ; i++ ) aux [i] = configJson ["ipDef"][i];
            ipDef = aux;
        }
        if ( configJson.containsKey ( "ipFija" ) )
        {
            for ( int i = 0 ; i < 4 ; i++ ) aux [i] = configJson ["ipFija"][i];
            ipFija = aux;
        }

        if ( configJson.containsKey ( "dhcp" ) ) flagDhcp = configJson ["dhcp"];

        if ( configJson.containsKey ( "tempMax" ) ) 
        {
            tempMax = configJson ["tempMax"];
            Serial.println ( tempMax );
            if ( tempMax > 125 || tempMax < -40 ) tempMax = 125;
        }
        if ( configJson.containsKey ( "tempMin" ) )
        {
            tempMin = configJson ["tempMin"];
            Serial.println ( tempMin );
            if ( tempMin > tempMax || tempMin < -40 ) tempMin = -40;
            //configJson ["tempMin"] <= tempMax && configJson ["tempMin"] >= -40
        }
        else tempMin = tempMax;

        if ( configJson.containsKey ( "estado" ) /*&& sizeOf ( configJson ["estado"] ) == 5*/ )
        {
            for ( int i = 0 ; i < N ; i++ ) 
            {
                estTomas [i] = configJson ["estado"][i];
                objTomas.conm ( i, estTomas [i] );
            }
        }

        if ( configJson.containsKey ( "usuario" ) ) 
        {
            String u = configJson ["usuario"];
            usuario = u;
        }
        if ( configJson.containsKey ( "clave" ) ) 
        {
            String c = configJson ["clave"];
            clave = c; 
        }
    }
    else 
    {
        Serial.println ( "No archivo SD" );
        crearSDdefecto (); //Si no se tiene un archivo se crea uno por defecto con todos los valores de la flash
    }
}

void crearSDdefecto ()
{
    for ( int i = 0 ; i < 4 ; i++ )
    {
        configJson ["ipDef"][i] = ipStored [i];
        configJson ["ipFija"][i] = ipStored [i];
    }
    configJson ["tempMax"] = 125;
    configJson ["tempMin"] = -40;
    configJson ["dhcp"] = 1;

    configJson ["usuario"] = "admin";
    configJson ["clave"] = "12345";

    for ( int i = 0 ; i < N ; i++ ) configJson ["estado"][i] = 1;

    if ( SD.exists ( "config.txt" ) ) SD.remove ( "config.txt" );
    File config = SD.open ( "config.txt", FILE_WRITE );
    serializeJsonPretty ( configJson, config );
    config.close ();
}

void funACS ()
{
    //Devuelve la corriente en milli Amper de toma
    /*corriente [0] = toma0.mA_AC ()/1000.0;
    corriente [1] = toma1.mA_AC ()/1000.0;
    corriente [2] = toma2.mA_AC ()/1000.0;
    corriente [3] = toma3.mA_AC ()/1000.0;
    corriente [4] = toma4.mA_AC ()/1000.0;*/

    for ( int i = 0 ; i < 5 ; i++ ) corriente [i] = _ACS [ i ].mA_AC ()/1000.0;
}
