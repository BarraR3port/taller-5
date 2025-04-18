#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <fstream> // For CSV file output
#include <sys/stat.h> // Para stat() en lugar de filesystem
#include <cerrno> // Para códigos de error

using namespace std;
using namespace std::chrono;

// Variables globales
atomic<int> totalThreads(0);
atomic<int> prunedPaths(0);
atomic<int> totalCells(0);
atomic<int> visitedCells(0);
atomic<int> bestDistanceFound(numeric_limits<int>::max());
atomic<chrono::steady_clock::time_point> searchStartTime;
mutex progressMutex;
string lastSequentialStatus;
string lastParallelStatus;

// Directory for results
const string RESULTS_DIR = "results";

// Structure to hold benchmark results
struct BenchmarkResult {
    int matrixSize;
    string executionType;
    int minDistance;
    double timeNanos;
    int visitedCells;
    int prunedPaths;
    int threadsCreated;
    double speedup;
};

vector<BenchmarkResult> benchmarkResults;

// ANSI Color Codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */

// Function to check if a directory exists
bool directoryExists(const string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // No existe o error al acceder
    }
    return (info.st_mode & S_IFDIR); // Devuelve true si es un directorio
}

// Function to create a directory
bool createDirectory(const string& path) {
#ifdef _WIN32
    int result = mkdir(path.c_str());
#else
    int result = mkdir(path.c_str(), 0755); // Permisos para Linux/Mac
#endif
    return result == 0 || errno == EEXIST;
}

// Function to ensure the results directory exists
void ensureResultsDirectoryExists() {
    if (!directoryExists(RESULTS_DIR)) {
        if (!createDirectory(RESULTS_DIR)) {
            cerr << RED << "Error al crear el directorio " << RESULTS_DIR << RESET << endl;
            exit(1);
        } else {
            cout << GREEN << "Directorio '" << RESULTS_DIR << "' creado correctamente." << RESET << endl;
        }
    } else {
        cout << YELLOW << "Usando directorio existente: '" << RESULTS_DIR << "'" << RESET << endl;
    }
}

// Function to display search status
void showSearchStatus(int row, int col, int currentDist, int depth, int bestDist, bool isParallel, int matrixSize) {
    auto now = chrono::steady_clock::now();
    
    lock_guard<mutex> lock(progressMutex);
    
    auto elapsedTime = chrono::duration_cast<chrono::nanoseconds>(now - searchStartTime.load()).count();
    string timeStr;
    
    if (elapsedTime < 1000) {
        timeStr = to_string(elapsedTime) + " ns";
    } else if (elapsedTime < 1000000) {
        timeStr = to_string(elapsedTime / 1000) + " µs";
    } else if (elapsedTime < 1000000000) {
        timeStr = to_string(elapsedTime / 1000000) + " ms";
    } else {
        double seconds = static_cast<double>(elapsedTime) / 1000000000.0;
        int hours = static_cast<int>(seconds / 3600);
        int minutes = static_cast<int>((seconds - hours * 3600) / 60);
        double remainingSeconds = seconds - hours * 3600 - minutes * 60;
        
        stringstream ss;
        if (hours > 0) {
            ss << hours << "h " << minutes << "m " << fixed << setprecision(3) << remainingSeconds << "s";
        } else if (minutes > 0) {
            ss << minutes << "m " << fixed << setprecision(3) << remainingSeconds << "s";
        } else {
            ss << fixed << setprecision(3) << seconds << "s";
        }
        timeStr = ss.str();
    }
    
    auto formatNumber = [](int num) -> string {
        string s = to_string(num);
        return (num < 10 ? "0" : "") + s;
    };
    
    stringstream status;
    status << "[" << formatNumber(matrixSize) << "x" << formatNumber(matrixSize) << "]"
           << "[t:" << timeStr << "]"
           << " Estado: " << (isParallel ? "[PARALELO]" : "[SECUENCIAL]") << ": "
           << "[" << formatNumber(row) << "," << formatNumber(col) << "] "
           << "Nivel: " << formatNumber(depth) << " | "
           << "Actual: " << formatNumber(currentDist) << " | "
           << "Mejor: " << (bestDist == numeric_limits<int>::max() ? "---" : formatNumber(bestDist)) << " | "
           << "Podados: " << formatNumber(prunedPaths.load());

    if (isParallel) {
        lastParallelStatus = status.str();
    } else {
        lastSequentialStatus = status.str();
    }
    
    cout << "\r\033[K"; // Limpiar la línea
    cout << BOLD << status.str() << RESET << flush;
}

// Función para obtener el número de cores disponibles
unsigned int getNumCores() {
    return thread::hardware_concurrency();
}

// Función para generar una matriz de costos aleatorios
vector<vector<int>> generateCostMatrix(int n, int min_val = 1, int max_val = 10) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(min_val, max_val);
    
    vector<vector<int>> matrix(n, vector<int>(n));
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            if(i == j) {
                matrix[i][j] = 0; // Costo 0 de un nodo a sí mismo
            } else {
                matrix[i][j] = dis(gen);
            }
        }
    }
    return matrix;
}

// Función para calcular el número total de caminos posibles
int calculateTotalPaths(int n) {
    if (n <= 1) return 1;
    int total = 1;
    for(int i = 1; i < n; i++) {
        total *= i;
    }
    return total;
}

// Function to display the cost matrix
void displayMatrix(const vector<vector<int>>& matrix) {
    cout << "\nMatriz de costos:\n";
    for(const auto& row : matrix) {
        for(int val : row) {
            cout << setw(3) << val << " ";
        }
        cout << endl;
    }
    cout << endl;
}

// Función de backtracking secuencial
void sequentialBacktracking(const vector<vector<int>>& matrix, int current, int end, 
                           int dist, int& minDist, vector<bool>& visited) {
    visitedCells++; // Incrementar contador de celdas visitadas
    
    if(current == end) {
        minDist = min(minDist, dist);
        bestDistanceFound.store(minDist);
        return;
    }
    
    // Poda de caminos no óptimos
    if (dist >= minDist) {
        prunedPaths++; // Incrementar contador de caminos podados
        return;
    }
    
    showSearchStatus(current, -1, dist, 0, bestDistanceFound.load(), false, matrix.size());
    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            showSearchStatus(current, i, dist + matrix[current][i], 1, bestDistanceFound.load(), false, matrix.size());
            sequentialBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited);
            visited[i] = false;
        }
    }
}

// Función de backtracking paralelo (versión con mutex)
void parallelBacktracking(const vector<vector<int>>& matrix, int current, int end,
                         int dist, atomic<int>& minDist, vector<bool>& visited,
                         int depth = 0) {
    visitedCells++;
    
    if(current == end) {
        // Actualización atómica del mínimo
        int currentMin = minDist.load();
        while(dist < currentMin && !minDist.compare_exchange_weak(currentMin, dist)) {
            currentMin = minDist.load();
        }
        if (dist < currentMin) {
            bestDistanceFound.store(dist);
        }
        return;
    }
    
    // Poda de caminos no óptimos
    if (dist >= minDist.load()) {
        prunedPaths++;
        return;
    }
    
    showSearchStatus(current, -1, dist, depth, bestDistanceFound.load(), true, matrix.size());
    
    // Explorar todos los posibles caminos
    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            
            showSearchStatus(current, i, dist + matrix[current][i], depth + 1, bestDistanceFound.load(), true, matrix.size());
            
            // Paralelización en niveles superiores del árbol de recursión
            if(depth < 2) { // Umbral de paralelización
                totalThreads++;
                thread t([&matrix, i, end, dist, &minDist, visited, depth, current]() mutable {
                    int localDist = dist + matrix[current][i];
                    vector<bool> localVisited = visited;
                    parallelBacktracking(matrix, i, end, localDist, minDist, localVisited, depth + 1);
                });
                t.detach();
            } else {
                parallelBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited, depth + 1);
            }
            
            visited[i] = false;
        }
    }
}

// Función para formatear el tiempo en una unidad apropiada
string formatTime(double nanoseconds) {
    if (nanoseconds < 1000.0) {
        return to_string(static_cast<int>(round(nanoseconds))) + " ns";
    } else if (nanoseconds < 1000000.0) {
        return to_string(static_cast<int>(round(nanoseconds / 1000.0))) + " µs";
    } else if (nanoseconds < 1000000000.0) {
        return to_string(static_cast<int>(round(nanoseconds / 1000000.0))) + " ms";
    } else {
        double seconds = nanoseconds / 1000000000.0;
        int hours = static_cast<int>(seconds / 3600);
        int minutes = static_cast<int>((seconds - hours * 3600) / 60);
        double remainingSeconds = seconds - hours * 3600 - minutes * 60;
        
        stringstream ss;
        if (hours > 0) {
            ss << hours << "h " << minutes << "m " << fixed << setprecision(3) << remainingSeconds << "s";
        } else if (minutes > 0) {
            ss << minutes << "m " << fixed << setprecision(3) << remainingSeconds << "s";
        } else {
            ss << fixed << setprecision(3) << seconds << "s";
        }
        return ss.str();
    }
}

// Función mejorada para medir el tiempo
template<typename Func>
double measureTime(Func func) {
    auto start = high_resolution_clock::now();
    func();
    auto stop = high_resolution_clock::now();
    return duration_cast<nanoseconds>(stop - start).count();
}

// Function to export benchmark results to CSV file
void exportToCSV(const vector<BenchmarkResult>& results, const string& filename) {
    // Volver a asegurar que el directorio existe
    ensureResultsDirectoryExists();
    
    string fullPath = RESULTS_DIR + "/" + filename;
    cout << YELLOW << "Exportando resultados a: " << fullPath << RESET << endl;
    
    ofstream csvFile(fullPath);
    
    if (!csvFile.is_open()) {
        cerr << RED << "Error al abrir el archivo " << fullPath << " para escribir resultados." << RESET << endl;
        
        // Intentar escribir en el directorio actual como fallback
        string fallbackPath = filename;
        cout << YELLOW << "Intentando escribir en el directorio actual: " << fallbackPath << RESET << endl;
        
        ofstream fallbackFile(fallbackPath);
        if (!fallbackFile.is_open()) {
            cerr << RED << "Error también al escribir en el directorio actual. No se pudieron guardar los resultados." << RESET << endl;
            return;
        }
        
        // Escribir en el archivo de fallback
        fallbackFile << "Tamaño de Matriz,Tipo de Ejecución,Distancia Mínima,Tiempo (ns),Tiempo (s),Celdas Visitadas,Caminos Podados,Hilos Creados,Speedup\n";
        
        for (const auto& result : results) {
            fallbackFile << result.matrixSize << ","
                    << result.executionType << ","
                    << (result.minDistance == numeric_limits<int>::max() ? "No encontrada" : to_string(result.minDistance)) << ","
                    << result.timeNanos << ","
                    << (result.timeNanos / 1000000000.0) << ","
                    << result.visitedCells << ","
                    << result.prunedPaths << ","
                    << result.threadsCreated << ","
                    << (result.executionType == "Secuencial" ? "1.0" : to_string(result.speedup)) << "\n";
        }
        
        fallbackFile.close();
        cout << GREEN << "Resultados exportados a '" << fallbackPath << "' (directorio actual)" << RESET << endl;
        return;
    }
    
    // Write CSV header
    csvFile << "Tamaño de Matriz,Tipo de Ejecución,Distancia Mínima,Tiempo (ns),Tiempo (s),Celdas Visitadas,Caminos Podados,Hilos Creados,Speedup\n";
    
    // Write each result
    for (const auto& result : results) {
        csvFile << result.matrixSize << ","
                << result.executionType << ","
                << (result.minDistance == numeric_limits<int>::max() ? "No encontrada" : to_string(result.minDistance)) << ","
                << result.timeNanos << ","
                << (result.timeNanos / 1000000000.0) << ","
                << result.visitedCells << ","
                << result.prunedPaths << ","
                << result.threadsCreated << ","
                << (result.executionType == "Secuencial" ? "1.0" : to_string(result.speedup)) << "\n";
    }
    
    csvFile.close();
    cout << GREEN << "Resultados exportados exitosamente a '" << fullPath << "'" << RESET << endl;
}

// Función para ejecutar el generador de gráficos
void runChartGenerator() {
    cout << YELLOW << "Ejecutando generador de gráficos..." << RESET << endl;
    
    // Comando para ejecutar Python con el entorno virtual
    string cmd;
    
    // Verificar si existe el entorno virtual
    if (directoryExists("venv")) {
        #ifdef _WIN32
            cmd = "venv\\Scripts\\python generate_charts.py";
        #else
            cmd = "source venv/bin/activate && python generate_charts.py";
        #endif
    } else {
        // Intentar con python3 directamente
        cmd = "python3 generate_charts.py";
    }
    
    cout << CYAN << "Ejecutando: " << cmd << RESET << endl;
    int result = system(cmd.c_str());
    
    if (result != 0) {
        cerr << RED << "Error al ejecutar el generador de gráficos. Código: " << result << RESET << endl;
        
        // Intentar con python como fallback
        cout << YELLOW << "Intentando con 'python' como alternativa..." << RESET << endl;
        result = system("python generate_charts.py");
        
        if (result != 0) {
            cerr << RED << "Error al ejecutar el generador de gráficos con 'python'. Código: " << result << RESET << endl;
        } else {
            cout << GREEN << "Gráficos generados exitosamente con 'python'." << RESET << endl;
        }
    } else {
        cout << GREEN << "Gráficos generados exitosamente." << RESET << endl;
    }
}

int main() {
    // Ensure results directory exists
    ensureResultsDirectoryExists();
    
    const int MAX_SIZE = 15; // Reducido para pruebas más rápidas
    
    cout << BOLD << "Sistema detectado:" << RESET << endl;
    cout << "Número de cores: " << getNumCores() << endl;
    cout << "Tamaño máximo de matriz: " << MAX_SIZE << "x" << MAX_SIZE << endl;
    cout << "----------------------------------------" << endl;
    
    vector<int> sizes;
    for(int i = 5; i <= MAX_SIZE; i++) {
        sizes.push_back(i);
    }
    
    // Clear the benchmark results container
    benchmarkResults.clear();
    
    for(int n : sizes) {
        cout << "\n" << BOLD << "Probando matriz de " << n << "x" << n << ":" << RESET << "\n";
        
        auto matrix = generateCostMatrix(n);
        
        // Display matrix for small sizes
        if (n <= 8) {
            displayMatrix(matrix);
        }
        
        int start = 0;
        int end = n - 1;
        
        totalThreads = 0;
        prunedPaths = 0;
        visitedCells = 0;
        bestDistanceFound.store(numeric_limits<int>::max());
        searchStartTime.store(chrono::steady_clock::now());
        
        // Sequential execution
        cout << "\nProcesando secuencial: ";
        cout.flush();
        int seqResult = numeric_limits<int>::max();
        vector<bool> seqVisited(n, false);
        seqVisited[start] = true;
        
        auto seqStart = chrono::high_resolution_clock::now();
        sequentialBacktracking(matrix, start, end, 0, seqResult, seqVisited);
        auto seqEnd = chrono::high_resolution_clock::now();
        double seqTimeNanos = chrono::duration_cast<chrono::nanoseconds>(seqEnd - seqStart).count();

        // Store sequential benchmark results
        BenchmarkResult seqBenchmark;
        seqBenchmark.matrixSize = n;
        seqBenchmark.executionType = "Secuencial";
        seqBenchmark.minDistance = seqResult;
        seqBenchmark.timeNanos = seqTimeNanos;
        seqBenchmark.visitedCells = visitedCells.load();
        seqBenchmark.prunedPaths = prunedPaths.load();
        seqBenchmark.threadsCreated = 1; // Main thread only
        seqBenchmark.speedup = 1.0; // Reference speedup

        cout << "\n" << BOLD YELLOW << "Secuencial:" << RESET << endl;
        cout << "  - Distancia minima: " << (seqResult == numeric_limits<int>::max() ? "No encontrada" : to_string(seqResult)) << endl;
        cout << "  - Tiempo: " << formatTime(seqTimeNanos) << endl;
        cout << "  - Celdas visitadas: " << visitedCells.load() << endl;
        cout << "  - Caminos podados: " << prunedPaths.load() << endl;
        cout << "  - Ultimo estado: " << lastSequentialStatus << endl;
        
        // Parallel execution
        cout << "\nProcesando paralelo: ";
        cout.flush();
        atomic<int> parResult(numeric_limits<int>::max());
        vector<bool> parVisited(n, false);
        parVisited[start] = true;
        visitedCells = 0;
        prunedPaths = 0;
        bestDistanceFound.store(numeric_limits<int>::max());
        searchStartTime.store(chrono::steady_clock::now());
        
        auto parStart = chrono::high_resolution_clock::now();
        
        // Ejecutamos el backtracking paralelo
        parallelBacktracking(matrix, start, end, 0, parResult, parVisited);
        
        // Esperamos un tiempo para que se completen los hilos (ya que usamos detach)
        cout << "\n" << BOLD << "Esperando a que los hilos terminen (2 segundos)..." << RESET << flush;
        this_thread::sleep_for(chrono::seconds(2));
        cout << GREEN << " Completado." << RESET << endl;
        
        auto parEnd = chrono::high_resolution_clock::now();
        double parTimeNanos = chrono::duration_cast<chrono::nanoseconds>(parEnd - parStart).count();
        
        int finalParResult = parResult.load();
        
        double speedup = 0.0;
        if (seqTimeNanos > 0 && parTimeNanos > 0) {
            speedup = seqTimeNanos / parTimeNanos;
        }
        
        // Store parallel benchmark results
        BenchmarkResult parBenchmark;
        parBenchmark.matrixSize = n;
        parBenchmark.executionType = "Paralelo";
        parBenchmark.minDistance = finalParResult;
        parBenchmark.timeNanos = parTimeNanos;
        parBenchmark.visitedCells = visitedCells.load();
        parBenchmark.prunedPaths = prunedPaths.load();
        parBenchmark.threadsCreated = totalThreads.load();
        parBenchmark.speedup = speedup;
        
        // Add both results to the benchmark collection
        benchmarkResults.push_back(seqBenchmark);
        benchmarkResults.push_back(parBenchmark);
        
        cout << BOLD CYAN << "Paralelo:" << RESET << endl;
        cout << "  - Distancia minima: " << (finalParResult == numeric_limits<int>::max() ? "No encontrada" : to_string(finalParResult)) << endl;
        cout << "  - Tiempo: " << formatTime(parTimeNanos) << endl;
        cout << "  - Threads creados: " << totalThreads.load() << endl;
        cout << "  - Caminos podados: " << prunedPaths.load() << endl;
        cout << "  - Celdas visitadas: " << visitedCells.load() << endl;
        cout << "  - Speedup: " << fixed << setprecision(2) << speedup << "x" << endl;
        cout << "  - Ultimo estado: " << lastParallelStatus << endl;
        
        cout << "----------------------------------------" << endl;
    }
    
    // Export results to CSV file
    exportToCSV(benchmarkResults, "benchmark_results.csv");
    
    // Ejecutar el generador de gráficos
    runChartGenerator();
    
    return 0;
}