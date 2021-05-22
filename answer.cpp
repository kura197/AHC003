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
    //// row[i][j] := (i, j) --> (i+1, j)
    array<array<ll, NUM_GRID-1>, NUM_GRID> row;
    //// col[i][j] := (i, j) --> (i, j+1)
    array<array<ll, NUM_GRID>, NUM_GRID-1> col;

    ll init;
    /// for random value
    mt19937 engine;

    Field(ll _init) : init(_init){
        ////TODO:
        //random_device seed_gen;
        //engine = mt19937(seed_gen());
        engine = mt19937(1);

        REP(r, NUM_GRID-1){
            REP(c, NUM_GRID){
                row[r][c] = init;
            }
        }
        REP(r, NUM_GRID){
            REP(c, NUM_GRID-1){
                col[r][c] = init;
            }
        }
    }

    //// (y, x) から dir方向の辺の重みを取得
    ll& get_dist(int y, int x, Dir dir){
        switch(dir){
            case Dir::U: return row[y-1][x];
            case Dir::D: return row[y][x];
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
            shuffle(dirs.begin(), dirs.end(), engine);
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

    //// 得られた経路長から、dist配列を更新 (推定)
    void update_path(Pos start, Pos goal, ll score, vector<Dir> path){
        //// 平均の経路長で、経路長の推定値を更新
        ll ave_score = score / path.size();
        //cerr << ave_score << endl;
        Pos player(start);
        static uniform_real_distribution<> rand(0.9, 1.1);
        for(auto& dir : path){
            //printf("%d, %d, %d : %lld\n", player.y, player.x, dir2int(dir), ave_score);
            ll update_score = ave_score;
            /////TODO:check validity
            get_dist(player.y, player.x, dir) = (get_dist(player.y, player.x, dir) + update_score) / 2;
            player.next(dir);
        }
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
    //if(q_idx < 100)
    //    path = path_naive(start, goal);
    //else
    path = field.get_path(start, goal);
    return path;
}

int main(){
    //// 未探索の経路から優先的に使用?
    Field field(4000);
    for(int qi = 0; qi < NUM_Q; qi++){
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        Pos player(si, sj), goal(ti, tj);

        vector<Dir> path = answer(qi, player, goal, field);
        string ans = path2string(path);
        cout << ans << endl;

        ll score;
        cin >> score;
        field.update_path(player, goal, score, path);
    }

    return 0;
}
