#include "BusquedaExhaustiva.h"

#include <iostream>
#include <omp.h>

using namespace std;


BusquedaExhaustiva::BusquedaExhaustiva(int longitudClave)
{
    longitud = longitudClave;

    cantidadCaracteres = 36;

    caracteres = new char[cantidadCaracteres];

    string conjunto = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = 0; i < cantidadCaracteres; i++)
    {
        caracteres[i] = conjunto[i];
    }
}



BusquedaExhaustiva::~BusquedaExhaustiva()
{
    delete[] caracteres;
}


unsigned long long BusquedaExhaustiva::calcularCombinaciones()
{
    unsigned long long total = 1;

    for (int i = 0; i < longitud; i++)
    {
        total *= cantidadCaracteres;
    }

    return total;
}


bool BusquedaExhaustiva::caracterValido(char c)
{
    for (int i = 0; i < cantidadCaracteres; i++)
    {
        if (caracteres[i] == c)
        {
            return true;
        }
    }

    return false;
}



bool BusquedaExhaustiva::validarClave(string clave)
{
    if (clave.empty())
    {
        cout << "Error: la clave no puede estar vacia.\n";
        return false;
    }

    if ((int)clave.length() != longitud)
    {
        cout << "Error: la clave debe tener "
             << longitud
             << " caracteres.\n";

        return false;
    }

    for (int i = 0; i < (int)clave.length(); i++)
    {
        if (!caracterValido(clave[i]))
        {
            cout << "Error: caracter no permitido: "
                 << clave[i] << endl;

            return false;
        }
    }

    return true;
}


string BusquedaExhaustiva::numeroAClave(unsigned long long numero)
{
    string clave(longitud, 'A');

    for (int i = longitud - 1; i >= 0; i--)
    {
        clave[i] = caracteres[numero % cantidadCaracteres];

        numero = numero / cantidadCaracteres;
    }

    return clave;
}


// ========================================
// BUSQUEDA SECUENCIAL
// ========================================

double BusquedaExhaustiva::busquedaSecuencial(
    string objetivo,
    unsigned long long& revisadas)
{
    unsigned long long total = calcularCombinaciones();

    revisadas = 0;

    cout << "\n========================================\n";
    cout << "        BUSQUEDA SECUENCIAL\n";
    cout << "========================================\n";

    double inicio = omp_get_wtime();

    for (unsigned long long i = 0; i < total; i++)
    {
        string actual = numeroAClave(i);

        revisadas++;

        if (actual == objetivo)
        {
            double fin = omp_get_wtime();

            cout << "Clave encontrada: "
                 << actual << endl;

            cout << "Combinaciones revisadas: "
                 << revisadas << endl;

            return fin - inicio;
        }
    }

    double fin = omp_get_wtime();

    cout << "Clave no encontrada.\n";

    return fin - inicio;
}


// ========================================
// BUSQUEDA PARALELA
// ========================================

double BusquedaExhaustiva::busquedaParalela(
    string objetivo,
    int& hiloGanador,
    unsigned long long& revisadasTotales,
    int& hilosUtilizados)
{
    unsigned long long total = calcularCombinaciones();

    int encontrada = 0;

    hiloGanador = -1;

    revisadasTotales = 0;

    hilosUtilizados = 0;

    string claveEncontrada = "";

    cout << "\n========================================\n";
    cout << "         BUSQUEDA PARALELA\n";
    cout << "========================================\n";

    double inicioTiempo = omp_get_wtime();

    #pragma omp parallel shared(encontrada, hiloGanador, claveEncontrada, revisadasTotales, hilosUtilizados)
    {
        int hilo = omp_get_thread_num();

        int totalHilos = omp_get_num_threads();

        #pragma omp single
        {
            hilosUtilizados = totalHilos;
        }


  
        unsigned long long base =
            total / totalHilos;

        unsigned long long sobrantes =
            total % totalHilos;


        unsigned long long inicio;
        unsigned long long cantidad;


 
        if ((unsigned long long)hilo < sobrantes)
        {
            cantidad = base + 1;

            inicio =
                hilo * cantidad;
        }
        else
        {
            cantidad = base;

            inicio =
                sobrantes * (base + 1)
                + (hilo - sobrantes) * base;
        }


        unsigned long long fin =
            inicio + cantidad - 1;


        string claveInicio =
            numeroAClave(inicio);

        string claveFin =
            numeroAClave(fin);


        #pragma omp critical
        {
            cout << "\nHilo " << hilo
                 << " -> Inicio: " << claveInicio
                 << " -> Fin: " << claveFin
                 << " -> Cantidad: " << cantidad
                 << endl;

            cout << "Hilo "
                 << hilo
                 << " inicio su busqueda.\n";
        }


        unsigned long long revisadasHilo = 0;

        bool encontroEsteHilo = false;


        for (unsigned long long i = inicio; i <= fin; i++)
        {
            int detener;

         
            #pragma omp atomic read
            detener = encontrada;


            if (detener == 1)
            {
                break;
            }


            string actual =
                numeroAClave(i);

            revisadasHilo++;


            if (actual == objetivo)
            {
                #pragma omp critical
                {
                    int yaEncontrada;

                    #pragma omp atomic read
                    yaEncontrada = encontrada;


                    if (yaEncontrada == 0)
                    {
                        hiloGanador = hilo;

                        claveEncontrada = actual;

                        encontroEsteHilo = true;

                        #pragma omp atomic write
                        encontrada = 1;
                    }
                }

                break;
            }
        }



        #pragma omp atomic
        revisadasTotales += revisadasHilo;



        #pragma omp critical
        {
            cout << "Hilo "
                 << hilo
                 << " finalizo.";

            if (encontroEsteHilo)
            {
                cout << " ENCONTRO LA CLAVE.";
            }
            else
            {
                cout << " No encontro la clave.";
            }

            cout << "\nCombinaciones revisadas: "
                 << revisadasHilo
                 << endl;
        }
    }


    double finTiempo = omp_get_wtime();


    if (hiloGanador != -1)
    {
        cout << "\nClave encontrada: "
             << claveEncontrada << endl;

        cout << "Clave encontrada por el hilo: "
             << hiloGanador << endl;
    }
    else
    {
        cout << "\nClave no encontrada.\n";
    }


    return finTiempo - inicioTiempo;
}