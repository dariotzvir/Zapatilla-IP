#ifndef __TOMAS_H__
#define __TOMAS_H__

#include <Arduino.h>

#ifndef N
#define N 5
#endif

class tomacorrientes
{
    public: 
        tomacorrientes ( bool *est );
        void conm ( int p, bool estado );   // Cmabiale el nombre a este conm por favor xdd.
        void conm ( int p );
        void begin ();

    private:
        bool *est;
        const byte pines [N] = { 33, 34, 37, 38, 41 };    // Agregale const por delante, así se sabe que son constantes y no lo cambiás por accidente.
};

#endif
