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
            if ( ( !cmdCliente.available () || c == '\n' ) )
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
    cmdCliente.println ( devolucion );

    peticion = "";
}

String server::lecturaServer ( int index )
{
    String retorno = ERRORPET;
    index += 4; //Desplaza el cursor al final del sector de string ya sea /lec o /cmd, el indice primero corresponde al "/" al sumarle 4 pasas al "?"

    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( peticion.substring ( index, index + 5 ).compareTo ( "?todo" ) == 0 ) 
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
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmax" ) == 0 ) retorno =  data->tempMax + 'C';
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmin" ) == 0 ) retorno =  data->tempMin + 'C';
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?temp" ) == 0 ) retorno =  data->temp + 'C';
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?hum" ) == 0 ) retorno =  data->hum + '%' ;
    if ( peticion.substring ( index, index + 8 ).compareTo ( "?tension" ) == 0 ) retorno =  data->tension + 'V' ;
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?dhcp" ) == 0 ) retorno = (data->dhcp ? "Si" : "No");
    if ( peticion.substring ( index, index + 7 ).compareTo ( "?puerto" ) == 0 ) retorno = data->puerto;

    if ( peticion.substring ( index, index + 6 ).compareTo ( "?ipdef" ) == 0 ) retorno = encodeIp (data->ipDef);
    if ( peticion.substring ( index, index + 4 ).compareTo ( "?mac" ) == 0 ) retorno = encodeMac (data->mac);
    if ( peticion.substring ( index, index + 6 ).compareTo ( "?tomas" ) == 0 ) retorno =  encodeTomas ( data->estTomas, data->corriente );
    
    return retorno;
}

String server::comandoServer ( int index )
{
    index += 4; //Desplaza el cursor al final del sector de string ya sea /lec o /cmd, el indice primero corresponde al "/" al sumarle 4 pasas al "?"

    //Se corroboran los substring a ver si las peticiones están bien formuladas, .substring no incluye el último caracter añadido por lo que (index, index+5) tomará hasta index+4

    if ( peticion.substring ( index, index + 4 ).compareTo ( "?mac" ) == 0 ) //GET /cmd?mac=DE+AD+BE+EF+FE+ED HTTP/1.1
    {
        if ( peticion [ index + 4 ] != '=' || peticion [ index + 22 ]) return ERRORPET;
    }
    else if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmax" ) == 0 ) //GET /cmd?tempmin=100 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );
        if ( temp == 126 ) return ERRORPET;
        if ( temp < data->tempMin || temp > 125 ) return FUERARANGO;

        data->tempMax = temp;
        flagGuardado = 1;
        return GUARDADO;
    }
    else if ( peticion.substring ( index, index + 8 ).compareTo ( "?tempmin" ) == 0 )  //GET /cmd?tempmin=-11 HTTP/1.1
    {
        int temp = leerTemp ( peticion, index );
        if ( temp == 126 ) return ERRORPET;
        if ( temp > data->tempMax || temp < -40 ) return FUERARANGO;

        data->tempMin = temp;
        flagGuardado = 2;
        return GUARDADO;
    }
    else if ( peticion.substring ( index, index + 6 ).compareTo ( "?tomas" ) == 0 ) //GET /cmd?tomas+1=0 HTTP/1.1
    {                                                                               //012345678901234567890123456
        if ( peticion [ index + 6 ] != '+' || peticion [ index + 8 ] != '=' || peticion [ index + 10 ] != ' ' ) return ERRORPET;

        int toma = peticion [index + 7] - 48;
        int estado = peticion [index + 9] - 48;

        if ( toma < 1 || toma > 5 ) return FUERARANGO;
        if ( estado == 1 || estado == 0 )
        {
            data->estTomas [toma-1] = estado;
            flagGuardado = 3;
            return GUARDADO;
        }
        
    }
    if ( peticion.substring ( index, index + 6 ).compareTo ( "?ipdef" ) == 0 ) //GET /cmd?ipdef=192.168.0.154 HTTP/1.1
    {
        if ( peticion [ index + 6 ] != '=' ) return ERRORPET;
        int fin = peticion.indexOf ( "HTTP" )-1;

        IPAddress aux;
        if ( aux.fromString ( peticion.substring ( index+7, fin ) ) == 0 ) return FUERARANGO;

        data->ipDef = aux;

        if ( data->dhcp == 0 ) 
        {
            Ethernet.begin ( data->mac, data->ipDef );
            flagGuardado = 4;
        }

        return GUARDADO;
    }
    if ( peticion.substring ( index, index + 5 ).compareTo ( "?dhcp" ) == 0 ) //GET /cmd?dhcp=1 HTTP/1.1
    {
        if ( peticion [ index + 5 ] != '=' || peticion [ index + 7 ] != ' ' ) return ERRORPET;
        if (  peticion [ index + 6 ] != '1' &&  peticion [ index + 6 ] != '0' ) return ERRORPET;

        int aux = peticion [ index + 6 ] - 48;

        if ( aux == 0 || aux == 1 ) data->dhcp = aux;

        flagGuardado = 6;

        if ( !data->dhcp )return String ( "Nueva IP:" + data->ipDef );
        else return GUARDADO;
    }
    if ( peticion.substring ( index, index + 7 ).compareTo ( "?puerto" ) == 0 ) //GET /cmd?puerto=10 HTTP/1.1
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

        flagGuardado = 6;

        return String ( "Nueva puerto:" +  String (data->puerto) + "\nReiniciando equipo" );
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