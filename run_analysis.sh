#!/bin/bash

# Colores para la terminal
YELLOW='\033[1;33m'
GREEN='\033[1;32m'
CYAN='\033[1;36m'
RED='\033[1;31m'
NC='\033[0m' # No Color

# Nombre del entorno virtual
VENV_DIR="venv"

# Carpeta de resultados
RESULTS_DIR="results"

# Encabezado
echo -e "${YELLOW}==================================================${NC}"
echo -e "${YELLOW}  ANÁLISIS DE RENDIMIENTO: BACKTRACKING SECUENCIAL VS PARALELO ${NC}"
echo -e "${YELLOW}==================================================${NC}"

# Verificar requisitos
echo -e "\n${CYAN}[1/5] Verificando requisitos...${NC}"

# Verificar compilador de C++
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}❌ Error: g++ no está instalado. Por favor, instala un compilador de C++.${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Compilador g++ encontrado.${NC}"

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew is not installed. Please install Homebrew from https://brew.sh/"
    exit 1
fi
echo "✅ Homebrew found."

echo "Installing GCC with OpenMP support and libomp..."
brew install gcc libomp

# Crear carpeta de resultados
echo -e "\n${CYAN}[2/5] Preparando carpeta de resultados...${NC}"
mkdir -p "$RESULTS_DIR"
echo -e "${GREEN}✅ Carpeta de resultados configurada.${NC}"

# Compile the program
echo "Compiling program..."
g++-14 -fopenmp -std=c++17 taller-5.cpp -o taller-5

# Run the program
echo "Running program..."
./taller-5

echo "Program completed."

# Mensaje final
echo -e "\n${GREEN}==================================================${NC}"
echo -e "${GREEN}  ¡ANÁLISIS COMPLETADO!${NC}"
echo -e "${GREEN}==================================================${NC}"
echo -e "\nSe han generado los siguientes archivos en la carpeta ${YELLOW}$RESULTS_DIR/${NC}:"
echo -e "  - benchmark_results.csv (datos en formato CSV)"
echo -e "  - time_comparison.png (comparación de tiempos)"
echo -e "  - speedup_chart.png (aceleración lograda)"
echo -e "  - visited_cells_chart.png (celdas visitadas)"
echo -e "  - pruned_paths_chart.png (caminos podados)"
echo -e "  - threads_chart.png (hilos creados)"
echo -e "  - combined_metrics.png (métricas combinadas)"
echo -e "\nPuede abrir los archivos PNG para ver los gráficos o importar el CSV en Excel/Google Sheets para análisis adicionales." 