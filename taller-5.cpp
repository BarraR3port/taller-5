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

using namespace std;
using namespace std::chrono;

mutex mtx;
atomic<int> totalThreads(0);
atomic<int> activeThreads(0);

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

// Función de backtracking secuencial
void sequentialBacktracking(const vector<vector<int>>& matrix, int current, int end, 
                           int dist, int& minDist, vector<bool>& visited) {
    if(current == end) {
        minDist = min(minDist, dist);
        return;
    }
    
    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            sequentialBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited);
            visited[i] = false;
        }
    }
}

// Función de backtracking paralelo (versión con mutex)
void parallelBacktracking(const vector<vector<int>>& matrix, int current, int end, 
                         int dist, atomic<int>& minDist, vector<bool>& visited, 
                         int depth = 0) {
    if(current == end) {
        // Actualización atómica del mínimo
        int currentMin = minDist.load();
        while(dist < currentMin && !minDist.compare_exchange_weak(currentMin, dist)) {
            currentMin = minDist.load();
        }
        return;
    }
    
    // Explorar todos los posibles caminos
    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            
            // Paralelización optimizada basada en el número de cores
            static const unsigned int numCores = getNumCores();
            if(depth == 0 && i < numCores) { // Usamos todos los cores disponibles
                totalThreads++;
                activeThreads++;
                thread t([&matrix, i, end, dist, &minDist, visited, depth, current]() mutable {
                    int localDist = dist + matrix[current][i];
                    vector<bool> localVisited = visited;
                    parallelBacktracking(matrix, i, end, localDist, minDist, localVisited, depth + 1);
                    activeThreads--;
                });
                t.detach();
            } else {
                parallelBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited, depth + 1);
            }
            
            visited[i] = false;
        }
    }
}

// Función para medir el tiempo de ejecución
template<typename Func>
long long measureTime(Func func) {
    auto start = high_resolution_clock::now();
    func();
    auto stop = high_resolution_clock::now();
    return duration_cast<milliseconds>(stop - start).count();
}

int main() {
    // Definir el tamaño máximo de las matrices
    const int MAX_SIZE = 20;
    
    cout << "Sistema detectado:" << endl;
    cout << "Número de cores: " << getNumCores() << endl;
    cout << "Tamaño máximo de matriz: " << MAX_SIZE << "x" << MAX_SIZE << endl;
    cout << "----------------------------------------" << endl;
    
    // Pruebas con diferentes tamaños de matriz
    vector<int> sizes;
    for(int i = 2; i <= MAX_SIZE; i++) {
        sizes.push_back(i);
    }
    vector<long long> seqTimes, parTimes;
    
    for(int n : sizes) {
        cout << "\nProbando matriz de " << n << "x" << n << ":\n";
        
        // Generar matriz de costos
        auto matrix = generateCostMatrix(n);
        
        // Imprimir matriz
        cout << "Matriz de costos:\n";
        for(const auto& row : matrix) {
            for(int val : row) cout << setw(3) << val << " ";
            cout << endl;
        }
        
        int start = 0;
        int end = n - 1;
        
        // Resetear contadores
        totalThreads = 0;
        activeThreads = 0;
        
        // Ejecución secuencial
        int seqMinDist = numeric_limits<int>::max();
        vector<bool> seqVisited(n, false);
        seqVisited[start] = true;
        
        auto seqTime = measureTime([&]() {
            sequentialBacktracking(matrix, start, end, 0, seqMinDist, seqVisited);
        });
        
        cout << "\nSecuencial:" << endl;
        cout << "  - Distancia minima: " << seqMinDist << endl;
        cout << "  - Tiempo: " << seqTime << " ms" << endl;
        seqTimes.push_back(seqTime);
        
        // Ejecución paralela
        atomic<int> parMinDist(numeric_limits<int>::max());
        vector<bool> parVisited(n, false);
        parVisited[start] = true;
        
        auto parTime = measureTime([&]() {
            parallelBacktracking(matrix, start, end, 0, parMinDist, parVisited);
            
            // Esperar a que todos los hilos terminen
            while(activeThreads > 0) {
                this_thread::sleep_for(milliseconds(100));
            }
        });
        
        cout << "\nParalelo:" << endl;
        cout << "  - Distancia minima: " << parMinDist.load() << endl;
        cout << "  - Tiempo: " << parTime << " ms" << endl;
        cout << "  - Threads creados: " << totalThreads << endl;
        cout << "  - Threads activos máximos: " << getNumCores() << endl;
        parTimes.push_back(parTime);
        
        // Calcular speedup
        if(seqTime > 0 && parTime > 0) {
            double speedup = static_cast<double>(seqTime) / parTime;
            cout << "  - Speedup: " << fixed << setprecision(2) << speedup << "x" << endl;
        }
        
        cout << "----------------------------------------" << endl;
    }
    
    // Mostrar resumen de resultados
    cout << "\nResumen de tiempos:\n";
    cout << setw(8) << "Tamaño" << setw(15) << "Secuencial(ms)" << setw(15) << "Paralelo(ms)" << setw(15) << "Speedup" << endl;
    for(size_t i = 0; i < sizes.size(); i++) {
        double speedup = (seqTimes[i] > 0 && parTimes[i] > 0) ? 
                        static_cast<double>(seqTimes[i]) / parTimes[i] : 0.0;
        cout << setw(4) << sizes[i] << "x" << sizes[i] << setw(15) << seqTimes[i] 
             << setw(15) << parTimes[i] << setw(15) << fixed << setprecision(2) << speedup << endl;
    }
    
    return 0;
}