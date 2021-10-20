#ifndef __TOMAS_H__
#define __TOMAS_H__

//Librerías:
#include <Arduino.h>

//Librerías locales:
#include "util.h"

#ifndef N
#define N 5
#endif

class Tomacorrientes
{
    public: 
        Tomacorrientes(DATA &, PINES &);
        void conm(uint8_t, bool);
        void invertir(uint8_t);
        void begin();

    private:
        DATA *data;
        PINES *pin;
};

#endif
