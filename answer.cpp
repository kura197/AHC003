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

//// number of queries
const int NUM_Q = 1000;

enum class Dir{U, D, L, R};

inline char dir2char(Dir dir){
    switch(dir){
        case Dir::U: return 'U';
        case Dir::D: return 'D';
        case Dir::L: return 'L';
        case Dir::R: return 'R';
    }

    return 'X';
}

struct Pos{
    int y, x;

    Pos(int _y, int _x) : y(_y), x(_x) {};
    void move(Dir dir){
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


int main(){

    for(int qi = 0; qi < NUM_Q; qi++){
        string answer;
        int si, sj, ti, tj;
        cin >> si >> sj >> ti >> tj;
        Pos player(si, sj), goal(ti, tj);

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

            player.move(dir);
            answer.push_back(dir2char(dir));
        }

        cout << answer << endl;

        ll score;
        cin >> score;
    }

    return 0;
}
