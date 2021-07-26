#include "headers/tomacorrientes.h"

tomacorrientes::tomacorrientes ( DATA &data, PINES &pin ) 
{ 
    this->data = &data; 
    this->pin = &pin;
}

void tomacorrientes::begin ()
{
    for ( int i = 0 ; i < N ; i++ ) 
    {
        pinMode ( pin->tomas [i], OUTPUT );
        conm ( i, data->estTomas [i] );
    }
}

void tomacorrientes::invertir ( int p )
{
    data->estTomas [p] = !data->estTomas [p];
    conm ( p, data->estTomas [p] );
    //digitalWrite ( pines [p], *( est + p ) );
    //Copiado desde la referencia de la funcion digitalWrite, quita los chequeos innecesarios en el mismo
}
void tomacorrientes::conm ( int p, bool estado )
{ 
    //digitalWrite ( pines [p], estado );
    //Copiado desde la referencia de la funcion digitalWrite, quita los chequeos innecesarios en el mismo
    uint8_t port = digitalPinToPort(pin->tomas [p]);
    uint8_t bit = digitalPinToBitMask(pin->tomas [p]);
    volatile uint8_t *out;
    out = portOutputRegister(port);
    if ( estado == 0 ) *out &= ~bit;
    else *out |= bit;
}
