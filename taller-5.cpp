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
#include <queue>
#include <condition_variable>
#include <sstream>

using namespace std;
using namespace std::chrono;

// Clase para el pool de threads
class ThreadPool {
private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    bool stop;
    atomic<int> active_tasks;
    atomic<int> total_tasks;
    atomic<chrono::high_resolution_clock::time_point> start_time;
    atomic<chrono::high_resolution_clock::time_point> end_time;

public:
    ThreadPool(size_t threads) : stop(false), active_tasks(0), total_tasks(0) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while(true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { 
                            return stop || !tasks.empty(); 
                        });
                        if(stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                        active_tasks++;
                    }
                    task();
                    active_tasks--;
                    total_tasks--;
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            unique_lock<mutex> lock(queue_mutex);
            if(stop) throw runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace(std::forward<F>(f));
            total_tasks++;
        }
        condition.notify_one();
    }

    void startTimer() {
        start_time.store(chrono::high_resolution_clock::now());
    }

    void stopTimer() {
        end_time.store(chrono::high_resolution_clock::now());
    }

    double getElapsedTime() {
        return chrono::duration_cast<chrono::nanoseconds>(
            end_time.load() - start_time.load()
        ).count();
    }

    void waitForCompletion() {
        while(total_tasks > 0 || active_tasks > 0) {
            this_thread::sleep_for(microseconds(100));
        }
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(thread &worker: workers) {
            worker.join();
        }
    }
};

// Variables globales
atomic<int> totalThreads(0);
atomic<int> prunedPaths(0);
atomic<int> totalCells(0);
atomic<int> visitedCells(0);
atomic<int> currentRow(0);
atomic<int> currentCol(0);
atomic<int> currentDistance(0);
atomic<int> currentDepth(0);
atomic<int> bestDistanceFound(numeric_limits<int>::max());
atomic<chrono::steady_clock::time_point> searchStartTime;
mutex progressMutex;
atomic<chrono::steady_clock::time_point> lastUpdate;

// Function to display search status
void showSearchStatus(int row, int col, int currentDist, int depth, int bestDist, bool isParallel, int matrixSize) {
    static const auto minUpdateInterval = chrono::milliseconds(100);
    auto now = chrono::steady_clock::now();
    auto last = lastUpdate.load();
    
    if (now - last < minUpdateInterval) {
        return;
    }
    
    lock_guard<mutex> lock(progressMutex);
    lastUpdate.store(now);
    
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
    
    // Función para formatear números con ceros a la izquierda
    auto formatNumber = [](int num) -> string {
        if (num < 10) return "0" + to_string(num);
        return to_string(num);
    };
    
    cout << "\r\033[K"; // Limpiar la línea
    cout << "[" << formatNumber(matrixSize) << "x" << formatNumber(matrixSize) << "] "
         << "Estado de búsqueda " << (isParallel ? "[PARALELO]" : "[SECUENCIAL]") << ": "
         << "[" << formatNumber(row) << "," << formatNumber(col) << "] "
         << "Nivel: " << formatNumber(depth) << " | "
         << "Dist actual: " << formatNumber(currentDist) << " | "
         << "Mejor dist: " << formatNumber(bestDist) << " | "
         << "Podados: " << formatNumber(prunedPaths.load()) << " | "
         << "Tiempo: " << timeStr << flush;
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
    if(current == end) {
        minDist = min(minDist, dist);
        bestDistanceFound.store(minDist);
        return;
    }
    
    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            visitedCells++;
            currentRow.store(current);
            currentCol.store(i);
            currentDistance.store(dist + matrix[current][i]);
            currentDepth.store(depth);
            showSearchStatus(current, i, dist + matrix[current][i], depth, bestDistanceFound.load(), false, matrix.size());
            sequentialBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited, depth + 1);
            visited[i] = false;
        }
    }
}

// Función de backtracking paralelo optimizada
void parallelBacktracking(const vector<vector<int>>& matrix, int current, int end, 
                         int dist, atomic<int>& minDist, vector<bool>& visited, 
                         ThreadPool& pool, int depth = 0) {
    if(dist >= minDist.load()) {
        prunedPaths++;
        return;
    }

    if(current == end) {
        int currentMin = minDist.load();
        while(dist < currentMin && !minDist.compare_exchange_weak(currentMin, dist)) {
            currentMin = minDist.load();
        }
        bestDistanceFound.store(currentMin);
        return;
    }
    
    for(int i = 0; i < matrix.size(); i++) {
        if(matrix[current][i] != 0 && !visited[i]) {
            visited[i] = true;
            visitedCells++;
            currentRow.store(current);
            currentCol.store(i);
            currentDistance.store(dist + matrix[current][i]);
            currentDepth.store(depth);
            showSearchStatus(current, i, dist + matrix[current][i], depth, bestDistanceFound.load(), true, matrix.size());
            
            if(depth == 0) {
                totalThreads++;
                pool.enqueue([&matrix, i, end, dist, &minDist, visited, depth, current, &pool]() mutable {
                    int localDist = dist + matrix[current][i];
                    vector<bool> localVisited = visited;
                    parallelBacktracking(matrix, i, end, localDist, minDist, localVisited, pool, depth + 1);
                });
            } else {
                parallelBacktracking(matrix, i, end, dist + matrix[current][i], minDist, visited, pool, depth + 1);
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

int main() {
    const int MAX_SIZE = 500;
    
    cout << "Sistema detectado:" << endl;
    cout << "Número de cores: " << getNumCores() << endl;
    cout << "Tamaño máximo de matriz: " << MAX_SIZE << "x" << MAX_SIZE << endl;
    cout << "----------------------------------------" << endl;
    
    ThreadPool pool(getNumCores());
    
    vector<int> sizes;
    for(int i = 2; i <= MAX_SIZE; i++) {
        sizes.push_back(i);
    }
    
    for(int n : sizes) {
        cout << "\nProbando matriz de " << n << "x" << n << ":\n";
        
        auto matrix = generateCostMatrix(n);
        
        // cout << "Matriz de costos:\n";
        // for(const auto& row : matrix) {
        //     for(int val : row) cout << setw(3) << val << " ";
        //     cout << endl;
        // }
        
        int start = 0;
        int end = n - 1;
        
        totalThreads = 0;
        prunedPaths = 0;
        totalCells = calculateTotalPaths(n);
        visitedCells = 0;
        lastUpdate.store(chrono::steady_clock::now());
        
        // Sequential execution
        cout << "\nProcesando secuencial: ";
        cout.flush();
        int seqResult = numeric_limits<int>::max();
        vector<bool> seqVisited(n, false);
        seqVisited[start] = true;
        bestDistanceFound.store(numeric_limits<int>::max());
        searchStartTime.store(chrono::steady_clock::now());
        
        auto seqStart = chrono::high_resolution_clock::now();
        sequentialBacktracking(matrix, start, end, 0, seqResult, seqVisited);
        auto seqEnd = chrono::high_resolution_clock::now();
        double seqTimeNanos = chrono::duration_cast<chrono::nanoseconds>(seqEnd - seqStart).count();
        
        cout << "\nSecuencial:" << endl;
        cout << "  - Distancia minima: " << seqResult << endl;
        cout << "  - Tiempo: " << formatTime(seqTimeNanos) << endl;
        
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
        parallelBacktracking(matrix, start, end, 0, parResult, parVisited, pool);
        pool.waitForCompletion();
        auto parEnd = chrono::high_resolution_clock::now();
        double parTimeNanos = chrono::duration_cast<chrono::nanoseconds>(parEnd - parStart).count();
        
        // Calculate speedup only for matrices larger than 5x5
        double speedup = 0.0;
        if (seqTimeNanos > 0 && parTimeNanos > 0 && n > 5) {
            speedup = seqTimeNanos / parTimeNanos;
        }
        
        cout << "\nParalelo:" << endl;
        cout << "  - Distancia minima: " << parResult.load() << endl;
        cout << "  - Tiempo: " << formatTime(parTimeNanos) << endl;
        cout << "  - Threads creados: " << totalThreads.load() << endl;
        cout << "  - Caminos podados: " << prunedPaths.load() << endl;
        cout << "  - Threads activos máximos: " << getNumCores() << endl;
        cout << "  - Speedup: " << fixed << setprecision(2) << speedup << "x" << endl;
        
        cout << "----------------------------------------" << endl;
    }
    
    return 0;
}