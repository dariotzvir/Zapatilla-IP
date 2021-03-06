#include "headers/PantallaOLED.h"
/**
 *   Acá solo tengo comentarios más generale y es que intentes hacer más funciones para
 * que se entienda más rápido lo que intentás hacer y sea menos imperativo, por ejemplo 
 * cuando tenés que dibujar una interfaz fija hacelo en un método o función que te pida 
 * como argumentos aquellas variables que tenés que mostrar.
 *
 * Ej.: void drawTemp}float temp) {
 *          this->setCursor(3, 36);  
 *          this->print("Tem: ");
 *          this->print(temp, 1);
 *          this->print("C");
 *      {
 *
 * También usá más el puntero this para identificar que algo es un atributo o método de tu
 * clase y no del exterior.
 */


PantallaOLED::PantallaOLED(DATA &data):Adafruit_SSD1306(128, 64, &Wire, 4)
{
    this->data = &data;
}

void PantallaOLED::setup()
{
    Wire.begin(); 
    begin(SSD1306_SWITCHCAPVCC, 0x3C); 
    
    bufferDHCP = data->dhcp;
    bufferTempMax = data->tempMax;
    bufferTempMin = data->tempMin;

    //Limpio y muestro la pantalla
    clearDisplay();
    setTextColor(WHITE); 
    display(); 
}

void PantallaOLED::grillaPrin()
{
    //Dibuja todas la tabla de la pantalla principal
    drawRect(0, 0, 127, 63, WHITE);       
    drawFastHLine(0, 18, 127, WHITE);         
    drawFastHLine(0, 31, 127, WHITE);         
    drawFastHLine(0, 47, 127, WHITE);         
    drawFastVLine(21, 0, 31, WHITE);          
    drawFastVLine(42, 0, 31, WHITE);
    drawFastVLine(63, 0, 47, WHITE);          
    drawFastVLine(84, 0, 31, WHITE);
    drawFastVLine(105, 0, 31, WHITE);
}

void PantallaOLED::textoFijoPrin()
{
    setTextSize(2); 

    //Titulos de cada columna
    setCursor(6, 2);
    print('T'); 
    setCursor(27, 2);
    print('1');
    setCursor(48, 2);
    print('2');
    setCursor(69, 2);
    print('3');
    setCursor(90, 2);
    print('4');
    setCursor(111, 2);
    print('5');

    setTextSize(1);

    setCursor(3, 21);                         
    print("Amp");       

    //Muestra temperatura humedad y tension lo siguiente
}

void PantallaOLED::grillaMenu()
{
    setTextSize(1);
    drawRect(0, 0, 127, 63, WHITE);//Hace un rectángulo de 1px en el borde

    setCursor(24, 51);
    setTextColor(BLACK);
    setTextSize(1);
    fillRect(0, 47, 127, 63, WHITE);//dibuja un rectangulo blanco abajo en la pantalla
    print("-   Selec.   +");//Es una referencia para poner debajo los botones

    setTextColor(WHITE);
    setTextSize(2);
}

void PantallaOLED::pantallaPrincipal()
{
    clearDisplay();
    ssd1306_command(SSD1306_DISPLAYON);
    grillaPrin();
    textoFijoPrin();

    setTextSize(1);

    int cursor = 24;

    for(int i = 0 ; i < N ; i++)
    {
        float amp = data->corriente[i];
        setCursor(cursor, 21);

        if(data->estTomas[i] == 0)
        {
            fillRect(cursor - 2, 18, 21, 14, WHITE);  
            setTextColor(BLACK);                  
            print("Off");
            setTextColor(WHITE);   
        }
        else if(amp <= 0.1)
        {
            setCursor(cursor + 5, 21);
            print(0);
        }
        else if(amp >= 10)
        {
            setCursor(cursor + 2, 21);
            print((int) amp);
        }
        else print(amp, 1);

        cursor += 21;
    }

    setCursor(3, 36);  
    print("Tem: ");
    print(data->temp, 1);
    print("C");  

    setCursor(66, 36);    
    print("Hum: ");
    print(data->hum, 1);
    print("%");

    setCursor(19, 51);     

    char strTen[16];
    for(int i=0; i<16; i++) strTen[i] = '\0';
    strcpy(strTen, "Tension: ");

    int aux =(data->tension >= 0) ? data->tension : -data->tension;
    int num[3];
    for(int i=0; i<3; i++)
    {
        num[i] = aux/pow(10, 2-i);
        aux -= num[i]*pow(10, 2-i);
    }

    char buf[4];
    int index = 0;
    for(int i=0; i<3; i++)
    {
        if(!num[i] && !index);
        else buf[index++] = '0' + num[i];
    }
    for(int i=index; i<4; i++) buf[i] = '\0';
    if(buf[0] == '\0') buf[0] = '0';

    strcat(strTen, buf);
    strcat(strTen, "V");
    
    uint16_t w, h, *w1, *h1;
    int16_t x, y, *x1, *y1;
    x1 = &x;
    y1 = &y;
    w1 = &w;
    h1 = &h;
    getTextBounds(strTen, 19, 51, x1, y1, w1, h1);
    setCursor(64-w/2, 48+(64-48)/2-h/2);

    print(strTen);

    display();
}

void PantallaOLED::menu(IPAddress localHost, bool flagSD)
{
    clearDisplay();
    ssd1306_command(SSD1306_DISPLAYON);
    grillaMenu();

    setCursor(4, 5);
    if(flagSelec)
    {
      fillRect(0, 0, 127, 24, WHITE);
      setTextColor(BLACK);
    }
    switch(pantallaSelec)
    {
        case 2:
            print("Temp Min:");   
            setTextColor(WHITE);
            setCursor(4, 27);
            print(bufferTempMin);

            break;
        case 3:
            print("Temp Max:");   
            setTextColor(WHITE);  
            setCursor(4, 27);
            print(bufferTempMax);

            break;

        case 4:
            print("DHCP: ");
            setTextColor(WHITE); 
            setCursor(4, 27);
            print(bufferDHCP ? "Si" : "No");

            break;
            
        case 5:  
            //acá se muestra la ip que está guardada en la memoria del arduino y se usará la próxima
            print("IP actual:");  
            setTextSize(1);
            setCursor(4, 30);
            print(localHost);
            print(":");
            print(data->puerto);

            setTextSize(2);

            break;
        case 6:
            //Esta pantalla muestra la ip en la que está hosteandose ahora el server
            print("IP fija:"); 
            setTextSize(1);
            setCursor(4, 30);
            print(data->ipString);
            print(":");
            print(data->puerto);

            setTextSize(2);

            break;
            
        case 7:
            print("MAC: ");
            setTextSize(1);
            setCursor(4, 25);
            for(int i=0; i<20; i++) print(data->macString[i]);
            setCursor(4, 35);
            for(int i=20; i<30; i++) print(data->macString[i]);
            
            setTextSize(2);

            break;

        case 8:
            print("SD: ");
            setCursor(4, 27);
            print(flagSD ? "Error" : "OK");
            
            break;
            
    }

    display();
}

void PantallaOLED::pantallaReset()
{
    /*
        Dibuja la pantalla de reseteo en general pinta la pantalla en blaco, dibuja un rectangulo
    de 2px de grosor, y muestra "Reset" porque no entraba nada en español
    */
    ssd1306_command(SSD1306_DISPLAYON);
    clearDisplay();

    fillRect(0, 0, 127, 63, WHITE);
    drawRect(2, 2, 123, 59, BLACK);
    drawRect(3, 3, 121, 57, BLACK);

    setTextColor(BLACK);
    setTextSize(3);
    
    /*
    *   Codigo auxiliar para centrar el string:
    *   uint16_t x, y, w, h;
    *   getTextBounds("RESET", 0, 0, &x, &y, &w, &h);
    *   Serial.print("w: ");
    *   Serial.println(w);
    *   Serial.print("h: ");
    *   Serial.println(h);
    *   setCursor(64-w/2, 32-h/2);
    */
    const int xCur = 19;
    const int yCur = 20;
    setCursor(xCur, yCur);
    print("RESET");
    
    display();
}

void PantallaOLED::pantallaBoot()
{
    /*
        Dibuja la pantalla de reseteo en general pinta la pantalla en blaco, dibuja un rectangulo
    de 2px de grosor, y muestra "Reset" porque no entraba nada en español
    */
    ssd1306_command(SSD1306_DISPLAYON);
    clearDisplay();

    fillRect(0, 0, 127, 63, WHITE);
    drawRect(2, 2, 123, 59, BLACK);
    drawRect(3, 3, 121, 57, BLACK);

    setTextColor(BLACK);
    setTextSize(3); 
    
    const int xCur = 28;
    const int yCur = 20;
    setCursor(xCur, yCur);
    print("BOOT");
    /*
    *   El string "BOOT" en tamaño 3 tiene 72px X 24px
    *   Para centrar es(ANCHOPANTALLA - ANCHOSTRING)/2 para x
    *   Para centrar es(LARGAPANTALLA - LARGOSTRING)/2 para y
    * 
    *   La pantalla es 128px X 64px
    * 
    *   uint16_t x, y, w, h;
    *   getTextBounds("BOOT", 0, 0, &x, &y, &w, &h);
    *   Serial.print("w: ");
    *   Serial.println(w);
    *   Serial.print("h: ");
    *   Serial.println(h);
    * 
    *   setCursor(64-w/2, 32-h/2);
    */

    display();

    setTextColor(WHITE);
}

void PantallaOLED::pantallaApagada()
{
    clearDisplay();
    ssd1306_command(SSD1306_DISPLAYOFF);
}

int PantallaOLED::logicaEnter()
{
    int retorno = 0;
    if(pantallaSelec < TMIN) pantallaSelec = 2; //Si está en una pantalla que no sea de menú cambia a una que lo es
    else if(pantallaSelec == TMIN || pantallaSelec == TMAX || pantallaSelec == DHCP) //Pantallas con varaiables a modificar
    {
        if(flagSelec) //Si ya se estaba modificando una variable se guarda el valor deseado desde la variable buffer a la original y se guarda en la SD
        {
            if(pantallaSelec == TMIN && data->tempMin != bufferTempMin) 
            {
                data->tempMin = bufferTempMin;
                retorno = 2;
            }
            else if(pantallaSelec == TMAX && data->tempMax != bufferTempMax)
            {
                retorno = 2;
                data->tempMax = bufferTempMax;
            } 
            else if(pantallaSelec == DHCP && data->dhcp != bufferDHCP) 
            {
                retorno = 3;
                data->dhcp = bufferDHCP;
            }
            else retorno = 1;
        }
        else //Si no se estaba modificando una variable se reinician las variables para actualizarlas
        {
            bufferTempMax = data->tempMax; //Reinicia las variables de buffer
            bufferTempMin = data->tempMin;
            bufferDHCP = data->dhcp;
        }
        flagSelec = !flagSelec; 
    }

    return retorno;
}

void PantallaOLED::logicaDer()
{
    if(pantallaSelec == PRINCIPAL || pantallaSelec == APAGADA) pantallaSelec = TMIN; //Si está en una pantalla que no sea de menú cambia a una que lo es
    if(flagSelec) //Si se está modificando una variable se aumenta la misma hasta su límite
    {
        if(pantallaSelec == TMIN && bufferTempMin < bufferTempMax) bufferTempMin++;
        if(pantallaSelec == TMAX && bufferTempMax < TEMP_MAX) bufferTempMax++;
        if(pantallaSelec == DHCP) bufferDHCP = !bufferDHCP;
    }
    else 
    {
        if(pantallaSelec < ESTSD) pantallaSelec++; //Si no se está modificando una variable se va a la siguiente o a la primera si se estaba en la última
        else pantallaSelec = TMIN;
    }
    
}

void PantallaOLED::logicaIzq()
{
    if(pantallaSelec == PRINCIPAL || pantallaSelec == APAGADA) pantallaSelec = TMIN; //Si está en una pantalla que no sea de menú cambia a una que lo es
    if(flagSelec) //Si se está modificando una variable se disminuye la misma hasta su límite
    {
        if(pantallaSelec == TMIN && bufferTempMin > TEMP_MIN) bufferTempMin--;
        if(pantallaSelec == TMAX && bufferTempMax > bufferTempMin) bufferTempMax--;
        if(pantallaSelec == DHCP) bufferDHCP = !bufferDHCP;
    }
    else
    {
        if(pantallaSelec > TMIN) pantallaSelec--;//Si no se está modificando una variable se va a la anterior o a la útlima si se estaba en la primera
        else pantallaSelec = ESTSD;
    }
    
}

void PantallaOLED::logicaOnOff()
{
    if(pantallaSelec != PRINCIPAL) pantallaSelec = PRINCIPAL; //Si está en una pantalla de menú o apagada lleva a la pantalla principal
    else if(pantallaSelec == PRINCIPAL) pantallaSelec = APAGADA; //Si está en la pantalla principal la apaga
    bufferTempMax = data->tempMax;
    bufferTempMin = data->tempMin;
    bufferDHCP = data->dhcp;
    flagSelec = 0;
}

void PantallaOLED::resetBuf()
{
    bufferTempMax = data->tempMax;
    bufferTempMin = data->tempMin;
    bufferDHCP = data->dhcp;
}