#include "headers/server.h"

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

server::server ( DATA &data ): EthernetServer ( data.puerto )
{
    this->data = &data;
}

void server::setup ()
{
    Ethernet.begin ( data->mac, data->ipDef );
    
    //if ( Ethernet.hardwareStatus() == EthernetNoHardware || Ethernet.linkStatus() != LinkON ) errorEthernet = 1;

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

        aux += "\"tempMax\" : " + String (data->tempMax) + coma + linea;
        aux += "\"tempMin\" : " + String (data->tempMin) + coma + linea;
        aux += "\"tempAct\" : " + String (data->temp) + coma + linea;
        aux += "\"humAct\" : " + String (data->hum) + coma + linea;

        aux += "\"tension\" : " + String (data->tension) + coma + linea;
        aux += "\"dhcp\" : " + String (data->dhcp) + coma + linea;

        aux += "\"mac\" : \""  + String ( encodeMac ( data->mac ) ) + "\"" + coma + linea;
        aux += "\"ipDef\" : \"" + String ( encodeIp ( data->ipDef ) ) + "\"" + coma + linea;

        String aux2 = "\"estTomas\" : [\n";
        for ( int i = 0 ; i < 5 ; i++ )
        {
            aux2 += String ( data->estTomas [i] );
            if ( i != 4 ) aux2 += coma;
            aux2 += linea;
        } 
        aux2 += ']';
        //Serial.println ( aux2 );

        aux += aux2 + coma + linea;

        aux2 = "\"corriente\" : [\n";
        for ( int i = 0 ; i < 5 ; i++ )
        {
            aux2 += String ( data->corriente [i] );
            if ( i != 4 ) aux2 += coma;
            aux2 += linea;
        } 
        aux2 += ']';
        //Serial.println ( aux2 );

        aux += aux2 + linea;

        aux += '}';
        return aux;
    }
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmax" ) == 0 ) return ( String (data->tempMax) + 'C' );
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmin" ) == 0 ) return ( String (data->tempMin) + 'C' );
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?temp" ) == 0 ) return ( String (data->temp) + 'C' );
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?hum" ) == 0 ) return ( String (data->hum) + '%' );
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tension" ) == 0 ) return ( String (data->tension) + 'V' );
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?dhcp" ) == 0 ) return ( String ((data->dhcp) ? "Si" : "No") );

    if ( peticion.substring ( index, index + 6 ).compareTo ( "?ipdef" ) == 0 ) return encodeIp (data->ipDef);
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?mac" ) == 0 ) return encodeMac (data->mac);
    if ( peticion.substring ( index, index + 6 ).compareTo ( "?tomas" ) == 0 ) return encodeTomas ( data->estTomas, data->corriente );
    
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

        data->ipDef = aux;

        if ( data->dhcp == 0 ) 
        {
            Ethernet.begin ( data->mac, data->ipDef );
            flagGuardado = 4;
        }

        return guardado;
    }
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?dhcp" ) == 0 ) //GET /cmd?dhcp=1 HTTP/1.1
    {
        if ( peticion [ index + 5 ] != '=' ||  peticion [ index + 7 ] != ' ' ) return error;
        if (  peticion [ index + 6 ] != '1' &&  peticion [ index + 6 ] != '0' ) return error;

        data->dhcp = peticion [ index + 6 ] - 48;

        if ( data->dhcp == 0 ) Ethernet.begin ( data->mac, data->ipDef );
        else if ( Ethernet.begin ( data->mac ) == 0 ) 
        {
            Ethernet.begin ( data->mac, data->ipDef );
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
        if ( temp < data->tempMin || temp > 125 ) return fueraRango;

        data->tempMax = temp;
        flagGuardado = 1;
        return guardado;
    }
    else if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmin" ) == 0 )  //GET /cmd?tempmin=-11 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );
        if ( temp == 126 ) return error;
        if ( temp > data->tempMax || temp < -40 ) return fueraRango;

        data->tempMin = temp;
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
            data->estTomas [toma] = 1;
            flagGuardado = 3;
            return guardado;
        }
        if ( estado == 0 )
        {
            data->estTomas [toma] = 0;
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