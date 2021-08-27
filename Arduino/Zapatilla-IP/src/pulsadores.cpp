#include "headers/pulsadores.h"

pulsadores::pulsadores ( PINES &pin )
{
    this->pin = &pin;
}

void pulsadores::begin ()
{
    for ( int i=0; i<4; i++ )  pinMode ( pin->pulMenu [i], INPUT_PULLUP );
    for ( int i=0; i<N; i++ )  pinMode ( pin->pulTomas [i], INPUT_PULLUP );
}
bool pulsadores::checkTomas ( int p )
{
    bool retorno = 0;
    if ( !digitalRead ( pin->pulTomas [p] ) && !flagTomas [p] ) 
    {
        flagTomas [p] = 1; 
        retorno = 1;
    }
    else if ( digitalRead ( pin->pulTomas [p] ) && flagTomas [p] ) 
    {
        flagTomas [p] = 0;
    }
    return retorno;
}
bool pulsadores::checkMenu ( int p )
{
    bool retorno = 0;
    if ( !digitalRead ( pin->pulMenu [p] ) && !flagMenu [p] )
    {
        flagMenu [p] = 1;
        retorno = 1;
    }
    else if ( digitalRead ( pin->pulMenu [p] ) && flagMenu [p] )
    {
        flagMenu [p] = 0;
        periodo = PERIODODEF;
    }
    if ( flagTimer )
    {
        if ( flagMenu [IZQ] && millis - tIzq >= periodo )
        {
            tIzq = millis ();
            periodo = periodo*0.95;
            retorno = 1;
        }
        if ( flagMenu [DER] && millis () - tDer >= periodo )
        {
            tDer = millis ();
            periodo = periodo*0.95;
            retorno = 1;
        }
    }
    return retorno;
}