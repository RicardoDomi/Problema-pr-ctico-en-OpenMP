#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include "BusquedaExhaustiva.h"

using namespace std;


// ============================================================
// LEER UN NUMERO ENTERO DE FORMA SEGURA
// ============================================================

int leerEntero(
    string mensaje,
    int minimo,
    int maximo)
{
    string linea;

    int numero;

    char extra;


    while (true)
    {
        cout << mensaje;

        getline(cin, linea);

        stringstream entrada(linea);


        if (entrada >> numero &&
            !(entrada >> extra) &&
            numero >= minimo &&
            numero <= maximo)
        {
            return numero;
        }


        cout << "Entrada no valida. Intenta nuevamente.\n";
    }
}


// ============================================================
// PEDIR UNA CLAVE VALIDA
// ============================================================

string pedirClave(
    BusquedaExhaustiva& buscador,
    int longitud)
{
    string clave;


    while (true)
    {
        cout << "\nIngresa una clave de prueba de "
             << longitud
             << " caracteres: ";

        getline(cin, clave);


        if (buscador.validarClave(clave))
        {
            return clave;
        }
    }
}


// ============================================================
// MOSTRAR TABLA PARA VERSION SECUENCIAL
// ============================================================

void mostrarResultadoSecuencial(
    int ejecucion,
    int longitud,
    double tiempo)
{
    cout << "\n";

    cout
        << "=====================================================================================\n";

    cout
        << "                                   RESULTADOS\n";

    cout
        << "=====================================================================================\n";


    cout << left
         << setw(12) << "Ejecucion"
         << setw(12) << "Longitud"
         << setw(18) << "Version"
         << setw(10) << "Hilos"
         << setw(18) << "Tiempo(s)"
         << setw(15) << "Hilo ganador"
         << endl;


    cout
        << "-------------------------------------------------------------------------------------\n";


    cout << fixed
         << setprecision(6);


    cout << left
         << setw(12) << ejecucion
         << setw(12) << longitud
         << setw(18) << "Secuencial"
         << setw(10) << 1
         << setw(18) << tiempo
         << setw(15) << "---"
         << endl;


    cout
        << "=====================================================================================\n";
}


// ============================================================
// MOSTRAR TABLA PARA VERSION PARALELA
// ============================================================

void mostrarResultadoParalelo(
    int ejecucion,
    int longitud,
    int hilos,
    double tiempo,
    int hiloGanador)
{
    cout << "\n";

    cout
        << "=====================================================================================\n";

    cout
        << "                                   RESULTADOS\n";

    cout
        << "=====================================================================================\n";


    cout << left
         << setw(12) << "Ejecucion"
         << setw(12) << "Longitud"
         << setw(18) << "Version"
         << setw(10) << "Hilos"
         << setw(18) << "Tiempo(s)"
         << setw(15) << "Hilo ganador"
         << endl;


    cout
        << "-------------------------------------------------------------------------------------\n";


    cout << fixed
         << setprecision(6);


    cout << left
         << setw(12) << ejecucion
         << setw(12) << longitud
         << setw(18) << "Paralela"
         << setw(10) << hilos
         << setw(18) << tiempo
         << setw(15) << hiloGanador
         << endl;


    cout
        << "=====================================================================================\n";
}


// ============================================================
// MOSTRAR COMPARACION
// ============================================================

void mostrarComparacion(
    int ejecucion,
    int longitud,
    double tiempoSecuencial,
    double tiempoParalelo,
    int hilos,
    int hiloGanador)
{
    cout << "\n";

    cout
        << "=====================================================================================\n";

    cout
        << "                                   RESULTADOS\n";

    cout
        << "=====================================================================================\n";


    cout << left
         << setw(12) << "Ejecucion"
         << setw(12) << "Longitud"
         << setw(18) << "Version"
         << setw(10) << "Hilos"
         << setw(18) << "Tiempo(s)"
         << setw(15) << "Hilo ganador"
         << endl;


    cout
        << "-------------------------------------------------------------------------------------\n";


    cout << fixed
         << setprecision(6);


    cout << left
         << setw(12) << ejecucion
         << setw(12) << longitud
         << setw(18) << "Secuencial"
         << setw(10) << 1
         << setw(18) << tiempoSecuencial
         << setw(15) << "---"
         << endl;


    cout << left
         << setw(12) << ejecucion
         << setw(12) << longitud
         << setw(18) << "Paralela"
         << setw(10) << hilos
         << setw(18) << tiempoParalelo
         << setw(15) << hiloGanador
         << endl;


    cout
        << "=====================================================================================\n";


    if (tiempoParalelo > 0)
    {
        double speedup =
            tiempoSecuencial /
            tiempoParalelo;


        cout << "\nSpeedup = "
             << tiempoSecuencial
             << " / "
             << tiempoParalelo
             << " = "
             << speedup
             << "x\n";


        if (speedup > 1)
        {
            cout
                << "La version paralela fue mas rapida.\n";
        }
        else
        {
            cout
                << "La version paralela no obtuvo mejora en esta ejecucion.\n";
        }
    }
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    int numeroEjecucion =
        1;


    bool salir =
        false;


    cout
        << "============================================================\n";

    cout
        << "          BUSQUEDA EXHAUSTIVA CON OPENMP\n";

    cout
        << "============================================================\n";

    cout
        << "Caracteres permitidos:\n";

    cout
        << "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n";


    while (!salir)
    {
        cout << "\n";
        cout
            << "============================================================\n";

        cout
            << "CONFIGURACION DE LA EJECUCION "
            << numeroEjecucion
            << endl;

        cout
            << "============================================================\n";


        int longitud =
            leerEntero(
                "Ingresa la longitud de la clave (1 - 10): ",
                1,
                10
            );


        BusquedaExhaustiva buscador(
            longitud
        );


        string clave =
            pedirClave(
                buscador,
                longitud
            );


        unsigned long long total =
            buscador.calcularCombinaciones();


        cout << "\n";

        cout
            << "============================================================\n";

        cout
            << "                     DATOS DE LA PRUEBA\n";

        cout
            << "============================================================\n";


        cout << left
             << setw(30) << "Ejecucion:"
             << numeroEjecucion
             << endl;


        cout << setw(30)
             << "Clave de prueba:"
             << clave
             << endl;


        cout << setw(30)
             << "Longitud:"
             << longitud
             << endl;


        cout << setw(30)
             << "Caracteres disponibles:"
             << 36
             << endl;


        cout << setw(30)
             << "Combinaciones posibles:"
             << total
             << endl;


        cout
            << "============================================================\n";


        bool cambiarClave =
            false;


        while (!salir && !cambiarClave)
        {
            cout << "\n";

            cout
                << "====================== MENU ======================\n";

            cout
                << "1. Ejecutar busqueda secuencial\n";

            cout
                << "2. Ejecutar busqueda paralela\n";

            cout
                << "3. Comparar ambas versiones\n";

            cout
                << "4. Realizar nueva ejecucion\n";

            cout
                << "5. Salir\n";

            cout
                << "==================================================\n";


            int opcion =
                leerEntero(
                    "Selecciona una opcion: ",
                    1,
                    5
                );


            // =================================================
            // OPCION 1 - SECUENCIAL
            // =================================================

            if (opcion == 1)
            {
                unsigned long long revisadas;


                double tiempo =
                    buscador.busquedaSecuencial(
                        clave,
                        revisadas
                    );


                mostrarResultadoSecuencial(
                    numeroEjecucion,
                    longitud,
                    tiempo
                );


                cout << "\nCombinaciones revisadas: "
                     << revisadas
                     << endl;


                cout << "Clave encontrada: "
                     << clave
                     << endl;
            }


            // =================================================
            // OPCION 2 - PARALELA
            // =================================================

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


                mostrarResultadoParalelo(
                    numeroEjecucion,
                    longitud,
                    hilosUtilizados,
                    tiempo,
                    hiloGanador
                );


                cout << "\nCombinaciones revisadas entre todos los hilos: "
                     << revisadas
                     << endl;


                cout << "Clave encontrada: "
                     << clave
                     << endl;


                cout << "Clave encontrada por el hilo: "
                     << hiloGanador
                     << endl;
            }


            // =================================================
            // OPCION 3 - COMPARAR
            // =================================================

            else if (opcion == 3)
            {
                unsigned long long revisadasSecuencial;

                unsigned long long revisadasParalelo;


                int hiloGanador;

                int hilosUtilizados;


                // ---------------------------------------------
                // SECUENCIAL
                // ---------------------------------------------

                double tiempoSecuencial =
                    buscador.busquedaSecuencial(
                        clave,
                        revisadasSecuencial
                    );


                // ---------------------------------------------
                // PARALELA
                // ---------------------------------------------

                double tiempoParalelo =
                    buscador.busquedaParalela(
                        clave,
                        hiloGanador,
                        revisadasParalelo,
                        hilosUtilizados
                    );


                // ---------------------------------------------
                // TABLA FINAL
                // ---------------------------------------------

                mostrarComparacion(
                    numeroEjecucion,
                    longitud,
                    tiempoSecuencial,
                    tiempoParalelo,
                    hilosUtilizados,
                    hiloGanador
                );


                cout << "\nClave encontrada: "
                     << clave
                     << endl;


                cout << "Combinaciones revisadas secuencial: "
                     << revisadasSecuencial
                     << endl;


                cout << "Combinaciones revisadas paralelo: "
                     << revisadasParalelo
                     << endl;


                cout << "Clave encontrada por el hilo: "
                     << hiloGanador
                     << endl;
            }


            // =================================================
            // OPCION 4 - NUEVA EJECUCION
            // =================================================

            else if (opcion == 4)
            {
                numeroEjecucion++;

                cambiarClave =
                    true;
            }


            // =================================================
            // OPCION 5 - SALIR
            // =================================================

            else if (opcion == 5)
            {
                salir =
                    true;
            }
        }
    }


    cout << "\nPrograma finalizado.\n";


    return 0;
}