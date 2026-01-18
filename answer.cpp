#ifdef ONLINE_JUDGE
#pragma GCC optimize "Ofast,omit-frame-pointer,inline,unroll-all-loops"
#define SUBMIT
#else
//#define SUBMIT
#endif

#include <bits/stdc++.h>

using namespace std;
using namespace chrono;

typedef long long ll;
typedef unsigned long long ull;
#define REP(i, n) for(int i=0; i<int(n); i++)
#define REPi(i, a, b) for(int i=int(a); i<int(b); i++)
#define MEMS(a,b) memset(a,b,sizeof(a))
#define mp make_pair
#define MOD(a, m) ((a % m + m) % m)
#define ALL(a) (a).begin(), (a).end()
#define RALL(a) (a).rbegin(), (a).rend()
template<class T>bool chmax(T &a, const T &b) { if (a<b) { a=b; return 1; } return 0; }
template<class T>bool chmin(T &a, const T &b) { if (b<a) { a=b; return 1; } return 0; }
constexpr ll MOD = 1e9+7;
constexpr ll INF_LL = 1e14; // 変数名重複回避のため変更

constexpr int SEED = 1000;

static mt19937 engine;

constexpr int N = 30;
constexpr int K = 1000;

namespace Env {
    constexpr double time_limit = 1.950;
    constexpr double start_temp = 1000;
    constexpr double end_temp = 10;
    constexpr bool minimization = false;
    constexpr int n_transition = 1;
    constexpr int beam_width = 10;
};

#ifdef SUBMIT
#define DEBUG(fmt, ...) ;
#define DUMP(fmt, ...) ;
#define ASSERT(expr, fmt_str, ...) ;
#else
#define DEBUG(fmt, ...) std::cerr << std::format(fmt __VA_OPT__(,) __VA_ARGS__)
#define DUMP(var) std::cerr << #var << " = " << (var) << std::endl;
#define ASSERT(expr, fmt_str, ...) \
    do { \
        if (!(expr)) { \
            std::cerr << "===================================================" << endl;  \
            std::cerr << std::format( \
                "Assertion failed: {}\nFile: {}:{}\nFunction: {}\nMessage: {}\n", \
                #expr, __FILE__, __LINE__, __func__, std::format(fmt_str __VA_OPT__(,) __VA_ARGS__) \
            ); \
            std::cerr << "===================================================" << endl;  \
            std::abort(); \
        } \
    } while (0)
#include "dbg_utils.h"
#endif // SUBMIT

////////////////////////////////////////////////////////////////////

using time_point_t = std::chrono::_V2::system_clock::time_point;

// R, D, L, U
constexpr int dx[] = {1, 0, -1, 0};
constexpr int dy[] = {0, 1, 0, -1};

constexpr char dirs[] = {'R', 'D', 'L', 'U'};

////////////////////////////////////////////////////////////////////

/// xor128
unsigned int randxor(){
    static unsigned int x=123456789, y=362436069, z=521288629, w=88675123;
    unsigned int t;
    t = (x^(x<<11));
    x = y;
    y = z;
    z = w; 
    return( w=(w^(w>>19))^(t^(t>>8)) );
}

/// return [0,1)
double rand01(){
    return 1.0 * randxor() / numeric_limits<unsigned int>::max();
}

/// return [left, right)
int rand_int(const int left, const int right){
    assert(right > left);
    return randxor() % (right - left) + left;
}

template<class T>
void shuffle(vector<T>& v) {
    int sz = v.size();
    for(int i = sz; i > 1; i--) {
        auto p = rand_int(0, i);
        swap(v[i-1], v[p]);
    }
}

template<class T>
inline T sample(const vector<T>& v) {
    ASSERT(v.size() > 0, "");
    return v[rand_int(0, v.size())];
}

////////////////////////////////////////////////////////////////////

struct Timer {
    std::chrono::_V2::system_clock::time_point sp;

    Timer() : sp(system_clock::now()) {}

    double get_time() const {
        const double t = duration_cast<microseconds>(system_clock::now() - sp).count() * 1e-6;
        return t;
    }
};

Timer timer;

//////////////////////////////////////////////////////////////////

template <std::floating_point T>
double get_linear_interpolate(T progress, double start_val, double end_val) {
    return start_val + (end_val - start_val) * progress;
}

template <std::floating_point T>
double get_exponential_interpolate(T progress, double start_val, double end_val) {
    return start_val * pow(end_val/start_val, progress);
}

array<int, 4> di;
void build_di(int width) {
    di[0] = 1;
    di[1] = width;
    di[2] = -1;
    di[3] = -width;
}

///////////////////////////////////////////////////////////////////

constexpr int M = N * (N - 1);  // num of edges of a direction
using edge_t = bitset<2 * M>;

// 各辺の推定値を格納する配列（グローバル）
// インデックス 0 ~ M-1: 横方向の辺 (y, x) -> (y, x+1) のID = y*(N-1) + x
// インデックス M ~ 2M-1: 縦方向の辺 (y, x) -> (y+1, x) のID = M + x*(N-1) + y
array<double, 2*M> estimated_weights;

int enc(int y, int x) {
    return y*N + x;
}

pair<int, int> dec(int v) {
    return {v / N, v % N};
}

struct Query {
    Query() {
    }

    pair<int, int> get_query() {
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        return {enc(si, sj), enc(ti, tj)};
    }

    int put_path(const vector<int>& path) {
        for (const auto& d : path) {
            cout << dirs[d];
        }
        cout << endl;

        int len;
        cin >> len;
        return len;
    }
};

////////////////////////////////////////////////////////////////////

template<typename T, typename U>
int inner_product(T a, U b) {
    const int size = a.size();
    int ret = 0;
    REP(i, size) {
        ret += a[i] * b[i];
    }
    return ret;
}

struct Model {
    int D;
    array<double, 2*N> hv;
    array<array<double, 2*N>, 2*N> hv_var;
    double log_likelihood;

    Model(int D) : D(D), log_likelihood(0.0) {
        for (auto& v : hv) v = 5000.0;
        REP(i, 2*N) {
            REP(j, 2*N) {
                hv_var[i][j] = (i == j) ? pow(8000 - 2*D, 2) / 12 : 0.0;
            }
        }
    }

    void update_estimate(int len, edge_t edges) {
        array<int, 2*N> simple_edges;
        for (auto& e : simple_edges) e = 0;
        REP(m, 2*M) {
            if (edges[m]) simple_edges[m / (N-1)] += 1;
        }

        // e = len - (c*x)
        //DEBUG("simple_edges = {}\n", simple_edges);
        //DEBUG("hv = {}\n", hv);
        const auto est_y = inner_product(simple_edges, hv);
        const auto err = len - est_y;
        DEBUG("len = {}, est_y = {}, err = {}\n", len, est_y, err);

        // S = cPc + R
        const double R = pow(0.2 * len, 2) / 12;
        double S = R;
        array<double, 2*N> cP;
        REP(j, 2*N) {
            cP[j] = 0;
            REP(i, 2*N) {
                cP[j] += simple_edges[i] * hv_var[i][j];
            }
            S += cP[j] * simple_edges[j];
        }
        //DEBUG("S = {}\n", S);

        // k = Pc / S
        array<double, 2*N> k;
        REP(i, 2*N) {
            double tmp = 0;
            REP(j, 2*N) tmp += hv_var[i][j] * simple_edges[j];
            k[i] = tmp / S;
        }
        //DEBUG("k = {}\n", k);

        // x_new = x + k*e
        REP(i, 2*N) {
            //hv[i] = hv[i] + k[i] * err;
            hv[i] = max(0.0, hv[i] + k[i] * err);
        }
        //DEBUG("x_new = {}\n", hv);

        // P_new = P - k*(cP)
        REP(i, 2*N) {
            REP(j, 2*N) {
                hv_var[i][j] -= k[i] * cP[j];
            }
        }

        //DEBUG("hv_var = [");
        //REP(i, 2*N) {
        //    DEBUG("{}, ", hv_var[i][i]);
        //}
        //DEBUG("]\n");

        log_likelihood -= (log(S) + err * err / S) / 2;
    }
};


// Dijkstra法を用いて最短パスを求める
pair<vector<int>, edge_t> get_path(int src, int dst, const Model& model) {
    // Dijkstraの準備
    vector<double> dist(N * N, 1e18);
    vector<int> prev_node(N * N, -1);
    vector<int> prev_dir(N * N, -1);
    
    // {cost, u}
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u]) continue;
        if (u == dst) break; // ゴールに到達

        auto [y, x] = dec(u);

        // 4方向へ探索 (R, D, L, U)
        REP(dir, 4) {
            int ny = y + dy[dir];
            int nx = x + dx[dir];

            if (ny < 0 || ny >= N || nx < 0 || nx >= N) continue;

            int v = enc(ny, nx);
            
            // 辺のインデックスを計算してコストを取得
            int edge_idx = -1;
            if (dir == 0) { // R: (y,x) -> (y,x+1)
                edge_idx = y * (N - 1) + x;
            } else if (dir == 1) { // D: (y,x) -> (y+1,x)
                edge_idx = M + x * (N - 1) + y;
            } else if (dir == 2) { // L: (y,x-1) -> (y,x)
                edge_idx = y * (N - 1) + (x - 1);
            } else if (dir == 3) { // U: (y-1,x) -> (y,x)
                edge_idx = M + x * (N - 1) + (y - 1);
            }

            //double weight = estimated_weights[edge_idx];
            double weight = model.hv[edge_idx / (N - 1)];
            //DEBUG("edge_idx = {}, weight = {}\n", edge_idx, weight);
            
            if (dist[v] > dist[u] + weight) {
                dist[v] = dist[u] + weight;
                prev_node[v] = u;
                prev_dir[v] = dir;
                pq.push({dist[v], v});
            }
        }
    }

    // パスの復元
    vector<int> path;
    edge_t edges;
    
    int curr = dst;
    while (curr != src) {
        int dir = prev_dir[curr];
        int prev = prev_node[curr];
        
        path.push_back(dir);
        
        // 通った辺をbitsetに記録
        auto [py, px] = dec(prev);
        int edge_idx = -1;
        // logic from previous implementation to maintain consistency
        if (dir == 0) { // R
             edge_idx = py * (N - 1) + px;
        } else if (dir == 1) { // D
             edge_idx = M + px * (N - 1) + py;
        } else if (dir == 2) { // L
             edge_idx = py * (N - 1) + (px - 1);
        } else if (dir == 3) { // U
             edge_idx = M + px * (N - 1) + (py - 1);
        }

        if(edge_idx != -1) edges.set(edge_idx);

        curr = prev;
    }
    
    reverse(ALL(path));
    return {path, edges};
}

void solve(const double end_time) {
    vector<Model> models;
    models.emplace_back((100 + 2000) * 1 / 5);
    models.emplace_back((100 + 2000) * 2 / 5);
    models.emplace_back((100 + 2000) / 2);
    models.emplace_back((100 + 2000) * 3 / 5);
    models.emplace_back((100 + 2000) * 4 / 5);

    Query query;
    REP(k, K) {
        vector<double> likelihoods;
        for (const auto& model : models) {
            likelihoods.push_back(model.log_likelihood);
        }
        DEBUG("k = {}, likelihoods = {}\n", k, likelihoods);
        const auto [s, t] = query.get_query();
        const auto& model = *max_element(ALL(models), [](const Model& a, const Model& b) {
            return a.log_likelihood < b.log_likelihood;
        });
        const auto [path, edges] = get_path(s, t, model);
        const auto len = query.put_path(path);
        for (auto& model : models) {
            model.update_estimate(len, edges);
        }
    }

    const auto& best_model = *max_element(ALL(models), [](const Model& a, const Model& b) {
        return a.log_likelihood < b.log_likelihood;
    });
    DEBUG("D = {}, likelihood = {}, estimated hv : {}\n", best_model.D, best_model.log_likelihood, best_model.hv);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]){
    ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    solve(Env::time_limit);

    const double time = timer.get_time();
    DEBUG("time: {:.3f} [s]\n", time);
    return 0;
}