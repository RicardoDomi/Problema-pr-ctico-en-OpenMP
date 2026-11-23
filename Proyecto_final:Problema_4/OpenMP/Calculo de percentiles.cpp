#include <iostream>
#include <fstream>
#include <chrono>
#include <stdio.h>
#include <omp.h>
#include <vector>
using namespace std;

bool ordenado = false;

void mostrarArreglo(int *arreglo, int size)
{
    int limite = (size < 20) ? size : 20;

    for (int i = 0; i < limite; i++)
    {
        printf("%d ", arreglo[i]);
    }
    printf("\n");
}

double calcularPercentil(int *arreglo, int size, int k)
{
    double posicion = (k * (size + 1.0)) / 100.0;

    if (posicion <= 1)
        return arreglo[0];

    if (posicion >= size)
        return arreglo[size - 1];

    int indice = (int)posicion;
    double decimal = posicion - indice;

    return arreglo[indice - 1] +
           decimal * (arreglo[indice] - arreglo[indice - 1]);
}

void ordenarParalelo(int *arreglo, int N, int hilos)
{
    const int RANGO = 1000001;

    vector<int> conteo(RANGO, 0);

    vector<vector<int>> locales(
        hilos, vector<int>(RANGO, 0));

#pragma omp parallel
    {
        int id = omp_get_thread_num();

#pragma omp for
        for (int i = 0; i < N; i++)
        {
            locales[id][arreglo[i]]++;
        }
    }

#pragma omp parallel for
    for (int i = 0; i < RANGO; i++)
    {
        for (int j = 0; j < hilos; j++)
        {
            conteo[i] += locales[j][i];
        }
    }

    vector<int> posiciones(RANGO, 0);

    for (int i = 1; i < RANGO; i++)
    {
        posiciones[i] = posiciones[i - 1] + conteo[i - 1];
    }

#pragma omp parallel for
    for (int i = 0; i < RANGO; i++)
    {
        for (int j = 0; j < conteo[i]; j++)
        {
            arreglo[posiciones[i] + j] = i;
        }
    }
}

int main()
{
    int hilos = omp_get_num_procs();

    if (hilos < 4)
    {
        hilos = 4;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(hilos);

    ifstream archivo("../10_millones_numeros.txt");

    if (!archivo.is_open())
    {
        cout << "Error al abrir el archivo" << endl;
        return 1;
    }

    cout << "Archivo abierto correctamente" << endl;

    int N = 0;
    int numero;

    while (archivo >> numero)
    {
        N++;
    }

    if (!archivo.eof())
    {
        cout << "Error: el archivo contiene datos invalidos" << endl;
        return 1;
    }

    if (N == 0)
    {
        cout << "El archivo esta vacio" << endl;
        return 1;
    }

    cout << "Total de numeros: " << N << endl;

    archivo.clear();
    archivo.seekg(0, ios::beg);

    int *arreglo = new int[N];

    for (int i = 0; i < N; i++)
    {
        archivo >> arreglo[i];
    }

    int opcion;
    int percentiles[] = {10, 25, 50, 75, 90, 95};
    double resultados[6];

    do
    {
        printf("\n========== MENU ==========\n");
        printf("1. Mostrar arreglo\n");
        printf("2. Ordenar de menor a mayor\n");
        printf("3. Calcular los percentiles\n");
        printf("4. Calcular los cuartiles\n");
        printf("5. Mostrar el valor correspondiente a cada percentil y cuartil \n");
        printf("6. Calcular el rango intercuartílico (RIC) \n");
        printf("7. Salir \n");
        printf("==========================\n");
        printf("Selecciona una opcion: ");
        scanf("%d", &opcion);

        switch (opcion)
        {
        case 1:
            mostrarArreglo(arreglo, N);
            break;

        case 2:
        {
            if (ordenado)
            {
                printf("El arreglo ya esta ordenado\n");
                break;
            }

            auto inicio = chrono::steady_clock::now();

            ordenarParalelo(arreglo, N, hilos);

#pragma omp parallel for
            for (int i = 0; i < 6; i++)
            {
                resultados[i] = calcularPercentil(
                    arreglo, N, percentiles[i]);
            }

            auto fin = chrono::steady_clock::now();

            chrono::duration<double> tiempo = fin - inicio;

            ordenado = true;

            printf("Procesamiento completado\n");
            printf("Tiempo: %.6f segundos\n", tiempo.count());

            break;
        }

        case 3:
            if (ordenado)
            {
                for (int i = 0; i < 6; i++)
                {
                    printf("P%d = %.2f\n",
                           percentiles[i], resultados[i]);
                }
            }
            else
            {
                printf("Primero debes ordenar el arreglo\n");
            }
            break;

        case 4:
            if (ordenado)
            {
                printf("Q1 = %.2f\n", resultados[1]);
                printf("Q2 = %.2f\n", resultados[2]);
                printf("Q3 = %.2f\n", resultados[3]);
            }
            else
            {
                printf("Primero debes ordenar el arreglo\n");
            }
            break;

        case 5:
            if (ordenado)
            {
                printf("\n===== PERCENTILES =====\n");

                for (int i = 0; i < 6; i++)
                {
                    printf("P%d = %.2f\n", percentiles[i], resultados[i]);
                }

                printf("\n===== CUARTILES =====\n");
                printf("Q1 = %.2f\n", resultados[1]);
                printf("Q2 = %.2f\n", resultados[2]);
                printf("Q3 = %.2f\n", resultados[3]);
            }
            else
            {
                printf("Primero debes ordenar el arreglo\n");
            }
            break;

        case 6:
            if (ordenado)
            {
                double RIC = resultados[3] - resultados[1];

                printf("RIC = %.2f\n", RIC);
            }
            else
            {
                printf("Primero debes ordenar el arreglo\n");
            }
            break;
        case 7:
            printf("Saliendo...\n");
            break;

        default:
            printf("Opcion no valida\n");
        }
    } while (opcion != 7);

    archivo.close();

    delete[] arreglo;

    return 0;
}