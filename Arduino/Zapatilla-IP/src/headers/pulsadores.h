#ifndef __PUL_H__
#define __PUL_H__

//Librerías:
#include <Arduino.h>

//Librerías locales:
#include "util.h"

//Si no pongo el define N se queja que no está declarado.
#ifndef N
#define N 5
#endif

//Periodos para ajustar la aceleración de cuando se deja pulsado un botón en el menú
#define PERIODODEF 250
#define PERIODOMIN 1

class Pulsadores
{
    public: 
        Pulsadores(PINES &);
        bool checkMenu(uint8_t p);
        bool checkTomas(uint8_t p);
        void begin();

        bool flagTimer = 0;

    private:
        bool flagTomas[N] = {0};
        bool flagMenu[N] = {0};
        unsigned long millisAcel = 0;
        uint8_t periodo = PERIODODEF;
        PINES *pin;
};

#endif 