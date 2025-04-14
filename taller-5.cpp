#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;
using namespace std::chrono;

mutex mtx;

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
            
            // Paralelización en niveles superiores del árbol de recursión
            if(depth < 2) { // Umbral de paralelización
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

// Función para medir el tiempo de ejecución
template<typename Func>
long long measureTime(Func func) {
    auto start = high_resolution_clock::now();
    func();
    auto stop = high_resolution_clock::now();
    return duration_cast<milliseconds>(stop - start).count();
}

int main() {
    // Pruebas con diferentes tamaños de matriz
    vector<int> sizes = {2, 3, 4, 5, 6, 7, 8, 9, 10};
    vector<long long> seqTimes, parTimes;
    
    for(int n : sizes) {
        cout << "\nProbando matriz de " << n << "x" << n << ":\n";
        
        // Generar matriz de costos
        auto matrix = generateCostMatrix(n);
        
        // Imprimir matriz (opcional para tamaños pequeños)
        if(n <= 5) {
            cout << "Matriz de costos:\n";
            for(const auto& row : matrix) {
                for(int val : row) cout << val << " ";
                cout << endl;
            }
        }
        
        int start = 0;
        int end = n - 1;
        
        // Ejecución secuencial
        int seqMinDist = numeric_limits<int>::max();
        vector<bool> seqVisited(n, false);
        seqVisited[start] = true;
        
        auto seqTime = measureTime([&]() {
            sequentialBacktracking(matrix, start, end, 0, seqMinDist, seqVisited);
        });
        
        cout << "Secuencial - Distancia minima: " << seqMinDist 
             << ", Tiempo: " << seqTime << " ms\n";
        seqTimes.push_back(seqTime);
        
        // Ejecución paralela
        atomic<int> parMinDist(numeric_limits<int>::max());
        vector<bool> parVisited(n, false);
        parVisited[start] = true;
        
        auto parTime = measureTime([&]() {
            parallelBacktracking(matrix, start, end, 0, parMinDist, parVisited);
            
            // Esperar a que todos los hilos terminen
            this_thread::sleep_for(milliseconds(100 * n)); // Ajuste empírico
        });
        
        cout << "Paralelo   - Distancia minima: " << parMinDist.load()
             << ", Tiempo: " << parTime << " ms\n";
        parTimes.push_back(parTime);
        
        // Calcular speedup
        if(seqTime > 0 && parTime > 0) {
            double speedup = static_cast<double>(seqTime) / parTime;
            cout << "Speedup: " << speedup << "x\n";
        }
    }
    
    // Mostrar resumen de resultados
    cout << "\nResumen de tiempos:\n";
    cout << "Tamaño\tSecuencial(ms)\tParalelo(ms)\n";
    for(size_t i = 0; i < sizes.size(); i++) {
        cout << sizes[i] << "x" << sizes[i] << "\t" 
             << seqTimes[i] << "\t\t" << parTimes[i] << "\n";
    }
    
    return 0;
}