#line 1 "c:\\Users\\user\\OneDrive - IT-ONE SRL\\Escritorio\\Zapatilla-IP\\Arduino\\Zapatilla-IP\\src\\Filters-master\\FilterDerivative.h"
#ifndef FilterDerivative_h
#define FilterDerivative_h

// returns the derivative
struct FilterDerivative {
  long LastUS;
  float LastInput;
  
  float Derivative;
  
  float input( float inVal );
  
  float output();
};

void testFilterDerivative();

#endif