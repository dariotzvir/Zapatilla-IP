#ifndef __PANTALLAOLED_H__
#define __PANTALLAOLED_H__

//Librerías:
#include <IPAddress.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

//Librerías locales:
#include "util.h"

//Si no pongo el define N se queja que no está declarado.
#ifndef N
#define N 5
#endif

/**
 * @brief clase para manejar la pantalla OLED, hereda el objeto de otra librería.
 */
class PantallaOLED: public Adafruit_SSD1306
{
    public:
        PantallaOLED(DATA &);
        void setup();

        void pantallaApagada();
        void pantallaPrincipal();
        void menu(IPAddress, bool); //TODO: Separar el switch interno en métodos.
        void pantallaReset();
        void pantallaBoot();

        void resetBuf(); //Resetea todas las variables auxiliares que se muestran en pantalla con las del objeto real.

        //Métodos para la lógica de navegar el menú.
        void logicaDer();
        void logicaIzq();
        void logicaOnOff();
        int logicaEnter();

        int pantallaSelec = 0;
        bool flagSelec = 0; //Flag para el menú indicando si se está modificando una variable o no.

        //Variables auxiliares que se modifican en el menú luego se copian al objeto de DATA si se efectuó la modificación
        int8_t bufferTempMax = 0;
        int8_t bufferTempMin = 0;
        bool bufferDHCP = 1;
    
    private:
        //Métodos para graficar las grillas y elementos estáticos de las distintas pantallas.
        void grillaPrin();
        void textoFijoPrin();
        void grillaMenu();

        DATA *data;
};

#endif