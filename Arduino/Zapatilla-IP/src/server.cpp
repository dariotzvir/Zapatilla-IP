#include "headers/server.h"

#include <SD.h>
#include <utility/w5100.h>

EthernetClient cmdCliente;

server::server ( DATA &data ): EthernetServer ( data.puerto )
{
    this->data = &data;
}

void server::setup ()
{
    Ethernet.begin ( data->mac, data->ipDef );
    
    bufferClave = data->clave;
    bufferUser = data->usuario;
    //if ( Ethernet.hardwareStatus() == EthernetNoHardware || Ethernet.linkStatus() != LinkON ) errorEthernet = 1;

    load ();

    begin ();
}

void server::load ()
{
    Ethernet.begin ( data->mac, data->ipDef );
    
    if ( data->dhcp == 1 ) 
    {
        #ifdef DEBUGDHCP
        Serial.println ( "DHCP" );
        #endif
        if ( Ethernet.begin ( data->mac ) == 0 ) 
        {
            #ifdef DEBUGDHCP
            Serial.println ( "Falla DHCP" );
            #endif
            Ethernet.begin ( data->mac, data->ipDef );
            data->dhcp = 0;
        }
    }

    contErrorDHCP = 0;
    millisDHCP = millis ();
}

int server::rutina ()
{
    #ifdef DEBUGANALOG
    unsigned long d = millis ();
    #endif
    retornoRutina = -1;
    checkDHCP ();
    cmdCliente.flush ();
    cmdCliente = available ();
    bool lineaEnBlanco = 1;
    char tipoPeticion = 0;

    if ( cmdCliente )
    {
        retornoRutina = 0;
        while ( cmdCliente.connected () )
        {
            char c;
            if ( cmdCliente.available () )
            {
                c = cmdCliente.read ();
                if ( tipoPeticion == 0 ) tipoPeticion = c;
                #ifdef DEBUGPET
                Serial.print ( c );
                #endif
                if ( header.length () < 100 ) header += c;
            }

            if ( tipoPeticion == 'G' && c == '\n' )
            {
                //Serial.println ( "\nRetorno GET" );
                #ifdef DEBUGANALOG
                Serial.print ( "Lectura buffer: " );
                Serial.println ( millis ()-d );
                #endif
                retorno ( GET );
                delay (1);
                cmdCliente.stop ();
                break;
            }
            //Leer POST
            /*if ( lineaEnBlanco && c == '\n' )
            {
                while ( cmdCliente.available () ) 
                {
                    c = cmdCliente.read ();
                    peticion += c;
                }
                cmdCliente.println ( "HTTP/1.1 200 OK" );
                cmdCliente.println ( "Content-Type: text/plain" );
                cmdCliente.println ( "Connection: close" );
                cmdCliente.println (); 
                cmdCliente.println ( peticion );
                Serial.println ( peticion );

                peticion = "";

                delay (1);
                cmdCliente.stop ();
                cmdCliente.clo
                break;
            }

            if ( c == '\n' ) lineaEnBlanco = 1;
            else if ( c != '\r' ) lineaEnBlanco = 0;*/
        }
    }
    return retornoRutina;   
}

void server::retorno ( bool tipo )
{
    
    #ifdef DEBUGANALOG
    unsigned long e = millis ();
    #endif
    String devolucion = "Peticion erronea";

    String cmd = header.substring (4, 8);

    if ( !cmd.compareTo ( "/ HT" ) > 0 ) devolucion = "Zapatilla IP OK";
    else if ( checkLogin ( 8 ) )
    {
        if ( !cmd.compareTo ( "/lec" ) ) devolucion = lecturaServer ( 8 );
        else if ( tipo == GET && !cmd.compareTo ( "/cmd" ) > 0 ) devolucion = comandoServerGET ( 8 );
        else if ( tipo == POST && !cmd.compareTo ( "/cmd" ) > 0 ) devolucion = comandoServerGET ( 8 );
    }

    cmdCliente.println ( "HTTP/1.1 200 OK" );
    cmdCliente.println ( "Content-Type: text/plain" );
    cmdCliente.println ( "Connection: close" );
    cmdCliente.println (); 
    cmdCliente.println ( devolucion );

    header = "";
    
    #ifdef DEBUGANALOG
    Serial.print ( "Tiempo retorno: " );
    Serial.println ( millis ()-e );
    #endif
}

bool server::checkLogin ( int index )
{
    /*
    *   GET /cmd?admin+12345+tempmax=123 HTTP/1.1
    *           ?admin+12345
    *   GET /cmd?tempmax=123 HTTP/1.1
    */
    
    String aux = '?' + data->usuario + '+' + data->clave; //Formatea a como deberían estar el usuario y contraseña en la peticion HTTP
    String cmd = header.substring ( index, index+aux.length () ); //Selecciona el segmento de string donde deberia estar el usuario y contraseña
    if ( cmd.compareTo ( aux ) == 0 ) //Chequea si son iguales
    {
        header.remove ( index+1, aux.length () ); //Remueve el sector de contraseña y usuario así se pueden utiliza los metodos que ya tenía escritos
        #ifdef DEBUGPET
        Serial.println ( header );
        #endif
        return 1;
    }
    return 0;
}

String server::lecturaServer ( int index )
{
    String retorno = ERRORPET;

    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( checkStr ( index, "?todo" ) ) 
    {
        StaticJsonDocument <300> jsonRequest;
        String aux = "";

        jsonRequest ["tempMax"] = data->tempMax;
        jsonRequest ["tempmin"] = data->tempMin;
        jsonRequest ["tempAct"] = data->temp;
        jsonRequest ["humAct"] = data->hum;
        jsonRequest ["tension"] = data->tension;
        jsonRequest ["dhcp"] = data->dhcp;
        jsonRequest ["ipDef"] = data->puerto;

        jsonRequest ["mac"] = data->macString;
        jsonRequest ["ipDef"] = data->ipString;
        for ( int i=0; i<N; i++ )jsonRequest ["estTomas"][i] = data->estTomas [i];
        for ( int i=0; i<N; i++ )jsonRequest ["corriente"][i] = data->corriente [i];

        serializeJsonPretty ( jsonRequest, aux );
        retorno = aux;
    }
    if ( checkStr ( index, "?tempmax" ) ) retorno =  data->tempMax + 'C';
    if ( checkStr ( index, "?tempmin" ) ) retorno =  data->tempMin + 'C';
    if ( checkStr ( index, "?temp " ) ) retorno =  data->temp + 'C';
    if ( checkStr ( index, "?hum" ) ) retorno =  data->hum + '%' ;
    if ( checkStr ( index, "?tension" ) ) retorno =  data->tension + 'V' ;
    if ( checkStr ( index, "?dhcp" ) ) retorno = (data->dhcp ? "Si" : "No");
    if ( checkStr ( index, "?puerto" ) ) retorno = data->puerto;

    if ( checkStr ( index, "?ipdef" ) ) retorno = data->ipString;
    if ( checkStr ( index, "?mac" ) ) retorno = data->macString;
    if ( checkStr ( index, "?tomas" ) ) retorno =  encodeTomas ( data->estTomas, data->corriente );
    
    return retorno;
}

String server::comandoServerGET ( int index )
{
    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( checkStr ( index, "?mac" ) ) //GET /cmd?mac=DE+AD+BE+EF+FE+ED HTTP/1.1
    {
        if ( header [ index + 4 ] != '=' ) return ERRORPET;
        int fin = header.indexOf ( "HTTP" );

        String aux = header.substring ( index+5, fin );
        #ifdef DEBUGPET
        Serial.println ( aux );
        #endif
        byte macAux [6] = {0};
        int c = 0;
        byte buffer = 0;
        for ( auto i : aux )
        {
            i = toupper ( i );
            
            if ( c>=6 )return ERRORPET;
            if ( !isHexadecimalDigit ( i ) && i!='+' && i!=' ' ) return ERRORPET;   
            if ( i == '+' || i== ' ' ) 
            {
                macAux [c++] = buffer;
                buffer = 0;
            }
            if ( i>='0' && i<='9' ) buffer = buffer*16 + i-'0';
            else if ( i>='A' && i<='F' ) buffer = buffer*16 + 10 + i-'A';
        }
        if ( c!=6 ) return ERRORPET;

        #ifdef DEBUGMAC
        for ( int i=0 ; i<6; i++ ) Serial.println ( macAux[i], 16 );
        Serial.println ( "\n" );

        for ( int i=0 ; i<6; i++ ) Serial.println ( macAux[i] );
        Serial.println ( "\n" );
        #endif

        for ( int i=0; i<6; i++ ) data->mac [i] = macAux [i];
        data->actMacString ();
        
        #ifdef DEBUGMAC
        Serial.println ( encodeMac (data->macString () );
        #endif

        retornoRutina = 7;
        return GUARDADO;
    }
    if ( checkStr ( index, "?tempmax" ) ) //GET /cmd?tempmin=100 HTTP/1.1
    {
        int temp = leerTemp ( header, index );

        if ( temp == 126 ) return ERRORPET;
        if ( temp < data->tempMin || temp > 125 ) return FUERARANGO;

        data->tempMax = temp;
        retornoRutina = 1;
        return GUARDADO;
    }
    if ( checkStr ( index, "?tempmin" ) )  //GET /cmd?tempmin=-11 HTTP/1.1
    {
        int temp = leerTemp ( header, index );

        if ( temp == 126 ) return ERRORPET;
        if ( temp > data->tempMax || temp < -40 ) return FUERARANGO;

        data->tempMin = temp;
        retornoRutina = 1;
        return GUARDADO;
    }
    if ( checkStr ( index, "?tomas" ) ) //GET /cmd?tomas+1=0 HTTP/1.1
    {
        if ( header [ index + 6 ] != '+' || header [ index + 8 ] != '=' || header [ index + 10 ] != ' ' ) return ERRORPET;

        int toma = header [index + 7] - '0';
        int estado = header [index + 9] - '0';

        if ( toma < 1 || toma > 5 ) return FUERARANGO;
        if ( estado != 1 && estado != 0 ) return ERRORPET;

        data->estTomas [toma-1] = estado;
        retornoRutina = 2;
        return GUARDADO;
    }
    if ( checkStr ( index, "?ipdef" ) ) //GET /cmd?ipdef=192.168.0.154 HTTP/1.1
    {
        if ( header [ index + 6 ] != '=' ) return ERRORPET;
        int fin = header.indexOf ( "HTTP" )-1;

        IPAddress aux;
        if ( aux.fromString ( header.substring ( index+7, fin ) ) == 0 ) return FUERARANGO;

        data->ipDef = aux;
        data->actIpString ();

        if ( data->dhcp == 0 ) retornoRutina = 3;
        return GUARDADO;
    }
    if ( checkStr ( index, "?dhcp" ) ) //GET /cmd?dhcp=1 HTTP/1.1
    {
        if ( header [ index + 5 ] != '=' || header [ index + 7 ] != ' ' ) return ERRORPET;
        if (  header [ index + 6 ] != '1' &&  header [ index + 6 ] != '0' ) return ERRORPET;

        int aux = header [ index + 6 ] - '0';
        if ( aux == 0 || aux == 1 ) data->dhcp = aux;

        retornoRutina = 3;
        return GUARDADO;
    }
    if ( checkStr ( index, "?puerto" ) ) //GET /cmd?puerto=10 HTTP/1.1
    {
        if ( header [ index + 7 ] != '=' ) return ERRORPET;

        int fin = header.indexOf ( "HTTP" )-1;
        String aux = header.substring ( index + 8, fin );
 
        for ( int i=0; i<aux.length (); i++ ) if ( !(aux [i] >= '0' && aux [i] <= '9') ) return ERRORPET;

        data->puerto = aux.toInt (); 
        retornoRutina = 4;
        return String ( "Nuevo puerto:" +  String (data->puerto) );
    }
    if ( checkStr ( index, "?clave" ) )
    {
        if ( header [ index + 6 ] != '=' ) return ERRORPET;

        int fin = header.indexOf ( "HTTP" )-1;
        String aux = header.substring ( index + 7, fin );

        for ( int i=0; i<aux.length (); i++ ) if ( !checkAlfaNum ( aux [i] ) ) return ERRORPET;
                
        bufferClave = aux;

        return GUARDADO;
    }
    if ( checkStr ( index, "?user" ) )
    {
        if ( header [ index + 5 ] != '=' ) return ERRORPET;

        int fin = header.indexOf ( "HTTP" )-1;
        String aux = header.substring ( index + 6, fin );

        for ( int i=0; i<aux.length (); i++ ) if ( !checkAlfaNum ( aux [i] ) ) return ERRORPET;

        bufferUser = aux;

        return GUARDADO;
    }
    if ( checkStr ( index, "?validar" ) )
    {
        if ( header [ index + 8 ] != '=' ) return ERRORPET;

        int fin = header.indexOf ( "HTTP" )-1;
        String aux = header.substring ( index + 9, fin );

        if ( aux.compareTo ( bufferUser + "+" + bufferClave ) == 0 ) 
        {
            data->clave = bufferClave;
            data->usuario = bufferUser;
            retornoRutina = 5;
            return "OK";
        }
        else return "Incorrecto";
    }
    return ERRORPET;
}

int server::leerTemp ( String &header, int index )
{
    int temp;
    String aux = header.substring ( index + 9, index + 12 );
    if ( aux [0] == '-' )
    {
        aux.remove ( 0, 1 );
        temp = -aux.toInt ();
    }
    else temp = aux.toInt ();

    if ( !comprobarTempBoundaries ( header, temp, index ) ) return 126;

    return temp;
}

bool server::comprobarTempBoundaries ( String &header, int temp, int index )
{
    if ( header [ index + 8 ] != '=' ) return 0;
    if ( temp >= 0 && temp < 10 && header [ index + 10 ] != ' ' ) return 0;

    if ( temp >= 10 && temp < 100 && header [ index + 11 ] != ' ' ) return 0;
    if ( temp >= -9 && temp < 0 && header [ index + 11 ] != ' ' ) return 0;

    if ( temp >= 100 && temp <= 125 && header [ index + 12 ] != ' ' ) return 0;
    if ( temp >= -40 && temp <= -10 && header [ index + 12 ] != ' ' ) return 0;

    return 1;
}

String server::encodeTomas ( bool *estTomas, float *corriente )
{
    String aux = "";
    for ( int i = 0 ; i < 5 ; i++ ) 
    {
        aux += "Toma: " + String ( i+1 ) + " = ";
        if ( *(estTomas + i) ) aux += String ( *(corriente + i) ) + 'A';
        else aux += "Off";
        aux += '\n';
    }
    return aux;
}

void server::checkDHCP ()
{
    /*
    *       Cada 30 minutos llama al método que comprueba que la ddirección DHCP siga siendo válida
    *   puede retornar:
    *   --0: el metoro hizo algo mal
    *   --1: DHCP renovado con la misma IP
    *   --2: Falló renovar la misma IP
    *   --3: Renovó el DHCP pero con una IP diferente
    *   --4: Falló de renovar el DHCP con una IP distinta
    *      
    *       Si cambia la IP en el proceso no es necesario cambiar ninguna cosa adicional ya que siempre
    *   se accede desde Ethernet.localIP () cuando se quiere saber la IP que se usa y luego no se guarda
    *   en ningún otro lado.
    *       Solo se necesita reiniciar los contadores.
    */
    if ( millis () - millisDHCP >= PERIODODHCP ) 
    {
        #ifdef DEBUGDHCP
        Serial.println ( "Check DHCP" );
        #endif
        millisDHCP = millis ();
        int retorno = Ethernet.maintain ();
        File log = SD.open ( "logDHCP.txt", FILE_WRITE );
        switch (retorno)
        {
        case 1:
            log.println ( "Renueva IP" );
            break;
        case 2:
            log.println ( "Falla renovar" );
            break;
        case 3:
            log.print ( "Nueva IP: " );
            log.println ( Ethernet.localIP () );
            break;
        case 4:
            log.println ( "Falla en tomar nueva IP" );
            break;
        default:
            log.println ( "Retorno 0" );
            break;
        }
        log.close ();
        if ( retorno == 0 || retorno == 1 || retorno == 3 )
        {
            if ( contErrorDHCP >= FALLOSDHCP ) load (); //millisDHCP y el contador se renuevan en load ()
            else contErrorDHCP++;
        }
        if ( retorno == 2 || retorno == 4 ) 
        {
            millisDHCP = millis ();
            contErrorDHCP = 0;
        }
        #ifdef DEBUGDHCP
        Serial.println ( "Check DHCP retorno: " + String ( retorno ) );
        #endif
    }
}

bool server::checkStr ( int index, const char *str )
{
    int largo = strlen ( str );
    return !(header.substring ( index, index + largo ).compareTo ( str ));
}

bool server::checkAlfaNum ( char c ) { return !(!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z') ); }