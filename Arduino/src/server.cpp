#include "headers/server.h"

EthernetClient cmdCliente;

server::server ( DATA &data ): EthernetServer ( data.puerto )
{
    this->data = &data;
}

void server::setup ()
{
    Ethernet.begin ( data->mac, data->ipDef );
    
    //if ( Ethernet.hardwareStatus() == EthernetNoHardware || Ethernet.linkStatus() != LinkON ) errorEthernet = 1;

    load ();

    begin ();
}

void server::load ()
{
    Ethernet.begin ( data->mac, data->ipDef );
    
    if ( data->dhcp == 1 ) 
    {
        Serial.println ( "DHCP" );
        if ( Ethernet.begin ( data->mac ) == 0 ) 
        {
            Serial.println ( "Falla DHCP" );
            Ethernet.begin ( data->mac, data->ipDef );
            data->dhcp = 0;
        }
    }

    contErrorDHCP = 0;
    millisDHCP = millis ();
}

int server::rutina ()
{
    checkDHCP ();
    cmdCliente.flush ();
    cmdCliente = available ();
    char tipoPeticion = '\0';
    bool lineaEnBlanco = 1;

    if ( cmdCliente )
        while ( cmdCliente.connected () )
            {
                char c;
                if ( cmdCliente.available () )
                {
                    c = cmdCliente.read ();
                    if ( tipoPeticion == '\0' ) tipoPeticion = toupper (c);
                    Serial.print ( c );
                    if ( peticion.length () < 100 ) peticion += c;
                }
                if ( tipoPeticion == 'G' && c == '\n' )
                {
                    retorno ();
                    delay (1);
                    cmdCliente.stop ();
                    break;
                }
                else if ( lineaEnBlanco && c == '\n' )
                {        
                    
                    cmdCliente.println ( "HTTP/1.1 200 OK" );
                    cmdCliente.println ( "Content-Type: text/plain" );
                    cmdCliente.println ( "Connection: close" );
                    cmdCliente.println ();
                    cmdCliente.println ( "CHACHACHACHA" );

                    delay (1);
                    cmdCliente.stop ();
                    break;
                }
                if ( c == '\n' ) lineaEnBlanco = 1;
                else if ( c != '\r' ) lineaEnBlanco = 0;
            }
    return flagGuardado;
}

void server::retorno ()
{
    String devolucion = "Peticion erronea";

    if ( peticion.indexOf ( "/lec" ) > 0 && checkLogin () ) devolucion = lecturaServer ( peticion.indexOf ( "/lec" ) );
    else if ( peticion.indexOf ( "/cmd" ) > 0 && checkLogin () ) devolucion = comandoServer ( peticion.indexOf ( "/cmd" ) );
    else if ( peticion.indexOf ( " / " ) > 0 ) devolucion = "Zapatilla IP OK";
    //Serial.println ( "Devolviendo" );

    cmdCliente.println ( "HTTP/1.1 200 OK" );
    cmdCliente.println ( "Content-Type: text/plain" );
    cmdCliente.println ( "Connection: close" );
    cmdCliente.println (); 
    cmdCliente.println ( devolucion );

    peticion = "";
}

bool server::checkLogin ()
{
    return 1;
}

String server::lecturaServer ( int index )
{
    String retorno = ERRORPET;
    index += 4; //Desplaza el cursor al final del sector de string ya sea /lec o /cmd, el indice primero corresponde al "/" al sumarle 4 pasas al "?"

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

        jsonRequest ["mac"] = encodeMac ( data->mac );
        jsonRequest ["ipDef"] = encodeIp ( data->ipDef );

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

    if ( checkStr ( index, "?ipdef" ) ) retorno = encodeIp (data->ipDef);
    if ( checkStr ( index, "?mac" ) ) retorno = encodeMac (data->mac);
    if ( checkStr ( index, "?tomas" ) ) retorno =  encodeTomas ( data->estTomas, data->corriente );
    
    return retorno;
}

String server::comandoServer ( int index )
{
    index += 4; //Desplaza el cursor al final del sector de string ya sea /lec o /cmd, el indice primero corresponde al "/" al sumarle 4 pasas al "?"

    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( checkStr ( index, "?mac" ) ) //GET /cmd?mac=DE+AD+BE+EF+FE+ED HTTP/1.1
    {
        if ( peticion [ index + 4 ] != '=' || peticion [ index + 22 ]) return ERRORPET;

    }
    if ( checkStr ( index, "?tempmax" ) ) //GET /cmd?tempmin=100 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );

        if ( temp == 126 ) return ERRORPET;
        if ( temp < data->tempMin || temp > 125 ) return FUERARANGO;

        data->tempMax = temp;
        flagGuardado = 1;
        return GUARDADO;
    }
    if ( checkStr ( index, "?tempmin" ) )  //GET /cmd?tempmin=-11 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );

        if ( temp == 126 ) return ERRORPET;
        if ( temp > data->tempMax || temp < -40 ) return FUERARANGO;

        data->tempMin = temp;
        flagGuardado = 1;
        return GUARDADO;
    }
    if ( checkStr ( index, "?tomas" ) ) //GET /cmd?tomas+1=0 HTTP/1.1
    {
        if ( peticion [ index + 6 ] != '+' || peticion [ index + 8 ] != '=' || peticion [ index + 10 ] != ' ' ) return ERRORPET;

        int toma = peticion [index + 7] - 48;
        int estado = peticion [index + 9] - 48;

        if ( toma < 1 || toma > 5 ) return FUERARANGO;
        if ( estado != 1 && estado != 0 ) return ERRORPET;

        data->estTomas [toma-1] = estado;
        flagGuardado = 2;
        return GUARDADO;
    }
    if ( checkStr ( index, "?ipdef" ) ) //GET /cmd?ipdef=192.168.0.154 HTTP/1.1
    {
        if ( peticion [ index + 6 ] != '=' ) return ERRORPET;
        int fin = peticion.indexOf ( "HTTP" )-1;

        IPAddress aux;
        if ( aux.fromString ( peticion.substring ( index+7, fin ) ) == 0 ) return FUERARANGO;

        data->ipDef = aux;
        if ( data->dhcp == 0 ) flagGuardado = 3;
        return GUARDADO;
    }
    if ( checkStr ( index, "?dhcp" ) ) //GET /cmd?dhcp=1 HTTP/1.1
    {
        if ( peticion [ index + 5 ] != '=' || peticion [ index + 7 ] != ' ' ) return ERRORPET;
        if (  peticion [ index + 6 ] != '1' &&  peticion [ index + 6 ] != '0' ) return ERRORPET;

        int aux = peticion [ index + 6 ] - 48;
        if ( aux == 0 || aux == 1 ) data->dhcp = aux;

        flagGuardado = 3;
        if ( !data->dhcp ) return String ( "Nueva IP:" + data->ipDef );
        return GUARDADO;
    }
    if ( checkStr ( index, "?puerto" ) ) //GET /cmd?puerto=10 HTTP/1.1
    {
        if ( peticion [ index + 7 ] != '=' ) return ERRORPET;

        int fin = peticion.indexOf ( "HTTP" )-1;
        String aux = peticion.substring ( index + 8, fin );
 
        for ( int i=0; i<aux.length (); i++ ) 
        {
            Serial.println (aux[i]);
            if ( !(aux [i] >= '0' && aux [i] <= '9') ) return ERRORPET;
        }

        data->puerto = aux.toInt (); 
        flagGuardado = 4;
        return String ( "Nueva puerto:" +  String (data->puerto) );
    }
    if ( checkStr ( index, "?clave" ) )
    {
        if ( peticion [ index + 6 ] != '=' ) return ERRORPET;

        int fin = peticion.indexOf ( "HTTP" )-1;
        String aux = peticion.substring ( index + 7, fin );

        for ( int i=0; i<aux.length (); i++ ) if ( !checkAlfaNum ( aux [i] ) ) return ERRORPET;
                
        bufferClave = aux;

        return GUARDADO;
    }
    if ( checkStr ( index, "?user" ) )
    {
        if ( peticion [ index + 5 ] != '=' ) return ERRORPET;

        int fin = peticion.indexOf ( "HTTP" )-1;
        String aux = peticion.substring ( index + 6, fin );

        for ( int i=0; i<aux.length (); i++ ) if ( !checkAlfaNum ( aux [i] ) ) return ERRORPET;

        bufferUser = aux;

        return GUARDADO;
    }
    if ( checkStr ( index, "?validar" ) )
    {
        if ( peticion [ index + 8 ] != '=' ) return ERRORPET;

        int fin = peticion.indexOf ( "HTTP" )-1;
        String aux = peticion.substring ( index + 9, fin );

        if ( aux.compareTo ( bufferUser + "+" + bufferUser ) == 0 ) 
        {
            data->clave = bufferClave;
            data->usuario = bufferUser;
            flagGuardado = 5;
            return "OK";
        }
        else return "Incorrecto";
    }
    return ERRORPET;
}

int server::leerTemp ( String &peticion, int index )
{
    int temp;
    String aux = peticion.substring ( index + 9, index + 12 );
    if ( aux [0] == '-' )
    {
        aux.remove ( 0, 1 );
        temp = -aux.toInt ();
    }
    else temp = aux.toInt ();

    if ( !comprobarTempBoundaries ( peticion, temp, index ) ) return 126;

    return temp;
}

bool server::comprobarTempBoundaries ( String &peticion, int temp, int index )
{
    if ( peticion [ index + 8 ] != '=' ) return 0;
    if ( temp >= 0 && temp < 10 && peticion [ index + 10 ] != ' ' ) return 0;

    if ( temp >= 10 && temp < 100 && peticion [ index + 11 ] != ' ' ) return 0;
    if ( temp >= -9 && temp < 0 && peticion [ index + 11 ] != ' ' ) return 0;

    if ( temp >= 100 && temp <= 125 && peticion [ index + 12 ] != ' ' ) return 0;
    if ( temp >= -40 && temp <= -10 && peticion [ index + 12 ] != ' ' ) return 0;

    return 1;
}

String server::encodeMac ( byte *mac )
{
    String aux = "";
    for ( int i = 0 ; i < 6 ; i++ ) 
    {
        String aux2 = String ( *( mac + i ), HEX );
        aux2.toUpperCase ();
        aux += aux2 + ' ';
    }
    return aux;
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

String server::encodeIp ( IPAddress &aux ) { return ( String ( aux [0] ) + '.' + String ( aux [1] ) + '.' + String ( aux [2] ) + '.' + String ( aux [3] ) ); }

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
    if ( millis () - millisDHCP >= periodoDHCP ) 
    {
        millisDHCP = millis ();
        int retorno = Ethernet.maintain ();
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
    }
}

bool server::checkStr ( int index, const char *str )
{
    int largo = strlen ( str );
    return !(peticion.substring ( index, index + largo ).compareTo ( str ));
}

bool server::checkAlfaNum ( char c ) { return !(!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z') ); }