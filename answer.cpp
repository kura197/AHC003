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
    using T = tuple<Pos, Pos, ll, vector<Dir>>;
    array<Pos, NUM_Q> start, goal;
    array<ll, NUM_Q> score;
    array<vector<Dir>, NUM_Q> path;

    void update(const int idx, const Pos start, const Pos goal, const ll score, const vector<Dir>& path){
        this->start[idx] = start;
        this->goal[idx] = goal;
        this->score[idx] = score;
        this->path[idx] = path;
    }

    T get(const int idx) const {
        if(idx < 0 || idx >= NUM_Q)
            assert(false);

        return T(start[idx], goal[idx], score[idx], path[idx]);
    }

};

//// for graph analysis
struct Field{
    //// row[i][j] := i列目のj番目の辺 : (j, i) --> (j+1, i)
    array<array<ll, NUM_GRID-1>, NUM_GRID> row;
    //// col[i][j] := i行目のj番目の辺 : (i, j) --> (i, j+1)
    array<array<ll, NUM_GRID-1>, NUM_GRID> col;

    //// update counter
    array<array<ll, NUM_GRID-1>, NUM_GRID> row_cnt;
    array<array<ll, NUM_GRID-1>, NUM_GRID> col_cnt;

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

    void print_edge(){
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                cerr << init + col[i][j] << " ";
            }
            cerr << endl;
        }
        REP(i, NUM_GRID-1){
            REP(j, NUM_GRID){
                cerr << init + row[j][i] << " ";
            }
            cerr << endl;
        }
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
                row_cnt[i][j] = 0;
            }
        }
        REP(i, NUM_GRID){
            REP(j, NUM_GRID-1){
                //col[i][j] = init;
                //col[i][j] = init * rand(engine);
                //col[i][j] = 1000 * rand(engine);
                col[i][j] = 100 * rand(engine);
                //col[i][j] = 1000;
                col_cnt[i][j] = 0;
            }
        }
    }

    //// (y, x) から dir方向の辺の重みを取得
    ll get_dist(int y, int x, Dir dir, bool search=false) const{
        static const int C = 4;
        switch(dir){
            case Dir::U: return max(1LL, init + row[x][y-1] - ((search) ? 500*max(0LL, (C - row_cnt[x][y-1])) : 0));
            case Dir::D: return max(1LL, init + row[x][y-0] - ((search) ? 500*max(0LL, (C - row_cnt[x][y-0])) : 0));
            case Dir::L: return max(1LL, init + col[y][x-1] - ((search) ? 500*max(0LL, (C - col_cnt[y][x-1])) : 0));
            case Dir::R: return max(1LL, init + col[y][x-0] - ((search) ? 500*max(0LL, (C - col_cnt[y][x-0])) : 0));
            default: assert(false);
        }
    }

    //// 頂点sから各頂点への経路長を計算
    void dijkstra(Pos s, vector<vector<ll>>& D, bool search=false){
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
                ll nd = d + get_dist(y, x, int2dir(dir), search);

                if(nd < D[ny][nx]){
                    D[ny][nx] = nd;
                    que.push(T(nd, ny, nx));
                }
            }

            //// ワープする場合
            //// TODO : ほぼ変化なし？
            //for(auto& [cost, npos] : ex_edge[Pos(y, x)]){
            //    int ny = npos.y;
            //    int nx = npos.x;
            //    if(ny < 0 || nx < 0) continue;
            //    if(ny >= NUM_GRID || nx >= NUM_GRID) continue;
            //    ll nd = d + cost;
            //    if(nd < 0){
            //        printf("nd < 0\n");
            //    }
            //    if(nd < D[ny][nx]){
            //        D[ny][nx] = nd;
            //        que.push(T(nd, ny, nx));
            //    }
            //}
        }
    }

    //// 経路長Dからgoal --> startの経路を復元
    vector<Dir> restore_path(Pos start, Pos goal, vector<vector<ll>>& D, int q_idx, bool search=false){
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
                if(D[player.y][player.x] == (D[ny][nx] + get_dist(ny, nx, int2dir(dir), search))){
                    player = Pos(ny, nx);
                    path.push_back(int2dir(dir));
                    update = true;
                    break;
                }
            }

            //// ワープした場合
            //if(!update){
            //    //cerr << "warp : " << q_idx << endl;
            //    for(auto& [cost, npos] : ex_edge[player]){
            //        int ny = npos.y;
            //        int nx = npos.x;
            //        if(ny < 0 || nx < 0) continue;
            //        if(ny >= NUM_GRID || nx >= NUM_GRID) continue;
            //        if(D[player.y][player.x] == (D[ny][nx] + cost)){
            //            // 反転が必要
            //            for(auto& dir : ex_path[mp(player, npos)])
            //                path.push_back(rev_dir(dir));
            //            player = npos;
            //            update = true;
            //            break;
            //        }
            //    }
            //}

            if(!update)
                assert(false);
        }

        reverse(path.begin(), path.end());
        return path;
    }

    vector<Dir> get_path(int q_idx, Pos start, Pos goal){
        /// temporary distance
        //// TODO:
        //const bool search = true;
        const bool search = false;
        vector<vector<ll>> D(NUM_GRID, vector<ll>(NUM_GRID, INF));
        dijkstra(start, D, search);
        auto path = restore_path(start, goal, D, q_idx, search);
        return path;
    }

    ////TODO
    void edge_cnt_update(Pos start, const vector<Dir>& path){
        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            switch(dir){
                case Dir::U: row_cnt[x][y-1]++; break;
                case Dir::D: row_cnt[x][y]++; break;
                case Dir::L: col_cnt[y][x-1]++; break;
                case Dir::R: col_cnt[y][x]++; break;
                default: assert(false);
            }
            player.next(dir);
        }
    }

    ll calc_loss(Pos start, ll score, const vector<Dir>& path) const {
        //// 現在のパス長を基にしたスコアの推定値
        ll predicted_score = 0;
        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            predicted_score += get_dist(y, x, dir);
            player.next(dir);
        }
        //ll loss_score = score - predicted_score;
        ll loss_score = predicted_score - score;
        //static uniform_real_distribution<> rand((1.0/1.2), (1.0/0.8));
        static uniform_real_distribution<> rand((1.0/1.1), (1.0/0.9));
        //cerr << loss_score << " --> " << static_cast<ll>(static_cast<double>(loss_score) * rand(engine)) << endl;
        //loss_score = static_cast<double>(loss_score) * rand(engine);
        //cerr << loss_score << endl;
        //TODO:
        //loss_score *= rand(engine);
        loss_score /= static_cast<ll>(path.size());
        return loss_score;
    }

    //// lossを近傍の頂点へ分配する
    void loss_distribution(int q_idx, Pos start, vector<Dir>& path, ll loss_score, auto& row_loss, auto& col_loss){
        //// 辺のloss値
        vector<vector<double>> row_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));
        vector<vector<double>> col_val(NUM_GRID, vector<double>(NUM_GRID-1, 0));

        //// 逆畳み込み
        auto kernel_mirror = [](vector<double>& ker){
            const int size = ker.size();
            for(int i = size-2; i >= 0; i--){
                ker.push_back(ker[i]);
            }
        };

        auto kernel_normalization = [](vector<double>& ker){
            double sum = 0;
            for(auto& k : ker)
                sum += k;
            for(auto& k : ker)
                k /= sum;
        };

        auto kernel_apply = [&loss_score](int i, int j, auto& edge_val, auto& kernel){
            const int ksize = kernel.size();
            for(int k = 0; k < ksize; k++){
                const int s = k - (ksize/2);
                ////TODO : 失われた分
                if(j+s < 0 || NUM_GRID-1 <= j+s)
                    continue;
                edge_val[i][j+s] += loss_score * kernel[k];
            }
        };

        //vector<double> kernel = {1, 2, 3};
        //vector<double> kernel = {1, 2, 3, 4, 5, 6, 7, 8, 9, 9};
        //vector<double> kernel = {1, 1, 2, 2, 3, 3, 4, 4, 5};
        //vector<double> kernel = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        //vector<double> kernel = {1, 2, 4, 8, 16, 32};
        vector<double> kernel;
        double val = 1.0;
        const double ratio = 1.25;
        const int len = 10;
        for(int i = 0; i < len; i++){
            kernel.push_back(val);
            val *= ratio;
        }
        kernel_mirror(kernel);
        kernel_normalization(kernel);
        assert(kernel.size() % 2 == 1);

        Pos player(start);
        for(auto& dir : path){
            auto x = player.x, y = player.y;
            if(dir == Dir::U)
                kernel_apply(x, y-1, row_val, kernel);
            else if(dir == Dir::D)
                kernel_apply(x, y, row_val, kernel);
            else if(dir == Dir::L)
                kernel_apply(y, x-1, col_val, kernel);
            else if(dir == Dir::R)
                kernel_apply(y, x, col_val, kernel);
            player.next(dir);
        }

        auto loss_update = [&loss_score](auto& edge_loss, auto& edge_val){
            REP(i, NUM_GRID){
                REP(j, NUM_GRID-1){
                    edge_loss[i][j] += edge_val[i][j];
                }
            }
        };
        loss_update(row_loss, row_val);
        loss_update(col_loss, col_val);
    }

    //// ミニバッチ単位で更新
    void update_path_batch(const int epoch, const vector<int>& q_indices, const Memory& mem){
        //// edge_loss[NUM_GRID][NUM_GRID-1]
        vector<vector<double>> row_loss(NUM_GRID, vector<double>(NUM_GRID-1, 0.0));
        vector<vector<double>> col_loss(NUM_GRID, vector<double>(NUM_GRID-1, 0.0));

        //// 10^3 ~ 10^5
        for(auto& idx : q_indices){
            auto [start, goal, score, path] = mem.get(idx);
            ll loss_score = calc_loss(start, score, path);
            loss_distribution(epoch, start, path, loss_score, row_loss, col_loss);
        }

        int batch_size = q_indices.size();
        auto edge_update = [&epoch, &batch_size](auto& edge_weight, auto& edge_loss, auto lower_bound){
            REP(i, NUM_GRID){
                REP(j, NUM_GRID-1){
                    edge_weight[i][j] -= static_cast<ll>(edge_loss[i][j] / batch_size);
                    edge_weight[i][j] = max(lower_bound, edge_weight[i][j]);
                }
            }
        };
        edge_update(row, row_loss, -init+1);
        edge_update(col, col_loss, -init+1);
    }

    void update_path(const int epoch, const vector<int>& q_indices, const Memory& mem){
        if(q_indices.size() == 0)
            return;

        update_path_batch(epoch, q_indices, mem);
    }
};

////////////////////////////////////////////////////////

//// naive answer
vector<Dir> path_naive(Pos player, Pos goal, bool tate=false){
    vector<Dir> path;

    while(player != goal){
        Dir dir;
        //if(idx % 2 == 0){
        if(tate){
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
    //const double PA = 1.0,  PB = -10.0;
    //const double P = PA + ((double)(PB - PA)/NUM_Q) * q_idx;
    ////cerr << q_idx << " : " << P << endl;
    ///static mt19937 engine = mt19937(1);
    //static uniform_real_distribution<> rand(0, 1);

    //if(rand(engine) < P){
    //    path = path_naive(start, goal, false);
    //}
    //if(q_idx < 75 && q_idx % 2 == 0){
    //if(q_idx < 75 && true){
    if(q_idx == 0 && false){
        const int my[] = {0, 0, 29, 29};
        const int mx[] = {0, 29, 0, 29};
        Pos player = start;
        for(int i = 0; i < 4; i++){
            Pos mid(my[i], mx[i]);
            auto tmp = path_naive(player, mid);
            for(auto& t : tmp)
                path.push_back(t);
            player = mid;
        }
        auto tmp = path_naive(player, goal);
        for(auto& t : tmp)
            path.push_back(t);
    }
    else if(q_idx < 50 && true){
        path = path_naive(start, goal, false);
    }
    else if(q_idx < 50 && false){
        Pos mid((start.y + goal.y)/2, (start.x + goal.x)/2);
        path = path_naive(start, mid);
        auto tmp = path_naive(mid, goal);
        for(auto& t : tmp)
            path.push_back(t);
    }
    else if(q_idx < 50 && false){
        //const int my[] = {0, 0, 29, 29};
        //const int mx[] = {0, 29, 0, 29};
        //Pos mid(my[q_idx%4], mx[q_idx%4]);

        static mt19937 engine = mt19937(1);
        static uniform_int_distribution<> rand(0, 29);
        Pos mid(rand(engine), rand(engine));
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

double calc_all_loss(const int idx, const Field& field, const Memory& mem){
    double ret = 0;
    for(int i = 0; i <= idx; i++){
        auto [start, _, score, path] = mem.get(i);
        ll loss = field.calc_loss(start, score, path);
        ret += abs(loss);
    }
    ret /= idx+1;
    return ret;
}

int main(){
    Field field(4000);
    
    static mt19937 engine = mt19937(10);
    const int BSIZE = 5;
    Memory mem;
    vector<int> qi_vec;
    //// {score, idx}
    for(int qi = 0; qi < NUM_Q; qi++){
        qi_vec.push_back(qi);
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

        //if(qi == 0){
        //    int size = path.size();
        //    field.initialize_field(score / size);
        //}
        
        mem.update(qi, start, goal, score, path);
        ////field.edge_cnt_update(start, path);
        //cerr << qi << " : " << field.calc_loss(start, score, path) << " --> ";
        
        field.save_path(start, goal, score, path);
        //field.update_path(qi, {qi}, mem);
        REP(i,5)
            field.update_path(qi, {qi}, mem);

        //vector<int> indices;
        //for(int i = qi; i >= 0; i-=100){
        //    indices.push_back(i);
        //    if(indices.size() == BSIZE)
        //        break;
        //}
        //field.update_path(qi, indices, mem);

        //cerr << field.calc_loss(start, score, path) << endl;

        ////TODO: パラメータ調整
        if(qi >= 10 && qi % 2 == 0){
            //uniform_int_distribution<> rand(0, qi);
            ////uniform_int_distribution<> rand(max(0, qi-100), qi);
            //static int tmp_idx = 0;
            //for(int i = 0; i < 50; i++){
            //    vector<int> indices;
            //    //for(int b = 0; b < BSIZE; b++){
            //    //    //indices.push_back(rand(engine));
            //    //    indices.push_back(tmp_idx);
            //    //    tmp_idx = (tmp_idx + 1) % (qi + 1);
            //    //}
            //    for(int b = 0; b < BSIZE/2; b++){
            //        indices.push_back(tmp_idx);
            //        tmp_idx = (tmp_idx + 1) % (qi + 1);
            //    }
            //    for(int b = 0; b < BSIZE/2; b++){
            //        indices.push_back(rand(engine));
            //    }
            //    field.update_path(qi, indices, mem);
            //}

            //using P = pair<ll, int>;
            //priority_queue<P> que;
            //for(int i = 0; i <= qi; i++){
            //    auto [_start, _goal, _score, _path] = mem.get(i);
            //    ll loss = field.calc_loss(start, score, path);
            //    que.push(P(abs(loss), i));
            //}

            //for(int i = 0; i < 50; i++){
            //    vector<int> indices;
            //    while(!que.empty()){
            //        indices.push_back(que.top().second);
            //        que.pop();
            //        if(indices.size() == BSIZE)
            //            break;
            //    }
            //    if(que.empty()) break;
            //    field.update_path(qi, indices, mem);
            //}

            //vector<int> _indices;
            //for(int tqi = qi-10; tqi >= 0; tqi -= 100){
            //    //field.update_path(qi, mem_start[tqi], mem_goal[tqi], mem_score[tqi], mem_path[tqi]);
            //    _indices.push_back(tqi);
            //}
            //field.update_path(qi, _indices, mem);
            
            //cerr << qi << " : " << field.calc_loss(start, score, path) << " --> ";
            //cerr << qi << " : " << calc_all_loss(qi, field, mem) << " --> ";
            //const int len = (qi % 700 == 0) ? 1000 : 1;
            //const int len = (qi % 20 == 0) ? 100 : 1;
            const int len = 1;
            
            for(int i = 0; i < len; i++){
                shuffle(qi_vec.begin(), qi_vec.end(), engine);
                vector<int> local_indices;
                for(auto& idx : qi_vec){
                    local_indices.push_back(idx);
                    if(local_indices.size() == BSIZE){
                        field.update_path(qi, local_indices, mem);
                        local_indices.clear();
                    }
                }
            }

            //cerr << field.calc_loss(start, score, path) << endl;
            //cerr << calc_all_loss(qi, field, mem) << endl;
        }

    }

    //field.print_edge();

    return 0;
}
