#include <bits/stdc++.h>

using namespace std;

typedef long long ll;
typedef unsigned long long ull;
#define REP(i, n) for(int i=0; i<n; i++)
#define REPi(i, a, b) for(int i=int(a); i<int(b); i++)
#define MEMS(a,b) memset(a,b,sizeof(a))
#define mp make_pair
#define MOD(a, m) ((a % m + m) % m)
template<class T>bool chmax(T &a, const T &b) { if (a<b) { a=b; return 1; } return 0; }
template<class T>bool chmin(T &a, const T &b) { if (b<a) { a=b; return 1; } return 0; }
const ll MOD = 1e9+7;
const ll INF = 1LL << 60;

//// number of queries
const int NUM_Q = 1000;
///const int NUM_Q = 5000;
///const int NUM_Q = 10000;

//// size of the grid
const int NUM_GRID = 30;

//// for directions
enum class Dir{U, D, L, R};

struct Pos;
struct Field;
Dir int2dir(int n);
int dir2int(Dir dir);
string path2string(vector<Dir>& path);

//// convert path to string(answer).
string path2string(vector<Dir>& path){
    string ret;
    const char dir_strs[] = {'U', 'D', 'L', 'R'};
    for(auto& dir : path){
        ret.push_back(dir_strs[dir2int(dir)]);
    }

    return ret;
}

//// convert integer to Dir
Dir int2dir(int n){
    switch(n){
        case 0: return Dir::U;
        case 1: return Dir::D;
        case 2: return Dir::L;
        case 3: return Dir::R;
        default: assert(false);
    }
}

//// convert Dir to integer
int dir2int(Dir dir){
    switch(dir){
        case Dir::U: return 0;
        case Dir::D: return 1;
        case Dir::L: return 2;
        case Dir::R: return 3;
        default: assert(false);
    }
}

//// position for palyer and goal
struct Pos{
    int y, x;

    Pos(int _y, int _x) : y(_y), x(_x) {};

    void next(Dir dir){
        switch(dir){
            case Dir::U: y--; break;
            case Dir::D: y++; break;
            case Dir::L: x--; break;
            case Dir::R: x++; break;
        }
    }

    bool operator==(const Pos& rhs){
        return (y == rhs.y) && (x == rhs.x);
    }
    bool operator!=(const Pos& rhs){
        return !operator==(rhs);
    }
    friend ostream& operator<<(ostream& os, const Pos& pos);
};

ostream& operator<<(ostream& os, const Pos& pos){
    os << "Pos(" << pos.y << ", " << pos.x << ")";
    return os;
}

//// for graph analysis
struct Field{
    //// row[i][j] := i列目のj番目の辺 : (j, i) --> (j+1, i)
    array<array<ll, NUM_GRID-1>, NUM_GRID> row;
    //// col[i][j] := i行目のj番目の辺 : (i, j) --> (i, j+1)
    array<array<ll, NUM_GRID-1>, NUM_GRID> col;

    ll init;
    /// for random value
    mt19937 engine;

    Field(ll _init) : init(_init){
        ////TODO:
        //random_device seed_gen;
        //engine = mt19937(seed_gen());
        engine = mt19937(1);

        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                row[i][j] = init;
            }
        }
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                col[i][j] = init;
            }
        }
    }

    //// (y, x) から dir方向の辺の重みを取得
    ll& get_dist(int y, int x, Dir dir){
        switch(dir){
            case Dir::U: return row[x][y-1];
            case Dir::D: return row[x][y];
            case Dir::L: return col[y][x-1];
            case Dir::R: return col[y][x];
            default: assert(false);
        }
    }

    //// 頂点sから各頂点への経路長を計算
    void dijkstra(Pos s, vector<vector<ll>>& D){
        D[s.y][s.x] = 0;
        //// {distance, y, x}
        using T = tuple<ll, int, int>;
        priority_queue<T, vector<T>, greater<T>> que;
        que.push(T(0, s.y, s.x));
        while(!que.empty()){
            auto [d, y, x] = que.top();
            que.pop();
            if(D[y][x] < d) continue;
            const int dy[] = {-1, 1, 0, 0};
            const int dx[] = {0, 0, -1, 1};
            for(int dir = 0; dir < 4; dir++){
                int ny = y + dy[dir];
                int nx = x + dx[dir];
                if(ny < 0 || nx < 0) continue;
                if(ny >= NUM_GRID || nx >= NUM_GRID) continue;
                ll nd = d + get_dist(y, x, int2dir(dir));

                if(nd < D[ny][nx]){
                    D[ny][nx] = nd;
                    que.push(T(nd, ny, nx));
                }
            }
        }
    }

    //// 経路長Dからgoal --> startの経路を復元
    vector<Dir> restore_path(Pos start, Pos goal, vector<vector<ll>>& D){
        vector<Dir> path;

        auto player = Pos(goal);
        const int dy[] = {1, -1, 0, 0};
        const int dx[] = {0, 0, 1, -1};
        static uniform_int_distribution<> rand(0, 3);
        array<int, 4> dirs = {0, 1, 2, 3};
        while(player != start){
            //// ランダムな方角から探索
            //// 経路長推定のためにやめるべき？
            //shuffle(dirs.begin(), dirs.end(), engine);
            for(int i = 0; i < 4; i++){
                int dir = dirs[i];
                int ny = player.y + dy[dir];
                int nx = player.x + dx[dir];
                if(ny < 0 || nx < 0) continue;
                if(ny >= NUM_GRID || nx >= NUM_GRID) continue;

                if(D[player.y][player.x] == (D[ny][nx] + get_dist(ny, nx, int2dir(dir)))){
                    player = Pos(ny, nx);
                    path.push_back(int2dir(dir));
                    break;
                }

                if(i == 3)
                    assert(false);
            }
        }

        reverse(path.begin(), path.end());
        return path;
    }

    vector<Dir> get_path(Pos start, Pos goal){
        /// temporary distance
        vector<vector<ll>> D(NUM_GRID, vector<ll>(NUM_GRID, INF));
        dijkstra(start, D);
        auto path = restore_path(start, goal, D);
        return path;
    }

    //// 通った経路(+上下左右の経路)をscoreの平均で更新
    //// TODO::前半は推定値の変化を大きくする？
    void update_path_uniform(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
        //// 平均の経路長で、経路長の推定値を更新
        ll ave_score = score / path.size();

        //// 通った行・列の辺の重みを全て更新
        //// スコアがかなり落ちた.なぜ？
        unordered_map<int, int> cols, rows;

        //// 通った辺を保存
        using P = pair<int, int>;
        set<P> used_row;
        set<P> used_col;

        Pos player(start);
        for(auto& dir : path){
            if(dir == Dir::U){
                cols[player.x]++;
                used_row.insert(P(player.x, player.y-1));
            }
            else if(dir == Dir::D){
                cols[player.x]++;
                used_row.insert(P(player.x, player.y));
            }
            else if(dir == Dir::L){
                rows[player.y]++;
                used_col.insert(P(player.y, player.x-1));
            }
            else if(dir == Dir::R){
                rows[player.y]++;
                used_col.insert(P(player.y, player.x));
            }
            player.next(dir);
        }

        //// いらない？
        //static uniform_int_distribution<> rand(-1050, 1050);

        ll update_score = ave_score;
        auto update_edges = [&update_score, &q_idx](auto& edge_weight, auto& edges, auto& used_edges){
            const int CNT = 7;

            //// 0 <= P <= 10
            //// 初期値・最終値
            const double P0A = 2,  P0B = 1;
            const double P1A = 4, P1B = 4;
            const double P2A = 2,  P2B = 1;
            //// その行をあまり使わなかった場合における、実際に通った辺の更新
            //const double P0 = 3;
            const double P0 = P0A + ((double)(P0B - P0A) / NUM_Q) * q_idx;
            //// その行をよく使った場合における、実際に通った辺の更新
            //const double P1 = 5;
            const double P1 = P1A + ((double)(P1B - P1A) / NUM_Q) * q_idx;
            //// その行をよく使った場合における、実際に通らなかった辺の更新
            //// TODO: 離れるほど減衰
            //const double P2 = 3;
            const double P2 = P2A + ((double)(P2B - P2A) / NUM_Q) * q_idx;

            for(auto& [r, cnt] : edges){
                REP(i, NUM_GRID-1){
                    if(cnt < CNT){
                        //// M == 1 では悪くなる？
                        //// M == 2 では良くなる？
                        if(used_edges.find(P(r, i)) != used_edges.end())
                            edge_weight[r][i] = ((10-P0)*edge_weight[r][i] + P0*update_score) / 10.0;
                    }
                    else{
                        if(used_edges.find(P(r, i)) != used_edges.end())
                            edge_weight[r][i] = ((10-P1)*edge_weight[r][i] + P1*update_score) / 10.0;
                        else
                            edge_weight[r][i] = ((10-P2)*edge_weight[r][i] + P2*update_score) / 10.0;
                    }
                }
            }
        };

        update_edges(col, rows, used_col);
        update_edges(row, cols, used_row);

    }

    //// ある行・列に対し、通った辺を中心として更新値を減衰
    void update_path_decay(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
        //// 辺の重み
        vector<vector<double>> row_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));
        vector<vector<double>> col_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));

        //// 現在のパス長を基にしたスコアの推定値
        ll predicted_score = 0;

        //// 畳み込み
        const int KER = 1;
        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            if(dir == Dir::U){
                predicted_score += row[x][y-1];
                for(int k = -KER; k <= KER; k++){
                    if(y-1+k < 0 || NUM_GRID-1 <= y-1+k) continue;
                    row_val[x][y-1+k] += 1 + (KER - abs(k));
                }
            }
            else if(dir == Dir::D){
                predicted_score += row[x][y];
                for(int k = -KER; k <= KER; k++){
                    if(y+k < 0 || NUM_GRID-1 <= y+k) continue;
                    row_val[x][y+k] += 1 + (KER - abs(k));
                }
            }
            else if(dir == Dir::L){
                predicted_score += col[y][x-1];
                for(int k = -KER; k <= KER; k++){
                    if(x-1+k < 0 || NUM_GRID-1 <= x-1+k) continue;
                    col_val[y][x-1+k] += 1 + (KER - abs(k));
                }
            }
            else if(dir == Dir::R){
                predicted_score += col[y][x];
                for(int k = -KER; k <= KER; k++){
                    if(x+k < 0 || NUM_GRID-1 <= x+k) continue;
                    col_val[y][x+k] += 1 + (KER - abs(k));
                }
            }
            player.next(dir);
        }

        //cerr << score << " " << predicted_score << endl;
        //if(q_idx % 100 == 0)
        //    cerr << abs(score - predicted_score) << endl;

        vector<double> row_max(NUM_GRID, 0), col_max(NUM_GRID, 0);
        auto calc_ratio = [](auto& edge_val, auto& edge_max){
            REP(i, NUM_GRID){
                ll sum = 0;
                REP(j, NUM_GRID-1)
                    sum += edge_val[i][j];
                if(sum == 0) continue;
                REP(j, NUM_GRID-1){
                    edge_val[i][j] /= sum;
                    chmax(edge_max[i], edge_val[i][j]);
                }
            }
        };
        calc_ratio(row_val, row_max);
        calc_ratio(col_val, col_max);

        auto edge_update = [&q_idx, &score, &path, &predicted_score](auto& edge_weight, auto& edge_val, auto& edge_max){
            ////TODO: 問題毎にこの辺を可変にしたい
            //const double PA = 0.8,  PB = 0.2;
            const double PA = 0.8,  PB = 0.4;
            //const double PA = 0.3,  PB = 0.2;
            //const double PA = 0.6,  PB = 0.1;
            //const double PA = 0.1,  PB = 0.01;
            const double Pmax = PA + ((double)(PB - PA)/NUM_Q) * q_idx;
            REP(i, NUM_GRID){
                if(edge_max[i] < 0.0001) continue;
                REP(j, NUM_GRID-1){
                    //const double update_score = (double)score / path.size();
                    const double update_score = (double)score * ((double)edge_weight[i][j] / predicted_score);
                    const double P = Pmax * (edge_val[i][j] / edge_max[i]);
                    //cerr << P << endl;
                    edge_weight[i][j] = ((1-P)*edge_weight[i][j] + P*update_score);
                }
            }
        };
        edge_update(row, row_val, row_max);
        edge_update(col, col_val, col_max);

    }

    //// 得られた経路長から、dist配列を更新 (推定)
    void update_path(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
        //update_path_uniform(q_idx, start, goal, score, path);
        update_path_decay(q_idx, start, goal, score, path);
    }
};

////////////////////////////////////////////////////////

//// naive answer
vector<Dir> path_naive(Pos player, Pos goal){
    vector<Dir> path;

    //// 縦横を交互に決定
    int idx = 0;
    while(player != goal){
        Dir dir;
        if(idx % 2 == 0){
            if(player.y < goal.y)
                dir = Dir::D;
            else if(player.y > goal.y)
                dir = Dir::U;
            else if(player.x < goal.x)
                dir = Dir::R;
            else if(player.x > goal.x)
                dir = Dir::L;
        }
        else{
            if(player.x < goal.x)
                dir = Dir::R;
            else if(player.x > goal.x)
                dir = Dir::L;
            else if(player.y < goal.y)
                dir = Dir::D;
            else if(player.y > goal.y)
                dir = Dir::U;
        }

        player.next(dir);
        path.push_back(dir);
        ////TODO: 直線で探索すべき？
        //idx = (idx + 1) % 2;
    }

    return path;
}

vector<Dir> answer(int q_idx, Pos start, Pos goal, Field& field){
    vector<Dir> path;
    if(q_idx < 0){
        path = path_naive(start, goal);
    }
    else if(q_idx < 0){
        Pos mid((start.y + goal.y)/2, (start.x + goal.x)/2);
        path = path_naive(start, mid);
        auto tmp = path_naive(mid, goal);
        for(auto& t : tmp)
            path.push_back(t);
    }
    else
        path = field.get_path(start, goal);
    return path;
}

int main(){
    //// TODO
    //// 未探索の経路から優先的に使用?
    //Field field(5000);
    Field field(3000);
    //Field field(100);
    for(int qi = 0; qi < NUM_Q; qi++){
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        Pos player(si, sj), goal(ti, tj);

        vector<Dir> path = answer(qi, player, goal, field);
        string ans = path2string(path);
        cout << ans << endl;

        ll score;
        cin >> score;
        field.update_path(qi, player, goal, score, path);
    }

    return 0;
}
