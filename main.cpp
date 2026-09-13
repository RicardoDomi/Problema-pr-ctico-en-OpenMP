#include <iostream>
#include <string>

#include "BusquedaExhaustiva.h"

using namespace std;


int main()
{
    int longitud;

    cout << "========================================\n";
    cout << "   BUSQUEDA EXHAUSTIVA CON OPENMP\n";
    cout << "========================================\n";

    cout << "Caracteres permitidos:\n";
    cout << "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n";


   
    do
    {
        cout << "\nIngresa la longitud de la clave (1 - 10): ";
        cin >> longitud;

        if (longitud < 1 || longitud > 10)
        {
            cout << "Longitud no valida.\n";
        }

    } while (longitud < 1 || longitud > 10);


    BusquedaExhaustiva buscador(longitud);


    string clave;



    do
    {
        cout << "Ingresa una clave de prueba de "
             << longitud
             << " caracteres: ";

        cin >> clave;

    } while (!buscador.validarClave(clave));


    unsigned long long total =
        buscador.calcularCombinaciones();


    cout << "\n========================================\n";
    cout << "DATOS\n";
    cout << "========================================\n";

    cout << "Clave: "
         << clave << endl;

    cout << "Longitud: "
         << longitud << endl;

    cout << "Combinaciones posibles: "
         << total << endl;


    int opcion;


    do
    {
        cout << "\n========================================\n";
        cout << "MENU\n";
        cout << "========================================\n";

        cout << "1. Busqueda secuencial\n";
        cout << "2. Busqueda paralela\n";
        cout << "3. Comparar ambas versiones\n";
        cout << "4. Salir\n";

        cout << "\nSelecciona una opcion: ";

        cin >> opcion;


        // ========================================
        // SECUENCIAL
        // ========================================

        if (opcion == 1)
        {
            unsigned long long revisadas;

            double tiempo =
                buscador.busquedaSecuencial(
                    clave,
                    revisadas
                );


            cout << "\nTiempo secuencial: "
                 << tiempo
                 << " segundos\n";
        }


        // ========================================
        // PARALELA
        // ========================================

        else if (opcion == 2)
        {
            int hiloGanador;

            int hilosUtilizados;

            unsigned long long revisadas;


            double tiempo =
                buscador.busquedaParalela(
                    clave,
                    hiloGanador,
                    revisadas,
                    hilosUtilizados
                );


            cout << "\nTiempo paralelo: "
                 << tiempo
                 << " segundos\n";

            cout << "Hilos utilizados: "
                 << hilosUtilizados << endl;

            cout << "Hilo ganador: "
                 << hiloGanador << endl;
        }


        // ========================================
        // COMPARACION
        // ========================================

        else if (opcion == 3)
        {
            unsigned long long revisadasSecuencial;

            unsigned long long revisadasParalelo;

            int hiloGanador;

            int hilosUtilizados;


            double tiempoSecuencial =
                buscador.busquedaSecuencial(
                    clave,
                    revisadasSecuencial
                );


            double tiempoParalelo =
                buscador.busquedaParalela(
                    clave,
                    hiloGanador,
                    revisadasParalelo,
                    hilosUtilizados
                );


            cout << "\n========================================\n";
            cout << "      COMPARACION DE RESULTADOS\n";
            cout << "========================================\n";


            cout << "Clave: "
                 << clave << endl;

            cout << "Combinaciones posibles: "
                 << total << endl;


            cout << "\nTiempo secuencial: "
                 << tiempoSecuencial
                 << " segundos\n";


            cout << "Tiempo paralelo: "
                 << tiempoParalelo
                 << " segundos\n";


            cout << "Hilos utilizados: "
                 << hilosUtilizados
                 << endl;


            cout << "Hilo ganador: "
                 << hiloGanador
                 << endl;


            if (tiempoParalelo > 0)
            {
                double speedup =
                    tiempoSecuencial /
                    tiempoParalelo;


                cout << "Speedup: "
                     << speedup
                     << "x\n";
            }


            cout << "========================================\n";
        }


        else if (opcion == 4)
        {
            cout << "\nPrograma finalizado.\n";
        }


        else
        {
            cout << "\nOpcion no valida.\n";
        }


    } while (opcion != 4);


    return 0;
}