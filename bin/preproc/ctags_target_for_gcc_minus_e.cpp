# 1 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino"
/*

    dariotzvir@gmail.com

*/
# 4 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino"
//Librerías:
# 6 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 7 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 8 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2



# 10 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino"
//#define DEBUGSD
//#define DEBUGMAC
//#define DEBUGDHCP
//#define DEBUGPET

# 16 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 17 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 18 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 19 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 20 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 21 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 22 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2
# 23 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 2

void guardarSD ();
void (*ptrGuardarSD) () = &guardarSD;

struct DATA data;
struct PINES pin;
DHT _dht ( pin.pinDHT , 22 /**< DHT TYPE 22 */ );
tomacorrientes _tomas ( data, pin );
pulsadores _pulsadores ( pin );
pantallaOLED _pantalla ( data );
server _server ( data, ptrGuardarSD );
RunningStatistics _zmpt;
RunningStatistics _ACS [5];
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

        Serial.println ("SD falla");

        flagErrorSD = 1;
    }
    else
    {

        Serial.println ("SD carga");

        cargarSD ();

        File log = SD.open ( "logDHCP.txt", (O_READ | O_WRITE | O_CREAT | O_APPEND) );
        log.println ( "***********BOOT***********" );
        log.close ();
    }

    _pantalla.setup ();
    _pantalla.pantallaBoot ();
    data.ipDef = ipStored;

    //pinMode ( pin.pinRst, INPUT );
    //attachInterrupt ( digitalPinToInterrupt ( pin.pinRst ), intReset, RISING );


    for ( int i : data.mac ) Serial.println ( i, 16 );


    server _aux (data, ptrGuardarSD); //Se crea un nuevo objeto que carga el puerto correctamente desde la SD, ya que al declarar globalmente
    _server = _aux; //queda inicializado con el puerto con el valor determinado (80)
    _server.setup ();
    data.actIpString ();
    data.actMacString ();

    Serial.println ( "Debug MAC:" );
    Serial.println ( data.macString );

    Serial.println ( Ethernet.localIP () );

    _tomas.begin ();
    _pulsadores.begin ();
    _dht.begin ();

    pinMode ( pin.pinZmpt, 0x0 );
    _zmpt.setWindowSecs ( 70.0/50 );

    for ( int i=0; i<5; i++ )
    {
        pinMode ( pin.ACS [i], 0x0 );
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
    for ( int i=0; i<5; i++ )
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
# 209 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino"
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
    wdt_enable( 
# 231 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino" 3
               2 
# 231 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\Zapatilla-IP.ino"
                         ); //Llama al watchdog

    while (1);
}

void guardarSD ()
{
    if ( flagErrorSD==0 )
    {

        Serial.println ( "Guardando" );

        configJson.clear ();

        for ( int i=0; i<4; i++ ) configJson ["ipDef"][i] = data.ipDef [i];
        configJson ["tempMax"] = data.tempMax;
        configJson ["tempMin"] = data.tempMin;

        configJson ["dhcp"] = data.dhcp;
        configJson ["usuario"] = data.usuario;
        configJson ["clave"] = data.clave;
        configJson ["puerto"] = data.puerto;

        for ( int i=0; i<5; i++ ) configJson ["estado"][i] = data.estTomas [i];
        for ( int i=0; i<6; i++ ) configJson ["mac"][i] = (int) data.mac [i];

        if ( SD.exists ("config.txt") ) SD.remove ("config.txt");
        File config = SD.open ( "config.txt", (O_READ | O_WRITE | O_CREAT | O_APPEND) );


        String buffer = "";
        serializeJsonPretty ( configJson, buffer );
        config.print ( buffer );
        Serial.println ( "Buffer JSON" );
        Serial.println ( buffer );




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


        Serial.println ( "Si hay archivo" );
        Serial.println ( error.c_str () );


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

            for ( int i=0; i<5; i++ )
            {
                data.estTomas [i] = configJson ["estado"][i];
                _tomas.conm ( i, data.estTomas [i] );
            }

            for ( int i=0; i<6; i++ ) data.mac [i] = (int) configJson ["mac"][i];

            String u = configJson ["usuario"];
            data.usuario = u;
            String c = configJson ["clave"];
            data.clave = c;


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
            for ( int i=0; i<5; i++ ) Serial.println ( (int) configJson ["estTomas"][i] );
            Serial.print ( "Mac: " );
            for ( int i=0; i<6; i++ ) Serial.println ( (int) configJson ["mac"][i] );
            Serial.print ( "ipDef: " );
            for ( int i=0; i<4; i++ ) Serial.println ( (int) configJson ["ipDef"][i] );

        }
        else crearSDdefecto ();
    }
    else
    {

            Serial.println ( "No archivo SD" );

        crearSDdefecto (); //Si no se tiene un archivo se crea uno por defecto con todos los valores de la flash
    }
}

void crearSDdefecto ()
{

    Serial.println ( "Creando por defecto" );

    configJson.clear ();

    for ( int i=0; i<4; i++ ) configJson ["ipDef"][i] = ipStored [i];
    configJson ["tempMax"] = 125;
    configJson ["tempMin"] = -40;
    configJson ["dhcp"] = 0;
    configJson ["puerto"] = 80;

    configJson ["usuario"] = "admin";
    configJson ["clave"] = "12345";

    for ( int i=0; i<5; i++ ) configJson ["estado"][i] = 1;
    for ( int i=0; i<6; i++ ) configJson ["mac"][i] = (int) macDef [i];

    if ( SD.exists ( "config.txt" ) ) SD.remove ( "config.txt" );
    File config = SD.open ( "config.txt", (O_READ | O_WRITE | O_CREAT | O_APPEND) );
    serializeJsonPretty ( configJson, config );
    config.close ();
}

void funAnalog ()
{
    for ( int i=0; i<5; i++ ) _ACS [i].input ( analogRead (pin.ACS [i]) );
    _zmpt.input ( analogRead ( pin.pinZmpt ) );

    if ( millis () - millisAnalog >= 1500 )
    {
        millisAnalog = millis ();
        for ( int i=0; i<5; i++ ) data.corriente [i] = data.sensACS*(_ACS [i].sigma ()-data.yCalibACS);
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
