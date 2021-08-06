/*
    dariotzvir@gmail.com
*/
//Librerías:
#include <SD.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>

#define N 5

#include "src/headers/util.h"
#include "src/headers/tomacorrientes.h"
#include "src/headers/pulsadores.h"
#include "src/headers/pantallaOLED.h"
#include "src/headers/server.h"
#include "src/ACS712/ACS712.h"
#include "src/DHT22/DHT.h"
#include "src/Filters-master/Filters.h"

struct DATA data;
struct PINES pin;
ACS712 _ACS [N] = 
{ 
    ACS712 ( A8, 5.0, 1023, 66 ), 
    ACS712 ( A9, 5.0, 1023, 66 ), 
    ACS712 ( A10, 5.0, 1023, 66 ), 
    ACS712 ( A11, 5.0, 1023, 66 ), 
    ACS712 ( A12, 5.0, 1023, 66 )
};
DHT _dht ( pin.pinDHT , DHT22 );
tomacorrientes _tomas ( data, pin );
pulsadores _pulsadores ( pin );
pantallaOLED _pantalla ( data );
server _server ( data );
RunningStatistics _zmpt;
StaticJsonDocument <300> configJson;
IPAddress ipStored ( 192, 168, 100, 150 ); //IP hardcodeada para cuando se resetea de fábrica

byte macDef [6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const int periodoDHT = 2000;
unsigned long millisDHT = 0;
unsigned long millisPan = 0;
unsigned long millisZmpt = 0;

const int periodoZmpt = 1000;
const int periodoPan = (int) 30000;             
bool flagErrorSD = 0;

void setup() 
{                   
    Serial.begin (9600);
    data.ipDef = ipStored;
    
    _zmpt.setWindowSecs ( 40.0/50 );
    SD.end ();
    delay (100);
    if ( !SD.begin (pin.pinSD) ) 
    {
        Serial.println ("SD falla");
        flagErrorSD = 1;
    }
    else cargarSD ();

    for ( int i : data.mac )Serial.println ( i, 16 );
    server _aux (data); //Se crea un nuevo objeto que carga el puerto correctamente desde la SD, ya que al declarar globalmente
    _server = _aux;       //queda inicializado con el puerto con el valor determinado (80)

    _pantalla.setup (); 
    _pantalla.pantallaBoot ();

    _server.setup (); 

    _tomas.begin ();
    _pulsadores.begin ();
    _dht.begin ();

    for ( int i=0; i<5; i++ ) _ACS [ i ].autoMidPoint ();
    _pantalla.pantallaPrincipal ();
}

void loop() 
{
    _zmpt.input ( analogRead ( A0 ) );
    funDHT ();
    funPul ();
    funACS ();
    _zmpt.input ( analogRead ( A0 ) );
    funPantalla ();

    _zmpt.input ( analogRead ( A0 ) );
    if ( millis () - millisZmpt >= 5000 )
    {
        millisZmpt = millis ();
        float aux = 0 + 0.0405*_zmpt.sigma ();
        aux = aux*40.3231;
        data.tension = aux;
    }

    int retorno = _server.rutina ();
_zmpt.input ( analogRead ( A0 ) );
    switch ( retorno )
    {
        case 1:
            _pantalla.bufferTempMax = data.tempMax;
            _pantalla.bufferTempMin = data.tempMin;
            break;
        case 2:
            for ( int i=0; i<5; i++ ) _tomas.conm ( i, data.estTomas [i] );
            break;
        case 3:
            delay (10);
            _server.load ();
            break;
        case 4:
            server _aux (data); 
            _server = _aux; 
            _server.load ();
            break;
    }

    if ( retorno ) guardarSD ();
}

void funDHT ()
{
  //Comprueba el tiempo entre cada muestra, las actualiza cada 2 segundos y luego comprueba si la temperatura está en los rangos de tempMax y tempMin.
  if ( millis ()-millisDHT >= periodoDHT )
  {
    millisDHT = millis ();
    data.temp = _dht.readTemperature ();
    data.hum = _dht.readHumidity ();
    //Si la temperatura está por arriba del límite y el toma está apagado lo prende, y viceversa.
    if ( data.temp>data.tempMax ) _tomas.conm ( 4, 1 );
    if ( data.temp<data.tempMin ) _tomas.conm ( 4, 0 );
  }
}

void funPul ()
{   
    for ( int i=0; i<N; i++ ) 
    {
        if ( _pulsadores.checkTomas (i) ) 
        {
            _tomas.invertir ( i );
            guardarSD ();
        }
    }
    for ( int i=0; i<5; i++ ) if ( _pulsadores.checkMenu (i) )
    {
        millisPan = millis ();
        switch ( i )
        {
            case 0:
                //ON/OFF
                _pantalla.logicaOnOff ();
                _pulsadores.flagTimer = 0;
                break;
            case 1:
                //Ent
                switch ( _pantalla.logicaEnter () )
                {
                    case 0:
                        if ( _pantalla.pantallaSelec == TMIN || _pantalla.pantallaSelec == TMAX ) _pulsadores.flagTimer = 1;
                        break;
                    case 3:
                        guardarSD ();
                        _pulsadores.flagTimer = 0;
                        _server.load ();
                        break;
                    default:
                        guardarSD ();
                        _pulsadores.flagTimer = 0;
                        break;
                }
                break;
            case 2:
                //Der
                _pantalla.logicaDer ();
                break;
            case 3:
                //Izq
                _pantalla.logicaIzq ();
                break;
            case 4:
                //Reset
                _pantalla.pantallaSelec = RESET; //Selecciona la pantalla de reset especial
                funPantalla ();
                reset ();
                break;
        }
    }
}

void funPantalla ()
{
    /*
        Pantallas:
        0 -- Apagada
        1 -- Principal
        2 -- Menú de temperatura mínima
        3 -- Menú de temeperatura máxima
        4 -- Menú de DHCP
        5 -- Menú de IP actual del server
        6 -- Menú de la IP éstatica que puede tener
        7 -- Menú de estado de la tarjeta SD
        8 -- Panatalla de reset
    */
    if ( millis ()-millisPan>=periodoPan && _pantalla.flagSelec == 0 ) _pantalla.pantallaSelec = 0; //Si se supera el tiempo del timer se apaga la pantalla
    switch ( _pantalla.pantallaSelec )
    {
        case APAGADA: 
            _pantalla.pantallaApagada (); //Pantalla apagada
            break;
        case PRINCIPAL:
            _pantalla.pantallaPrincipal (); //Pantalla del menu principal, los vectores se pasan con el constructor
            break;
        case RESET:
            _pantalla.pantallaReset (); //Pantalla de reset, no necesita otra cosa
            break;
        default:
            _pantalla.menu ( Ethernet.localIP (), flagErrorSD ); //Todaas las pantallas del menu en funcion de que pantalla se pasa
            break;
    }
}

void reset ()
{
    if ( flagErrorSD == 0 ) crearSDdefecto (); //Si se levantó una SD durante el boot se crea el archivo por defecto que se hubiera creado de no tener un archivo de configuración
    
    wdt_enable( WDTO_60MS ); //Llama al watchdog
    while (1);
}

void guardarSD ()
{
    if ( flagErrorSD==0 )
    {
        for ( int i=0; i<4; i++ ) configJson ["ipDef"][i] = data.ipDef [i];
        configJson ["tempMax"] = data.tempMax;
        configJson ["tempMin"] = data.tempMin;

        configJson ["dhcp"] = data.dhcp;
        configJson ["usuario"] = data.usuario;
        configJson ["clave"] = data.clave;
        configJson ["puerto"] = data.puerto;

        for ( int i=0; i<N; i++ ) configJson ["estado"][i] = data.estTomas [i];
        for ( int i=0; i<6; i++ ) configJson ["mac"][i] = (int) data.mac [i];

        if ( SD.exists ("config.txt") ) SD.remove ("config.txt");
        File config = SD.open ( "config.txt", FILE_WRITE );
        serializeJsonPretty ( configJson, config );
        config.close ();
    }  
}

void cargarSD ()
{   
    if ( SD.exists ("config.txt") )
    {
        Serial.println ( "Si hay archivo" );
        File config = SD.open ( "config.txt" );
        deserializeJson ( configJson, config );
        config.close ();

        byte aux [4];

        if ( configJson.containsKey ("ipDef") )
        {
            for ( int i=0; i<4; i++ ) aux [i] = configJson ["ipDef"][i];
            data.ipDef = aux;
        }
        if ( configJson.containsKey ("puerto") ) data.puerto = configJson ["puerto"];

        if ( configJson.containsKey ("dhcp") ) data.dhcp = configJson ["dhcp"];

        if ( configJson.containsKey ("tempMax") ) 
        {
            data.tempMax = configJson ["tempMax"];
            if ( data.tempMax > 125 || data.tempMax < -40 ) data.tempMax = 125;
        }
        if ( configJson.containsKey ("tempMin") )
        {
            data.tempMin = configJson ["tempMin"];
            if ( data.tempMin > data.tempMax || data.tempMin < -40 ) data.tempMin = -40;
        }
        else data.tempMin = data.tempMax;

        if ( configJson.containsKey ("estado")  )
        {
            for ( int i=0; i<N; i++ ) 
            {
                data.estTomas [i] = configJson ["estado"][i];
                _tomas.conm ( i, data.estTomas [i] );
            }
        }

        if ( configJson.containsKey ( "mac" ) ) for ( int i=0; i<6; i++ ) data.mac [i] = (int) configJson ["mac"][i];


        if ( configJson.containsKey ("usuario") ) 
        {
            String u = configJson ["usuario"];
            data.usuario = u;
        }
        if ( configJson.containsKey ("clave") ) 
        {
            String c = configJson ["clave"];
            data.clave = c; 
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
    for ( int i=0; i<4; i++ ) configJson ["ipDef"][i] = ipStored [i];
    configJson ["tempMax"] = 125;
    configJson ["tempMin"] = -40;
    configJson ["dhcp"] = 1;
    configJson ["puerto"] = 80;

    configJson ["usuario"] = "admin";
    configJson ["clave"] = "12345";

    for ( int i=0; i<N; i++ ) configJson ["estado"][i] = 1;
    for ( int i=0; i<6; i++ ) configJson ["mac"][i] = (int) macDef [i];

    if ( SD.exists ( "config.txt" ) ) SD.remove ( "config.txt" );
    File config = SD.open ( "config.txt", FILE_WRITE );
    serializeJsonPretty ( configJson, config );
    config.close ();
}

void funACS ()
{
    for ( int i=0; i<5; i++ ) if ( data.estTomas [i] == 1 ) data.corriente [i] = _ACS [i].mA_AC ()/1000.0;
}
