#ifndef __PUL_H__
#define __PUL_H__

#include <Arduino.h>

class pulsadores
{
    public: 
        bool check ( int p );
        void begin ();
        int periodo = 100;
        bool flagTimer = 0;

    private:
        bool *est;
        bool flag [10];   //T1, T2, T3, T4, T5, ON, ENT, DER, IZQ, RESET
        byte pines [10] = { 22, 25, 26, 29, 30, 49, 45, 46, 42, 18 };
        unsigned long tDer = 0, tIzq = 0;
};

#endif 