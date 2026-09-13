#include "BusquedaExhaustiva.h"

#include <iostream>
#include <iomanip>
#include <omp.h>

using namespace std;



struct DatosHilo
{
    unsigned long long inicio;
    unsigned long long fin;
    unsigned long long cantidad;
    unsigned long long revisadas;

    int resultado;
};


// ============================================================
// CONSTRUCTOR
// ============================================================

BusquedaExhaustiva::BusquedaExhaustiva(int longitudClave)
{
    longitud = longitudClave;

    cantidadCaracteres = 36;

    // Arreglo dinamico
    caracteres = new char[cantidadCaracteres];

    string conjunto =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = 0; i < cantidadCaracteres; i++)
    {
        caracteres[i] = conjunto[i];
    }
}


// ============================================================
// DESTRUCTOR
// ============================================================

BusquedaExhaustiva::~BusquedaExhaustiva()
{
    delete[] caracteres;
}


// ============================================================
// CALCULAR TOTAL DE COMBINACIONES
// ============================================================

unsigned long long BusquedaExhaustiva::calcularCombinaciones()
{
    unsigned long long total = 1;

    for (int i = 0; i < longitud; i++)
    {
        total = total * cantidadCaracteres;
    }

    return total;
}


// ============================================================
// COMPROBAR SI UN CARACTER ES VALIDO
// ============================================================

bool BusquedaExhaustiva::caracterValido(char caracter)
{
    for (int i = 0; i < cantidadCaracteres; i++)
    {
        if (caracteres[i] == caracter)
        {
            return true;
        }
    }

    return false;
}


// ============================================================
// VALIDAR CLAVE
// ============================================================

bool BusquedaExhaustiva::validarClave(string clave)
{
    if (clave.empty())
    {
        cout << "\nError: la clave no puede estar vacia.\n";
        return false;
    }


    if ((int)clave.length() != longitud)
    {
        cout << "\nError: la clave debe tener exactamente "
             << longitud
             << " caracteres.\n";

        return false;
    }


    for (int i = 0; i < (int)clave.length(); i++)
    {
        if (!caracterValido(clave[i]))
        {
            cout << "\nError: el caracter '"
                 << clave[i]
                 << "' no esta permitido.\n";

            cout << "Solo se permiten A-Z y 0-9.\n";

            return false;
        }
    }

    return true;
}


// ============================================================
// CONVERTIR NUMERO A CLAVE
// ============================================================

string BusquedaExhaustiva::numeroAClave(
    unsigned long long numero)
{
    string clave(longitud, 'A');

    for (int i = longitud - 1; i >= 0; i--)
    {
        int posicion =
            numero % cantidadCaracteres;

        clave[i] = caracteres[posicion];

        numero =
            numero / cantidadCaracteres;
    }

    return clave;
}


// ============================================================
// BUSQUEDA SECUENCIAL
// ============================================================

double BusquedaExhaustiva::busquedaSecuencial(
    string objetivo,
    unsigned long long& revisadas)
{
    unsigned long long total =
        calcularCombinaciones();

    revisadas = 0;


    double inicio =
        omp_get_wtime();


    for (unsigned long long i = 0;
         i < total;
         i++)
    {
        string actual =
            numeroAClave(i);

        revisadas++;


        if (actual == objetivo)
        {
            break;
        }
    }


    double fin =
        omp_get_wtime();


    return fin - inicio;
}


// ============================================================
// BUSQUEDA PARALELA
// ============================================================

double BusquedaExhaustiva::busquedaParalela(
    string objetivo,
    int& hiloGanador,
    unsigned long long& revisadasTotales,
    int& hilosUtilizados)
{
    unsigned long long total =
        calcularCombinaciones();


   
    int encontrada = 0;


    hiloGanador = -1;

    revisadasTotales = 0;

    hilosUtilizados = 0;


    int maxHilos =
        omp_get_max_threads();



    DatosHilo* datos =
        new DatosHilo[maxHilos];


    double inicioTiempo =
        omp_get_wtime();


    // ========================================================
    // REGION PARALELA
    // ========================================================

    #pragma omp parallel shared(encontrada, hiloGanador, datos, revisadasTotales, hilosUtilizados)
    {
        int hilo =
            omp_get_thread_num();


        int totalHilos =
            omp_get_num_threads();


        #pragma omp single
        {
            hilosUtilizados =
                totalHilos;
        }


        // ----------------------------------------------------
        // DIVIDIR EL ESPACIO DE BUSQUEDA
        // ----------------------------------------------------

        unsigned long long base =
            total / totalHilos;


        unsigned long long sobrantes =
            total % totalHilos;


        unsigned long long inicioRango;

        unsigned long long cantidad;


     
        if ((unsigned long long)hilo < sobrantes)
        {
            cantidad =
                base + 1;


            inicioRango =
                (unsigned long long)hilo
                * cantidad;
        }
        else
        {
            cantidad =
                base;


            inicioRango =
                sobrantes * (base + 1)
                +
                ((unsigned long long)hilo - sobrantes)
                * base;
        }


        unsigned long long finRango =
            inicioRango + cantidad - 1;


        datos[hilo].inicio =
            inicioRango;

        datos[hilo].fin =
            finRango;

        datos[hilo].cantidad =
            cantidad;

        datos[hilo].revisadas =
            0;

        datos[hilo].resultado =
            0;


        bool detenido =
            false;


        // ----------------------------------------------------
        // BUSQUEDA DEL HILO
        // ----------------------------------------------------

        for (unsigned long long i = inicioRango;
             i <= finRango;
             i++)
        {
            int terminar;


 
            #pragma omp atomic read
            terminar = encontrada;


            if (terminar == 1)
            {
                detenido = true;
                break;
            }


            string actual =
                numeroAClave(i);


            datos[hilo].revisadas++;


            if (actual == objetivo)
            {
               
                #pragma omp atomic write
                encontrada = 1;


             
                #pragma omp critical(registroGanador)
                {
                    if (hiloGanador == -1)
                    {
                        hiloGanador =
                            hilo;

                        datos[hilo].resultado =
                            1;
                    }
                }


                break;
            }
        }


        if (detenido)
        {
            datos[hilo].resultado =
                2;
        }



        #pragma omp atomic update
        revisadasTotales +=
            datos[hilo].revisadas;
    }


    double finTiempo =
        omp_get_wtime();


    // ========================================================
    // TABLA DE DISTRIBUCION DE RANGOS
    // ========================================================

    cout << "\n";

    cout
        << "=====================================================================================\n";

    cout
        << "                         DISTRIBUCION DE LOS RANGOS\n";

    cout
        << "=====================================================================================\n";


    cout << left
         << setw(8)  << "Hilo"
         << setw(15) << "Inicio"
         << setw(15) << "Fin"
         << setw(18) << "Cantidad"
         << setw(20) << "Resultado"
         << endl;


    cout
        << "-------------------------------------------------------------------------------------\n";


    for (int i = 0;
         i < hilosUtilizados;
         i++)
    {
        string resultado;


        if (datos[i].resultado == 1)
        {
            resultado =
                "Encontrada";
        }
        else if (datos[i].resultado == 2)
        {
            resultado =
                "Detenido";
        }
        else
        {
            resultado =
                "No encontrada";
        }


        cout << left
             << setw(8) << i
             << setw(15)
             << numeroAClave(datos[i].inicio)
             << setw(15)
             << numeroAClave(datos[i].fin)
             << setw(18)
             << datos[i].cantidad
             << setw(20)
             << resultado
             << endl;
    }


    cout
        << "=====================================================================================\n";



    delete[] datos;


    return finTiempo -
           inicioTiempo;
}