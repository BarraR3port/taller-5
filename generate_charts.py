import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Set style for plots
plt.style.use('ggplot')
sns.set_theme(style="whitegrid")

# Directory for results
RESULTS_DIR = "results"

def ensure_results_directory_exists():
    """Ensure the results directory exists"""
    if not os.path.exists(RESULTS_DIR):
        try:
            os.makedirs(RESULTS_DIR)
            print(f"Directorio {RESULTS_DIR} creado.")
        except Exception as e:
            print(f"Error al crear el directorio {RESULTS_DIR}: {str(e)}")
            exit(1)

def load_data(csv_file=None):
    """Load data from CSV file"""
    if csv_file is None:
        csv_file = os.path.join(RESULTS_DIR, "benchmark_results.csv")
        
    if not os.path.exists(csv_file):
        raise FileNotFoundError(f"El archivo {csv_file} no existe. Ejecuta primero el programa C++.")
    
    # Load the data
    df = pd.read_csv(csv_file)
    
    # Convert 'No encontrada' to NaN for numerical processing
    df['Distancia Mínima'] = pd.to_numeric(df['Distancia Mínima'], errors='coerce')
    
    # Replace 'N/A' with NaN for numerical processing
    df['Hilos Creados'] = pd.to_numeric(df['Hilos Creados'], errors='coerce')
    df['Speedup'] = pd.to_numeric(df['Speedup'], errors='coerce')
    
    return df

def create_time_comparison_chart(df, output_file=None):
    """Create a chart comparing execution time for sequential vs parallel by matrix size"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "time_comparison.png")
        
    plt.figure(figsize=(12, 8))
    
    # Plot data
    sns.barplot(
        x='Tamaño de Matriz',
        y='Tiempo (s)',
        hue='Tipo de Ejecución',
        data=df
    )
    
    plt.title('Comparación de Tiempo de Ejecución: Secuencial vs Paralelo', fontsize=16)
    plt.xlabel('Tamaño de Matriz', fontsize=14)
    plt.ylabel('Tiempo (segundos)', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.legend(title='Tipo de Ejecución', fontsize=12)
    
    # Add value labels on bars
    for container in plt.gca().containers:
        plt.gca().bar_label(container, fmt='%.2f', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_file)
    print(f"Gráfico guardado como {output_file}")

def create_speedup_chart(df, output_file=None):
    """Create a chart showing speedup by matrix size"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "speedup_chart.png")
        
    # Filter only parallel execution data for speedup
    df_parallel = df[df['Tipo de Ejecución'] == 'Paralelo']
    
    plt.figure(figsize=(12, 8))
    
    # Plot data
    bar = sns.barplot(
        x='Tamaño de Matriz',
        y='Speedup',
        data=df_parallel,
        color='orange'
    )
    
    plt.title('Speedup por Tamaño de Matriz (Paralelo vs Secuencial)', fontsize=16)
    plt.xlabel('Tamaño de Matriz', fontsize=14)
    plt.ylabel('Speedup (x veces)', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    
    # Add value labels on bars
    for container in plt.gca().containers:
        plt.gca().bar_label(container, fmt='%.2f', fontsize=10)
    
    # Add horizontal line at y=1 (no speedup)
    plt.axhline(y=1, color='red', linestyle='--', label='No Speedup')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig(output_file)
    print(f"Gráfico guardado como {output_file}")

def create_visited_cells_chart(df, output_file=None):
    """Create a chart comparing visited cells for sequential vs parallel by matrix size"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "visited_cells_chart.png")
        
    plt.figure(figsize=(12, 8))
    
    # Plot data
    sns.barplot(
        x='Tamaño de Matriz',
        y='Celdas Visitadas',
        hue='Tipo de Ejecución',
        data=df
    )
    
    plt.title('Comparación de Celdas Visitadas: Secuencial vs Paralelo', fontsize=16)
    plt.xlabel('Tamaño de Matriz', fontsize=14)
    plt.ylabel('Celdas Visitadas', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.legend(title='Tipo de Ejecución', fontsize=12)
    
    # Add value labels on bars (only showing numbers of thousands or more)
    for container in plt.gca().containers:
        plt.gca().bar_label(container, fmt='%.0f', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_file)
    print(f"Gráfico guardado como {output_file}")

def create_pruned_paths_chart(df, output_file=None):
    """Create a chart comparing pruned paths for sequential vs parallel by matrix size"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "pruned_paths_chart.png")
        
    plt.figure(figsize=(12, 8))
    
    # Plot data
    sns.barplot(
        x='Tamaño de Matriz',
        y='Caminos Podados',
        hue='Tipo de Ejecución',
        data=df
    )
    
    plt.title('Comparación de Caminos Podados: Secuencial vs Paralelo', fontsize=16)
    plt.xlabel('Tamaño de Matriz', fontsize=14)
    plt.ylabel('Caminos Podados', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.legend(title='Tipo de Ejecución', fontsize=12)
    
    # Add value labels on bars (only showing numbers of thousands or more)
    for container in plt.gca().containers:
        plt.gca().bar_label(container, fmt='%.0f', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_file)
    print(f"Gráfico guardado como {output_file}")

def create_threads_chart(df, output_file=None):
    """Create a chart showing threads created by matrix size"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "threads_chart.png")
        
    # Filter only parallel execution data for threads
    df_parallel = df[df['Tipo de Ejecución'] == 'Paralelo']
    
    plt.figure(figsize=(12, 8))
    
    # Plot data
    sns.barplot(
        x='Tamaño de Matriz',
        y='Hilos Creados',
        data=df_parallel,
        color='green'
    )
    
    plt.title('Hilos Creados por Tamaño de Matriz', fontsize=16)
    plt.xlabel('Tamaño de Matriz', fontsize=14)
    plt.ylabel('Número de Hilos', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    
    # Add value labels on bars
    for container in plt.gca().containers:
        plt.gca().bar_label(container, fmt='%.0f', fontsize=10)
    
    plt.tight_layout()
    plt.savefig(output_file)
    print(f"Gráfico guardado como {output_file}")

def create_combined_metrics_chart(df, output_file=None):
    """Create a chart showing multiple metrics in one figure"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "combined_metrics.png")
    
    # Create figure with subplots
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    fig.suptitle('Métricas Combinadas - Secuencial vs Paralelo', fontsize=18)
    
    # Time comparison
    sns.barplot(
        x='Tamaño de Matriz',
        y='Tiempo (s)',
        hue='Tipo de Ejecución',
        data=df,
        ax=axes[0, 0]
    )
    axes[0, 0].set_title('Tiempo de Ejecución', fontsize=14)
    axes[0, 0].set_xlabel('Tamaño de Matriz', fontsize=12)
    axes[0, 0].set_ylabel('Tiempo (segundos)', fontsize=12)
    
    # Speedup (only for parallel)
    df_parallel = df[df['Tipo de Ejecución'] == 'Paralelo']
    sns.barplot(
        x='Tamaño de Matriz',
        y='Speedup',
        data=df_parallel,
        color='orange',
        ax=axes[0, 1]
    )
    axes[0, 1].set_title('Speedup', fontsize=14)
    axes[0, 1].set_xlabel('Tamaño de Matriz', fontsize=12)
    axes[0, 1].set_ylabel('Speedup (x veces)', fontsize=12)
    axes[0, 1].axhline(y=1, color='red', linestyle='--')
    
    # Visited cells
    sns.barplot(
        x='Tamaño de Matriz',
        y='Celdas Visitadas',
        hue='Tipo de Ejecución',
        data=df,
        ax=axes[1, 0]
    )
    axes[1, 0].set_title('Celdas Visitadas', fontsize=14)
    axes[1, 0].set_xlabel('Tamaño de Matriz', fontsize=12)
    axes[1, 0].set_ylabel('Celdas Visitadas', fontsize=12)
    
    # Pruned paths
    sns.barplot(
        x='Tamaño de Matriz',
        y='Caminos Podados',
        hue='Tipo de Ejecución',
        data=df,
        ax=axes[1, 1]
    )
    axes[1, 1].set_title('Caminos Podados', fontsize=14)
    axes[1, 1].set_xlabel('Tamaño de Matriz', fontsize=12)
    axes[1, 1].set_ylabel('Caminos Podados', fontsize=12)
    
    # Adjust layout
    plt.tight_layout()
    fig.subplots_adjust(top=0.9)
    
    # Save figure
    plt.savefig(output_file)
    print(f"Gráfico combinado guardado como {output_file}")

def main():
    # Ensure results directory exists
    ensure_results_directory_exists()
    
    # Load data
    try:
        df = load_data()
        print("Datos cargados correctamente del archivo CSV.")
        
        # Create individual charts
        create_time_comparison_chart(df)
        create_speedup_chart(df)
        create_visited_cells_chart(df)
        create_pruned_paths_chart(df)
        create_threads_chart(df)
        
        # Create combined chart
        create_combined_metrics_chart(df)
        
        print("\nTodos los gráficos han sido generados exitosamente.")
        print(f"Abre los archivos PNG en la carpeta '{RESULTS_DIR}' para ver los resultados o utiliza Excel para visualizar los datos CSV.")
        
    except Exception as e:
        print(f"Error: {str(e)}")
        print("Asegúrate de ejecutar primero el programa C++ para generar el archivo CSV.")

if __name__ == "__main__":
    main() 