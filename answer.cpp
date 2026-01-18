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

// 【変更点1】変数の数を4倍(120個)にする
// 0-29: Row Left, 30-59: Row Right, 60-89: Col Top, 90-119: Col Bottom
constexpr int N_VARS = 4 * N;

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

// ... (Utility functions omitted for brevity, same as before) ...
unsigned int randxor(){ static unsigned int x=123456789,y=362436069,z=521288629,w=88675123; unsigned int t=(x^(x<<11));x=y;y=z;z=w; return(w=(w^(w>>19))^(t^(t>>8))); }
double rand01(){ return 1.0*randxor()/numeric_limits<unsigned int>::max(); }
int rand_int(const int left, const int right){ return randxor()%(right-left)+left; }
struct Timer { std::chrono::_V2::system_clock::time_point sp; Timer():sp(system_clock::now()){} double get_time()const{return duration_cast<microseconds>(system_clock::now()-sp).count()*1e-6;} };
Timer timer;

constexpr int M = N * (N - 1); 
using edge_t = bitset<2 * M>;

int enc(int y, int x) { return y*N + x; }
pair<int, int> dec(int v) { return {v / N, v % N}; }

// 【変更点2】座標と方向から変数のインデックス(0~119)を取得する関数
int get_var_idx(int dir, int y, int x) {
    if (dir == 0 || dir == 2) { // 横移動 (Row)
        // xが中央(15)より左なら前半(0~29), 右なら後半(30~59)
        int is_right = (x >= N / 2);
        return y + (is_right ? N : 0);
    } else { // 縦移動 (Col)
        // yが中央(15)より上なら前半(60~89), 下なら後半(90~119)
        int is_bottom = (y >= N / 2);
        return 2 * N + x + (is_bottom ? N : 0);
    }
}

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

struct Model {
    int D;
    array<double, N_VARS> hv; // サイズ変更
    array<array<double, N_VARS>, N_VARS> hv_var; // サイズ変更
    double log_likelihood;

    Model(int D) : D(D), log_likelihood(0.0) {
        for (auto& v : hv) v = 5000.0;
        // 初期分散の設定
        // M=2対応で変数を分割したが、初期状態では「左右(上下)は同じ値に近い」という相関を入れるのが理想
        // 今回は簡単のため独立として初期化するが、プロセスノイズで調整される
        REP(i, N_VARS) {
            REP(j, N_VARS) {
                hv_var[i][j] = (i == j) ? pow(8000 - 2*D, 2) / 12 : 0.0;
            }
        }
    }

    void update_estimate(int len, edge_t edges) {
        array<int, N_VARS> simple_edges;
        simple_edges.fill(0);

        // edge_t から 変数IDへのマッピング
        REP(m, 2*M) {
            if (edges[m]) {
                int y, x, dir;
                if (m < M) { // 横辺
                    y = m / (N - 1);
                    x = m % (N - 1); // (y, x) -> (y, x+1)
                    // xとx+1の間の辺。
                    // マッピングの都合上、左側のセル(x)基準で判定するか、辺の中点で判定するか。
                    // ここでは「辺がある場所」が右半分か左半分かで判定
                    dir = 0; // Rとして扱う
                } else { // 縦辺
                    int mm = m - M;
                    x = mm / (N - 1); // 縦方向の並び順注意
                    y = mm % (N - 1); // (y, x) -> (y+1, x)
                    dir = 1; // Dとして扱う
                }
                
                // M=2対応: 辺の位置に応じて適切な変数IDを加算
                // 横辺のxは 0~28. 14以下なら左, 15以上なら右とみなす
                // 縦辺のyは 0~28. 14以下なら上, 15以上なら下とみなす
                int v_idx = get_var_idx(dir, y, x);
                simple_edges[v_idx] += 1;
            }
        }

        const auto est_y = inner_product(simple_edges, hv);
        const auto err = len - est_y;
        
        // 推定値ベースでノイズを見積もる方が安定する
        const double R = pow(0.2 * max(100, est_y), 2) / 12.0; 
        double S = R;
        
        array<double, N_VARS> cP;
        cP.fill(0.0);
        REP(j, N_VARS) {
            REP(i, N_VARS) cP[j] += simple_edges[i] * hv_var[i][j];
            S += cP[j] * simple_edges[j];
        }

        array<double, N_VARS> k;
        k.fill(0.0);
        REP(i, N_VARS) {
            double tmp = 0;
            REP(j, N_VARS) tmp += hv_var[i][j] * simple_edges[j];
            k[i] = tmp / S;
        }

        REP(i, N_VARS) {
            hv[i] = max(0.0, hv[i] + k[i] * err); // 負になるとDijkstraが壊れるので下限設定
        }

        REP(i, N_VARS) {
            REP(j, N_VARS) {
                hv_var[i][j] -= k[i] * cP[j];
            }
        }
        
        // 【変更点3】安定化処理の復元（これが無いとM=2対応しても精度が出ない）
        REP(i, N_VARS) {
            REP(j, i) {
                double val = (hv_var[i][j] + hv_var[j][i]) * 0.5;
                //hv_var[i][j] = val;
                //hv_var[j][i] = val;
            }
            //if (hv_var[i][i] < 1e-4) hv_var[i][i] = 1e-4;
            
            // プロセスノイズ (忘却効果)
            // これにより、クエリ後半でも柔軟に「実はここコスト高かった」と修正できるようになる
            //hv_var[i][i] += 100.0; 
        }

        log_likelihood -= (log(S) + err * err / S) / 2;
    }
};

pair<vector<int>, edge_t> get_path(int src, int dst, const Model& model) {
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
            
            // 【変更点】get_var_idxを使ってコストを取得
            // 辺の座標判定:
            // (y,x) -> (y,x+1) : 横辺, 座標は(y,x)で判定
            // (y,x) -> (y+1,x) : 縦辺, 座標は(y,x)で判定
            // (y,x) -> (y,x-1) : 横辺, 座標は(y,x-1)で判定
            // (y,x) -> (y-1,x) : 縦辺, 座標は(y-1,x)で判定
            int ty = y, tx = x;
            if (dir == 2) tx = x - 1; // L
            if (dir == 3) ty = y - 1; // U
            
            int v_idx = get_var_idx(dir, ty, tx);
            
            // 探索と活用: 平均 - alpha * 標準偏差
            double mu = model.hv[v_idx];
            double sigma = sqrt(model.hv_var[v_idx][v_idx]);
            double alpha = 1.0; // 探索係数（調整の余地あり）
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
        // マッピングは変えず、edge_tの記録のためだけに残す
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
    vector<Model> models;
    // Dのバリエーション
    models.emplace_back(500);
    models.emplace_back(1000);
    models.emplace_back(1500);

    Query query;
    REP(k, K) {
        // デバッグ出力
        // vector<double> likelihoods;
        // for (const auto& model : models) likelihoods.push_back(model.log_likelihood);
        // DEBUG("k = {}, likelihoods = {}\n", k, likelihoods);

        const auto [s, t] = query.get_query();
        
        // 尤度が最大のモデルを採用
        const auto& best_model = *max_element(ALL(models), [](const Model& a, const Model& b) {
            return a.log_likelihood < b.log_likelihood;
        });
        
        const auto [path, edges] = get_path(s, t, best_model);
        const auto len = query.put_path(path);
        
        for (auto& model : models) {
            model.update_estimate(len, edges);
        }
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]){
    ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    solve(Env::time_limit);
    return 0;
}