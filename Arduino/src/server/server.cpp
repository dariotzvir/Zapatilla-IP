#include "server.h"

EthernetClient cmdCliente;
String fueraRango = "Fuera de rango";
String guardado = "Guardado";
String error = "&";

bool comprobarTempBoundaries ( String &peticion, int temp, int index );
int leerTemp ( String &peticion, int index );
String encodeTomas ( bool *estTomas, float *corriente );
String encodeMac ( byte *mac );
const char coma = ',', linea = '\n';
String encodeIp ( IPAddress &ip );

server::server ( byte *mac, int &puerto, IPAddress &ipFija, float *corriente, bool *estTomas,
                float &tempAct, float &humAct, float &tension, int &tempMax, int &tempMin, bool &flagDhcp ):
                EthernetServer ( puerto )
{
    this->mac = mac;
    this->puerto = &puerto; 
    this->ipFija = &ipFija;
    this->corriente = corriente;
    this->estTomas = estTomas;
    this->tempAct = &tempAct;
    this->humAct = &humAct;
    this->tension = &tension;
    this->tempMax = &tempMax;
    this->tempMin = &tempMin;
    this->flagDhcp = &flagDhcp;
}

void server::setup ()
{
    Ethernet.begin ( mac, *ipFija );
    
    //if ( Ethernet.hardwareStatus() == EthernetNoHardware || Ethernet.linkStatus() != LinkON ) errorEthernet = 1;

    if ( *flagDhcp == 1 ) 
    {
        Serial.println ( "DHCP" );
        if ( Ethernet.begin ( mac ) == 0 ) 
        {
            Serial.println ( "Falla DHCP" );
            Ethernet.begin ( mac, *ipFija );
        }
    }
        

    begin ();
}

int server::rutina ()
{
    cmdCliente.flush ();
    cmdCliente = available ();
    
    if ( cmdCliente ) while ( cmdCliente.connected () )
        if ( cmdCliente.available () )
        {
            char c;
            if ( cmdCliente.available () )
            {
                c = cmdCliente.read ();
                Serial.print ( c );
                if ( peticion.length () < 100 ) peticion += c;
            }
            if ( !cmdCliente.available () || c == '\n' ) 
            {
                retorno ();
                delay (1);
                cmdCliente.stop ();
                break;
            }
        }
    
    return flagGuardado;
}

void server::retorno ()
{
    String devolucion = "Peticion erronea";

    if ( peticion.indexOf ( "/lec" ) > 0 ) devolucion = lecturaServer ( peticion.indexOf ( "/lec" ) );
    else if ( peticion.indexOf ( "/cmd" ) > 0 ) devolucion = comandoServer ( peticion.indexOf ( "/cmd" ) );
    else if ( peticion.indexOf ( " / " ) > 0 ) devolucion = "Zapatilla IP OK";
    //Serial.println ( "Devolviendo" );

    cmdCliente.println ( "HTTP/1.1 400 OK" );
    cmdCliente.println ( "Content-Type: text/plain" );
    cmdCliente.println ( "Connection: close" );
    cmdCliente.println (); 
    cmdCliente.println ( devolucion == "&" ? "Peticion erronea" : devolucion );

    peticion = "";
}

String server::lecturaServer ( int index )
{
    index += 4; //Desplaza el cursor al final del sector de string ya sea /lec o /cmd, el indice primero corresponde al "/" al sumarle 4 pasas al "?"

    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( peticion.substring ( index, index + 5 ).compareTo ( "?todo" ) == 0 ) 
    {
        String aux = "{\n";

        aux += "\"tempMax\" : " + String (*tempMax) + coma + linea;
        aux += "\"tempMin\" : " + String (*tempMin) + coma + linea;
        aux += "\"tempAct\" : " + String (*tempAct) + coma + linea;
        aux += "\"humAct\" : " + String (*humAct) + coma + linea;

        aux += "\"tension\" : " + String (*tension) + coma + linea;
        aux += "\"dhcp\" : " + String (*flagDhcp) + coma + linea;

        aux += "\"mac\" : \""  + String ( encodeMac ( mac ) ) + "\"" + coma + linea;
        aux += "\"ipDef\" : \"" + String ( encodeIp ( *ipFija ) ) + "\"" + coma + linea;

        String aux2 = "\"estTomas\" : [\n";
        for ( int i = 0 ; i < 5 ; i++ )
        {
            aux2 += String ( *(estTomas + i) );
            if ( i != 4 ) aux2 += coma;
            aux2 += linea;
        } 
        aux2 += ']';
        //Serial.println ( aux2 );

        aux += aux2 + coma + linea;

        aux2 = "\"corriente\" : [\n";
        for ( int i = 0 ; i < 5 ; i++ )
        {
            aux2 += String ( *(corriente + i) );
            if ( i != 4 ) aux2 += coma;
            aux2 += linea;
        } 
        aux2 += ']';
        //Serial.println ( aux2 );

        aux += aux2 + linea;

        aux += '}';
        return aux;
    }
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmax" ) == 0 ) return ( String (*tempMax) + 'C' );
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmin" ) == 0 ) return ( String (*tempMin) + 'C' );
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?temp" ) == 0 ) return ( String (*tempAct) + 'C' );
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?hum" ) == 0 ) return ( String (*humAct) + '%' );
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tension" ) == 0 ) return ( String (*tension) + 'V' );
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?dhcp" ) == 0 ) return ( String (*flagDhcp ? "Si" : "No") );

    if ( peticion.substring ( index, index + 6 ).compareTo ( "?ipdef" ) == 0 ) return encodeIp ( *ipFija );
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?mac" ) == 0 ) return encodeMac ( mac );
    if ( peticion.substring ( index, index + 6 ).compareTo ( "?tomas" ) == 0 ) return encodeTomas ( estTomas, corriente );
    
    return error;
}

String server::comandoServer ( int index )
{
    index += 4; //Desplaza el cursor al final del sector de string ya sea /lec o /cmd, el indice primero corresponde al "/" al sumarle 4 pasas al "?"

    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( peticion.substring ( index, index + 6 ).compareTo ( "?ipdef" ) == 0 ) //GET /cmd?ipdef=192.168.0.154 HTTP/1.1
    {
        if ( peticion [ index + 6 ] != '=' ) return error;
        int fin = peticion.indexOf ( "HTTP" )-1;

        IPAddress aux;
        if ( aux.fromString ( peticion.substring ( index+7, fin ) ) == 0 ) return fueraRango;

        *ipFija = aux;

        if ( *flagDhcp == 0 ) 
        {
            Ethernet.begin ( mac, *ipFija );
            flagGuardado = 4;
        }

        return guardado;
    }
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?dhcp" ) == 0 ) //GET /cmd?dhcp=1 HTTP/1.1
    {
        if ( peticion [ index + 5 ] != '=' ||  peticion [ index + 7 ] != ' ' ) return error;
        if (  peticion [ index + 6 ] != '1' &&  peticion [ index + 6 ] != '0' ) return error;

        *flagDhcp = peticion [ index + 6 ] - 48;

        if ( *flagDhcp == 0 ) Ethernet.begin ( mac, *ipFija );
        else if ( Ethernet.begin ( mac ) == 0 ) 
        {
            Ethernet.begin ( mac, *ipFija );
            return ( F( "Configuracion de DHCP fallida, se mantiene el host" ) );
        }

        //Serial.println ( Ethernet.localIP () );
        return String ( "Nueva IP:" + Ethernet.localIP () );
    }
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?mac" ) == 0 ) //GET /cmd?mac=DE+AD+BE+EF+FE+ED HTTP/1.1
    {
        if ( peticion [ index + 4 ] != '=' || peticion [ index + 22 ]) return error;
    }
    else if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmax" ) == 0 ) //GET /cmd?tempmin=100 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );
        if ( temp == 126 ) return error;
        if ( temp < *tempMin || temp > 125 ) return fueraRango;

        *tempMax = temp;
        flagGuardado = 1;
        return guardado;
    }
    else if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmin" ) == 0 )  //GET /cmd?tempmin=-11 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );
        if ( temp == 126 ) return error;
        if ( temp > *tempMax || temp < -40 ) return fueraRango;

        *tempMin = temp;
        flagGuardado = 2;
        return guardado;
    }
    else if ( peticion.substring ( index, index + 6 ).compareTo ( "?tomas" ) == 0 ) //GET /cmd?tomas+1=0 HTTP/1.1
    {                                                                               //012345678901234567890123456
        if ( peticion [ index + 6 ] != '+' || peticion [ index + 8 ] != '=' || peticion [ index + 10 ] != ' ' ) return error;

        int toma = peticion [index + 7] - 48;
        int estado = peticion [index + 9] - 48;

        if ( toma < 1 || toma > 5 ) return fueraRango;
        if ( estado == 1 )
        {
            *( estTomas + toma - 1 ) = 1;
            flagGuardado = 3;
            return guardado;
        }
        if ( estado == 0 )
        {
            *( estTomas + toma - 1 ) = 0;
            flagGuardado = 3;
            return guardado;
        }
        
    }
    return error;
}

int leerTemp ( String &peticion, int index )
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

bool comprobarTempBoundaries ( String &peticion, int temp, int index )
{
    if ( peticion [ index + 8 ] != '=' ) return 0;
    if ( temp >= 0 && temp < 10 && peticion [ index + 10 ] != ' ' ) return 0;

    if ( temp >= 10 && temp < 100 && peticion [ index + 11 ] != ' ' ) return 0;
    if ( temp >= -9 && temp < 0 && peticion [ index + 11 ] != ' ' ) return 0;

    if ( temp >= 100 && temp <= 125 && peticion [ index + 12 ] != ' ' ) return 0;
    if ( temp >= -40 && temp <= -10 && peticion [ index + 12 ] != ' ' ) return 0;

    return 1;
}

String encodeMac ( byte *mac )
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

String encodeTomas ( bool *estTomas, float *corriente )
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

String encodeIp ( IPAddress &aux ) { return ( String ( aux [0] ) + '.' + String ( aux [1] ) + '.' + String ( aux [2] ) + '.' + String ( aux [3] ) ); }