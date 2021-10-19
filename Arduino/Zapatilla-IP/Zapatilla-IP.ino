/*
    dariotzvir@gmail.com
*/

//Constantes
#define N 5
#define PERIODODHT 2000 //Tiempo entre muestras sensor de temperatura.
#define PERIODOPAN 30000 //Tiempo para que la pantalla quede encendida.
#define NMUESTRAS 256
#define MIDPOINTZMPT 25
#define MIDPOINTACS 25
#define FACTORZMPT 2.87

//Librerías:
#include <SD.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>

//Librerías con ruta relativa:
#include "src/headers/util.h"
#include "src/headers/tomacorrientes.h"
#include "src/headers/pulsadores.h"
#include "src/headers/pantallaOLED.h"
#include "src/headers/servidor.h"
#include "src/ACS712/ACS712.h"
#include "src/DHT22/DHT.h"
#include "src/Filters-master/Filters.h"

//Estructuras:
struct DATA data;//Se pasa este struct a todos los objetos que necesiten guardar data
struct PINES pin;//Data de todo el IO

//Objetos:
DHT _dht(pin.pinDHT , DHT22);
Tomacorrientes _tomas(data, pin);
Pulsadores _pulsadores(pin);
PantallaOLED _pantalla(data);
Servidor _server(data);
IPAddress ipStored(192, 168, 254, 154); //IP hardcodeada para cuando se resetea de fábrica
StaticJsonDocument <300> configJson;

byte macDef[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //Dirección MAC hardcodeada

//Contadores de tiempo miscelanes:
unsigned long millisDHT = 0;
unsigned long millisPan = 0;
unsigned long millisAnalog = 0;

//Flags:
bool flagErrorSD = 0;
bool flagReset = 0; //El reset tiene un flag que se ejecuta en el loop ya que no se puede llamar al WDT durante una ISR

void setup() 
{         
    analogReference(EXTERNAL);

    Serial.begin(BAUD2);
    data.ipDef = ipStored; //Cargo la ip fija ya que no se puede al crear al prototipo de la estructura
    
    /**
     * Se inicia dos veces la instancia de la tarjeta SD ya que usalmente falla al hacerlo solo una vez 
     * falla.
     * Si aún así falla se corre con las configuraciones de fábrica.
     * Si inicializa bien la tarjeta se cargan los datos.
     */
    SD.begin(pin.pinSD);
    SD.end();
    if(!SD.begin(pin.pinSD)) 
    {
        #ifdef DEBUGSD
        Serial.println("SD falla");
        #endif
        flagErrorSD = 1;
    }
    else 
    {
        #ifdef DEBUGSD
        Serial.println("SD carga");
        #endif
        cargarSD();
    }
    
    _pantalla.setup(); 
    _pantalla.pantallaBoot();

    /**
     * Setteo del reset y la interrupción de hardware
     */ 
    pinMode(pin.pinRst, INPUT);
    attachInterrupt(digitalPinToInterrupt(pin.pinRst), intReset, RISING);

    #ifdef DEBUGMAC
    for(int i : data.mac) Serial.println(i, 16);
    #endif

    /**
     * Se actualiza la data con lo cargado desde la SD, creo un nuevo objeto y lo copio con la data desada.
     * Actualizo también la MAC e IP en forma de string que después se usa.
     */

    Servidor _aux(data);
    _server = _aux;
    _server.setup(); 
    data.actIpString();
    data.actMacString();

    #ifdef DEBUGMAC
    Serial.println("Debug MAC:");
    Serial.println(data.macString);
    #endif

    _tomas.begin();
    _pulsadores.begin();
    _dht.begin();

    pinMode(pin.pinZmpt, INPUT);
    for(uint8_t i=0; i<N; i++)
        pinMode(pin.ACS[i], INPUT);

    _pantalla.pantallaPrincipal();
}
void loop() 
{
    if(flagReset) reset();
    funDHT();
    funPul();
    funAnalog();
    funPantalla();
    funserver();
}
void funDHT()
{
  //Comprueba el tiempo entre cada muestra, las actualiza cada 2 segundos y luego comprueba si la temperatura está en los rangos de tempMax y tempMin.
  if(millis()-millisDHT >= PERIODODHT)
  {
    millisDHT = millis();
    data.temp = _dht.readTemperature();
    data.hum = _dht.readHumidity();

    //Si la temperatura está por arriba del límite y el toma está apagado lo prende, y viceversa.
    if(data.temp>data.tempMax && data.estTomas[4] == 0) _tomas.invertir(4);
    if(data.temp<data.tempMin && data.estTomas[4] == 1) _tomas.invertir(4);
  }
}
void funPul()
{   
    for(uint8_t i=0; i<N; i++) 
        if(_pulsadores.checkTomas(i)) 
        {
            _tomas.invertir(i);
            guardarSD();
        }
    for(uint8_t i=0; i<3; i++) 
        if(_pulsadores.checkMenu(i))
        {
            millisPan = millis();
            switch(i)
            {
                case ONOFF:
                    _pantalla.logicaOnOff();
                    _pulsadores.flagTimer = 0;
                    break;
                case ENTER:
                    switch(_pantalla.logicaEnter())
                    {
                        case 0:
                            if(_pantalla.pantallaSelec == TMIN || _pantalla.pantallaSelec == TMAX) _pulsadores.flagTimer = 1;
                            break;
                        case 3:
                            guardarSD();
                            _pulsadores.flagTimer = 0;
                            _server.load();
                            _pantalla.resetBuf();
                            break;
                        default:
                            guardarSD();
                            _pulsadores.flagTimer = 0;
                            break;
                    }
                    break;
                case DER:
                    _pantalla.logicaDer();
                    break;
                case IZQ:
                    _pantalla.logicaIzq();
                    break;
            }
        }
}
void funPantalla()
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
    if(millis()-millisPan>=PERIODOPAN && _pantalla.flagSelec == 0) _pantalla.pantallaSelec = 0; //Si se supera el tiempo del timer se apaga la pantalla
    switch(_pantalla.pantallaSelec)
    {
        case APAGADA: 
            _pantalla.pantallaApagada(); //Pantalla apagada
            break;
        case PRINCIPAL:
            _pantalla.pantallaPrincipal(); //Pantalla del menu principal, los vectores se pasan con el constructor
            break;
        case RESET:
            _pantalla.pantallaReset(); //Pantalla de reset, no necesita otra cosa
            break;
        default:
            _pantalla.menu(Ethernet.localIP(), flagErrorSD); //Todas las pantallas del menu en funcion de que pantalla se pasa
            break;
    }
}
void reset()
{
    _pantalla.pantallaReset();
    if(!flagErrorSD) crearSDdefecto(); //Si se levantó una SD durante el boot se crea el archivo por defecto que se hubiera creado de no tener un archivo de configuración
    wdt_enable(WDTO_60MS); //Llama al watchdog

    while(1);
}
void guardarSD()
{
    if(!flagErrorSD)
    {
        #ifdef DEBUGSD
        Serial.println("Guardando");
        #endif
        configJson.clear();

        for(int i=0; i<4; i++) configJson["ipDef"][i] = data.ipDef[i];
        configJson["tempMax"] = data.tempMax;
        configJson["tempMin"] = data.tempMin;

        configJson["dhcp"] = data.dhcp;
        configJson["usuario"] = data.usuario;
        configJson["clave"] = data.clave;
        configJson["puerto"] = data.puerto;

        for(int i=0; i<N; i++) configJson["estado"][i] = data.estTomas[i];
        for(int i=0; i<6; i++) configJson["mac"][i] = (int)data.mac[i];

        if(SD.exists("config.txt")) SD.remove("config.txt"); //Borra el archivo de configuración.
        File config = SD.open("config.txt", FILE_WRITE); //Crea un nuevo archivo de configuración.
        serializeJsonPretty(configJson, config); //Escribe el archivo.
        config.close(); //Cierra el archivo.

        #ifdef DEBUGSD
        String buffer = "";
        serializeJsonPretty(configJson, buffer);
        config.print(buffer);
        Serial.println("Buffer JSON");
        Serial.println(buffer);
        #endif
    }  
}
/**
 * @brief TODO verificar que los datos existan antes de leerlos.
 */
void cargarSD()
{   
    if(SD.exists("config.txt"))
    {
        configJson.clear(); //Limpia el JSON por las dudas.
        File config = SD.open("config.txt"); //Abre el archivo de configuración.

        DeserializationError error = deserializeJson(configJson, config);
        config.close(); //Cierra el archivo.
        
        #ifdef DEBUGSD
        Serial.println("Si hay archivo");
        Serial.println(error.c_str());
        #endif

        if(!error)
        {
            byte aux[4];

            for(uint8_t i=0; i<4; i++) aux[i] = configJson["ipDef"][i];
            data.ipDef = aux;
            
            data.puerto = configJson["puerto"];

            data.dhcp = configJson["dhcp"];

            data.tempMax = configJson["tempMax"];
            if(data.tempMax > 125 || data.tempMax < -40) data.tempMax = 125;
            
            data.tempMin = configJson["tempMin"];
            if(data.tempMin > data.tempMax || data.tempMin < -40) data.tempMin = -40;
            //else data.tempMin = data.tempMax;

            for(uint8_t i=0; i<N; i++) 
            {
                data.estTomas[i] = configJson["estado"][i];
                _tomas.conm(i, data.estTomas[i]);
            }

            for(uint8_t i=0; i<6; i++) data.mac[i] = (int)configJson["mac"][i]; //Se castea porque sinó da problemas

            String u = configJson["usuario"]; //Escribo a un objeto de String porque el casteo no funciona
            data.usuario = u;
            String c = configJson["clave"];
            data.clave = c; 

            #ifdef DEBUGSD
            Serial.print("Tempmax: ");
            Serial.println((int) configJson["tempMax"]);
            Serial.print("Tempmin: ");
            Serial.println((int) configJson["tempMin"]);
            Serial.print("DHCP: ");
            Serial.println((int) configJson["dhcp"]);
            Serial.print("Puerto: ");
            Serial.println((int) configJson["puerto"]);

            Serial.print("Usuario: ");
            String uu = configJson["usuario"];
            Serial.println(uu);

            Serial.print("Clave: ");
            String cc = configJson["clave"];
            Serial.println(cc);

            Serial.print("Estado: ");
            for(int i=0; i<N; i++) Serial.println((int) configJson["estTomas"][i]);
            Serial.print("Mac: ");
            for(int i=0; i<6; i++) Serial.println((int) configJson["mac"][i]);
            Serial.print("ipDef: ");
            for(int i=0; i<4; i++) Serial.println((int) configJson["ipDef"][i]);
            #endif
        }
        else crearSDdefecto(); //Si el archivo por alguna razón tiene un problema crea el archivo default.
    }
    else 
    {
        #ifdef DEBUGSD
            Serial.println("No archivo SD");
        #endif
        crearSDdefecto(); //Si no se tiene un archivo se crea uno por defecto con todos los valores de la flash
    }
}
/**
 * @brief Escribe el archivo default de configuración para la tarjeta SD.
 */
void crearSDdefecto()
{
    #ifdef DEBUGSD
    Serial.println("Creando por defecto");
    #endif
    configJson.clear(); //Limpia el JSON por las dudas.

    for(int i=0; i<4; i++) configJson["ipDef"][i] = ipStored[i];
    /**
     * Los valores default de temperatura son según las especificaciones del sensor:
     * 125°C tempMax
     * -40°C tempMin
     */
    configJson["tempMax"] = 125; 
    configJson["tempMin"] = -40;
    configJson["dhcp"] = 0;
    configJson["puerto"] = 80;

    configJson["usuario"] = "admin";
    configJson["clave"] = "12345";

    for(int i=0; i<N; i++) configJson["estado"][i] = 1;
    for(int i=0; i<6; i++) configJson["mac"][i] =(int) macDef[i];

    if(SD.exists("config.txt")) SD.remove("config.txt"); //Borra el archivo de configuración.
    File config = SD.open("config.txt", FILE_WRITE); //Crea uno nuevo en blanco.
    serializeJsonPretty(configJson, config); //Lo escribe en formatop JSON.
    config.close(); //Cierra el archivo.
}
/**
 * 
 * RMS = sqrt( Sumatoria(muestras^2)/NUM_muestras )
 * Con el valor RMS luego se lo escala por un factor tomado prácticamente, ya que al dar los valores del ADC no se
 * obtiene el dato escalado a la magnitud deseada.
 * 
 * @brief La función calcula el RMS de los sensores de tensión y corriente.
 * 
 */
void funAnalog()
{
    static int c=0;
    static unsigned long sumZMPTSQ=0, sumACSSQ[N]={0};
    int8_t muestra;
    for(uint8_t i=0; i<N; i++) 
    {
        muestra = analogRead(pin.ACS[i]) - 512; //Asume que 512 es el 0 de la señal (tendría que corregirse el sensor).
        sumACSSQ[i] += (muestra*muestra); //Sumatoria
    }
    muestra = analogRead(pin.pinZmpt) - 512;
    sumZMPTSQ += (muestra*muestra);

    #ifdef DEBUGANALOG
    Serial.print("\n");
    Serial.print("ADC:");
    Serial.print((10000+muestra));
    Serial.print(",");
    Serial.print("SumSQ:");
    Serial.print((0.001*sumZMPTSQ));
    Serial.print(",");
    Serial.print("SQINST:");
    Serial.print((muestra*muestra));
    Serial.print(",");
    Serial.print("Tension:");
    Serial.println((100.0*data.tension));
    #endif

    if(++c==NMUESTRAS)
    {
        static unsigned long a=0;

        for(uint8_t i=0; i<5; i++) 
            data.corriente[i] = sqrt((double)sumACSSQ[i] / NMUESTRAS);

        data.tension = FACTORZMPT * sqrt((double)sumZMPTSQ / NMUESTRAS); 
    
        //Reseto de las variables 
        for(uint8_t i=0; i<5; i++) sumACSSQ[i]=0; //TODO borrar contenido con PROGMEM
        sumZMPTSQ=0;
        c=0;

        #ifdef DEBUGANALOG
        Serial.print(" Tension:");
        Serial.print(data.tension);
        Serial.print(" Sigma: ");
        Serial.println(data.tension/FACTORZMPT);
        #endif
    }
}
/**
 * @brief Llama al método del objeto de Server y maneja los retornos con cambios externos a la clase
 */
void funserver()
{
    int retorno = _server.rutina();
    if(retorno >= 0)
    {
        #ifdef DEBUGPET
        Serial.print("Retorno rutina server: ");
        Serial.println(retorno);
        #endif
        switch(retorno)
        {
            case 3:
                for(uint8_t i=0; i<N; i++) _tomas.conm(i, data.estTomas[i]);
                break;
            case 4:
                _pantalla.pantallaBoot();
                if(!data.dhcp) _server.load();
                break;
            case 5:
                _pantalla.pantallaBoot();
                _server.load();
                break;
            case 6:
                _pantalla.pantallaBoot();
                Servidor _aux(data); 
                _server = _aux; 
                _server.load();
                break;
        }
        if(retorno != 7 && retorno != 8) guardarSD();
    }
}
/**
 * @brief ISR, cambia el flag de reset.
 */
void intReset(){flagReset = 1;}