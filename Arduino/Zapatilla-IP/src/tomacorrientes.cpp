#include "headers/tomacorrientes.h"

tomacorrientes::tomacorrientes ( DATA &data, PINES &pin ) 
{ 
    this->data = &data; 
    this->pin = &pin;
}

void tomacorrientes::begin ()
{
    for ( int i=0 ;i<N ;i++ ) 
    {
        pinMode ( pin->tomas [i], OUTPUT );
        pinMode ( pin->leds [i], OUTPUT );
        conm ( i, data->estTomas [i] );
    }
}

void tomacorrientes::invertir ( int p )
{
    data->estTomas [p] = !data->estTomas [p];
    conm ( p, data->estTomas [p] );
    //digitalWrite ( pines [p], *( est + p ) );
}
void tomacorrientes::conm ( int p, bool estado )
{ 
    estado = !estado;
    digitalWrite ( pin->tomas [p], estado );
    digitalWrite ( pin->leds [p], !estado );
}
