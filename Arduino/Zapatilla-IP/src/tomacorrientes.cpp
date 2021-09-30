#include "headers/tomacorrientes.h"

Tomacorrientes::Tomacorrientes(DATA &data, PINES &pin) 
{ 
    this->data = &data; 
    this->pin = &pin;
}

void Tomacorrientes::begin()
{
    for(uint8_t i=0 ;i<N ;i++) 
    {
        pinMode(pin->tomas[i], OUTPUT);
        pinMode(pin->leds[i], OUTPUT);
        conm(i, data->estTomas[i]);
    }
}

void Tomacorrientes::invertir(uint8_t p)
{
    data->estTomas[p] = !data->estTomas[p];
    conm(p, data->estTomas[p]);
}
void Tomacorrientes::conm(uint8_t p, bool estado)
{ 
    estado = !estado;
    digitalWrite(pin->tomas[p], estado);
    digitalWrite(pin->leds[p], !estado);
}
