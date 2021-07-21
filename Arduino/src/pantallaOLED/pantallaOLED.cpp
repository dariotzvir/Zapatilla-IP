#include "pantallaOLED.h"

/**
 *   Acá solo tengo comentarios más generale y es que intentes hacer más funciones para
 * que se entienda más rápido lo que intentás hacer y sea menos imperativo, por ejemplo 
 * cuando tenés que dibujar una interfaz fija hacelo en un método o función que te pida 
 * como argumentos aquellas variables que tenés que mostrar.
 *
 * Ej.: void drawTemp (float temp) {
 *          this->setCursor(3, 36);  
 *          this->print("Tem: ");
 *          this->print(temp, 1);
 *          this->print("C");
 *      {
 *
 * También usá más el puntero this para identificar que algo es un atributo o método de tu
 * clase y no del exterior.
 */


pantallaOLED::pantallaOLED ( float*corriente, bool *estTomas ):Adafruit_SSD1306 ( 128 , 64, &Wire, 4 )
{
    this->corriente = corriente;
    this->estTomas = estTomas;
}

void pantallaOLED::setup ()
{
    Wire.begin (); 
    begin( SSD1306_SWITCHCAPVCC, 0x3C ); 

    //Limpio y muestro la pantalla
    clearDisplay ();
    setTextColor(WHITE); 
    display (); 
}

void pantallaOLED::grillaPrin ()
{
    //Dibuja todas la tabla de la pantalla principal
    drawRect ( 0 , 0 , 127 , 63 , WHITE );       
    drawFastHLine ( 0, 18, 127, WHITE );         
    drawFastHLine ( 0, 31, 127, WHITE );         
    drawFastHLine ( 0, 47, 127, WHITE );         
    drawFastVLine ( 21, 0, 31, WHITE );          
    drawFastVLine ( 42, 0, 31, WHITE );
    drawFastVLine ( 63, 0, 47, WHITE );          
    drawFastVLine ( 84, 0, 31, WHITE );
    drawFastVLine ( 105, 0, 31, WHITE );
}

void pantallaOLED::textoFijoPrin ()
{
    setTextSize ( 2 ); 

    //Titulos de cada columna
    setCursor ( 6, 2 );
    print ( 'T' ); 
    setCursor ( 27, 2 );
    print ( '1' );
    setCursor ( 48, 2 );
    print ( '2' );
    setCursor ( 69, 2 );
    print ( '3' );
    setCursor ( 90, 2 );
    print ( '4' );
    setCursor ( 111, 2 );
    print ( '5' );

    setTextSize ( 1 );

    setCursor ( 3, 21 );                         
    print ( "Amp" );       

    //Muestra temperatura humedad y tension lo siguiente
}

void pantallaOLED::grillaMenu ()
{
    setTextSize ( 1 );
    drawRect ( 0 , 0 , 127 , 63 , WHITE );//Hace un rectángulo de 1px en el borde

    setCursor ( 24, 51 );
    setTextColor ( BLACK );
    setTextSize ( 1 );
    fillRect ( 0 , 47 , 127 , 63 , WHITE );//dibuja un rectangulo blanco abajo en la pantalla
    print ( "-   Selec.   +" );//Es una referencia para poner debajo los botones

    setTextColor ( WHITE );
    setTextSize ( 2 );
}

void pantallaOLED::pantallaPrincipal ( float &temp, float &hum, float &tension )
{
    clearDisplay ();
    ssd1306_command ( SSD1306_DISPLAYON );
    grillaPrin ();
    textoFijoPrin ();

    setTextSize ( 1 );

    int cursor = 24;

    for ( int i = 0 ; i < N ; i++ )
    {
        float amp = *(corriente + i);
        setCursor ( cursor, 21 );

        if ( *(estTomas + i) == 0 )
        {
            fillRect ( cursor - 2 , 18 , 21 , 14 , WHITE );  
            setTextColor ( BLACK );                  
            print ( "Off" );
            setTextColor ( WHITE );   
        }
        else if ( amp <= 0.1 )
        {
            setCursor ( cursor + 5 , 21 );
            print ( 0 );
        }
        else if ( amp >= 10 )
        {
            setCursor ( cursor + 2, 21 );
            print ( (int) amp );
        }
        else print ( amp, 1 );

        cursor += 21;
    }

    setCursor ( 3, 36 );  
    print ( "Tem: " );
    print ( temp, 1 );
    print ( "C" );  

    setCursor ( 66, 36 );    
    print ( "Hum: " );
    print ( hum, 1 );
    print ( "%" );

    setCursor ( 14, 51 );                        
    print ( "Tension: " );
    print ( tension, 2 );                      
    print ( "V" );

    display ();
}

void pantallaOLED::menu ( int &pantalla, bool &selec, int &tempM, int &tempm, IPAddress &ipFija, IPAddress localHost, bool &flagErrorSd, bool &flagDhcp )
{
    clearDisplay ();
    ssd1306_command ( SSD1306_DISPLAYON );
    grillaMenu ();

    setCursor ( 4, 5 );
    if ( selec )
    {
      fillRect ( 0, 0, 127, 24, WHITE );
      setTextColor ( BLACK );
    }
    switch ( pantalla )
    {
        case 2:
            print ( "Temp Min:" );   
            setTextColor ( WHITE );
            setCursor ( 4 , 28 );
            print ( tempm );

            break;
        case 3:
            print ( "Temp Max:" );   
            setTextColor ( WHITE );  
            setCursor ( 4 , 28 );
            print ( tempM );

            break;

        case 4:
            print ( "DHCP: " );
            setTextColor ( WHITE ); 
            setCursor ( 4 , 27 );
            print ( flagDhcp ? "Si" : "No" );

            break;
            
        case 5:  
            //acá se muestra la ip que está guardada en la memoria del arduino y se usará la próxima
            print ( "IP actual:" );  
            setTextSize ( 1 );
            setCursor ( 4 , 30 );
            print ( localHost );

            setTextSize ( 2 );

            break;
        case 6:
            //Esta pantalla muestra la ip en la que está hosteandose ahora el server
            print ( "IP fija:" ); 
            setTextSize ( 1 );
            setCursor ( 4 , 30 );
            print ( ipFija );

            setTextSize ( 2 );

            break;
            
        case 7:
            print ( "SD: " );
            setCursor ( 4 , 27 );
            print ( flagErrorSd ? "Error" : "OK" );

            break;
            
    }

    display ();
}

void pantallaOLED::pantallaReset ()
{
    /*
        Dibuja la pantalla de reseteo en general pinta la pantalla en blaco, dibuja un rectangulo
    de 2px de grosor, y muestra "Reset" porque no entraba nada en español
    */
    ssd1306_command (SSD1306_DISPLAYON);
    clearDisplay ();

    fillRect ( 0 , 0 , 127 , 63 , WHITE );
    drawRect ( 2 , 2 , 123 , 59 , BLACK );
    drawRect ( 3 , 3 , 121 , 57 , BLACK );

    setTextColor ( BLACK );
    setTextSize ( 2 );

    setCursor ( 13 ,22 );

    print ( "Reset" );

    display ();
}

void pantallaOLED::pantallaBoot ()
{
    /*
        Dibuja la pantalla de reseteo en general pinta la pantalla en blaco, dibuja un rectangulo
    de 2px de grosor, y muestra "Reset" porque no entraba nada en español
    */
    ssd1306_command (SSD1306_DISPLAYON);
    clearDisplay ();

    fillRect ( 0 , 0 , 127 , 63 , WHITE );
    drawRect ( 2 , 2 , 123 , 59 , BLACK );
    drawRect ( 3 , 3 , 121 , 57 , BLACK );

    setTextColor ( BLACK );
    setTextSize ( 2 );

    setCursor ( 13 ,22 );

    print ( "Boot" );

    display ();

    setTextColor ( WHITE );
}

void pantallaOLED::pantallaApagada ()
{
    clearDisplay ();
    ssd1306_command (SSD1306_DISPLAYOFF);
}
