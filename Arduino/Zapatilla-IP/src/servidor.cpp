#include "headers/servidor.h"

#include <SD.h>
#include <utility/w5100.h>

#define isBool(c)(c=='0' || c=='1')
#define isInteger(c)(c >= '0' && c <= '9') 
#define isAlfaNum(c)((c >= '0' && c <= '9') ||(c >= 'A' && c <= 'Z') ||(c >= 'a' && c <= 'z'))
#define isHexadecimalDigit(c)((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
#define fromCharToInt(c)(c-48)
#define round(c)((int)(100*c)/100.0)

EthernetClient cmdCliente;

Servidor::Servidor(DATA &data): EthernetServer(data.puerto)
{
    this->data = &data;
}

void Servidor::setup()
{
    Ethernet.begin(data->mac, data->ipDef);

    load();

    begin();
}

void Servidor::load()
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

int8_t Servidor::rutina()
{
    int estadoPeticion = -2; //Si retorna -2 es porque no hubo un log de un usuario
    
    if(data->dhcp) checkDHCP();

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

        tipoPet=lectura();  
        if(tipoPet!=NOPET)
        {
            ruta=parsePet();

            if(ruta!=ERROR) errorLogin=!checkLogin();
            if(ruta==CMD && !errorLogin) 
            {
                int8_t retorno=ejecutarCmd();
                if(retorno != -1 && !errorParam) estadoPeticion=retorno;
            }

            devolucion();
        }
        delay(1);
        cmdCliente.stop();
    }

    //if(retornoRutina > 0)(*guardarSD)();
    return estadoPeticion;   
}
int8_t Servidor::lectura()
{
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

            return POST;
        }
        else return GET;
    }
    return NOPET;
}
int8_t Servidor::parsePet()
{      
    int ini=req.indexOf('/');
    int fin=req.indexOf(' ', ini+1);

    //GET /cmd     POST /cmd
    //GET /lec     POST /lec
    //GET / HT     POST / HT
    String ruteStr=req.substring(ini, fin);
    ruteStr.toLowerCase();

    #ifdef DEBUGPET
    Serial.print("Ruta: $");
    Serial.print(ruteStr);
    Serial.println("$");
    #endif

    if(ruteStr.length()==1 && ruteStr[0]=='/') return HOME;
    if(tipoPet==GET) 
    {
        int finGET=ruteStr.indexOf('?');
        #ifdef DEBUGPET
        Serial.print("finGET: $");
        Serial.print(finGET);
        Serial.println("$");
        #endif
        if(finGET!=-1) ruteStr.remove(finGET);
        else return ERROR;
    }

    #ifdef DEBUGPET
    Serial.print("Ruta: $");
    Serial.print(ruteStr);
    Serial.println("$");
    #endif

    if(ruteStr=="/cmd") ruta=CMD;
    else if(ruteStr=="/lec") ruta=LEC;
    if(ruta==CMD || ruta==LEC)
    {
        if(tipoPet==GET) errorParse=!parseGET();
        if(tipoPet==POST) errorParse=!parsePOST();
    }

    #ifdef DEBUGPET
    Serial.print("Ruta: $");
    Serial.print(ruta);
    Serial.println("$");
    #endif

    return ruta;
}
bool Servidor::checkLogin()
{
    if(clave!=data->clave) return 0;
    if(user!=data->usuario) return 0;
    return 1;
}
int8_t Servidor::ejecutarCmd()
{
    cmd.toLowerCase();
    int8_t i=0, index=-1;

    errorParam=1;
    
    for(i=0; i<M; i++) 
        if(cmd==str[i]) 
        {
            errorParam=!(this->*fun [i])();
            index=i;
            break;
        }

    #ifdef DEBUGPET
    Serial.print("index cmd: $");
    Serial.print(index);
    Serial.println('$');    
    #endif

    return index;
}
void Servidor::devolucion()
{
    cmdCliente.println("HTTP/1.1 200 OK");
    cmdCliente.println("Content-Type: text/plain");
    cmdCliente.println("Connection: close");
    cmdCliente.println(); 
    
    if(ruta==HOME) cmdCliente.println("Zapatilla IP OK");
    else if(ruta==ERROR) cmdCliente.println("Ruta incorrecta");

    else if(errorLogin) cmdCliente.println("Login incorrecto");
    else if(errorCmd) cmdCliente.println("Comando erroneo");

    else if(errorParse) cmdCliente.println("Formato de peticion incorrecto");
    else if(errorParam) cmdCliente.println("Formato de parametro incorrecto");

    else if(ruta==LEC) cmdCliente.println(retornoLecturas());
    else if(ruta==CMD) cmdCliente.println("Comando ejecutado");

    errorLogin=0;
    errorParam=0;
    errorParse=0;
    errorCmd=0;
}
bool Servidor::parseGET()
{
    int ini=req.indexOf('?');
    int fin=req.indexOf(' ', ini);

    String buf=req.substring(ini+1, fin);

    return (parseStr (buf));
}
bool Servidor::parsePOST()
{
    message.remove(message.length()-1);

    return (parseStr (message));
}
bool Servidor::parseStr(String str)
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
        int finCmd=str.indexOf('=', finClave+1); //Fuerza a que todos los comandos lleven un parametero
        if(finCmd==-1) return 0;                 //Molesta luego para los metodos de poner en cero

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
String Servidor::retornoLecturas ()
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
    else if(cmd=="temp") r= String(data->temp) + 'C';
    else if(cmd=="hum") r= String(data->hum) + '%' ;
    else if(cmd=="tension") r= String(int(data->tension)) + 'V' ;
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
        jsonRequest["corriente"][i] = (data->corriente[i] >= 0.1 ? data->corriente[i] : 0);
        serializeJsonPretty(jsonRequest, r);
    } 

    /**
     * Estos comandos son para usar con PRTG y los sensores de HTTP y JSON, formatea
     * distintos JSON con tags propios de PRTG para detectar correctamente las variables:
     * 
     * El JSON primero tine un campo "prtg" dentro de el un vector "result" con cada valor.
     * 
     * {
     *      "prtg"{
     *          "result"[
     *              {
     *                  "channel": "Corriente 1",
     *                  "value": "0.00",
     *                  "float": "1",
     *                  "unit": "custom",
     *                  "customunit": "A"
     *              },
     *              {
     *                  "channel": "Corriente 1",
     *                  "value": "0.00",
     *                  "float": "1",
     *                  "unit": "custom",
     *                  "customunit": "A"
     *              }
     *          ]
     *      }
     * }
     * 
     * "channel" es para el nombre de la variable.
     * "value" valor de la variable.
     * "unit" indica cual unidad es o si es una variable customizada por el usuario. (Se le pone "custom" para que decir que es personalizada).
     * "customunit" pone el nombre personalizado de la unidad de la varible si se lo indio en unit.
     * "float" flag que indica si la variable es float, PRTG soporta ints y floats.
     * 
     * No se ponen todas las variables juntas en un solo JSON ya que con todos los tags el archivo se hace muy largo para la RAM del micro
     * por lo que los agrupé en tres comandos más cortos, también hay problemas con la memoria si el código dentro de los if se los lleva a una función,
     * en ese caso la memoria se llena antes.
     */ 
    else if(cmd == "prtgtomas") 
    {
        StaticJsonDocument <400> jsonRequest;
        JsonObject prtg = jsonRequest.createNestedObject("prtg"); //Crea la variable "prtg"
        JsonArray vec = prtg.createNestedArray("result");//Crea el vector "prtg"
        String canales[5]= { "Toma 1", "Toma 2", "Toma 3", "Toma 4", "Toma 5"}; //Lista de variables

        for(int i=0; i<5; i++)
        {
            //Añade a cada objeto el campo
            JsonObject elemento = vec.createNestedObject();
            elemento["channel"] = canales[i];
            //Lo transforma primero en int ya que sinó al castear el bool como String lo pone como true o false
            elemento["value"] = (String)((int)data->estTomas[i]); 
            elemento["unit"] = "custom";
            elemento["customunit"] = "";
        }
        serializeJsonPretty(jsonRequest, r);
    }
    else if(cmd == "prtgenergia") 
    {
        StaticJsonDocument <400> jsonRequest;
        JsonObject prtg = jsonRequest.createNestedObject("prtg");
        JsonArray vec = prtg.createNestedArray("result");
        String canales[6]= {"Corriente 1", "Corriente 2", "Corriente 3", "Corriente 4", "Corriente 5", "Tension"};

        for(int i=0; i<5; i++)
        {
            JsonObject elemento = vec.createNestedObject();
            elemento["channel"] = canales[i];
            //Redondea a dos decimales y luego castea a String
            elemento["value"] = (String)(round(data->corriente[i]));
            elemento["float"] = "1";
            elemento["unit"] = "custom";
            elemento["customunit"] = "A";
        }
        JsonObject elemento = vec.createNestedObject();
        elemento["channel"] = canales[5];
        elemento["value"] = (String)(round(data->tension));
        elemento["float"] = "1";
        elemento["unit"] = "custom";
        elemento["customunit"] = "V";

        serializeJsonPretty(jsonRequest, r);
    }
    else if(cmd == "prtgtemp")
    {
        StaticJsonDocument <400> jsonRequest;
        JsonObject prtg = jsonRequest.createNestedObject("prtg");
        JsonArray vec = prtg.createNestedArray("result");

        JsonObject temp = vec.createNestedObject();
        temp["channel"] = "Temperatura";
        temp["value"] = (String)(round(data->temp));
        temp["float"] = "1";
        temp["unit"] = "custom";
        temp["customunit"] = "Celsius";

        JsonObject hum = vec.createNestedObject();
        hum["channel"] = "Humedad";
        hum["value"] = (String)(round(data->hum));
        hum["float"] = "1";
        hum["unit"] = "custom";
        hum["customunit"] = "%";

        serializeJsonPretty(jsonRequest, r);
    }
    else r="Peticion erronea";

    return r;
}
bool Servidor::cambioMac()
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

        if(!isHexadecimalDigit(c) && c!=':') return 0;
        if(c==':')
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
bool Servidor::cambioTempMax() {return cambioTemp (1);}
bool Servidor::cambioTempMin() {return cambioTemp (0);}
bool Servidor::cambioTemp(bool flag)
{
    int n=param.length();
    bool negative=0;
    if(param[0] == '-') 
    {
        negative=1;
        param = param.substring (1);
        n--;
    }
    else for(int i=0; i<n; i++) if(!isInteger(param[i])) return 0;

    int aux=param.toInt();
    if(negative) aux*=-1;

    #ifdef DEBUGPET
    Serial.print("Temp decode: $");
    Serial.print(aux);
    Serial.println('$');    
    #endif

    if (flag)
    {
        if(aux<=TEMP_MAX && aux>=data->tempMin) 
        {
            data->tempMax=aux;
            return 1;
        }
        else return 0;
    }
    else
    {
        if(aux>=TEMP_MIN && aux<=data->tempMax) 
        {
            data->tempMin=aux;
            return 1;
        }
        return 0;
    }
    return 0;
}
bool Servidor::cambioTomas()
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
bool Servidor::cambioIp()
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
bool Servidor::cambioDhcp()
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
bool Servidor::cambioPuerto()
{
    int n=param.length();
    for(int i=0; i<n; i++) if(!isInteger(param[i])) return 0;

    #ifdef DEBUGPET
    Serial.println("Puerto decode: $" + param + "$");
    #endif

    data->puerto=param.toInt ();
    return 1;
}
bool Servidor::cambioClave()
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
bool Servidor::cambioUser()
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
bool Servidor::verificarCambio()
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
bool Servidor::cambioCteZTMP()
{
    float vAct = param.toFloat();
    #ifdef DEBUGPET
    Serial.println("vAct: " + String (vAct));
    #endif

    if(vAct <= 0) return 0;
    else
    {
        float sigma = data->tension/data->factorZMPT;
        data->factorZMPT = vAct/sigma;

        #ifdef DEBUGPET
        Serial.println("factorZMPT: " + String (data->factorZMPT));
        #endif
    }
    
    return 1;
}
bool Servidor::cambioCteACS()
{
    float iAct = param.toFloat();
    #ifdef DEBUGPET
    Serial.println("iAct: " + String (iAct));
    #endif

    if(iAct <= 0) return 0;
    else
    {
        float sigma = data->corriente[0]/data->factorACS;
        data->factorACS = iAct/sigma;
        Serial.print("Sigma: ");
        Serial.print(sigma);
        Serial.print("factor: ");
        Serial.print(data->factorACS);

        #ifdef DEBUGPET
        Serial.println("factorACS: " + String (data->factorACS));
        #endif    
    }
    
    return 1;
}
void Servidor::checkDHCP ()
{
    /*
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
    if (millis ()-millisDHCP>=PERIODODHCP) 
    {
        millisDHCP=millis ();
        int retorno=Ethernet.maintain(); 
        if (retorno==1 || retorno==3) load ();
    }
}