#include <iostream>

#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <deque>

const int N = 500;

int get_id(int i, int j, int k) {
    return i * N * N + j * N + k;
}

std::vector<std::tuple<int, int, int>> NEIGHBOURS = {
        {1, 0, 0},
        {-1, 0, 0},
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1}
};

std::vector<int> neighbours(int id) {
    int i = (id / N) / N;
    int j = (id / N) % N;
    int k = id % N;

    std::vector<int> result;

    for (auto [x, y, z] : NEIGHBOURS) {
        if (i + x >= 0 && i + x < N && j + y >= 0 && j + y < N && k + z >= 0 && k + z < N) {
            result.push_back(get_id(i + x, j + y, k + z));
        }
    }

    return result;
}

void seq_bfs(int start, std::vector<int>& distance, std::function<std::vector<int>(int)> const& ns) {
    std::deque<int> q = {start};

    distance[start] = 0;
    while (!q.empty()) {
        int v = q.front();
        q.pop_front();

        for (int u : ns(v)) {
            if (distance[u] == -1) {
                distance[u] = distance[v] + 1;
                q.push_back(u);
            }
        }
    }
}

void par_bfs(int start, std::vector<int>& distance, std::function<std::vector<int>(int)> const& ns) {
    distance[start] = 0;

    auto visited = parlay::tabulate<std::atomic<bool>>(distance.size(), [&] (long i) {
        return (i == start); });

    parlay::sequence<int> frontier(1,start);

    int frontier_number = 0;

    while (!frontier.empty()) {
        parlay::sequence<int> const& candidates =
                flatten(map(frontier, [&] (int u) { return ns(u); }));

        frontier = filter(candidates, [&] (auto&& v) {
            bool expected = false;
            return visited[v].compare_exchange_strong(expected, true);});

        frontier_number++;

        map(frontier, [&] (int u) {
            distance[u] = frontier_number;
            return 0;
        });
    }
}

int main() {

    auto timer = parlay::internal::timer("", false);
    std::cout << std::setprecision(5) << std::fixed;
    auto g = [](int i) { return neighbours(i); };

    auto distance = std::vector<int>(N * N * N, -1);
    timer.start();
    seq_bfs(0, distance, g);
    double seq_time = timer.stop();

    std::cout << seq_time << '\n';
    bool valid = true;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                valid &= (distance[get_id(i, j, k)] == i + j + k);
            }
        }
    }
    std::cout << "valid: " << valid << '\n';

    distance = std::vector<int>(N * N * N, -1);
    timer.start();
    par_bfs(0, distance, g);
    double par_time = timer.stop();

    std::cout << par_time << '\n';
    valid = true;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                valid &= (distance[get_id(i, j, k)] == i + j + k);
            }
        }
    }
    std:: cout << "valid: " << valid << '\n';

    std::cout << "speedup: " << seq_time / par_time << '\n';
    return 0;
}

