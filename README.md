# Taller 5 - Algoritmos Paralelos

Este proyecto implementa una solución paralela para el problema del camino más corto usando backtracking.

## Requisitos

- macOS (probado en macOS 24.4.0)
- Compilador g++ con soporte para C++11
- Terminal con soporte para bash

## Instrucciones de Ejecución

1. Abre una terminal en el directorio del proyecto
2. Dale permisos de ejecución al script:
   ```bash
   chmod +x start.sh
   ```
3. Ejecuta el programa:
   ```bash
   ./start.sh
   ```

## Descripción del Programa

El programa realiza las siguientes operaciones:
- Genera matrices de costos aleatorias de diferentes tamaños (desde 2x2 hasta 14x14)
- Resuelve el problema del camino más corto usando dos enfoques:
  - Backtracking secuencial
  - Backtracking paralelo
- Compara los tiempos de ejecución entre ambos enfoques
- Muestra un resumen de los resultados

## Salida del Programa

El programa mostrará:
- Las matrices de costos (para tamaños pequeños)
- La distancia mínima encontrada
- El tiempo de ejecución para cada enfoque
- El speedup obtenido (cuando aplicable)
- Un resumen final con todos los tiempos

## Notas

- Para matrices pequeñas (hasta 8x8), el tiempo secuencial puede mostrar 0ms debido a la alta velocidad de ejecución
- La versión paralela tiene un overhead significativo debido a la creación de threads
- Los resultados pueden variar entre ejecuciones debido a la naturaleza aleatoria de las matrices generadas
- El tamaño máximo de las matrices se puede ajustar modificando la constante MAX_SIZE en el código fuente 