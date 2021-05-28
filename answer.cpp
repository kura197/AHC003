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

/// for saving past inputs, outputs, ...
struct Memory{
    using T = tuple<Pos, Pos, ll, ll, vector<Dir>>;
    array<Pos, NUM_Q> start, goal;
    array<ll, NUM_Q> score, loss;
    array<vector<Dir>, NUM_Q> path;

    void update(const int idx, const Pos start, const Pos goal, const ll score, const ll loss, const vector<Dir>& path){
        this->start[idx] = start;
        this->goal[idx] = goal;
        this->score[idx] = score;
        this->loss[idx] = loss;
        this->path[idx] = path;
    }

    void update_loss(const int idx, const ll loss){
        this->loss[idx] = loss;
    }

    T get(const int idx){
        if(idx < 0 || idx >= NUM_Q)
            assert(false);

        return T(start[idx], goal[idx], score[idx], loss[idx], path[idx]);
    }

};

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

    void save_path(Pos start, Pos goal, ll score, vector<Dir>& path){
        ///score = (1.0 / 0.9) * score;
        /// TODO: 同じ頂点対の場合,良い方を採用
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

    void initialize_field(ll _init){
        init = _init;
        ////TODO
        //static uniform_real_distribution<> rand(0.95, 1.05);
        static uniform_real_distribution<> rand(-1.0,1.0);
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                //row[i][j] = init;
                //row[i][j] = init * rand(engine);
                //row[i][j] = 1000 * rand(engine);
                row[i][j] = 100 * rand(engine);
                //row[i][j] = 1000;
            }
        }
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                //col[i][j] = init;
                //col[i][j] = init * rand(engine);
                //col[i][j] = 1000 * rand(engine);
                col[i][j] = 100 * rand(engine);
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
            //// TODO : ほぼ変化なし？
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

    ll calc_loss(Pos start, ll score, vector<Dir>& path){
        //// 現在のパス長を基にしたスコアの推定値
        ll predicted_score = 0;
        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            if(dir == Dir::U)
                predicted_score += init + row[x][y-1];
            else if(dir == Dir::D)
                predicted_score += init + row[x][y];
            else if(dir == Dir::L)
                predicted_score += init + col[y][x-1];
            else if(dir == Dir::R)
                predicted_score += init + col[y][x];
            player.next(dir);
        }
        ll loss_score = score - predicted_score;
        return loss_score;
    }

    //// lossを近傍の頂点へ分配する
    void loss_distribution(int q_idx, Pos start, vector<Dir>& path, ll loss_score, auto& row_loss, auto& col_loss){
        //// 辺の系数値
        vector<vector<double>> row_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));
        vector<vector<double>> col_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));

        //// 畳み込み
        const double KPA = 30, KPB = 20, KPC = 10;
        const int BD = 500;
        const double KER = (q_idx < BD) ? KPA + ((double)(KPB - KPA)/BD) * q_idx :
                                          KPB + ((double)(KPC - KPB)/(NUM_Q - BD)) * (q_idx - BD); 
        const double ratio = 1.0;
        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            //cerr << y << " " << x << endl;
            if(dir == Dir::U){
                for(int k = -KER; k <= KER; k++){
                    if(y-1+k < 0 || NUM_GRID-1 <= y-1+k) continue;
                    row_val[x][y-1+k] += 1 + ratio * (KER - abs(k));
                }
            }
            else if(dir == Dir::D){
                for(int k = -KER; k <= KER; k++){
                    if(y+k < 0 || NUM_GRID-1 <= y+k) continue;
                    row_val[x][y+k] += 1 + ratio * (KER - abs(k));
                }
            }
            else if(dir == Dir::L){
                for(int k = -KER; k <= KER; k++){
                    if(x-1+k < 0 || NUM_GRID-1 <= x-1+k) continue;
                    col_val[y][x-1+k] += 1 + ratio * (KER - abs(k));
                }
            }
            else if(dir == Dir::R){
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

        auto loss_update = [&loss_score](auto& edge_loss, auto& edge_val, auto& edge_max){
            REP(i, NUM_GRID){
                if(edge_max[i] < 0.0001) continue;
                REP(j, NUM_GRID-1){
                    edge_loss[i][j] += loss_score * (edge_val[i][j] / edge_max[i]);
                }
            }
        };
        loss_update(row_loss, row_val, row_max);
        loss_update(col_loss, col_val, col_max);
    }

    //// ある行・列に対し、通った辺を中心として更新値を減衰
    void update_path_decay(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
        //// edge_loss[NUM_GRID][NUM_GRID-1]
        vector<vector<double>> row_loss(NUM_GRID, vector<double>(NUM_GRID-1, 0.0));
        vector<vector<double>> col_loss(NUM_GRID, vector<double>(NUM_GRID-1, 0.0));

        //// 10^3 ~ 10^5
        ll loss_score = calc_loss(start, score, path);
        //if(q_idx % 100 == 0)
            //cerr << abs(loss_score) << endl;
            //cerr << loss_score_sum << endl;

        loss_distribution(q_idx, start, path, loss_score, row_loss, col_loss);

        auto edge_update = [&q_idx](auto& edge_weight, auto& edge_loss, auto lower_bound){
            const double learning_rate = 0.02 * cos((M_PI / 2)/NUM_Q * q_idx);
            const double lambda = 0.00;
            REP(i, NUM_GRID){
                REP(j, NUM_GRID-1){
                    edge_weight[i][j] += static_cast<ll>(learning_rate * (edge_loss[i][j] + lambda * edge_weight[i][j]));
                    edge_weight[i][j] = max(lower_bound, edge_weight[i][j]);
                }
            }
        };
        edge_update(row, row_loss, -init+1);
        edge_update(col, col_loss, -init+1);
    }

    //// ミニバッチ単位で更新
    //void update_path_batch(const vector<int>& q_indices, const vector<Pos>& starts, const vector<Pos>& goals, const vector<ll>& scores, vector<Dir>& path){
    //    //// edge_loss[NUM_GRID][NUM_GRID-1]
    //    vector<vector<double>> row_loss(NUM_GRID, vector<double>(NUM_GRID-1, 0.0));
    //    vector<vector<double>> col_loss(NUM_GRID, vector<double>(NUM_GRID-1, 0.0));

    //    //// 10^3 ~ 10^5
    //    ll loss_score = calc_loss(start, score, path);
    //    //if(q_idx % 100 == 0)
    //        //cerr << abs(loss_score) << endl;
    //        //cerr << loss_score_sum << endl;

    //    loss_distribution(q_idx, start, path, loss_score, row_loss, col_loss);

    //    auto edge_update = [&q_idx](auto& edge_weight, auto& edge_loss, auto lower_bound){
    //        const double learning_rate = 0.02 * cos((M_PI / 2)/NUM_Q * q_idx);
    //        const double lambda = 0.00;
    //        REP(i, NUM_GRID){
    //            REP(j, NUM_GRID-1){
    //                edge_weight[i][j] += static_cast<ll>(learning_rate * (edge_loss[i][j] + lambda * edge_weight[i][j]));
    //                edge_weight[i][j] = max(lower_bound, edge_weight[i][j]);
    //            }
    //        }
    //    };
    //    edge_update(row, row_loss, -init+1);
    //    edge_update(col, col_loss, -init+1);
    //}

    //// 得られた経路長から、dist配列を更新 (推定)
    void update_path(int q_idx, Pos start, Pos goal, ll score, vector<Dir>& path){
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
    Field field(3000);
    //Field field(1000);
    //Field field(100);
    
    //const int init_epoch = 74;
    //vector<Pos> mem_start(NUM_Q), mem_goal(NUM_Q);
    //vector<ll> mem_score(NUM_Q), mem_loss(NUM_Q);
    //vector<vector<Dir>> mem_path(NUM_Q);
    Memory mem;
    //// {score, idx}
    using P = pair<ll, int>;
    priority_queue<P> que;
    for(int qi = 0; qi < NUM_Q; qi++){
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        //cerr << si << " " << sj << " " << ti << " " << tj << endl;
        Pos start(si, sj), goal(ti, tj);

        vector<Dir> path = answer(qi, start, goal, field);
        string ans = path2string(path);
        cout << ans << endl;

        ll score;
        cin >> score;
        //cerr << score << endl;
        
        field.save_path(start, goal, score, path);
        field.update_path(qi, start, goal, score, path);

        //mem_start[qi] = start;
        //mem_goal[qi] = goal;
        //mem_score[qi] = score;
        //mem_loss[qi] = field.calc_loss(start, score, path);
        //mem_path[qi] = path;
        auto loss = field.calc_loss(start, score, path);
        mem.update(qi, start, goal, score, loss, path);
        que.push(P(abs(loss), qi));

        ///TODO
        //static ll sum = 0;
        //if(qi < init_epoch){
        //    sum += score / path.size();
        //}
        //else if(qi == init_epoch){
        //    field.initialize_field(sum / init_epoch);
        //    cerr << sum / init_epoch << endl;
        //    for(int tqi = 0; tqi < qi; tqi++){
        //        field.update_path(tqi, mem_start[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
        //    }
        //    field.update_path(qi, start, goal, score, path);
        //}
        //else{
        //    field.update_path(qi, start, goal, score, path);
        //}

        ////TODO: パラメータ調整
        if(qi >= 75 && qi % 5 == 0){
            //// 960671688
            static mt19937 engine = mt19937(1);
            uniform_int_distribution<> rand(0, qi-1);
            //uniform_real_distribution<> scale((1.0/1.1), (1.0/0.9));
            for(int i = 0; i < qi/4; i++){
                int tqi = rand(engine);
                auto [_start, _goal, _score, _loss, _path] = mem.get(tqi);
                field.update_path(qi, _start, _goal, _score, _path);
                auto new_loss = field.calc_loss(_start, _score, _path);
                mem.update_loss(tqi, new_loss);
            }

            //// 951390700
            //vector<int> q_index;
            //for(int i = 0; i < qi/4; i++){
            //    while(1){
            //        auto [s, tqi] = que.top();
            //        que.pop();
            //        if(mem_loss[tqi] != s)
            //            que.push(P(mem_loss[tqi], tqi));
            //        else{
            //            q_index.push_back(tqi);
            //            break;
            //        }
            //    }
            //}
            //for(auto& tqi : q_index){
            //    field.update_path(qi, mem_start[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
            //    mem_loss[tqi] = field.calc_loss(mem_start[tqi], mem_score[tqi], mem_path[tqi]);
            //    que.push(P(mem_loss[tqi], tqi));
            //}
        }
        //for(int tqi = qi-10; tqi >= 0; tqi -= 100){
        //    field.update_path(qi, mem_start[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
        //}
    }

    return 0;
}
