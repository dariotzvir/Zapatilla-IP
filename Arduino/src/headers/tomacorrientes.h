#ifndef __TOMAS_H__
#define __TOMAS_H__

#include <Arduino.h>
#include "util.h"

#ifndef N
#define N 5
#endif

class tomacorrientes
{
    public: 
        tomacorrientes ( DATA &, PINES & );
        void conm ( int, bool );
        void invertir ( int );
        void begin ();

    private:
        DATA *data;
        PINES *pin;
};

#endif
