#include "headers/server.h"

#include <SD.h>
#include <utility/w5100.h>

#define isBool(c)(c=='0' || c=='1')
#define isInteger(c)(c >= '0' && c <= '9') 
#define isAlfaNum(c)((c >= '0' && c <= '9') ||(c >= 'A' && c <= 'Z') ||(c >= 'a' && c <= 'z'))
#define isHexadecimalDigit(c)((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
#define fromCharToInt(c)(c-48)

EthernetClient cmdCliente;

server::server(DATA &data): EthernetServer(data.puerto)
{
    this->data = &data;
}

void server::setup()
{
    Ethernet.begin(data->mac, data->ipDef);

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
}

int8_t server::rutina()
{
    int estadoPeticion = -2; //Si retorna -2 es porque no hubo un log de un usuario
    
    //if(data->dhcp) checkDHCP();

    cmdCliente.flush();
    cmdCliente=available();

    if(cmdCliente)
    {
        estadoPeticion=-1; //Si retorna -1 es porque se conectaron pero no tiraron ningun comando
        req="";
        message="";
        errorParse=0;
        errorCmd=0;
        errorLogin=0;

        #ifdef DEBUGPET
        Serial.print("\nConectado\n");
        #endif

        lectura();  
        conversion();
        if(ruta!=ERROR && ruta!=HOME) checkLogin();
        if(ruta==CMD && !errorParse) 
        {
            estadoPeticion=ejecutarCmd();//Si retorna entre 1 y 10 se tirò un comando y si retorna 0 era un comando erroneo
            errorCmd=(estadoPeticion==-1 ? 1 : 0);
        }
        devolucion();

        delay(1);
        cmdCliente.stop();
    }

    //if(retornoRutina > 0)(*guardarSD)();
    return estadoPeticion;   
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

        #ifdef DEBUGPET
        Serial.print("Request: ");
        Serial.println(req);
        #endif

        if(req[0]!='G')
        {
            while(c!=-1)
            {
                c=cmdCliente.read();
                resto.concat(c);
            }
            int index=resto.indexOf("\r\n\r\n");
            message=resto.substring(index+4,-1);

            #ifdef DEBUGPET
            Serial.print("Message: $");
            Serial.println(message);
            #endif

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
        if(fin!=-1) ruteStr.remove(fin, ruteStr.length());
        else ruta=ERROR;
    }

    #ifdef DEBUGPET
    Serial.print("Ruta: $");
    Serial.print(ruteStr);
    Serial.println("$");
    #endif

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

    #ifdef DEBUGPET
    Serial.print("Ruta: $");
    Serial.print(ruta);
    Serial.println("$");
    #endif

}
void server::checkLogin()
{
    if(clave!=data->clave) errorLogin=1;
    if(user!=data->usuario) errorLogin=1;
}
int8_t server::ejecutarCmd()
{
    cmd.toLowerCase();
    int i=0;

    errorParam=0;
    
    for(i=0; i<10; i++) 
        if(cmd==str[i]) 
        {
            errorParam=!(this->*fun [i])();
            break;
        }
    return ( !errorParam && i!=10 ? i : -1);
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
    else if(errorCmd) cmdCliente.println("Comando erroneo");
    else if(errorParam) cmdCliente.println("Formato de parametro incorrecto");

    else if(ruta==LEC) cmdCliente.println(retornoLecturas());
    else if(ruta==CMD) cmdCliente.println("Comando ejecutado");

    errorLogin=0;
    errorParam=0;
    errorParse=0;
    errorCmd=0;
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

        #ifdef DEBUGPET
        Serial.println("***COMANDO***");
        #endif
    }
    else
    {
        cmd=str.substring(finClave+1);

        #ifdef DEBUGPET
        Serial.println("***LECTURA***");
        #endif
    } 

    #ifdef DEBUGPET
    Serial.println("Str: $" + str + "$");
    Serial.println("User: $" + user + "$");
    Serial.println("Clave: $" + clave + "$");
    Serial.println("Cmd: $" + cmd + "$");
    Serial.println("Param: $" + param + "$");
    #endif

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
    else r="Peticion erronea";

    return r;
}
bool server::cambioMac()
{
    int n=param.length();
    int contEsp=0; //Contador de espacios de losdiferentes campos
    byte macAux[6], buffer=0;
    for(auto i : macAux) i=0;

    #ifdef DEBUGPET
    Serial.print("Buffer MAC decode: ");
    #endif

    for(int i=0; i<n && contEsp<6; i++)
    {
        char c=toupper(param[i]);

        if(!isHexadecimalDigit(c) && c!='+') return 0;
        if(c=='+')
        {
            #ifdef DEBUGPET
            Serial.print('$');
            Serial.print(buffer);
            Serial.print('$');
            #endif

            macAux[contEsp++]=buffer;
            buffer=0;
        }
        if(c>='0' && c<='9') buffer=buffer*16 + c-'0';
        else if(c>='A' && c<='F') buffer=buffer*16 + 10 + c-'A';
    }

    #ifdef DEBUGPET
    Serial.println();
    #endif

    macAux[contEsp++]=buffer;
    if(contEsp!=6) return 0;

    for(int i=0; i<6; i++) data->mac[i]=macAux[i];
    data->actMacString();

    #ifdef DEBUGPET
    Serial.print ("Mac decode: ");
    for(int i=0; i<6; i++) 
    {
        Serial.print('$');
        Serial.print(macAux[i]);
        Serial.print('$');
    }
    Serial.println();
    Serial.println(data->macString);
    Serial.println();
    #endif

    return 1;
}
bool server::cambioTempMax() {return cambioTemp (1);}
bool server::cambioTempMin() {return cambioTemp (0);}
bool server::cambioTemp(bool flag)
{
    int n=param.length();
    bool negative=0;
    if(param[0] == '-') 
    {
        negative=1;
        param = param.substring (1);
        Serial.println (param);
    }
    for(int i=0; i<n; i++) if(!isInteger(param[i])) return 0;

    int aux=param.toInt();
    if(negative) aux+=-1;

    #ifdef DEBUGPET
    Serial.print("Temp decode: $");
    Serial.print(aux);
    Serial.println('$');    
    #endif

    if (flag)
    {
        if(aux<=125 && aux>=data->tempMin) data->tempMax=aux;
        return 0;
    }
    else
    {
        if(aux<=-40 && aux<=data->tempMax) data->tempMin=aux;
        return 0;
    }

    return 1;
}
bool server::cambioTomas()
{
    if(param.length()!=3 && !isInteger(param[0]) && param[1]=='+' && !isBool(param [2])) return 0;
    
    int toma=fromCharToInt(param[0])-1;
    int accion=fromCharToInt(param[2]);

    #ifdef DEBUGPET
    Serial.println("Toma decode: $" + String(toma) + "$");
    Serial.println("Accion decode: $" + String(accion) + "$");
    #endif

    if( !(accion==1 || accion==0) || toma>4 || toma<0) return 0;
    else data->estTomas[toma]=accion;
    return 1;
}
bool server::cambioIp()
{
    IPAddress aux;
    if(!aux.fromString(param)) return 0;

    #ifdef DEBUGPET
    Serial.println("IP decode: $" + String(aux[0]) + "." + String(aux[1]) + "." + String(aux[2]) + "." + String(aux[3]) + "$");
    #endif

    data->ipDef=aux;
    data->actIpString();

    if (!data->dhcp);
    return 1;
}
bool server::cambioDhcp()
{
    int n=param.length();
    if(n!=1) return 0;
    for(int i=0; i<n; i++) if(!isBool(param[i])) return 0;

    #ifdef DEBUGPET
    Serial.println("DHCPdecode: $" + param + "$");
    #endif

    data->dhcp=param.toInt ();
    return 1;
}
bool server::cambioPuerto()
{
    int n=param.length();
    for(int i=0; i<n; i++) if(!isInteger(param[i])) return 0;

    #ifdef DEBUGPET
    Serial.println("Puerto decode: $" + param + "$");
    #endif

    data->puerto=param.toInt ();
    return 1;
}
bool server::cambioClave()
{
    int n=param.length();
    if(n<5 || n>15) return 0;
    for(char c : param) if(!isAlfaNum(c)) return 0;

    #ifdef DEBUGPET
    Serial.println("Clave decode: $" + param + "$");
    #endif 

    bufferClave=param;    
    return 1;
}
bool server::cambioUser()
{
    int n=param.length();
    if(n<5 || n>15) return 0;
    for(char c : param) if(!isAlfaNum(c)) return 0;

    #ifdef DEBUGPET
    Serial.println("Usuario decode: $" + param + "$");
    #endif

    bufferUser=param;    
    return 1;
}
bool server::verificarCambio()
{
    int finUser=param.indexOf('+');
    if(finUser==-1) return 0;

    String auxUser=param.substring(0, finUser);
    String auxClave=param.substring(finUser+1, param.length());

    #ifdef DEBUGPET
    Serial.println("AuxUser: $" + auxUser + "$");
    Serial.println("AuxClave: $" + auxClave + "$");
    #endif

    int n=auxUser.length();
    if(n<5 || n>15) return 0;
    for(char c : auxUser) if(!isAlfaNum(c)) return 0;

    n=auxClave.length();
    if(n<5 || n>15) return 0;
    for(char c : auxClave) if(!isAlfaNum(c)) return 0;

    if(bufferUser=="")bufferUser=data->usuario;
    if(bufferClave=="")bufferClave=data->clave;

    if(bufferUser==auxUser && bufferClave==auxClave)
    {
        data->usuario=bufferUser; 
        data->clave=bufferClave;
        bufferUser="";
        bufferClave="";
    }
    else return 0;
    return 1;
}