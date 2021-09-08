/*
    dariotzvir@gmail.com
*/
//Librerías:
#include <SD.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>

#define N 5
//#define DEBUGSD
//#define DEBUGMAC
//#define DEBUGDHCP
//#define DEBUGPET

#include "src/headers/util.h"
#include "src/headers/tomacorrientes.h"
#include "src/headers/pulsadores.h"
#include "src/headers/pantallaOLED.h"
#include "src/headers/server.h"
#include "src/ACS712/ACS712.h"
#include "src/DHT22/DHT.h"
#include "src/Filters-master/Filters.h"

void guardarSD ();
void (*ptrGuardarSD) () = &guardarSD;

struct DATA data;
struct PINES pin;
DHT _dht ( pin.pinDHT , DHT22 );
tomacorrientes _tomas ( data, pin );
pulsadores _pulsadores ( pin );
pantallaOLED _pantalla ( data );
server _server ( data, ptrGuardarSD );
RunningStatistics _zmpt;
RunningStatistics _ACS [N];
IPAddress ipStored ( 192, 168, 254, 154 ); //IP hardcodeada para cuando se resetea de fábrica
StaticJsonDocument <300> configJson;
byte macDef [6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const int periodoDHT = 2000;
unsigned long millisDHT = 0;
unsigned long millisPan = 0;
unsigned long millisAnalog = 0;

const int periodoAnalog = 1000;
const int periodoPan = (int) 30000;             
bool flagErrorSD = 0;
bool flagReset = 0;

unsigned long a = 0;

void setup() 
{         
    Serial.begin (9600);

    SD.begin (4);
    SD.end ();
    if ( !SD.begin (4) ) 
    {
        #ifdef DEBUGSD
        Serial.println ("SD falla");
        #endif
        flagErrorSD = 1;
    }
    else 
    {
        #ifdef DEBUGSD
        Serial.println ("SD carga");
        #endif
        cargarSD ();

        File log = SD.open ( "logDHCP.txt", FILE_WRITE );
        log.println ( "***********BOOT***********" );
        log.close ();
    }
    
    _pantalla.setup (); 
    _pantalla.pantallaBoot ();
    data.ipDef = ipStored;

    //pinMode ( pin.pinRst, INPUT );
    //attachInterrupt ( digitalPinToInterrupt ( pin.pinRst ), intReset, RISING );

    #ifdef DEBUGMAC
    for ( int i : data.mac ) Serial.println ( i, 16 );
    #endif
    
    server _aux (data, ptrGuardarSD);   //Se crea un nuevo objeto que carga el puerto correctamente desde la SD, ya que al declarar globalmente
    _server = _aux;       //queda inicializado con el puerto con el valor determinado (80)
    _server.setup (); 
    data.actIpString ();
    data.actMacString ();
    #ifdef DEBUGMAC
    Serial.println ( "Debug MAC:" );
    Serial.println ( data.macString );
    #endif
    Serial.println ( Ethernet.localIP () );
    _tomas.begin ();
    _pulsadores.begin ();
    _dht.begin ();

    pinMode ( pin.pinZmpt, INPUT );
    _zmpt.setWindowSecs ( 70.0/50 );

    for ( int i=0; i<N; i++ )
    {
        pinMode ( pin.ACS [i], INPUT );
        _ACS [i].setWindowSecs ( 70.0/50 );
    }

    _pantalla.pantallaPrincipal ();
}

void loop() 
{
    if ( flagReset ) reset ();
    funDHT ();
    funPul ();
    funAnalog ();
    funPantalla ();
    funServer ();
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
    if ( data.temp>data.tempMax && data.estTomas [4] == 0 ) _tomas.invertir ( 4 );
    if ( data.temp<data.tempMin && data.estTomas [4] == 1 ) _tomas.invertir ( 4 );
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
    for ( int i=0; i<4; i++ ) if ( _pulsadores.checkMenu (i) )
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
                        _pantalla.resetBuf ();
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
            _pantalla.menu ( Ethernet.localIP (), flagErrorSD ); //Todas las pantallas del menu en funcion de que pantalla se pasa
            break;
    }
}

void reset ()
{
    if ( flagErrorSD == 0 ) crearSDdefecto (); //Si se levantó una SD durante el boot se crea el archivo por defecto que se hubiera creado de no tener un archivo de configuración
    _pantalla.pantallaReset ();
    wdt_enable( WDTO_60MS ); //Llama al watchdog

    while (1);
}

void guardarSD ()
{
    if ( flagErrorSD==0 )
    {
        #ifdef DEBUGSD
        Serial.println ( "Guardando" );
        #endif
        configJson.clear ();

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
        
        #ifdef DEBUGSD
        String buffer = "";
        serializeJsonPretty ( configJson, buffer );
        config.print ( buffer );
        Serial.println ( "Buffer JSON" );
        Serial.println ( buffer );
        #else 
        serializeJsonPretty ( configJson, config );
        #endif

        config.close ();
    }  
}

void cargarSD ()
{   
    if ( SD.exists ("config.txt") )
    {
        configJson.clear ();
        File config = SD.open ( "config.txt" );

        DeserializationError error = deserializeJson ( configJson, config );
        config.close ();
        
        #ifdef DEBUGSD
        Serial.println ( "Si hay archivo" );
        Serial.println ( error.c_str () );
        #endif

        if ( !error )
        {
            byte aux [4];

            for ( int i=0; i<4; i++ ) aux [i] = configJson ["ipDef"][i];
            data.ipDef = aux;
            
            data.puerto = configJson ["puerto"];

            data.dhcp = configJson ["dhcp"];

            data.tempMax = configJson ["tempMax"];
            if ( data.tempMax > 125 || data.tempMax < -40 ) data.tempMax = 125;
            
            data.tempMin = configJson ["tempMin"];
            if ( data.tempMin > data.tempMax || data.tempMin < -40 ) data.tempMin = -40;
            //else data.tempMin = data.tempMax;

            for ( int i=0; i<N; i++ ) 
            {
                data.estTomas [i] = configJson ["estado"][i];
                _tomas.conm ( i, data.estTomas [i] );
            }

            for ( int i=0; i<6; i++ ) data.mac [i] = (int) configJson ["mac"][i];

            String u = configJson ["usuario"];
            data.usuario = u;
            String c = configJson ["clave"];
            data.clave = c; 

            #ifdef DEBUGSD
            Serial.print ( "Tempmax: " );
            Serial.println ( (int) configJson ["tempMax"] );
            Serial.print ( "Tempmin: " );
            Serial.println ( (int) configJson ["tempMin"]);
            Serial.print ( "DHCP: " );
            Serial.println ( (int) configJson ["dhcp"] );
            Serial.print ( "Puerto: " );
            Serial.println ( (int) configJson ["puerto"] );

            Serial.print ( "Usuario: " );
            String uu = configJson ["usuario"];
            Serial.println ( uu );

            Serial.print ( "Clave: " );
            String cc = configJson ["clave"];
            Serial.println ( cc );

            Serial.print ( "Estado: " );
            for ( int i=0; i<N; i++ ) Serial.println ( (int) configJson ["estTomas"][i] );
            Serial.print ( "Mac: " );
            for ( int i=0; i<6; i++ ) Serial.println ( (int) configJson ["mac"][i] );
            Serial.print ( "ipDef: " );
            for ( int i=0; i<4; i++ ) Serial.println ( (int) configJson ["ipDef"][i] );
            #endif
        }
        else crearSDdefecto ();
    }
    else 
    {
        #ifdef DEBUGSD
            Serial.println ( "No archivo SD" );
        #endif
        crearSDdefecto (); //Si no se tiene un archivo se crea uno por defecto con todos los valores de la flash
    }
}

void crearSDdefecto ()
{
    #ifdef DEBUGSD
    Serial.println ( "Creando por defecto" );
    #endif
    configJson.clear ();

    for ( int i=0; i<4; i++ ) configJson ["ipDef"][i] = ipStored [i];
    configJson ["tempMax"] = 125;
    configJson ["tempMin"] = -40;
    configJson ["dhcp"] = 0;
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

void funAnalog ()
{
    for ( int i=0; i<N; i++ ) _ACS [i].input ( analogRead (pin.ACS [i]) ); 
    _zmpt.input ( analogRead ( pin.pinZmpt ) );
    
    if ( millis () - millisAnalog >= 1500 )
    {
        millisAnalog = millis ();
        for ( int i=0; i<N; i++ ) data.corriente [i] = data.sensACS*(_ACS [i].sigma ()-data.yCalibACS);
        data.tension = data.sensZMPT*(_zmpt.sigma ()-data.yCalibZMPT);
    }
}

void funServer ()
{
    int retorno = _server.rutina ();
    switch ( retorno )
    {
        case 1:
            _pantalla.resetBuf ();
            break;
        case 2:
            for ( int i=0; i<5; i++ ) _tomas.conm ( i, data.estTomas [i]);
            break;
        case 3:
            delay (10);
            _server.load ();
            break;
        case 4:
            server _aux (data, ptrGuardarSD); 
            _server = _aux; 
            _server.load ();
            break;
    }
    //if ( retorno > 0 ) guardarSD ();
}

void intReset (){flagReset = 1;}