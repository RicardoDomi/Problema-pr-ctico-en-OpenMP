# Problema-pr-ctico-en-OpenMP
# Búsqueda exhaustiva secuencial y paralela con OpenMP

Práctica de implementación y comparación de una búsqueda exhaustiva de claves de prueba, desarrollando una versión secuencial y otra paralela utilizando OpenMP.

## Integrantes

- DIEGO EDUARDO SILVA MONTES
- JORGE NATANAEL CASTOLO GONZALEZ
- RICARDO DOMINGUEZ ALCALA

## Solución implementada

El programa realiza una búsqueda exhaustiva sobre una clave de prueba ingresada por el usuario.

El espacio de búsqueda está formado por los siguientes 36 caracteres:

```text
ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789

El número de combinaciones se calcula dependiendo de la longitud de la clave. Por ejemplo:

36^3 = 46,656
36^5 = 60,466,176
36^10 = 3,656,158,440,062,976

Las combinaciones no se almacenan todas en memoria. Cada posición del espacio de búsqueda se representa mediante un número y se convierte a su combinación correspondiente cuando se necesita.

Se utilizan arreglos dinámicos y no se utiliza vector.

Algoritmos implementados
Búsqueda secuencial: recorre las combinaciones una por una hasta encontrar la clave de prueba.
Búsqueda paralela: divide el espacio total de búsqueda entre los hilos disponibles. Cada hilo recibe un rango diferente y realiza la búsqueda dentro de ese rango.
Generación de combinaciones: convierte una posición numérica a una cadena formada por los caracteres permitidos.
Distribución de rangos: divide las combinaciones de la forma más equilibrada posible entre los hilos. Si existen combinaciones sobrantes, se reparten entre los primeros hilos.

Cuando un hilo encuentra la clave, registra su identificador y comunica el resultado para que los demás hilos puedan detener su búsqueda.

Directivas y funciones de OpenMP
parallel: crea el grupo de hilos que participa en la búsqueda paralela.
single: permite que solamente un hilo realice una operación determinada dentro de la región paralela.
critical: protege el registro del hilo ganador para evitar que varios hilos modifiquen la información al mismo tiempo.
atomic: permite leer y modificar de manera segura variables compartidas entre los hilos.
omp_get_thread_num(): obtiene el identificador del hilo actual.
omp_get_num_threads(): obtiene la cantidad de hilos dentro de la región paralela.
omp_get_max_threads(): obtiene la cantidad máxima de hilos disponibles.
omp_get_wtime(): mide los tiempos de ejecución de las versiones secuencial y paralela.

La sincronización permite evitar condiciones de carrera y registrar correctamente cuál hilo encontró la clave.

Requisitos
Compilador compatible con C++.
OpenMP habilitado.
g++ o un compilador equivalente con soporte para OpenMP.
Archivos del proyecto
main.cpp
BusquedaExhaustiva.cpp
BusquedaExhaustiva.h
README.md
main.cpp: contiene el menú, la entrada de datos y la presentación de resultados.
BusquedaExhaustiva.h: contiene la definición de la clase.
BusquedaExhaustiva.cpp: contiene la implementación de la búsqueda secuencial, paralela, generación de combinaciones y distribución de rangos.
Compilación y ejecución en Windows

Desde la carpeta donde se encuentran los archivos:

g++ main.cpp BusquedaExhaustiva.cpp -o busqueda.exe -fopenmp

Para ejecutar:

.\busqueda.exe
Menú de ejecución

El programa cuenta con las siguientes opciones:

1. Ejecutar búsqueda secuencial
2. Ejecutar búsqueda paralela
3. Comparar ambas versiones
4. Realizar nueva ejecución
5. Salir

La opción de comparación ejecuta la búsqueda secuencial y paralela utilizando la misma clave de prueba para poder comparar correctamente los resultados.

Configuración de las dos ejecuciones
Primera ejecución

Se utiliza una clave de 3 caracteres para observar fácilmente el funcionamiento del algoritmo.

Ejemplo:

Longitud: 3
Clave: B4Z

En esta ejecución se muestran los rangos asignados a cada hilo, la cantidad de combinaciones y el hilo que encuentra la clave.

Segunda ejecución

Se utiliza una clave de mayor longitud para observar mejor la diferencia de rendimiento entre la versión secuencial y la paralela.

Por ejemplo:

Longitud: 5
Clave: B12CD

La longitud puede aumentarse dependiendo del tiempo de ejecución del equipo.

Resultados

El programa muestra una tabla con la comparación de ambas versiones:

Ejecucion   Longitud   Version      Hilos   Tiempo(s)   Hilo ganador
--------------------------------------------------------------------
1           3          Secuencial   1       ...         ---
1           3          Paralela     8       ...         ...

La versión paralela también muestra la distribución de trabajo:

Hilo    Inicio    Fin       Cantidad     Resultado
---------------------------------------------------
0       ...       ...       ...          ...
1       ...       ...       ...          ...

También se calcula el speedup:

Speedup = Tiempo secuencial / Tiempo paralelo

Un valor mayor a 1 indica que la ejecución paralela fue más rápida que la secuencial.

Pruebas realizadas

Se realizaron pruebas para verificar:

Validación de claves incorrectas.
Claves con caracteres no permitidos.
Búsqueda secuencial.
Búsqueda paralela.
Distribución correcta de los rangos.
Identificación del hilo ganador.
Detención de los demás hilos al encontrar la clave.
Comparación de tiempos.
Cálculo del speedup.

Las claves utilizadas son únicamente claves de prueba creadas específicamente para esta práctica y no corresponden a contraseñas o credenciales reales.
