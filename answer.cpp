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

// 構造の仮定
enum class Structure {
    M1, // 全結合的相関 (行・列内で一様)
    M2  // 距離減衰相関 (行・列内で分離あり)
};

// 基底クラス
struct BaseModel {
    double log_likelihood;
    Structure type; // デバッグ用
    int D;          // デバッグ用
    
    BaseModel(Structure t, int d) : log_likelihood(0.0), type(t), D(d) {}
    virtual ~BaseModel() = default;

    virtual void update_estimate(int len, edge_t edges) = 0;
    virtual double get_edge_weight(int dir, int y, int x) const = 0;
    virtual double get_variance(int dir, int y, int x) const = 0;
};

// ==========================================================
// 1740変数モデル (Block Diagonal Covariance)
// 構造(M1/M2)の違いは初期共分散行列に埋め込む
// ==========================================================
struct ModelEdge : public BaseModel {
    static constexpr int B_SIZE = N - 1; // 29
    static constexpr int N_BLOCKS = 2 * N; // 60 blocks (30 rows + 30 cols)
    
    array<double, TOTAL_EDGES> hv;
    
    // 共分散行列をブロック対角化して保持
    vector<array<double, B_SIZE * B_SIZE>> blocks; 

    ModelEdge(Structure type, int D) : BaseModel(type, D) {
        hv.fill(5000.0);
        blocks.resize(N_BLOCKS);
        
        // パラメータ設定
        // base_var: H_i (行・列全体のベース) の分散
        // noise_var: delta_ij (個別の辺) の分散
        double base_var = pow(6000, 2) / 12.0;  // 幅6000程度と仮定
        
        // Dによってノイズの大きさを変える
        // Dが大きい = 個別性が強い = noise_varが大きい
        // Dが小さい = 全体性が強い = noise_varが小さい
        // ここでは簡易的に D=200~2000 の範囲で設定
        double noise_width = 2.0 * D; 
        double noise_var = pow(noise_width, 2) / 12.0;

        // 全ブロック共通の初期化
        REP(b, N_BLOCKS) {
            REP(i, B_SIZE) {
                REP(j, B_SIZE) {
                    int idx = i * B_SIZE + j;
                    if (i == j) {
                        // 対角成分: ベース分散 + 個別ノイズ分散
                        blocks[b][idx] = base_var + noise_var;
                    } else {
                        // 非対角成分: 構造仮定による共分散
                        if (type == Structure::M1) {
                            // M=1: どこでも強い相関 (距離によらない)
                            // 完全に1.0にすると特異になるので 0.95 程度にしておく
                            blocks[b][idx] = base_var * 0.95; 
                        } else {
                            // M=2: 距離に応じた線形減衰
                            double dist = abs(i - j);
                            double correlation = max(0.0, 1.0 - (dist / 28.0));
                            blocks[b][idx] = base_var * correlation;
                        }
                    }
                }
            }
        }
    }

    // 辺ID (0~1739) -> (BlockID, LocalID)
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
        // パスに含まれる辺をブロックごとに振り分け
        // simple_edges[block_id][local_id] = 1 or 0
        static vector<vector<int>> simple_edges(N_BLOCKS); // staticで再利用
        REP(b, N_BLOCKS) {
            if(!simple_edges[b].empty()) fill(ALL(simple_edges[b]), 0);
            else simple_edges[b].resize(B_SIZE, 0);
        }
        
        vector<int> active_blocks; // 今回更新対象のブロック

        REP(m, TOTAL_EDGES) {
            if (edges[m]) {
                auto [bid, lid] = get_block_info(m);
                if (simple_edges[bid][0] == 0 && inner_product(simple_edges[bid], simple_edges[bid]) == 0) {
                     // check empty logic strictly if needed, but here vector is reused
                     active_blocks.push_back(bid);
                }
                simple_edges[bid][lid] = 1;
            }
        }

        double est_y = 0;
        REP(m, TOTAL_EDGES) {
            if (edges[m]) est_y += hv[m];
        }
        
        const double err = len - est_y;
        const double R = pow(0.2 * max(100.0, (double)len), 2) / 12.0;
        double S = R;
        
        // ブロックごとの cP = P * h を計算
        // cP_storage[block_id][local_idx]
        static vector<vector<double>> cP_storage(N_BLOCKS);
        REP(b, N_BLOCKS) {
             if(cP_storage[b].size() != B_SIZE) cP_storage[b].resize(B_SIZE);
        }

        for (int b : active_blocks) {
            // cP[b] = P_b * simple_edges[b]
            // 行列ベクトル積
            REP(i, B_SIZE) {
                double val = 0;
                // P_b は対称行列だが blocks[b] には全成分入っている
                // スパース性を利用: simple_edges[b][j] == 1 の列だけ足す
                int row_offset = i * B_SIZE;
                REP(j, B_SIZE) {
                    if (simple_edges[b][j]) {
                        val += blocks[b][row_offset + j];
                    }
                }
                cP_storage[b][i] = val;
            }

            // S += h^T * P * h = h^T * cP
            double block_S = 0;
            REP(j, B_SIZE) {
                if (simple_edges[b][j]) {
                    block_S += cP_storage[b][j];
                }
            }
            S += block_S;
        }

        // 更新
        for (int b : active_blocks) {
            // Kalman Gain K = cP / S
            REP(i, B_SIZE) {
                double k = cP_storage[b][i] / S;
                
                // 変数更新
                int edge_idx = (b < N) ? (b * (N - 1) + i) : (M + (b - N) * (N - 1) + i);
                hv[edge_idx] = max(10.0, hv[edge_idx] + k * err);
                
                // 分散更新 P_new = P - K * S * K^T = P - cP * cP^T / S
                int row_offset = i * B_SIZE;
                REP(j, B_SIZE) {
                    blocks[b][row_offset + j] -= k * cP_storage[b][j];
                }
            }
        }
        
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

pair<vector<int>, edge_t> get_path(int src, int dst, const BaseModel& model, int k) {
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
            
            // L, U は逆方向の辺 (R, D) として参照
            int q_dir = (dir == 2) ? 0 : (dir == 3 ? 1 : dir);
            
            double mu = model.get_edge_weight(q_dir, ty, tx);
            double sigma = sqrt(max(0.0, model.get_variance(q_dir, ty, tx)));
            
            // LCB: 動的alpha
            double progress = (double)k / K;
            double alpha = 1.5 * (1.0 - progress); 
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
    vector<unique_ptr<BaseModel>> models;
    
    // M=1 仮定モデル (一様コスト)
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
        
        // 尤度最大モデル選択
        int best_idx = 0;
        double max_ll = -1e18;
        REP(i, models.size()) {
            if (models[i]->log_likelihood > max_ll) {
                max_ll = models[i]->log_likelihood;
                best_idx = i;
            }
        }
        
        const auto& m = models[best_idx];
        string type_str = (m->type == Structure::M1) ? "M1" : "M2";
        DEBUG("k={:3d} | Sel: {} (D={:4d}) | LL={:.1f}\n", k, type_str, m->D, max_ll);

        const auto [path, edges] = get_path(s, t, *m, k);
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