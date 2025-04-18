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

# Verificar Python
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}❌ Error: Python 3 no está instalado. Por favor, instala Python 3.${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Python 3 encontrado.${NC}"

# Crear carpeta de resultados
echo -e "\n${CYAN}[2/5] Preparando carpeta de resultados...${NC}"
mkdir -p "$RESULTS_DIR"
echo -e "${GREEN}✅ Carpeta de resultados configurada.${NC}"

# Crear y configurar entorno virtual de Python
echo -e "\n${CYAN}[3/5] Configurando entorno virtual Python...${NC}"

# Verificar si ya existe el entorno virtual
if [ ! -d "$VENV_DIR" ]; then
    echo "Creando entorno virtual Python en $VENV_DIR..."
    python3 -m venv "$VENV_DIR"
    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Error al crear el entorno virtual Python.${NC}"
        exit 1
    fi
else
    echo "Entorno virtual ya existe, utilizándolo..."
fi

# Activar entorno virtual
echo "Activando entorno virtual..."
source "$VENV_DIR/bin/activate"
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Error al activar el entorno virtual Python.${NC}"
    exit 1
fi

# Instalar dependencias en el entorno virtual
echo "Instalando dependencias de Python en el entorno virtual..."
pip install pandas matplotlib seaborn
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Error al instalar las dependencias de Python.${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Entorno virtual configurado correctamente con todas las dependencias.${NC}"

# Compilar el programa
echo -e "\n${CYAN}[4/5] Compilando el programa...${NC}"
if g++ -std=c++11 taller-5.cpp -o taller-5 -pthread; then
    echo -e "${GREEN}✅ Compilación exitosa.${NC}"
else
    echo -e "${RED}❌ Error durante la compilación.${NC}"
    deactivate # Desactivar entorno virtual antes de salir
    exit 1
fi

# Ejecutar el programa
echo -e "\n${CYAN}[5/5] Ejecutando el programa...${NC}"
echo -e "📊 Este proceso puede tardar varios minutos para matrices grandes."
echo -e "📊 El programa mostrará el progreso en tiempo real.\n"

./taller-5

# Generar gráficos
echo -e "\n${CYAN}Generando gráficos...${NC}"
python generate_charts.py

# Desactivar entorno virtual
deactivate

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