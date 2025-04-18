import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import numpy as np
import matplotlib.gridspec as gridspec
from matplotlib.ticker import FuncFormatter

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
    """Carga datos desde el archivo CSV"""
    if csv_file is None:
        csv_file = os.path.join(RESULTS_DIR, "benchmark_results.csv")
        
    if not os.path.exists(csv_file):
        raise FileNotFoundError(f"El archivo {csv_file} no existe. Ejecuta primero el programa C++.")
    
    # Cargar los datos
    df = pd.read_csv(csv_file)
    
    # Convertir 'No encontrada' a NaN para procesamiento numérico
    df['Distancia Mínima'] = pd.to_numeric(df['Distancia Mínima'], errors='coerce')
    
    # Reemplazar 'N/A' con NaN para procesamiento numérico
    df['Hilos Creados'] = pd.to_numeric(df['Hilos Creados'], errors='coerce')
    df['Speedup'] = pd.to_numeric(df['Speedup'], errors='coerce')
    
    # Asegurar que 'Tiempo (ns)' y 'Tiempo (s)' sean numéricos con precisión
    df['Tiempo (ns)'] = pd.to_numeric(df['Tiempo (ns)'], errors='coerce')
    
    # No recreamos la columna 'Tiempo (s)' porque ya existe en el CSV
    # Además convertimos los valores de Celdas Visitadas y Caminos Podados a numéricos
    df['Celdas Visitadas'] = pd.to_numeric(df['Celdas Visitadas'], errors='coerce')
    df['Caminos Podados'] = pd.to_numeric(df['Caminos Podados'], errors='coerce')
    
    return df

def format_value(x, pos):
    """
    Formatea valores grandes con sufijos apropiados según el tipo de dato.
    Maneja tiempos, recuentos de celdas, rutas y más.
    """
    # Manejar valores NaN
    if pd.isna(x) or np.isnan(x):
        return 'N/A'
    
    # Para tiempos, convertir a la unidad apropiada
    if 'Tiempo' in plt.gca().get_ylabel():
        if x < 1e-6:  # Menos de 1 microsegundo
            return f'{x*1e9:.2f} ns'
        elif x < 1e-3:  # Menos de 1 milisegundo
            return f'{x*1e6:.2f} µs'
        elif x < 1:  # Menos de 1 segundo
            return f'{x*1e3:.2f} ms'
        elif x < 60:  # Menos de 1 minuto
            return f'{x:.2f} s'
        else:  # Minutos o más
            return f'{x/60:.2f} min'
    # Para speedup
    elif 'Speedup' in plt.gca().get_ylabel():
        return f'{x:.2f}x'
    # Para celdas visitadas
    elif 'Celdas Visitadas' in plt.gca().get_ylabel():
        if x >= 1e9:
            return f'{x/1e9:.2f}B'
        elif x >= 1e6:
            return f'{x/1e6:.2f}M'
        elif x >= 1e3:
            return f'{x/1e3:.2f}K'
        else:
            return f'{int(x)}'
    # Para caminos podados
    elif 'Caminos Podados' in plt.gca().get_ylabel():
        if x >= 1e9:
            return f'{x/1e9:.2f}B'
        elif x >= 1e6:
            return f'{x/1e6:.2f}M'
        elif x >= 1e3:
            return f'{x/1e3:.2f}K'
        else:
            return f'{int(x)}'
    # Para hilos creados
    elif 'Hilos Creados' in plt.gca().get_ylabel():
        if x >= 1e9:
            return f'{x/1e9:.2f}B'
        elif x >= 1e6:
            return f'{x/1e6:.2f}M'
        elif x >= 1e3:
            return f'{x/1e3:.2f}K'
        else:
            return f'{int(x)}'
    # Formato predeterminado
    else:
        return f'{x:.2f}'

def create_time_comparison_chart(df, output_file=None):
    """Crea un gráfico comparativo de tiempos entre ejecuciones secuenciales y paralelas."""
    plt.figure(figsize=(20, 12))
    
    # Definir los rangos de tamaño para agrupar
    ranges = [(4, 20), (25, 100), (150, 300), (400, 500)]
    
    # Crear un GridSpec para organizar los subgráficos
    gs = gridspec.GridSpec(2, 2, figure=plt.gcf())
    
    # Para cada rango, crear un subgráfico
    for i, (min_size, max_size) in enumerate(ranges):
        ax = plt.subplot(gs[i//2, i%2])
        
        # Filtrar datos para este rango
        range_data = df[(df['Tamaño de Matriz'] >= min_size) & (df['Tamaño de Matriz'] <= max_size)]
        
        # Si no hay datos para este rango, continuar
        if range_data.empty:
            ax.text(0.5, 0.5, f'No hay datos para matrices {min_size}x{min_size} a {max_size}x{max_size}',
                   horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            continue
        
        # Agrupar por tamaño de matriz y tipo de ejecución
        grouped_data = range_data.groupby(['Tamaño de Matriz', 'Tipo de Ejecución'])['Tiempo (ns)'].mean() / 1e9  # Convertir a segundos
        
        # Reformatear los datos para graficar
        sizes = sorted(range_data['Tamaño de Matriz'].unique())
        seq_times = [grouped_data.get((size, 'Secuencial'), 0) for size in sizes]
        par_times = [grouped_data.get((size, 'Paralelo'), 0) for size in sizes]
        
        # Configurar las posiciones de las barras
        x = np.arange(len(sizes))
        width = 0.35
        
        # Crear las barras
        rects1 = ax.bar(x - width/2, seq_times, width, label='Secuencial', color='skyblue')
        rects2 = ax.bar(x + width/2, par_times, width, label='Paralelo', color='salmon')
        
        # Añadir labels y título
        ax.set_xlabel('Tamaño de la Matriz')
        ax.set_ylabel('Tiempo')
        ax.set_title(f'Comparación de Tiempos: Matrices {min_size}x{min_size} a {max_size}x{max_size}')
        ax.set_xticks(x)
        ax.set_xticklabels([f'{size}x{size}' for size in sizes])
        ax.legend()
        
        # Formatear el eje Y
        ax.yaxis.set_major_formatter(FuncFormatter(format_value))
        
        # Añadir valores en las barras
        for rect in rects1 + rects2:
            height = rect.get_height()
            if not pd.isna(height) and height > 0:
                ax.annotate(format_value(height, 0),
                            xy=(rect.get_x() + rect.get_width() / 2, height),
                            xytext=(0, 3),
                            textcoords="offset points",
                            ha='center', va='bottom', fontsize=8, rotation=0)
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"Gráfico de comparación de tiempos guardado en: {output_file}")
    else:
        plt.show()

def create_speedup_chart(df, output_file=None):
    """Crea un gráfico de speedup para diferentes tamaños de matriz."""
    plt.figure(figsize=(20, 12))
    
    # Definir los rangos de tamaño para agrupar
    ranges = [(4, 20), (25, 100), (150, 300), (400, 500)]
    
    # Crear un GridSpec para organizar los subgráficos
    gs = gridspec.GridSpec(2, 2, figure=plt.gcf())
    
    # Para cada rango, crear un subgráfico
    for i, (min_size, max_size) in enumerate(ranges):
        ax = plt.subplot(gs[i//2, i%2])
        
        # Filtrar datos para este rango
        range_data = df[(df['Tamaño de Matriz'] >= min_size) & (df['Tamaño de Matriz'] <= max_size)]
        
        # Si no hay datos para este rango, continuar
        if range_data.empty:
            ax.text(0.5, 0.5, f'No hay datos para matrices {min_size}x{min_size} a {max_size}x{max_size}',
                   horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            continue
        
        # Calcular el speedup para cada tamaño de matriz
        # Agrupar y calcular tiempos medios
        grouped_times = range_data.groupby(['Tamaño de Matriz', 'Tipo de Ejecución'])['Tiempo (ns)'].mean().unstack()
        
        # Verificar que tenemos los dos tipos de ejecución
        if 'Secuencial' not in grouped_times.columns or 'Paralelo' not in grouped_times.columns:
            ax.text(0.5, 0.5, 'Faltan datos de ejecución secuencial o paralela',
                horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            continue
        
        speedups = grouped_times['Secuencial'] / grouped_times['Paralelo']
        
        # Gráfico de barras para el speedup
        bars = speedups.plot(kind='bar', ax=ax, color='lightgray')
        
        # Colorear barras según el valor de speedup
        for j, bar in enumerate(bars.patches):
            if speedups.iloc[j] > 1:  # Mejora de rendimiento (verde)
                bar.set_color('green')
                bar.set_alpha(min(speedups.iloc[j]/5, 0.9))  # Transparencia basada en magnitud
            else:  # Empeoramiento (rojo)
                bar.set_color('red')
                bar.set_alpha(0.6)
        
        # Añadir una línea de referencia en speedup = 1
        ax.axhline(y=1, color='black', linestyle='--', alpha=0.7)
        
        # Añadir etiquetas y título
        ax.set_xlabel('Tamaño de la Matriz')
        ax.set_ylabel('Speedup')
        ax.set_title(f'Speedup: Matrices {min_size}x{min_size} a {max_size}x{max_size}')
        ax.set_xticklabels([f'{size}x{size}' for size in speedups.index], rotation=45)
        
        # Formatear el eje Y
        ax.yaxis.set_major_formatter(FuncFormatter(format_value))
        
        # Añadir valores en las barras con colores según speedup
        for j, v in enumerate(speedups):
            if not pd.isna(v):
                color = 'darkgreen' if v > 1 else 'darkred'
                weight = 'bold' if v > 5 else 'normal'
                ax.text(j, v + 0.1, format_value(v, 0), ha='center', fontsize=9, 
                       color=color, weight=weight)
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"Gráfico de speedup guardado en: {output_file}")
    else:
        plt.show()

def create_visited_cells_chart(df, output_file=None):
    """Crea un gráfico de celdas visitadas para diferentes tamaños de matriz y tipos de ejecución."""
    plt.figure(figsize=(20, 12))
    
    # Definir los rangos de tamaño para agrupar
    ranges = [(4, 20), (25, 100), (150, 300), (400, 500)]
    
    # Crear un GridSpec para organizar los subgráficos
    gs = gridspec.GridSpec(2, 2, figure=plt.gcf())
    
    # Para cada rango, crear un subgráfico
    for i, (min_size, max_size) in enumerate(ranges):
        ax = plt.subplot(gs[i//2, i%2])
        
        # Filtrar datos para este rango
        range_data = df[(df['Tamaño de Matriz'] >= min_size) & (df['Tamaño de Matriz'] <= max_size)]
        
        # Si no hay datos en este rango, continuar
        if range_data.empty:
            ax.text(0.5, 0.5, f'No hay datos para matrices {min_size}x{min_size} a {max_size}x{max_size}',
                   horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            continue
        
        # Agrupar por tamaño de matriz y tipo de ejecución
        grouped_data = range_data.groupby(['Tamaño de Matriz', 'Tipo de Ejecución'])['Celdas Visitadas'].mean()
        
        # Reformatear los datos para graficar
        sizes = sorted(range_data['Tamaño de Matriz'].unique())
        seq_cells = [grouped_data.get((size, 'Secuencial'), 0) for size in sizes]
        par_cells = [grouped_data.get((size, 'Paralelo'), 0) for size in sizes]
        
        # Configurar las posiciones de las barras
        x = np.arange(len(sizes))
        width = 0.35
        
        # Crear las barras
        rects1 = ax.bar(x - width/2, seq_cells, width, label='Secuencial', color='skyblue')
        rects2 = ax.bar(x + width/2, par_cells, width, label='Paralelo', color='salmon')
        
        # Añadir labels y título
        ax.set_xlabel('Tamaño de la Matriz')
        ax.set_ylabel('Celdas Visitadas')
        ax.set_title(f'Celdas Visitadas: Matrices {min_size}x{min_size} a {max_size}x{max_size}')
        ax.set_xticks(x)
        ax.set_xticklabels([f'{size}x{size}' for size in sizes])
        ax.legend()
        
        # Formatear el eje Y
        ax.yaxis.set_major_formatter(FuncFormatter(format_value))
        
        # Añadir valores en las barras
        for rect in rects1 + rects2:
            height = rect.get_height()
            if not pd.isna(height) and height > 0:
                ax.annotate(format_value(height, 0),
                            xy=(rect.get_x() + rect.get_width() / 2, height),
                            xytext=(0, 3),
                            textcoords="offset points",
                            ha='center', va='bottom', fontsize=8, rotation=0)
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"Gráfico de celdas visitadas guardado en: {output_file}")
    else:
        plt.show()

def create_pruned_paths_chart(df, output_file=None):
    """Crea un gráfico de caminos podados para diferentes tamaños de matriz y tipos de ejecución."""
    plt.figure(figsize=(20, 12))
    
    # Definir los rangos de tamaño para agrupar
    ranges = [(4, 20), (25, 100), (150, 300), (400, 500)]
    
    # Crear un GridSpec para organizar los subgráficos
    gs = gridspec.GridSpec(2, 2, figure=plt.gcf())
    
    # Para cada rango, crear un subgráfico
    for i, (min_size, max_size) in enumerate(ranges):
        ax = plt.subplot(gs[i//2, i%2])
        
        # Filtrar datos para este rango
        range_data = df[(df['Tamaño de Matriz'] >= min_size) & (df['Tamaño de Matriz'] <= max_size)]
        
        # Si no hay datos en este rango, continuar
        if range_data.empty:
            ax.text(0.5, 0.5, f'No hay datos para matrices {min_size}x{min_size} a {max_size}x{max_size}',
                   horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            continue
        
        # Agrupar por tamaño de matriz y tipo de ejecución
        grouped_data = range_data.groupby(['Tamaño de Matriz', 'Tipo de Ejecución'])['Caminos Podados'].mean()
        
        # Reformatear los datos para graficar
        sizes = sorted(range_data['Tamaño de Matriz'].unique())
        seq_paths = [grouped_data.get((size, 'Secuencial'), 0) for size in sizes]
        par_paths = [grouped_data.get((size, 'Paralelo'), 0) for size in sizes]
        
        # Configurar las posiciones de las barras
        x = np.arange(len(sizes))
        width = 0.35
        
        # Crear las barras
        rects1 = ax.bar(x - width/2, seq_paths, width, label='Secuencial', color='skyblue')
        rects2 = ax.bar(x + width/2, par_paths, width, label='Paralelo', color='salmon')
        
        # Añadir labels y título
        ax.set_xlabel('Tamaño de la Matriz')
        ax.set_ylabel('Caminos Podados')
        ax.set_title(f'Caminos Podados: Matrices {min_size}x{min_size} a {max_size}x{max_size}')
        ax.set_xticks(x)
        ax.set_xticklabels([f'{size}x{size}' for size in sizes])
        ax.legend()
        
        # Formatear el eje Y
        ax.yaxis.set_major_formatter(FuncFormatter(format_value))
        
        # Añadir valores en las barras
        for rect in rects1 + rects2:
            height = rect.get_height()
            if not pd.isna(height) and height > 0:
                ax.annotate(format_value(height, 0),
                            xy=(rect.get_x() + rect.get_width() / 2, height),
                            xytext=(0, 3),
                            textcoords="offset points",
                            ha='center', va='bottom', fontsize=8, rotation=0)
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"Gráfico de caminos podados guardado en: {output_file}")
    else:
        plt.show()

def create_threads_chart(df, output_file=None):
    """Crea un gráfico de hilos creados para diferentes tamaños de matriz."""
    plt.figure(figsize=(20, 12))
    
    # Filtrar solo datos de ejecución paralela
    parallel_data = df[df['Tipo de Ejecución'] == 'Paralelo']
    
    # Definir los rangos de tamaño para agrupar
    ranges = [(4, 20), (25, 100), (150, 300), (400, 500)]
    
    # Crear un GridSpec para organizar los subgráficos
    gs = gridspec.GridSpec(2, 2, figure=plt.gcf())
    
    # Para cada rango, crear un subgráfico
    for i, (min_size, max_size) in enumerate(ranges):
        ax = plt.subplot(gs[i//2, i%2])
        
        # Filtrar datos para este rango
        range_data = parallel_data[(parallel_data['Tamaño de Matriz'] >= min_size) & 
                                  (parallel_data['Tamaño de Matriz'] <= max_size)]
        
        # Si no hay datos para este rango, continuar
        if range_data.empty:
            ax.text(0.5, 0.5, f'No hay datos para matrices {min_size}x{min_size} a {max_size}x{max_size}',
                   horizontalalignment='center', verticalalignment='center', transform=ax.transAxes)
            continue
        
        # Agrupar por tamaño de matriz
        grouped_data = range_data.groupby('Tamaño de Matriz')['Hilos Creados'].mean()
        
        # Gráfico de barras
        bars = grouped_data.plot(kind='bar', ax=ax, color='green')
        
        # Añadir etiquetas y título
        ax.set_xlabel('Tamaño de la Matriz')
        ax.set_ylabel('Hilos Creados')
        ax.set_title(f'Hilos Creados: Matrices {min_size}x{min_size} a {max_size}x{max_size}')
        ax.set_xticklabels([f'{size}x{size}' for size in grouped_data.index], rotation=45)
        
        # Formatear el eje Y
        ax.yaxis.set_major_formatter(FuncFormatter(format_value))
        
        # Añadir valores en las barras
        for j, v in enumerate(grouped_data):
            if not pd.isna(v) and v > 0:
                ax.text(j, v + 0.1, format_value(v, 0), ha='center', fontsize=8)
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"Gráfico de hilos creados guardado en: {output_file}")
    else:
        plt.show()

def create_combined_metrics_chart(df, output_file=None):
    """Create a chart showing multiple metrics in one figure"""
    if output_file is None:
        output_file = os.path.join(RESULTS_DIR, "combined_metrics.png")
    
    # Get max size to determine if multiple charts are needed
    max_size = df['Tamaño de Matriz'].max()
    
    if max_size > 30:
        # Create multiple charts for different ranges
        ranges = [(2, 15), (16, 30), (31, max_size)]
        for i, (min_range, max_range) in enumerate(ranges):
            # Filter data for this range
            range_df = df[df['Tamaño de Matriz'].between(min_range, max_range)]
            
            range_output_file = os.path.join(RESULTS_DIR, f"combined_metrics_{min_range}_to_{max_range}.png")
            create_combined_range_chart(range_df, min_range, max_range, range_output_file)
    else:
        create_combined_range_chart(df, 2, max_size, output_file)

def create_combined_range_chart(df, min_range, max_range, output_file):
    """Create a combined chart for a specific range of matrix sizes"""
    # Create figure with subplots
    fig, axes = plt.subplots(2, 2, figsize=(18, 14))
    fig.suptitle(f'Métricas Combinadas - Matrices de {min_range}x{min_range} a {max_range}x{max_range}', fontsize=18)
    
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
    axes[0, 0].set_ylabel('Tiempo', fontsize=12)
    axes[0, 0].tick_params(axis='x', labelrotation=90 if max_range-min_range > 20 else 0)
    
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
    axes[0, 1].tick_params(axis='x', labelrotation=90 if max_range-min_range > 20 else 0)
    
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
    axes[1, 0].tick_params(axis='x', labelrotation=90 if max_range-min_range > 20 else 0)
    
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
    axes[1, 1].tick_params(axis='x', labelrotation=90 if max_range-min_range > 20 else 0)
    
    # Add value labels with appropriate formatting
    for container in axes[0, 0].containers:
        labels = [format_value(v, None) for v in container.datavalues]
        axes[0, 0].bar_label(container, labels=labels, fontsize=7, rotation=90 if max_range-min_range > 20 else 0)
    
    # Add value labels on bars for speedup
    for container in axes[0, 1].containers:
        labels = [f"{v:.2f}x" for v in container.datavalues]
        axes[0, 1].bar_label(container, labels=labels, fontsize=7, rotation=90 if max_range-min_range > 20 else 0)
    
    # Add value labels on bars for visited cells
    for container in axes[1, 0].containers:
        axes[1, 0].bar_label(container, fontsize=7, rotation=90 if max_range-min_range > 20 else 0)
    
    # Add value labels on bars for pruned paths
    for container in axes[1, 1].containers:
        axes[1, 1].bar_label(container, fontsize=7, rotation=90 if max_range-min_range > 20 else 0)
    
    # Adjust layout
    plt.tight_layout()
    fig.subplots_adjust(top=0.92)
    
    # Save figure
    plt.savefig(output_file)
    print(f"Gráfico combinado guardado como {output_file}")

def create_summary_chart(df, output_file=None):
    """Crea un gráfico de resumen que muestra el comportamiento de speedup, tiempo y métricas por rangos."""
    plt.figure(figsize=(24, 16))
    
    # Crear una figura de 2x2 para mostrar diferentes métricas
    gs = gridspec.GridSpec(2, 2, figure=plt.gcf())
    
    # 1. Gráfico de Speedup por rangos
    ax1 = plt.subplot(gs[0, 0])
    speedup_data = df[df['Tipo de Ejecución'] == 'Paralelo'].copy()
    
    # Definir rangos de tamaño de matriz
    bins = [0, 10, 20, 30, 40, 50, 60, 70]
    labels = ['1-10', '11-20', '21-30', '31-40', '41-50', '51-60', '61-70']
    
    speedup_data['Rango'] = pd.cut(speedup_data['Tamaño de Matriz'], bins=bins, labels=labels, right=False)
    
    # Calcular speedup promedio por rango
    speedup_avg = speedup_data.groupby('Rango', observed=True)['Speedup'].mean()
    
    # Graficar barras de speedup
    bars = speedup_avg.plot(kind='bar', ax=ax1, color='lightgray')
    
    # Colorear barras según el valor de speedup
    for j, bar in enumerate(bars.patches):
        value = speedup_avg.iloc[j]
        if not pd.isna(value):
            if value > 1:  # Mejora de rendimiento (verde)
                bar.set_color('green')
                bar.set_alpha(min(value/5, 0.9))  # Transparencia basada en magnitud
            else:  # Empeoramiento (rojo)
                bar.set_color('red')
                bar.set_alpha(0.6)
    
    # Añadir línea de referencia en speedup = 1
    ax1.axhline(y=1, color='black', linestyle='--', alpha=0.7)
    
    # Configurar gráfico
    ax1.set_title('Speedup Promedio por Rango de Tamaño')
    ax1.set_xlabel('Rango de Tamaño de Matriz')
    ax1.set_ylabel('Speedup Promedio')
    
    # Añadir valores
    for j, v in enumerate(speedup_avg):
        if not pd.isna(v):
            color = 'darkgreen' if v > 1 else 'darkred'
            weight = 'bold' if v > 5 else 'normal'
            ax1.text(j, v + 0.1, format_value(v, 0), ha='center', fontsize=10, 
                   color=color, weight=weight)
    
    # 2. Gráfico de tiempos secuenciales vs paralelos por rango
    ax2 = plt.subplot(gs[0, 1])
    
    # Preparar datos para tiempos
    time_data = df.copy()
    time_data['Rango'] = pd.cut(time_data['Tamaño de Matriz'], bins=bins, labels=labels, right=False)
    
    # Calcular tiempos promedio por rango y tipo de ejecución
    time_avg = time_data.groupby(['Rango', 'Tipo de Ejecución'], observed=True)['Tiempo (s)'].mean().unstack()
    
    # Graficar barras de tiempo
    time_avg.plot(kind='bar', ax=ax2)
    
    # Configurar gráfico
    ax2.set_title('Tiempo de Ejecución Promedio por Rango de Tamaño')
    ax2.set_xlabel('Rango de Tamaño de Matriz')
    ax2.set_ylabel('Tiempo Promedio (s)')
    ax2.set_yscale('log')  # Escala logarítmica para mejor visualización
    ax2.yaxis.set_major_formatter(FuncFormatter(format_value))
    
    # 3. Gráfico de celdas visitadas por rango
    ax3 = plt.subplot(gs[1, 0])
    
    # Calcular celdas visitadas promedio por rango y tipo de ejecución
    cells_avg = time_data.groupby(['Rango', 'Tipo de Ejecución'], observed=True)['Celdas Visitadas'].mean().unstack()
    
    # Graficar barras de celdas visitadas
    cells_avg.plot(kind='bar', ax=ax3)
    
    # Configurar gráfico
    ax3.set_title('Celdas Visitadas Promedio por Rango de Tamaño')
    ax3.set_xlabel('Rango de Tamaño de Matriz')
    ax3.set_ylabel('Celdas Visitadas Promedio')
    ax3.set_yscale('log')  # Escala logarítmica para mejor visualización
    ax3.yaxis.set_major_formatter(FuncFormatter(format_value))
    
    # 4. Gráfico de hilos creados por rango
    ax4 = plt.subplot(gs[1, 1])
    
    # Calcular hilos creados promedio por rango para ejecuciones paralelas
    threads_data = time_data[time_data['Tipo de Ejecución'] == 'Paralelo'].copy()
    threads_avg = threads_data.groupby('Rango', observed=True)['Hilos Creados'].mean()
    
    # Graficar barras de hilos creados
    threads_avg.plot(kind='bar', ax=ax4, color='purple')
    
    # Configurar gráfico
    ax4.set_title('Hilos Creados Promedio por Rango de Tamaño')
    ax4.set_xlabel('Rango de Tamaño de Matriz')
    ax4.set_ylabel('Hilos Creados Promedio')
    ax4.set_yscale('log')  # Escala logarítmica para mejor visualización
    ax4.yaxis.set_major_formatter(FuncFormatter(format_value))
    
    # Ajustar layout
    plt.tight_layout()
    plt.suptitle('Análisis de Rendimiento por Rangos: Algoritmo Branch & Bound Secuencial vs Paralelo', fontsize=16, y=0.99)
    plt.subplots_adjust(top=0.95)
    
    # Guardar o mostrar
    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"Gráfico de resumen guardado en: {output_file}")
    else:
        plt.show()

def main():
    """Función principal que coordina la generación de gráficos"""
    # Asegurar que existe el directorio de resultados
    ensure_results_directory_exists()
    
    # Cargar datos
    try:
        df = load_data()
        print("Datos cargados correctamente del archivo CSV.")
        
        # Mostrar información básica del DataFrame
        print("\nInformación básica del DataFrame:")
        print(f"Total de filas: {len(df)}")
        print(f"Columnas: {df.columns.tolist()}")
        print(f"Tamaños de matriz: {sorted(df['Tamaño de Matriz'].unique())}")
        print(f"Tipos de ejecución: {df['Tipo de Ejecución'].unique()}")
        
        # Imprimir información sobre valores de tiempo para depuración
        print("\nEjemplos de valores de tiempo:")
        print(df[['Tamaño de Matriz', 'Tipo de Ejecución', 'Tiempo (ns)', 'Tiempo (s)']].head(10).to_string())
        
        # Crear gráficos
        print("\nGenerando gráficos...")
        
        output_dir = os.path.join(RESULTS_DIR, "charts")
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        
        # Crear gráficos individuales
        create_time_comparison_chart(df, os.path.join(output_dir, "tiempo_comparacion.png"))
        create_speedup_chart(df, os.path.join(output_dir, "speedup.png"))
        create_visited_cells_chart(df, os.path.join(output_dir, "celdas_visitadas.png"))
        create_pruned_paths_chart(df, os.path.join(output_dir, "caminos_podados.png"))
        create_threads_chart(df, os.path.join(output_dir, "hilos_creados.png"))
        
        # Crear gráfico de resumen
        create_summary_chart(df, os.path.join(output_dir, "resumen.png"))
        
        print("\nTodos los gráficos han sido generados exitosamente.")
        print(f"Los gráficos están disponibles en el directorio: {output_dir}")
        
    except Exception as e:
        print(f"Error: {str(e)}")
        import traceback
        traceback.print_exc()
        print("Asegúrate de ejecutar primero el programa C++ para generar el archivo CSV correctamente.")

if __name__ == "__main__":
    main() 