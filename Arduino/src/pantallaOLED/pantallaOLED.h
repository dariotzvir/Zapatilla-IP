#ifndef __PANTALLAOLED_H__
#define __PANTALLAOLED_H__

#include "src/Adafruit_SSD1306/Adafruit_SSD1306.h"
#include <IPAddress.h>
#include <Wire.h>

#ifndef N
#define N 5
#endif

class pantallaOLED: public Adafruit_SSD1306
{
    public:
        pantallaOLED ( float*corriente, bool *estTomas );
        void pantallaPrincipal ( float &temp, float &hum, float &tension );
        void pantallaApagada ();
        void menu ( int &, bool &, int &, int &, IPAddress &, IPAddress, bool &, bool & );
       // void menu ( int &pantalla, bool &selec, int &tempM, int &tempm, IPAddress &ipFija, IPAddress localHost, bool &flagErrorSd );
        void pantallaReset ();
        void setup ();
        void pantallaBoot ();
    
    private:
        void grillaPrin ();
        void textoFijoPrin ();
        void grillaMenu ();
        float *corriente;
        bool *estTomas;
};


#endif