#include "pulsadores.h"

void pulsadores::begin ()
{
    for ( int i = 0 ; i < 10 ; i++ )  pinMode ( pines [i], INPUT_PULLUP );
}
bool pulsadores::check ( int p )
{
    if ( flagTimer )
    {
        if ( flag [7] && millis ()- tDer >= periodo ) 
        {
            tDer = millis ();
            flag [7] = 0;
        }
        if ( flag [8] && millis ()- tIzq >= periodo )
        {
            tIzq = millis ();
            flag [8] = 0;
        } 
    }
    if ( !digitalRead ( pines [p] ) && !flag [p] ) 
    {
        flag [p] = 1;       
        return 1;
    }
    else if ( digitalRead ( pines [p] ) && flag [p] ) flag [p] = 0;
    return 0;
}