#ifndef __PUL_H__
#define __PUL_H__

#include "util.h"
#include <Arduino.h>

#ifndef N
#define N 5
#endif

#define PERIODODEF 100

class pulsadores
{
    public: 
        pulsadores ( PINES & );
        bool checkMenu ( int );
        bool checkTomas ( int );
        void begin ();

        bool flagTimer = 0;

    private:
        bool flagTomas [5] = {0};
        bool flagMenu [5] = {0};   //T1, T2, T3, T4, T5, ON, ENT, DER, IZQ
        unsigned long tDer = 0, tIzq = 0;
        int periodo = PERIODODEF;
        PINES *pin;
};

#endif 