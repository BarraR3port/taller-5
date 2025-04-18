# Análisis de Rendimiento: Algoritmo de Backtracking Secuencial vs Paralelo

Este proyecto analiza y compara el rendimiento de algoritmos de backtracking en implementaciones secuenciales y paralelas, visualizando los resultados a través de gráficos generados automáticamente.

## Requisitos

### Para compilar y ejecutar el programa C++:
- Compilador de C++ con soporte para C++11 o superior
- Soporte para hilos (threading)

### Para generar los gráficos:
- Python 3.6 o superior
- Bibliotecas de Python:
  - pandas
  - matplotlib
  - seaborn

## Organización de archivos

El proyecto está organizado de la siguiente manera:
- `taller-5.cpp`: Código fuente principal
- `generate_charts.py`: Script para generar gráficos
- `run_analysis.sh`: Script de automatización
- `results/`: Carpeta donde se guardan todos los resultados (CSV y PNG)

## Forma rápida (recomendada)

El proyecto incluye un script de automatización que configura todo lo necesario y ejecuta el análisis completo:

```bash
# Dar permisos de ejecución al script
chmod +x run_analysis.sh

# Ejecutar el script
./run_analysis.sh
```

Este script:
1. Verifica los requisitos del sistema
2. Prepara la carpeta de resultados
3. Crea un entorno virtual de Python
4. Instala las dependencias necesarias automáticamente
5. Compila y ejecuta el programa C++
6. Genera todos los gráficos en la carpeta `results/`
7. Limpia el entorno al finalizar

## Instalación manual

Si prefiere realizar los pasos manualmente:

### 1. Crear carpeta de resultados

```bash
mkdir -p results
```

### 2. Configurar entorno virtual Python

```bash
# Crear entorno virtual
python3 -m venv venv

# Activar entorno virtual
source venv/bin/activate

# Instalar dependencias
pip install pandas matplotlib seaborn
```

### 3. Compilar y ejecutar el programa C++

```bash
g++ -std=c++11 taller-5.cpp -o taller-5 -pthread
./taller-5
```

Este programa:
- Prueba el algoritmo de búsqueda de camino mínimo en matrices de tamaño 5x5 hasta 10x10
- Ejecuta versiones secuenciales y paralelas del algoritmo
- Recopila y muestra métricas como tiempo de ejecución, caminos podados, celdas visitadas, etc.
- Genera un archivo CSV (`results/benchmark_results.csv`) con todos los resultados

### 4. Generar gráficos a partir de los resultados

Una vez ejecutado el programa C++, ejecute el script Python para generar los gráficos:

```bash
# Asegúrese de que el entorno virtual esté activado
python generate_charts.py

# Desactivar el entorno virtual cuando termine
deactivate
```

## Resultados generados

Todos los resultados se guardan en la carpeta `results/`:

- `benchmark_results.csv`: Datos en formato CSV con todas las métricas
- `time_comparison.png`: Comparación de tiempos de ejecución entre implementaciones secuenciales y paralelas
- `speedup_chart.png`: Aceleración (speedup) lograda por la implementación paralela
- `visited_cells_chart.png`: Comparación de celdas visitadas
- `pruned_paths_chart.png`: Comparación de caminos podados
- `threads_chart.png`: Número de hilos creados en la implementación paralela
- `combined_metrics.png`: Gráfico combinado con todas las métricas principales

## Explicación de los resultados

### Tiempo de ejecución
Compara directamente el tiempo necesario para completar la búsqueda entre las implementaciones secuencial y paralela.

### Speedup
Muestra la ganancia de rendimiento de la implementación paralela, calculada como:
```
Speedup = Tiempo secuencial / Tiempo paralelo
```
Un valor mayor a 1 indica que la versión paralela es más rápida.

### Celdas visitadas
Compara el número de celdas visitadas por cada implementación. La versión paralela podría visitar más celdas debido a que múltiples hilos exploran diferentes caminos simultáneamente.

### Caminos podados
Muestra los caminos que fueron desestimados durante la búsqueda por no ser óptimos. Un mayor número indica una exploración más amplia del espacio de búsqueda.

## Notas importantes

- Para matrices grandes, el proceso puede tardar significativamente. El programa muestra el progreso en tiempo real.
- Los resultados pueden variar según la capacidad de procesamiento de su sistema (número de núcleos, velocidad, etc.).
- Para obtener mediciones más precisas, cierre otras aplicaciones que consuman recursos significativos durante la ejecución.
- La carpeta `results/` está incluida en el archivo `.gitignore` para evitar subir los resultados al repositorio. 