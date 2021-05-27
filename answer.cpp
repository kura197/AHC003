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
//const int NUM_Q = 10000;

//// size of the grid
const int NUM_GRID = 30;

//// for directions
enum class Dir{U, D, L, R};

struct Pos;
struct Field;
Dir int2dir(int n);
int dir2int(Dir dir);
string path2string(const vector<Dir>& path);

//// convert path to string(answer).
string path2string(const vector<Dir>& path){
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

//// reverse direction
Dir rev_dir(Dir dir){
    switch(dir){
        case Dir::U: return Dir::D;
        case Dir::D: return Dir::U;
        case Dir::L: return Dir::R;
        case Dir::R: return Dir::L;
        default: assert(false);
    }
}

//// position for palyer and goal
struct Pos{
    int y, x;

    Pos() : y(0), x(0) {};
    Pos(int _y, int _x) : y(_y), x(_x) {};

    void next(const Dir dir){
        switch(dir){
            case Dir::U: y--; break;
            case Dir::D: y++; break;
            case Dir::L: x--; break;
            case Dir::R: x++; break;
            default: assert(false);
        }
    }

    bool compare(const Pos& rhs){
        return (100*y + x) < (100*rhs.y + rhs.x);
    }

    bool operator==(const Pos& rhs){
        return (y == rhs.y) && (x == rhs.x);
    }
    bool operator!=(const Pos& rhs){
        return !operator==(rhs);
    }
    friend ostream& operator<<(ostream& os, const Pos& pos);
};

struct PosCompare{
    bool operator()(const Pos& lhs, const Pos& rhs) const{
        return (100*lhs.y + lhs.x) < (100*rhs.y + rhs.x);
    }

    bool operator()(const pair<Pos, Pos>& lhs, const pair<Pos, Pos>& rhs) const{
        return 1000000*(100*lhs.first.y + lhs.first.x) + (100*lhs.second.y + lhs.second.x)
            < 1000000*(100*rhs.first.y + rhs.first.x) + (100*rhs.second.y + rhs.second.x);
    }
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

    //// 各epochで得られるスコアを基にした辺
    //// ex_edge[Pos(y, x)] --> {cost, Pos(ny, nx)}
    map<Pos, vector<pair<ll, Pos>>, PosCompare> ex_edge;
    //// ex_path[(y1, x1), (y2, x2)] --> {U, D, R, R, D, ...}
    map<pair<Pos, Pos>, vector<Dir>, PosCompare> ex_path;
    //// ex_edge_cost[(y1, x1), (y2, x2)] --> cost
    map<pair<Pos, Pos>, ll, PosCompare> ex_edge_cost;

    ll init;
    /// for random value
    mt19937 engine;

    Field(ll _init) : init(_init){
        //random_device seed_gen;
        //engine = mt19937(seed_gen());
        engine = mt19937(100);
        initialize_field(init);
    }

    void initialize_field(ll _init){
        init = _init;
        ////TODO
        static uniform_real_distribution<> rand(0.95, 1.05);
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                //row[i][j] = init;
                //row[i][j] = init * rand(engine);
                row[i][j] = 1000 * rand(engine);
                //row[i][j] = 1000;
            }
        }
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                //col[i][j] = init;
                //col[i][j] = init * rand(engine);
                col[i][j] = 1000 * rand(engine);
                //col[i][j] = 1000;
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
                //ll nd = d + get_dist(y, x, int2dir(dir));
                ll nd = d + (init + get_dist(y, x, int2dir(dir)));

                if(nd < D[ny][nx]){
                    D[ny][nx] = nd;
                    que.push(T(nd, ny, nx));
                }
            }

            //// ワープする場合
            //// TODO : check
            for(auto& [cost, npos] : ex_edge[Pos(y, x)]){
                int ny = npos.y;
                int nx = npos.x;
                if(ny < 0 || nx < 0) continue;
                if(ny >= NUM_GRID || nx >= NUM_GRID) continue;
                ll nd = d + cost;
                if(nd < 0){
                    printf("nd < 0\n");
                }
                if(nd < D[ny][nx]){
                    D[ny][nx] = nd;
                    que.push(T(nd, ny, nx));
                }
            }
        }
    }

    //// 経路長Dからgoal --> startの経路を復元
    vector<Dir> restore_path(Pos start, Pos goal, vector<vector<ll>>& D, int q_idx){
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
            bool update = false;
            for(int i = 0; i < 4; i++){
                int dir = dirs[i];
                int ny = player.y + dy[dir];
                int nx = player.x + dx[dir];
                if(ny < 0 || nx < 0) continue;
                if(ny >= NUM_GRID || nx >= NUM_GRID) continue;

                //if(D[player.y][player.x] == (D[ny][nx] + get_dist(ny, nx, int2dir(dir)))){
                if(D[player.y][player.x] == (D[ny][nx] + (init + get_dist(ny, nx, int2dir(dir))))){
                    player = Pos(ny, nx);
                    path.push_back(int2dir(dir));
                    update = true;
                    break;
                }
            }

            //// ワープした場合
            if(!update){
                //cerr << "warp : " << q_idx << endl;
                for(auto& [cost, npos] : ex_edge[player]){
                    int ny = npos.y;
                    int nx = npos.x;
                    if(ny < 0 || nx < 0) continue;
                    if(ny >= NUM_GRID || nx >= NUM_GRID) continue;
                    if(D[player.y][player.x] == (D[ny][nx] + cost)){
                        // 反転が必要
                        for(auto& dir : ex_path[mp(player, npos)])
                            path.push_back(rev_dir(dir));
                        player = npos;
                        update = true;
                        break;
                    }
                }
            }

            if(!update)
                assert(false);
        }

        reverse(path.begin(), path.end());
        return path;
    }

    vector<Dir> get_path(int q_idx, Pos start, Pos goal){
        /// temporary distance
        vector<vector<ll>> D(NUM_GRID, vector<ll>(NUM_GRID, INF));
        dijkstra(start, D);
        auto path = restore_path(start, goal, D, q_idx);
        return path;
    }

    /*
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
    */

    //// ある行・列に対し、通った辺を中心として更新値を減衰
    void update_path_decay(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
        //// 辺の重み
        vector<vector<double>> row_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));
        vector<vector<double>> col_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));

        //// 現在のパス長を基にしたスコアの推定値
        ll predicted_score = 0;

        //// 畳み込み
        ////TODO
        //const double KPA = 23,  KPB = 1;
        //const double KER = KPA + ((double)(KPB - KPA)/NUM_Q) * q_idx;
        const double KPA = 30, KPB = 20, KPC = 10;
        const int BD = 500;
        const double KER = (q_idx < BD) ? KPA + ((double)(KPB - KPA)/BD) * q_idx :
                                          KPB + ((double)(KPC - KPB)/(NUM_Q - BD)) * (q_idx - BD); 
        const double ratio = 1.0;
        //cerr << q_idx << " : " << KER << endl;
        //const int KER = 7;
        //const int KER = 10;
        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            //cerr << y << " " << x << endl;
            if(dir == Dir::U){
                predicted_score += init + row[x][y-1];
                for(int k = -KER; k <= KER; k++){
                    if(y-1+k < 0 || NUM_GRID-1 <= y-1+k) continue;
                    row_val[x][y-1+k] += 1 + ratio * (KER - abs(k));
                }
            }
            else if(dir == Dir::D){
                predicted_score += init + row[x][y];
                for(int k = -KER; k <= KER; k++){
                    if(y+k < 0 || NUM_GRID-1 <= y+k) continue;
                    row_val[x][y+k] += 1 + ratio * (KER - abs(k));
                }
            }
            else if(dir == Dir::L){
                predicted_score += init + col[y][x-1];
                for(int k = -KER; k <= KER; k++){
                    if(x-1+k < 0 || NUM_GRID-1 <= x-1+k) continue;
                    col_val[y][x-1+k] += 1 + ratio * (KER - abs(k));
                }
            }
            else if(dir == Dir::R){
                predicted_score += init + col[y][x];
                for(int k = -KER; k <= KER; k++){
                    if(x+k < 0 || NUM_GRID-1 <= x+k) continue;
                    col_val[y][x+k] += 1 + ratio * (KER - abs(k));
                }
            }
            player.next(dir);
        }

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

        //// 10^3 ~ 10^5
        //ll loss_score = abs(score - predicted_score);
        ll loss_score = score - predicted_score;
        //static ll loss_score_sum;
        //if(q_idx > 50)
        //    loss_score_sum += abs(loss_score);
        //if(q_idx % 100 == 0)
            //cerr << abs(loss_score) << endl;
            //cerr << loss_score_sum << endl;
        //cerr << score << " " << predicted_score << endl;
        //    cerr << abs(score - predicted_score) << endl;

        auto edge_update = [&q_idx, &score, &path, &predicted_score, &loss_score](auto& edge_weight, auto& edge_val, auto& edge_max, auto lower_bound){
            ////TODO: 問題毎にこの辺を可変にしたい
            ///const double PA = 1.0,  PB = 1.0;
            ///const double PA = 0.6,  PB = 0.4;
            //const double PA = 0.8,  PB = 0.6;
            //const double Pmax = PA + ((double)(PB - PA)/NUM_Q) * q_idx;
            //
            //const double PA = 0.0,  PB = 0.8;
            //const ll LOSS_MAX = 10000;
            //const ll loss_score_clip = min(abs(loss_score), LOSS_MAX);
            //const double Pmax = PA + ((double)(PB - PA)/LOSS_MAX) * loss_score_clip;
            
            //const double learning_rate = 0.03 * cos((M_PI / 2)/NUM_Q * q_idx);
            const double learning_rate = 0.02 * cos((M_PI / 2)/NUM_Q * q_idx);
            //const double learning_rate = (q_idx < 500) ? 0.03 : (q_idx < 800) ? 0.006 : 0.002;
            const double lambda = 0.00;
            REP(i, NUM_GRID){
                if(edge_max[i] < 0.0001) continue;
                REP(j, NUM_GRID-1){
                    //const double update_score = (double)score / path.size();
                    //const double update_score = (double)score * ((double)edge_weight[i][j] / predicted_score);
                    //const double P = Pmax * (edge_val[i][j] / edge_max[i]);
                    /////何故か重みが0である辺が出現する --> 経路探索不可
                    /////const double update_score = (double)score * ((double)edge_weight[i][j] / predicted_score) * (edge_val[i][j] / edge_max[i]);
                    /////const double P = Pmax;
                    //edge_weight[i][j] = ((1-P)*edge_weight[i][j] + P*update_score);

                    //const double loss = edge_weight[i][j] * (((double)score / predicted_score) - 1.0) * (edge_val[i][j] / edge_max[i]);
                    //const double loss = ((double)score - predicted_score) * (edge_val[i][j] / edge_max[i]);
                    const double loss = ((double)score - predicted_score) * (edge_val[i][j] / edge_max[i]) + lambda * edge_weight[i][j];
                    //if(q_idx % 10 == 0 && i == 15 && j == 15){
                    //    //cerr << "loss : " << learning_rate * loss << endl;
                    //    //cerr << "loss : " << loss << endl;
                    //    cerr << "edge_weight (" << i << ", " << j << ") : " << edge_weight[i][j] << " --> " << edge_weight[i][j] + learning_rate*loss << endl;
                    //}
                    edge_weight[i][j] += learning_rate * loss;
                    edge_weight[i][j] = max(lower_bound, edge_weight[i][j]);
                }
            }
        };
        edge_update(row, row_val, row_max, -init+1);
        edge_update(col, col_val, col_max, -init+1);

    }

    //// 得られた経路長から、dist配列を更新 (推定)
    void update_path(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
        ///score = (1.0 / 0.9) * score;
        /// TODO: 同じ頂点対の場合,良い方を採用
        {
            auto tmp_score = (1.0 / 0.9) * score;
            ex_edge_cost[mp(start, goal)] = ex_edge_cost[mp(start, goal)] = tmp_score;
            ex_edge[start].push_back(mp(tmp_score, goal));
            ex_edge[goal].push_back(mp(tmp_score, start));
            ex_path[mp(start, goal)] = path;
            //// 反転 & 逆順
            ex_path[mp(goal, start)] = vector<Dir>();
            auto tmp = path;
            reverse(tmp.begin(), tmp.end());
            for(auto& dir : tmp)
                ex_path[mp(goal, start)].push_back(rev_dir(dir));
        }

        //if(q_idx % 4 == 0)
        //    update_path_uniform(q_idx, start, goal, score, path);
        //else
        update_path_decay(q_idx, start, goal, score, path);
    }
};

////////////////////////////////////////////////////////

//// naive answer
vector<Dir> path_naive(Pos player, Pos goal, bool zig=false){
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
        idx = (zig) ? (idx + 1) % 2 : idx;
    }

    return path;
}

//// 不要な頂点移動を除去
vector<Dir> check_path(Pos start, Pos goal, const vector<Dir>& path){
    vector<Dir> valid_path;
    auto player = Pos(start);
    set<Pos, PosCompare> pos_set;
    pos_set.insert(start);
    for(auto& dir : path){
        player.next(dir);
        valid_path.push_back(dir);
        ////既に通った頂点の場合、そこまで戻る。
        if(pos_set.find(player) != pos_set.end()){
            auto pos = Pos(player);
            pos.next(rev_dir(valid_path.back()));
            valid_path.pop_back();
            while(pos != player){
                pos_set.erase(pos);
                pos.next(rev_dir(valid_path.back()));
                valid_path.pop_back();
            }
        }
        pos_set.insert(player);
    }
    return valid_path;
}

vector<Dir> answer(int q_idx, Pos start, Pos goal, Field& field){
    vector<Dir> path;
    //const double PA = 1.0,  PB = -5.0;
    //const double P = PA + ((double)(PB - PA)/NUM_Q) * q_idx;
    ////cerr << q_idx << " : " << P << endl;
    //static mt19937 engine = mt19937(1);
    //static uniform_real_distribution<> rand(0, 1);

    //if(rand(engine) < P){
    //if(q_idx == 0){
    //    path = path_naive(start, goal, true);
    //}
    //if(q_idx < 75 && q_idx % 2 == 0){
    if(q_idx < 75 && true){
        path = path_naive(start, goal, false);
    }
    else if(q_idx < 150 && false){
        Pos mid((start.y + goal.y)/2, (start.x + goal.x)/2);
        path = path_naive(start, mid);
        auto tmp = path_naive(mid, goal);
        for(auto& t : tmp)
            path.push_back(t);
    }
    else
        path = field.get_path(q_idx, start, goal);
    //return path;
    return check_path(start, goal, path);
}

int main(){
    //// TODO
    //// 未探索の経路から優先的に使用?
    Field field(2000);
    //Field field(1000);
    //Field field(100);
    
    //const int init_epoch = 74;
    vector<Pos> mem_player(NUM_Q), mem_goal(NUM_Q);
    vector<ll> mem_score(NUM_Q);
    vector<vector<Dir>> mem_path(NUM_Q);
    for(int qi = 0; qi < NUM_Q; qi++){
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        //cerr << si << " " << sj << " " << ti << " " << tj << endl;
        Pos player(si, sj), goal(ti, tj);

        vector<Dir> path = answer(qi, player, goal, field);
        string ans = path2string(path);
        cout << ans << endl;

        ll score;
        cin >> score;
        //cerr << score << endl;
        
        mem_player[qi] = player;
        mem_goal[qi] = goal;
        mem_score[qi] = score;
        mem_path[qi] = path;

        ///TODO
        //static ll sum = 0;
        //if(qi < init_epoch){
        //    sum += score / path.size();
        //}
        //else if(qi == init_epoch){
        //    field.initialize_field(sum / init_epoch);
        //    cerr << sum / init_epoch << endl;
        //    for(int tqi = 0; tqi < qi; tqi++){
        //        field.update_path(tqi, mem_player[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
        //    }
        //    field.update_path(qi, player, goal, score, path);
        //}
        //else{
        //    field.update_path(qi, player, goal, score, path);
        //}
        
        field.update_path(qi, player, goal, score, path);

        ////TODO: パラメータ調整
        if(qi >= 75 && qi % 5 == 0){
            static mt19937 engine = mt19937(1);
            uniform_int_distribution<> rand(0, qi-1);
            //uniform_real_distribution<> scale((1.0/1.1), (1.0/0.9));
            for(int i = 0; i < qi/5; i++){
                int tqi = rand(engine);
                field.update_path(qi, mem_player[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
            }
        }
        //for(int tqi = qi-10; tqi >= 0; tqi -= 100){
        //    field.update_path(qi, mem_player[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
        //}
    }

    return 0;
}
