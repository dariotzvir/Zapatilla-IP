#include "headers/pulsadores.h"

pulsadores::pulsadores(PINES &pin)
{
    this->pin = &pin;
}

void pulsadores::begin()
{
    for(uint8_t i=0; i<4; i++)  pinMode(pin->pulMenu[i], INPUT_PULLUP);
    for(uint8_t i=0; i<N; i++)  pinMode(pin->pulTomas[i], INPUT_PULLUP);
}
bool pulsadores::checkTomas(uint8_t p)
{
    bool retorno = 0;
    if(!digitalRead(pin->pulTomas[p]) && !flagTomas[p]) 
    {
        flagTomas[p] = 1; 
        retorno = 1;
    }
    else if(digitalRead(pin->pulTomas[p]) && flagTomas[p]) 
    {
        flagTomas[p] = 0;
    }
    return retorno;
}
bool pulsadores::checkMenu(uint8_t p)
{
    bool retorno = 0;
    if(flagTimer && (p == IZQ || p == DER))
    {
        if(!digitalRead(pin->pulMenu[p]) && flagMenu[p] && millis()-millisAcel >= periodo)
        {
           // millisAcel = millis();
            flagMenu[p] = 0;
                
            #ifdef DEBUGPUL
            Serial.println(periodo);
            #endif
            periodo *= ((periodo > PERIODOMIN) ? 0.8 : 1);
        }
    }
    if(!digitalRead(pin->pulMenu[p]) && !flagMenu[p])
    {
        retorno = 1;
        flagMenu[p] = 1;
        millisAcel = millis();
    }
    else if(digitalRead(pin->pulMenu[p]) && flagMenu[p])
    {
        retorno = 0;
        flagMenu[p] = 0;
        periodo = PERIODODEF;
        millisAcel = millis();
    }
    return retorno;
}