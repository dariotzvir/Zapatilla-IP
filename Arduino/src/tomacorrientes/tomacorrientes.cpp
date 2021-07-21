#include "tomacorrientes.h"


/**
 * Cuando inicializás un atributo con el valor de un argumento del costructor
 * es mejor hacerlo como cuando inicializas una clase madre:
 *
 * tomacorrientes::tomacorrientes (bool *est) : est(est) {}
 *
 * Por otro lado siempre es mejor que las clases sean camelcase iniciando con
 * mayúsculas simpre, así se difiere mejor de los métodos, atriburos u otras
 * funciones.
 */
tomacorrientes::tomacorrientes ( bool *est ) { this->est = est; }

void tomacorrientes::begin ()
{
    for ( int i = 0 ; i < N ; i++ ) 
    {
        pinMode ( pines [i], OUTPUT );
        conm ( i, *( est + i ) );
    }
}

void tomacorrientes::conm ( int p )
{
    *( est + p ) = !*( est + p );
    conm ( p, *( est + p ) );
    //digitalWrite ( pines [p], *( est + p ) );
    //Copiado desde la referencia de la funcion digitalWrite, quita los chequeos innecesarios en el mismo
}
void tomacorrientes::conm ( int p, bool estado )
{ 
    //digitalWrite ( pines [p], estado );
    //Copiado desde la referencia de la funcion digitalWrite, quita los chequeos innecesarios en el mismo
    uint8_t port = digitalPinToPort(pines[p]);
    uint8_t bit = digitalPinToBitMask(pines[p]);
    volatile uint8_t *out;
    out = portOutputRegister(port);
    if ( estado == 0 ) *out &= ~bit;
    else *out |= bit;
    
    
    /**
     * Como conocés los pines digitales que vas a leer ({ 33, 34, 37, 38, 41 }) conociendo
     * el datasheet del Arduino MEGA 2560, podés buscar sus correspondencias en el ATMega2560,
     * las cuales serían:
     * 
     * {PC4, PC3, PC0, PD7, PG0}  ---> { 33, 34, 37, 38, 41 }
     *
     * {PC4, PC3, PC0, PC1, PC5}  ---> { 33, 34, 37, 36, 32 }
     *
     * Suponiendo que en lugar de guardarlos por el número de pin del Arduino los guardás por
     * la constante correspondiente a los del AVR (PC4 es 0b00000100 y PC3 es 0b00000011):
     *
     *
     * void writeState(int p, bool state) {     // Si cambiás los pines para que todos estén en el registro C.
     *     PORTC = (state) ? PORTC | (1 << p) : PORTC & ~(1 << p);
     * }
     *
     * Si los dejás con los mismos pines entonces mejor dejá la función con el que te ofrece el señor Arduino.h
     */
}
