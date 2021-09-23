#include "headers/server.h"

#include <SD.h>
#include <utility/w5100.h>

#define alfaNum(c)((c >= '0' && c <= '9') ||(c >= 'A' && c <= 'Z') ||(c >= 'a' && c <= 'z'))

EthernetClient cmdCliente;

server::server(DATA &data, void(*guardarSD)()): EthernetServer(data.puerto)
{
    this->data = &data;
    this->guardarSD = guardarSD;
}

void server::setup()
{
    Ethernet.begin(data->mac, data->ipDef);
    //if(Ethernet.hardwareStatus() == EthernetNoHardware || Ethernet.linkStatus() != LinkON) errorEthernet = 1;

    load();

    begin();
}

void server::load()
{
    Ethernet.begin(data->mac, data->ipDef);
    
    if(data->dhcp == 1) 
    {
        #ifdef DEBUGDHCP
        Serial.println("DHCP");
        #endif
        if(Ethernet.begin(data->mac) == 0) 
        {
            #ifdef DEBUGDHCP
            Serial.println("Falla DHCP");
            #endif
            Ethernet.begin(data->mac, data->ipDef);
            data->dhcp = 0;
        }
    }

    //contErrorDHCP = 0;
    //millisDHCP = millis();
}

int8_t server::rutina()
{
    retornoRutina = -1;
    
    //if(data->dhcp) checkDHCP();

    cmdCliente.flush();
    cmdCliente=available();

    if(cmdCliente)
    {
        req="";
        message="";
        errorParse=0;
        errorCmd=0;
        errorLogin=0;

        #ifdef DEBUGPET
        Serial.print("\nConectado\n");
        #endif
        retornoRutina=0;

        lectura();  
        conversion();
        if(ruta!=ERROR && ruta!=HOME) checkLogin();
        if(ruta==CMD && !errorParse) ejecutarCmd();
        devolucion();

        delay(1);
        cmdCliente.stop();
    }

    if(retornoRutina > 0)(*guardarSD)();
    return retornoRutina;   
}

void server::lectura()
{
    tipo=GET;
    while(cmdCliente.connected())
    {
        String resto="";
        char c=0;
        while(c!='\n') 
        {
            c=cmdCliente.read();
            req.concat(c);
        }
        req.remove(req.length()-1);

        Serial.print("Request: ");
        Serial.println(req);

        if(req[0]!='G')
        {
            while(c!=-1)
            {
                c=cmdCliente.read();
                resto.concat(c);
            }
            int index=resto.indexOf("\r\n\r\n");
            message=resto.substring(index+4,-1);
            Serial.print("Message: ");
            Serial.println(message);

            tipo=POST;
            break;
        }
        else break;
    }
}

void server::conversion()
{        
    ruta=HOME;
    
    int ini=req.indexOf('/');
    int fin=req.indexOf(' ', ini+1);

    //GET /cmd     POST /cmd
    //GET /lec     POST /lec
    //GET / HT     POST / HT
    String ruteStr=req.substring(ini, fin);

    if(tipo==GET && ruteStr.length() > 1) 
    {
        int fin=ruteStr.indexOf('?');
        if(fin!=-1) ruteStr.remove(fin, -1);
        else ruta=ERROR;
    }

    Serial.print("Ruta: ");
    Serial.print(ruteStr);
    Serial.println("$");

    if(ruta!=ERROR)
    {
        if(ruteStr=="/") ruta=HOME;
        else if(ruteStr=="/cmd" || ruteStr=="/CMD") ruta=CMD;
        else if(ruteStr=="/lec" || ruteStr=="/LEC") ruta=LEC;
        else ruta=ERROR;
    }
    if(ruta==CMD || ruta==LEC)
    {
        if(tipo==GET) errorParse=!parseGET();
        if(tipo==POST) errorParse=!parsePOST();
    }
}

void server::checkLogin()
{
    if(clave!=data->clave) errorLogin=1;
    if(user!=data->usuario) errorLogin=1;
}

void server::ejecutarCmd()
{

}

void server::devolucion()
{
    cmdCliente.println("HTTP/1.1 200 OK");
    cmdCliente.println("Content-Type: text/plain");
    cmdCliente.println("Connection: close");
    cmdCliente.println(); 
    
    if(ruta==HOME) cmdCliente.println("Zapatilla IP OK");
    else if(ruta==ERROR) cmdCliente.println("Ruta incorrecta");
    else if(errorLogin) cmdCliente.println("Login incorrecto");
    else if(errorParse) cmdCliente.println("Formato de peticion incorrecto");
    else if(ruta==LEC) cmdCliente.println(retornoLecturas());
    else if(ruta==CMD) cmdCliente.println( (errorCmd ? "Comando erroneo" : "Comando ejecutado") );
}

bool server::parseGET()
{
    int ini=req.indexOf('?');
    int fin=req.indexOf(' ', ini);

    String buf=req.substring(ini+1, fin);

    return (parseStr (buf));
}
bool server::parsePOST()
{
    message.remove(message.length()-1);

    return (parseStr (message));
}
bool server::parseStr(String str)
{
    user="";
    clave="";
    cmd="";
    param="";
    
    int finUser=str.indexOf('+');
    if(finUser==-1) return 0;

    user=str.substring(0, finUser);

    int finClave=str.indexOf('+', finUser+1);
    if(finClave==-1) return 0; 

    clave=str.substring(finUser+1, finClave);

    if(ruta==CMD)
    {
        int finCmd=str.indexOf('=', finClave+1);
        if(finCmd==-1) return 0;

        cmd=str.substring(finClave+1, finCmd);
        param=str.substring(finCmd+1);
        Serial.println("***COMANDO***");
    }
    else
    {
        cmd=str.substring(finClave+1);
        Serial.println("***LECTURA***");
    } 

    Serial.println("Str: " + str + "$");
    Serial.println("User: " + user + "$");
    Serial.println("Clave: " + clave + "$");
    Serial.println("Cmd: " + cmd + "$");
    Serial.println("Param: " + param + "$");

    return 1;
}

String server::retornoLecturas ()
{
    String r;
    cmd.toLowerCase();
    if(cmd=="todo")
    {
        StaticJsonDocument <300> jsonRequest;

        jsonRequest["tempMax"] = data->tempMax;
        jsonRequest["tempmin"] = data->tempMin;
        jsonRequest["tempAct"] = data->temp;
        jsonRequest["humAct"] = data->hum;
        jsonRequest["tension"] = data->tension;
        jsonRequest["dhcp"] = data->dhcp;
        jsonRequest["ipDef"] = data->puerto;

        jsonRequest["mac"] = data->macString;
        jsonRequest["ipDef"] = data->ipString;
        for(int i=0; i<N; i++)jsonRequest["estTomas"][i] = data->estTomas[i];
        for(int i=0; i<N; i++)jsonRequest["corriente"][i] = data->corriente[i];

        serializeJsonPretty(jsonRequest, r);
    }
    else if(cmd=="tempmax") r= String(data->tempMax) + 'C';
    else if(cmd=="tempmin") r= String(data->tempMin) + 'C';
    else if(cmd=="temp ") r= String(data->temp) + 'C';
    else if(cmd=="hum") r= String(data->hum) + '%' ;
    else if(cmd=="tension") r= String(data->tension) + 'V' ;
    else if(cmd=="dhcp") r=(data->dhcp ? "Si" : "No");
    else if(cmd=="puerto") r= String(data->puerto);
    else if(cmd=="ipdef") r=data->ipString;
    else if(cmd=="mac") r=data->macString;
    else if(cmd=="tomas")
    {
        StaticJsonDocument <300> jsonRequest;
        for(int i=0; i<N; i++)
        jsonRequest["estTomas"][i] = data->estTomas[i];
        serializeJsonPretty(jsonRequest, r);
    } 
    else if(cmd=="corriente")
    {
        StaticJsonDocument <300> jsonRequest;
        for(int i=0; i<N; i++)
        jsonRequest["corriente"][i] = data->corriente[i];
        serializeJsonPretty(jsonRequest, r);
    } 
    else r="Peticion errornea";

    return r;
}