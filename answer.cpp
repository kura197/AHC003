#ifdef ONLINE_JUDGE
#pragma GCC optimize "Ofast,omit-frame-pointer,inline,unroll-all-loops"
#define SUBMIT
#else
//#define SUBMIT
#endif

#include <bits/stdc++.h>
#include <format>

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

// ==========================================
// Debug Utils
// ==========================================
template<typename T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    os << "[";
    for(size_t i=0; i<v.size(); ++i) {
        os << v[i];
        if(i != v.size() - 1) os << ", ";
    }
    os << "]";
    return os;
}

#ifdef SUBMIT
#define DEBUG(fmt, ...) ;
#else
#define DEBUG(fmt_str, ...) std::cerr << std::format(fmt_str __VA_OPT__(,) __VA_ARGS__)
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
constexpr int TOTAL_EDGES = 2 * M; // 1740

using edge_t = bitset<TOTAL_EDGES>;

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

// 構造の仮定 (ModelEdge用)
enum class Structure {
    NONE, // Grid Model用
    M1,   // 全結合的相関
    M2    // 距離減衰相関
};

// 基底クラス
struct BaseModel {
    double log_likelihood;
    int n_vars;    // デバッグ用: 変数の数
    Structure type;// デバッグ用: 構造タイプ
    int D;         // デバッグ用: パラメータD
    
    BaseModel(int n, Structure t, int d) : log_likelihood(0.0), n_vars(n), type(t), D(d) {}
    virtual ~BaseModel() = default;

    virtual void update_estimate(int len, edge_t edges) = 0;
    virtual double get_edge_weight(int dir, int y, int x) const = 0;
    virtual double get_variance(int dir, int y, int x) const = 0;
};

// ==========================================================
// 従来モデル (60, 120, 240変数)
// ==========================================================
template <int DIV>
struct Model : public BaseModel {
    static constexpr int N_VARS_LOCAL = 2 * N * DIV;
    
    array<double, N_VARS_LOCAL> hv;
    array<array<double, N_VARS_LOCAL>, N_VARS_LOCAL> hv_var;

    Model(int D) : BaseModel(N_VARS_LOCAL, Structure::NONE, D) {
        hv.fill(5000.0);
        // 初期分散
        REP(i, N_VARS_LOCAL) {
            REP(j, N_VARS_LOCAL) {
                hv_var[i][j] = (i == j) ? pow(8000 - 2*D, 2) / 12 : 0.0;
            }
        }
    }

    int get_var_idx(int dir, int y, int x) const {
        if (dir == 0 || dir == 2) { // 横 (Row)
            int seg = (x * DIV) / N;
            if (seg >= DIV) seg = DIV - 1;
            return y * DIV + seg;
        } else { // 縦 (Col)
            int seg = (y * DIV) / N;
            if (seg >= DIV) seg = DIV - 1;
            return N * DIV + x * DIV + seg;
        }
    }

    void update_estimate(int len, edge_t edges) override {
        array<int, N_VARS_LOCAL> simple_edges;
        simple_edges.fill(0);

        REP(m, TOTAL_EDGES) {
            if (edges[m]) {
                int y, x, dir;
                if (m < M) { y = m / (N - 1); x = m % (N - 1); dir = 0; }
                else { int mm = m - M; x = mm / (N - 1); y = mm % (N - 1); dir = 1; }
                simple_edges[get_var_idx(dir, y, x)] += 1;
            }
        }

        const auto est_y = inner_product(simple_edges, hv);
        const auto err = len - est_y;
        
        const double R = pow(0.2 * max(100.0, (double)len), 2) / 12.0;
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

        REP(i, N_VARS_LOCAL) hv[i] = max(10.0, hv[i] + k[i] * err);
        REP(i, N_VARS_LOCAL) REP(j, N_VARS_LOCAL) hv_var[i][j] -= k[i] * cP[j];

        // === 安定化処理 (コメントアウト中) ===
        /*
        REP(i, N_VARS_LOCAL) {
             REP(j, i) {
                double val = (hv_var[i][j] + hv_var[j][i]) * 0.5;
                hv_var[i][j] = val;
                hv_var[j][i] = val;
            }
            if(hv_var[i][i] < 1.0) hv_var[i][i] = 1.0;
            hv_var[i][i] += 50.0; // プロセスノイズ
        }
        */
        // ===================================

        log_likelihood -= (log(S) + err * err / S) / 2;
    }

    double get_edge_weight(int dir, int y, int x) const override {
        return hv[get_var_idx(dir, y, x)];
    }
    double get_variance(int dir, int y, int x) const override {
        int idx = get_var_idx(dir, y, x);
        return hv_var[idx][idx];
    }
};

// ==========================================================
// 1740変数モデル (Block Diagonal Covariance)
// ==========================================================
struct ModelEdge : public BaseModel {
    static constexpr int B_SIZE = N - 1; // 29
    static constexpr int N_BLOCKS = 2 * N; // 60 blocks (30 rows + 30 cols)
    
    array<double, TOTAL_EDGES> hv;
    
    // 共分散行列をブロック対角化して保持
    vector<array<double, B_SIZE * B_SIZE>> blocks; 

    ModelEdge(Structure type, int D) : BaseModel(TOTAL_EDGES, type, D) {
        hv.fill(5000.0);
        blocks.resize(N_BLOCKS);
        
        // パラメータ設定
        double base_var = pow(6000, 2) / 12.0;
        double noise_width = 2.0 * D; 
        double noise_var = pow(noise_width, 2) / 12.0;

        REP(b, N_BLOCKS) {
            REP(i, B_SIZE) {
                REP(j, B_SIZE) {
                    int idx = i * B_SIZE + j;
                    if (i == j) {
                        blocks[b][idx] = base_var + noise_var;
                    } else {
                        if (type == Structure::M1) {
                            // M1: どこでも強い相関
                            blocks[b][idx] = base_var * 0.95; 
                        } else {
                            // M2: 距離に応じた線形減衰
                            double dist = abs(i - j);
                            double correlation = max(0.0, 1.0 - (dist / 28.0));
                            blocks[b][idx] = base_var * correlation;
                        }
                    }
                }
            }
        }
    }

    pair<int, int> get_block_info(int edge_idx) const {
        if (edge_idx < M) { // 横辺
            int row = edge_idx / (N - 1);
            int col = edge_idx % (N - 1);
            return {row, col};
        } else { // 縦辺
            int idx = edge_idx - M;
            int col = idx / (N - 1);
            int row = idx % (N - 1);
            return {N + col, row};
        }
    }

    void update_estimate(int len, edge_t edges) override {
        static vector<vector<int>> simple_edges(N_BLOCKS); 
        REP(b, N_BLOCKS) {
            if(!simple_edges[b].empty()) fill(ALL(simple_edges[b]), 0);
            else simple_edges[b].resize(B_SIZE, 0);
        }
        
        vector<int> active_blocks; 

        REP(m, TOTAL_EDGES) {
            if (edges[m]) {
                auto [bid, lid] = get_block_info(m);
                if (simple_edges[bid][0] == 0 && inner_product(simple_edges[bid], simple_edges[bid]) == 0) {
                     active_blocks.push_back(bid);
                }
                simple_edges[bid][lid] = 1;
            }
        }

        double est_y = 0;
        REP(m, TOTAL_EDGES) if (edges[m]) est_y += hv[m];
        
        const double err = len - est_y;
        const double R = pow(0.2 * max(100.0, (double)len), 2) / 12.0;
        double S = R;
        
        static vector<vector<double>> cP_storage(N_BLOCKS);
        REP(b, N_BLOCKS) if(cP_storage[b].size() != B_SIZE) cP_storage[b].resize(B_SIZE);

        for (int b : active_blocks) {
            REP(i, B_SIZE) {
                double val = 0;
                int row_offset = i * B_SIZE;
                REP(j, B_SIZE) {
                    if (simple_edges[b][j]) val += blocks[b][row_offset + j];
                }
                cP_storage[b][i] = val;
            }

            double block_S = 0;
            REP(j, B_SIZE) {
                if (simple_edges[b][j]) block_S += cP_storage[b][j];
            }
            S += block_S;
        }

        for (int b : active_blocks) {
            REP(i, B_SIZE) {
                double k = cP_storage[b][i] / S;
                int edge_idx = (b < N) ? (b * (N - 1) + i) : (M + (b - N) * (N - 1) + i);
                hv[edge_idx] = max(10.0, hv[edge_idx] + k * err);
                
                int row_offset = i * B_SIZE;
                REP(j, B_SIZE) {
                    blocks[b][row_offset + j] -= k * cP_storage[b][j];
                }
            }
        }
        
        // === 安定化処理 (コメントアウト中) ===
        /*
        for (int b : active_blocks) {
             REP(i, B_SIZE) {
                 // 対角成分クリップ & プロセスノイズ
                 int idx = i*B_SIZE+i;
                 if (blocks[b][idx] < 1.0) blocks[b][idx] = 1.0;
                 blocks[b][idx] += 50.0; 
                 // 対称化はブロック全体に対して行う必要があるが計算量削減のため省略可、
                 // あるいは適宜行う
             }
        }
        */
        // ===================================

        log_likelihood -= (log(S) + err * err / S) / 2;
    }

    double get_edge_weight(int dir, int y, int x) const override {
        int idx = -1;
        if (dir == 0) idx = y * (N - 1) + x;
        else idx = M + x * (N - 1) + y;
        return hv[idx];
    }
    
    double get_variance(int dir, int y, int x) const override {
        int idx = -1;
        if (dir == 0) idx = y * (N - 1) + x;
        else idx = M + x * (N - 1) + y;
        
        auto [bid, lid] = get_block_info(idx);
        return blocks[bid][lid * B_SIZE + lid];
    }
};

// ==========================================================
// パス探索 (アンサンブル + LCB)
// ==========================================================
pair<vector<int>, edge_t> get_path(int src, int dst, 
                                   const vector<unique_ptr<BaseModel>>& models, 
                                   const vector<double>& weights, 
                                   int k) {
    vector<double> dist(N * N, 1e18);
    vector<int> prev_node(N * N, -1);
    vector<int> prev_dir(N * N, -1);
    
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    dist[src] = 0;
    pq.push({0, src});

    // 探索係数 (動的alpha)
    double progress = (double)k / K;
    double alpha = 1.5 * pow(1.0 - progress, 2.0);

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
            
            // L, U は逆方向の辺 (R, D) として正規化
            int q_dir = (dir == 2) ? 0 : (dir == 3 ? 1 : dir);
            
            // アンサンブルコスト計算
            double combined_weight = 0.0;
            for(size_t i=0; i<models.size(); ++i) {
                if(weights[i] < 1e-4) continue; // 高速化

                double mu = models[i]->get_edge_weight(q_dir, ty, tx);
                double sigma = sqrt(max(0.0, models[i]->get_variance(q_dir, ty, tx)));
                double lcb = max(10.0, mu - alpha * sigma);
                
                combined_weight += weights[i] * lcb;
            }

            if (dist[v] > dist[u] + combined_weight) {
                dist[v] = dist[u] + combined_weight;
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
    vector<unique_ptr<BaseModel>> models;
    
    // --- Model Definition ---
    
    // 1. Grid Models (解像度バリエーション)
    models.push_back(make_unique<Model<1>>(600)); // 60vars
    models.push_back(make_unique<Model<1>>(1200)); // 60vars
    models.push_back(make_unique<Model<2>>(600)); // 120vars
    models.push_back(make_unique<Model<2>>(1200)); // 120vars
    models.push_back(make_unique<Model<4>>(600)); // 240vars
    models.push_back(make_unique<Model<4>>(1200)); // 240vars
    
    // 2. Edge Models (1740vars, M1/M2 Structure)
    // ノイズの大きさ D を変えてバリエーションを持たせる
    models.push_back(make_unique<ModelEdge>(Structure::M1, 250));
    models.push_back(make_unique<ModelEdge>(Structure::M1, 750));
    models.push_back(make_unique<ModelEdge>(Structure::M1, 1250));
    models.push_back(make_unique<ModelEdge>(Structure::M1, 1750));
    
    // M=2 仮定モデル (分離コスト)
    models.push_back(make_unique<ModelEdge>(Structure::M2, 250));
    models.push_back(make_unique<ModelEdge>(Structure::M2, 750));
    models.push_back(make_unique<ModelEdge>(Structure::M2, 1250));
    models.push_back(make_unique<ModelEdge>(Structure::M2, 1750));

    Query query;
    REP(k, K) {
        const auto [s, t] = query.get_query();
        
        // --- Softmax Weighting ---
        vector<double> weights(models.size());
        
        // max_ll 取得
        double max_ll = -1e18;
        for(const auto& m : models) chmax(max_ll, m->log_likelihood);
        
        // exp計算 & 合計
        double sum_exp = 0.0;
        REP(i, models.size()) {
            double diff = models[i]->log_likelihood - max_ll;
            // 極端に小さい値は0にして計算誤差を防ぐ
            if(diff < -20.0) weights[i] = 0.0;
            else weights[i] = exp(diff);
            sum_exp += weights[i];
        }
        
        // 正規化
        REP(i, models.size()) weights[i] /= sum_exp;
        
        // DEBUG: 支配的なモデル情報を表示
        #ifndef SUBMIT
        int best_idx = 0; double max_w = -1.0;
        REP(i, weights.size()) if(weights[i] > max_w) { max_w = weights[i]; best_idx = i; }
        string type_s = "Grid";
        if(models[best_idx]->type == Structure::M1) type_s = "M1";
        if(models[best_idx]->type == Structure::M2) type_s = "M2";
        DEBUG("k={:3d} | Main: {:4s}, n={}, D={:<4} (w={:.2f}) | MaxLL={:.1f}\n", 
              k, type_s, models[best_idx]->n_vars, models[best_idx]->D, max_w, max_ll);
        #endif

        const auto [path, edges] = get_path(s, t, models, weights, k);
        const auto len = query.put_path(path);
        
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