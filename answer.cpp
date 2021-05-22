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
};

//// for graph analysis
struct Field{
    /// dist[y][x][dir] := path length from (y, x)
    array<array<array<ll, 4>, NUM_GRID>, NUM_GRID> dist;
    ll init;
    /// for random value
    mt19937 engine;

    Field(ll _init) : init(_init){
        random_device seed_gen;
        engine = mt19937(seed_gen());

        REP(i, NUM_GRID){
            REP(j, NUM_GRID){
                REP(k, 4){
                    dist[i][j][k] = init;
                }
            }
        }
    }

    //// 頂点sから各頂点への経路長を計算
    //// epsの確率で未探索の経路を選択するようにする。
    void dijkstra(Pos s, vector<vector<ll>>& D, double eps){
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
                ll nd = d;
                //// TODO
                //static uniform_real_distribution<> rand(0.0, 1.0);
                //if(dir != 3 && dist[y][x][dir] != init && rand(engine) < eps)
                //    nd += init;
                //else
                //    nd += dist[y][x][dir];
                nd += dist[y][x][dir];
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
        while(player != start){
            for(int dir = 0; dir < 4; dir++){
                int ny = player.y + dy[dir];
                int nx = player.x + dx[dir];
                if(ny < 0 || nx < 0) continue;
                if(ny >= NUM_GRID || nx >= NUM_GRID) continue;

                //printf("%lld ?== %lld + %lld\n", D[player.y][player.x], D[ny][nx], dist[ny][nx][dir]);
                if(D[player.y][player.x] == (D[ny][nx] + dist[ny][nx][dir])){
                    player = Pos(ny, nx);
                    path.push_back(int2dir(dir));
                    break;
                }

                if(dir == 3)
                    assert(false);
            }
        }

        reverse(path.begin(), path.end());
        return path;
    }

    vector<Dir> get_path(Pos start, Pos goal){
        /// temporary distance
        vector<vector<ll>> D(NUM_GRID, vector<ll>(NUM_GRID, INF));
        dijkstra(start, D, 1.0);
        auto path = restore_path(start, goal, D);
        return path;
    }

    //// 得られた経路長から、dist配列を更新 (推定)
    void update_path(Pos start, Pos goal, ll score, vector<Dir> path){
        //// 平均の経路長で、経路長の推定値を更新
        ll ave_score = score / path.size();
        //cerr << ave_score << endl;
        Pos player(start);
        for(auto& dir : path){
            //printf("%d, %d, %d : %lld\n", player.y, player.x, dir2int(dir), ave_score);
            dist[player.y][player.x][dir2int(dir)] = ave_score;
            player.next(dir);
        }
    }
};

////////////////////////////////////////////////////////

//// useless answer
vector<Dir> path_naive(Pos player, Pos goal){
    vector<Dir> path;

    //// test
    while(player != goal){
        Dir dir;
        if(player.y < goal.y)
            dir = Dir::D;
        else if(player.y > goal.y)
            dir = Dir::U;
        else if(player.x < goal.x)
            dir = Dir::R;
        else if(player.x > goal.x)
            dir = Dir::L;

        player.next(dir);
        path.push_back(dir);
    }

    return path;
}

vector<Dir> answer(int q_idx, Pos start, Pos goal, Field& field){
    vector<Dir> path;
    //path = path_naive(start, goal);
    path = field.get_path(start, goal);
    return path;
}

int main(){
    Field field(9000);
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
