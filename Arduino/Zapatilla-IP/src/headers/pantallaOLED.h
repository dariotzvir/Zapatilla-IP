#ifndef __PANTALLAOLED_H__
#define __PANTALLAOLED_H__

//#include "src/Adafruit_SSD1306/Adafruit_SSD1306.h"
#include <IPAddress.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "util.h"
#include <string.h>

#ifndef N
#define N 5
#endif

class pantallaOLED: public Adafruit_SSD1306
{
    public:
        pantallaOLED ( DATA & );
        void setup ();

        void pantallaApagada ();
        void pantallaPrincipal ();
        void menu ( IPAddress, bool );
        void pantallaReset ();
        void pantallaBoot ();
        void pantallaMAC ();

        void resetBuf ();

        void logicaDer ();
        void logicaIzq ();
        void logicaOnOff ();
        int logicaEnter ();


        int pantallaSelec = 0;
        bool flagSelec = 0;

        int bufferTempMax = 0;
        int bufferTempMin = 0;
        bool bufferDHCP = 1;
    
    private:
        void grillaPrin ();
        void textoFijoPrin ();
        void grillaMenu ();

        DATA *data;
};


#endif