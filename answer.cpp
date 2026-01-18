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
constexpr ll INF_LL = 1e14; 

constexpr int SEED = 1000;

static mt19937 engine;

constexpr int N = 30;
constexpr int K = 1000;

namespace Env {
    constexpr double time_limit = 1.950;
};

#ifdef SUBMIT
#define DEBUG(fmt, ...) ;
#define DUMP(fmt, ...) ;
#define ASSERT(expr, fmt_str, ...) ;
#else
#define DEBUG(fmt, ...) std::cerr << std::format(fmt __VA_OPT__(,) __VA_ARGS__)
#define DUMP(var) std::cerr << #var << " = " << (var) << std::endl;
#define ASSERT(expr, fmt_str, ...) 
#include "dbg_utils.h"
#endif 

using time_point_t = std::chrono::_V2::system_clock::time_point;

constexpr int dx[] = {1, 0, -1, 0};
constexpr int dy[] = {0, 1, 0, -1};
constexpr char dirs[] = {'R', 'D', 'L', 'U'};

// ... Utility functions ...
unsigned int randxor(){ static unsigned int x=123456789,y=362436069,z=521288629,w=88675123; unsigned int t=(x^(x<<11));x=y;y=z;z=w; return(w=(w^(w>>19))^(t^(t>>8))); }
double rand01(){ return 1.0*randxor()/numeric_limits<unsigned int>::max(); }
int rand_int(const int left, const int right){ return randxor()%(right-left)+left; }
struct Timer { std::chrono::_V2::system_clock::time_point sp; Timer():sp(system_clock::now()){} double get_time()const{return duration_cast<microseconds>(system_clock::now()-sp).count()*1e-6;} };
Timer timer;

constexpr int M = N * (N - 1); 
using edge_t = bitset<2 * M>;

int enc(int y, int x) { return y*N + x; }
pair<int, int> dec(int v) { return {v / N, v % N}; }

struct Query {
    pair<int, int> get_query() {
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        return {enc(si, sj), enc(ti, tj)};
    }
    int put_path(const vector<int>& path) {
        for (const auto& d : path) cout << dirs[d];
        cout << endl;
        int len; cin >> len; return len;
    }
};

template<typename T, typename U>
int inner_product(T a, U b) {
    int ret = 0;
    REP(i, a.size()) ret += a[i] * b[i];
    return ret;
}

// 基底クラス
struct BaseModel {
    double log_likelihood;
    int n_vars; // 識別用(デバッグ等)
    
    BaseModel(int n) : log_likelihood(0.0), n_vars(n) {}
    virtual ~BaseModel() = default;

    virtual void update_estimate(int len, edge_t edges) = 0;
    virtual double get_edge_weight(int dir, int y, int x) const = 0;
    virtual double get_variance(int dir, int y, int x) const = 0;
};

// テンプレートモデルクラス
// DIV=1 -> 60変数, DIV=2 -> 120変数, DIV=4 -> 240変数
template <int DIV>
struct Model : public BaseModel {
    static constexpr int N_VARS_LOCAL = 2 * N * DIV;
    
    int D;
    array<double, N_VARS_LOCAL> hv;
    array<array<double, N_VARS_LOCAL>, N_VARS_LOCAL> hv_var;

    Model(int D) : BaseModel(N_VARS_LOCAL), D(D) {
        hv.fill(5000.0);
        // 初期分散
        REP(i, N_VARS_LOCAL) {
            REP(j, N_VARS_LOCAL) {
                hv_var[i][j] = (i == j) ? pow(8000 - 2*D, 2) / 12 : 0.0;
            }
        }
    }

    // 座標から変数インデックスを計算
    int get_var_idx(int dir, int y, int x) const {
        // セグメント計算: (x * DIV) / N をクリップ
        if (dir == 0 || dir == 2) { // 横 (Row) 0 ~ N*DIV-1
            // Row ID: y * DIV + seg
            int seg = (x * DIV) / N;
            if (seg >= DIV) seg = DIV - 1;
            return y * DIV + seg;
        } else { // 縦 (Col) N*DIV ~ 2*N*DIV-1
            // Col ID: x * DIV + seg (+ offset)
            int seg = (y * DIV) / N;
            if (seg >= DIV) seg = DIV - 1;
            return N * DIV + x * DIV + seg;
        }
    }

    void update_estimate(int len, edge_t edges) override {
        array<int, N_VARS_LOCAL> simple_edges;
        simple_edges.fill(0);

        REP(m, 2*M) {
            if (edges[m]) {
                int y, x, dir;
                if (m < M) { // 横
                    y = m / (N - 1); x = m % (N - 1); dir = 0;
                } else { // 縦
                    int mm = m - M; x = mm / (N - 1); y = mm % (N - 1); dir = 1;
                }
                simple_edges[get_var_idx(dir, y, x)] += 1;
            }
        }

        const auto est_y = inner_product(simple_edges, hv);
        const auto err = len - est_y;
        
        // 推定値ベースでノイズ分散を計算
        //const double R = pow(0.2 * max(100, est_y), 2) / 12.0;
        const double R = pow(0.2 * max(100, len), 2) / 12.0;
        double S = R;
        
        array<double, N_VARS_LOCAL> cP;
        cP.fill(0.0);
        REP(j, N_VARS_LOCAL) {
            REP(i, N_VARS_LOCAL) cP[j] += simple_edges[i] * hv_var[i][j];
            S += cP[j] * simple_edges[j];
        }

        array<double, N_VARS_LOCAL> k;
        k.fill(0.0);
        REP(i, N_VARS_LOCAL) {
            double tmp = 0;
            REP(j, N_VARS_LOCAL) tmp += hv_var[i][j] * simple_edges[j];
            k[i] = tmp / S;
        }

        REP(i, N_VARS_LOCAL) {
            hv[i] = max(10.0, hv[i] + k[i] * err);
        }

        REP(i, N_VARS_LOCAL) {
            REP(j, N_VARS_LOCAL) {
                hv_var[i][j] -= k[i] * cP[j];
            }
        }
        
        // === 安定化処理 (実験のためコメントアウト) ===
        /*
        REP(i, N_VARS_LOCAL) {
             REP(j, i) {
                double val = (hv_var[i][j] + hv_var[j][i]) * 0.5;
                hv_var[i][j] = val;
                hv_var[j][i] = val;
            }
            if(hv_var[i][i] < 1.0) hv_var[i][i] = 1.0;
            
            // プロセスノイズ
            hv_var[i][i] += 50.0;
        }
        */
        // ==========================================

        log_likelihood -= (log(S) + err * err / S) / 2;
    }

    double get_edge_weight(int dir, int y, int x) const override {
        int v_idx = get_var_idx(dir, y, x);
        return hv[v_idx];
    }
    
    double get_variance(int dir, int y, int x) const override {
        int v_idx = get_var_idx(dir, y, x);
        return hv_var[v_idx][v_idx];
    }
};

pair<vector<int>, edge_t> get_path(int src, int dst, const BaseModel& model) {
    vector<double> dist(N * N, 1e18);
    vector<int> prev_node(N * N, -1);
    vector<int> prev_dir(N * N, -1);
    
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;
        if (u == dst) break;

        auto [y, x] = dec(u);

        REP(dir, 4) {
            int ny = y + dy[dir];
            int nx = x + dx[dir];
            if (ny < 0 || ny >= N || nx < 0 || nx >= N) continue;

            int v = enc(ny, nx);
            
            int ty = y, tx = x;
            if (dir == 2) tx = x - 1; 
            if (dir == 3) ty = y - 1; 
            
            double mu = model.get_edge_weight(dir, ty, tx);
            double sigma = sqrt(model.get_variance(dir, ty, tx));
            double alpha = 1.0; 
            double weight = max(10.0, mu - alpha * sigma);

            if (dist[v] > dist[u] + weight) {
                dist[v] = dist[u] + weight;
                prev_node[v] = u;
                prev_dir[v] = dir;
                pq.push({dist[v], v});
            }
        }
    }

    vector<int> path;
    edge_t edges;
    int curr = dst;
    while (curr != src) {
        int dir = prev_dir[curr];
        int prev = prev_node[curr];
        path.push_back(dir);
        
        auto [py, px] = dec(prev);
        int edge_idx = -1;
        if (dir == 0) edge_idx = py * (N - 1) + px;
        else if (dir == 1) edge_idx = M + px * (N - 1) + py;
        else if (dir == 2) edge_idx = py * (N - 1) + (px - 1);
        else if (dir == 3) edge_idx = M + px * (N - 1) + (py - 1);
        
        if(edge_idx != -1) edges.set(edge_idx);
        curr = prev;
    }
    reverse(ALL(path));
    return {path, edges};
}

void solve(const double end_time) {
    // マルチ解像度モデルの構築
    vector<unique_ptr<BaseModel>> models;
    
    // 解像度1 (60変数) - 低解像度、高安定
    models.push_back(make_unique<Model<1>>(500));
    models.push_back(make_unique<Model<1>>(1000));
    models.push_back(make_unique<Model<1>>(1500));
    
    // 解像度2 (120変数) - 中解像度、バランス
    // Dを変えてバリエーションを持たせる
    models.push_back(make_unique<Model<2>>(500));
    models.push_back(make_unique<Model<2>>(1000));
    models.push_back(make_unique<Model<2>>(1500));
    
    // 解像度4 (240変数) - 高解像度、情報が必要
    models.push_back(make_unique<Model<4>>(500));
    models.push_back(make_unique<Model<4>>(1000));
    models.push_back(make_unique<Model<4>>(1500));

    Query query;
    REP(k, K) {
        const auto [s, t] = query.get_query();
        
        // 尤度が最大のモデルを選択
        const auto& best_model = *max_element(ALL(models), [](const auto& a, const auto& b) {
            return a->log_likelihood < b->log_likelihood;
        });
        
        // デバッグ用: どの解像度が選ばれたかを確認したい場合は以下を有効化
        DEBUG("k={}, Selected Vars={}, Likelihood={}\n", k, best_model->n_vars, best_model->log_likelihood);

        const auto [path, edges] = get_path(s, t, *best_model);
        const auto len = query.put_path(path);
        
        // 全モデル更新
        for (auto& model : models) {
            model->update_estimate(len, edges);
        }
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]){
    ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    solve(Env::time_limit);
    DEBUG("time : {}\n", timer.get_time());
    return 0;
}