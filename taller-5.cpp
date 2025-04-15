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

// ANSI Color Codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */

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
           << " Estado: " << (isParallel ? "[PARALELO]" : "[SECUENCIAL]") << ": "
           << "[" << formatNumber(row) << "," << formatNumber(col) << "] "
           << "Nivel: " << formatNumber(depth) << " | "
           << "Actual: " << formatNumber(currentDist) << " | "
           << "Mejor: " << (bestDist == numeric_limits<int>::max() ? "---" : formatNumber(bestDist)) << " | "
           << "Podados: " << formatNumber(prunedPaths.load()) << " | "
           << "Tiempo: " << timeStr;
    
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

// Función de backtracking secuencial
void sequentialBacktracking(const vector<vector<int>>& matrix, int current, int end,
                           int dist, int& minDist, vector<bool>& visited, int depth = 0) {
    if (dist >= bestDistanceFound.load()) {
        prunedPaths++;
        return;
    }

    if(current == end) {
        if (dist < minDist) {
            minDist = dist;
            bestDistanceFound.store(minDist);
        }
        return;
    }

    showSearchStatus(current, -1, dist, depth, bestDistanceFound.load(), false, matrix.size());

    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            visitedCells++;
            showSearchStatus(current, i, dist + matrix[current][i], depth + 1, bestDistanceFound.load(), false, matrix.size());
            sequentialBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited, depth + 1);
            visited[i] = false;
        }
    }
}

// Worker function for parallel execution (runs sequentially within its thread)
void parallelBacktrackingWorker(const vector<vector<int>>& matrix, int current, int end,
                         int dist, atomic<int>& minDist, vector<bool> visited,
                         int depth) {
    visitedCells++;

    if(dist >= minDist.load()) {
        prunedPaths++;
        return;
    }

    if(current == end) {
        int currentMin = minDist.load();
        while(dist < currentMin && !minDist.compare_exchange_weak(currentMin, dist)) {
        }
        if (dist < currentMin) {
            bestDistanceFound.store(dist);
        }
        return;
    }

    showSearchStatus(current, -1, dist, depth, bestDistanceFound.load(), true, matrix.size());

    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            vector<bool> nextVisited = visited;
            nextVisited[i] = true;

            showSearchStatus(current, i, dist + matrix[current][i], depth + 1, bestDistanceFound.load(), true, matrix.size());

            parallelBacktrackingWorker(matrix, i, end, dist + matrix[current][i], minDist, nextVisited, depth + 1);
        }
    }
}

// Main parallel function - only creates initial threads from the start node
void parallelBacktracking(const vector<vector<int>>& matrix, int startNode, int endNode,
                         int initialDist, atomic<int>& minDist, const vector<bool>& initialVisited,
                         vector<thread>& workerThreads) {
    int depth = 0;
    visitedCells++;

    showSearchStatus(startNode, -1, initialDist, depth, bestDistanceFound.load(), true, matrix.size());

    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[startNode][i] != 0 && !initialVisited[i]) {
            vector<bool> visitedCopy = initialVisited;
            visitedCopy[i] = true;

            int nextDist = initialDist + matrix[startNode][i];
            int nextDepth = depth + 1;

            showSearchStatus(startNode, i, nextDist, nextDepth, bestDistanceFound.load(), true, matrix.size());

            totalThreads++;
            workerThreads.emplace_back([&matrix, nextNode = i, endNode, dist = nextDist, &minDist, visitedState = visitedCopy, startDepth = nextDepth]() {
                parallelBacktrackingWorker(matrix, nextNode, endNode, dist, minDist, visitedState, startDepth);
            });
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

int main() {
    const int MAX_SIZE = 500;
    
    cout << BOLD << "Sistema detectado:" << RESET << endl;
    cout << "Número de cores: " << getNumCores() << endl;
    cout << "Tamaño máximo de matriz: " << MAX_SIZE << "x" << MAX_SIZE << endl;
    cout << "----------------------------------------" << endl;
    
    vector<int> sizes;
    for(int i = 5; i <= MAX_SIZE; i++) {
        sizes.push_back(i);
    }
    
    for(int n : sizes) {
        cout << "\n" << BOLD << "Probando matriz de " << n << "x" << n << ":" << RESET << "\n";
        
        auto matrix = generateCostMatrix(n);
        
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
        
        vector<thread> workerThreads;
        
        auto parStart = chrono::high_resolution_clock::now();
        parallelBacktracking(matrix, start, end, 0, parResult, parVisited, workerThreads);
        
        cout << "\n" << BOLD << "Esperando " << workerThreads.size() << " hilos paralelos..." << RESET << flush;
        for (auto& t : workerThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        cout << "\r" << BOLD << "Hilos paralelos completados.          " << RESET << endl;
        
        auto parEnd = chrono::high_resolution_clock::now();
        double parTimeNanos = chrono::duration_cast<chrono::nanoseconds>(parEnd - parStart).count();
        
        int finalParResult = parResult.load();
        
        double speedup = 0.0;
        if (seqTimeNanos > 0 && parTimeNanos > 0 && n > 5) {
            speedup = seqTimeNanos / parTimeNanos;
        }
        
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
    
    return 0;
}