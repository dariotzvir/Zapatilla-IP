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
    //for ( int i = 0 ; i < 5 ; i++ ) Serial.print ( flagTomas [i] );
    //Serial.println ("");
    return retorno;
}
bool pulsadores::checkMenu ( int p )
{
    bool retorno = 0;
    //Serial.print ( "toma " );
    //Serial.println ( p );
    if ( flagTimer )
    {
        if ( flagMenu [3] && millis ()- tDer >= periodo ) 
        {
            tDer = millis ();
            flagMenu [3] = 0;
        }
        if ( flagMenu [2] && millis ()- tIzq >= periodo )
        {
            tIzq = millis ();
            flagMenu [2] = 0;
        } 
    }
    if ( !digitalRead ( pin->pulMenu [p] ) && !flagMenu [p] ) 
    {
        flagMenu [p] = 1;     
        retorno = 1;
    }   
    else if ( digitalRead ( pin->pulMenu [p] ) && flagMenu [p] ) flagMenu [p] = 0;
    return retorno;
}