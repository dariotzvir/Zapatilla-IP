#ifndef __SERVER_H__
#define __SERVER_H__


/**
 * Esto es más que nada por no incluir cosas de más, dejarlo más legible y salvar un poco de
 * memoria, pero en lugar de incluir todo Arduino.h (por lo que ví solo usas String y Serial, a lo
 * mejor se me pasó algo), podés incluir específicamente las librerías que vayas a usar, como lo
 * serían WString.h (Para los String) y USBAPI.h (Para el Serial). Cuando usás, por ejemplo, uint8_t podés
 * definirlo vos mismo escribiendo typedef unsigned char uint8_t.
 */
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

#ifndef N
#define N 5
#endif

/*
struct measurements {
    float corriente;
    float tempAct;
    float humAct;
    .
    .
    .
};  // Esto lo declarás y definís donde manejes estos datos (fuera de esta librería imagino)
    // y pasás como argumento del constructor un puntero al objeto creado.

// Si no podés probar esto:
struct measurements {
    float *corriente;
    float &tempAct;
    float &humAct;
    .
    .
    .
    
    inline measurments (float *corriente, float &tempAct, . . .) : corriente(corriente), tempAct(tempAct), x(y) {}
    
};  // Entonces antes de pasar esos valores como argumentos, creas el objeto y pasas
    // una copia o una referencia, como gustes. Seguirías teniendo muchos argumentos
    // pero si los separás bien al menos sería más legible (Personalmente prefiero 
    // la opción anterior, pero implicaría cambios en más archivos para que todos
    // usen los struct en lugar de las variables individuales).

*/


class server: EthernetServer
{
    public:
        /**
         * Probá colocando los parámetros relacionados, como las de mediciones (Ej.: tempAct, humAct, etc.),
         * en structs así reducís la cantidad de argumentos y que pida un puntero o referencias es estas
         * (o sí solo usás un parámetro que pida un puntero a dicho elemento del struct).
         */
        server ( byte *mac, int &puerto, IPAddress &ipFija, float *corriente, bool *estTomas, float &tempAct, float &humAct, float &tension, int &tempMax, int &tempMin, bool &flagDhcp );
        void setup ();
        int rutina ();
    private:
        /**
         * Los atributos normalmente se suelen poner arriba de todo, por cierto, si usás class, por defecto, 
         * todos los atributos y métodos que declares son private a no ser que explicitamente indiques lo 
         * contrario, por lo que podés prescindir de la palabra reservada y ponerlos arriba de todo. (struct
         * es lo opuesto, todos son public por defecto, pero funciona igual que un class).
         */
        int *puerto;
        IPAddress *ipFija;
        float *corriente; 
        bool *estTomas;
        float *tempAct, *humAct, *tension;
        int *tempMax, *tempMin;
        byte *mac;
        int flagGuardado = 0;
        bool *flagDhcp;

        String peticion;

        void retorno ();
        String comandoServer ( int index );
        String lecturaServer ( int index );
};


#endif
