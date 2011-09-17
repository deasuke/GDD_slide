#include "stdafx.h"
#include <iostream>
#include <algorithm>
#include <time.h>

// 60, 300, 600, 1200, 2400
#define TIMEOUT 1200
// 100, 150, 200, 300, 5000(X_WEIGHT)
#define MAX_DEPTH 5000
// 16, 8
#define NUM_PART 16
// 1, 2
//#define FAR_FIRST 1
#define HINT1
#define EXTRA1

// H_CUT, V_CUT: 1,2,3,4,(NONE)
//#define H_CUT 1
//#define V_CUT 4
// X_WEIGHT 1, 2, 3, 4, 5, (NONE)
#define X_WEIGHT 3
//#define D_WEIGHT 2

typedef unsigned char tile;
// 0: blank(0)
// 1-9  : 1-9
// 10-35: A-Z
// 36  : wall(=)
// 37  : mark(*)

// operation
// 0:L 1:R 2:U 3:D
static int dx[4] = { -1,  1,  0,  0 };
static int dy[4] = {  0,  0, -1,  1 };

struct Board {
  // Board (with sentinel: max: 6x6)
  tile b[64];
  tile* operator[](int x){ return b+(x<<3); }
  tile const* operator[](int x) const { return b+(x<<3); }
  
  // size:(mx, my) blank-pos:(zx, zy)
  int mx, my, zx, zy;
  
  // ctors
  Board() : mx(0), my(0), zx(0), zy(0) {
    std::fill(b, b+64, 36);
  }
  Board(char const* str) {
    std::fill(b, b+64, 36);
    mx = str[0] - '0';
    my = str[2] - '0';
    char const *p = str+4;
    for(int iy = 1; iy <= my; ++iy) for(int ix = 1; ix <= mx; ++ix) {
      tile t = char2tile(*p++);
      (*this)[ix][iy] = t;
      if(t == 0) { zx = ix; zy = iy; }
    }
  }
  static tile char2tile(char c) {
    if('A' <= c && c <= 'Z') return c - 'A' + 10;
    if('0' <= c && c <= '9') return c - '0';
    return 36;
  }
  
  // judge goal
  bool is_goal() const {
    tile s = 1;
    for(int iy = 1; iy <= my; ++iy) for(int ix = 1; ix <= mx; ++ix) {
      tile t = (*this)[ix][iy];
      if(t != 0 && t != 36 && t != s) return false;
      ++s;
    }
    return true;
  }
  
  // operation
  bool can_op(int op) const {
    int x = zx + dx[op];
    int y = zy + dy[op];
    return (*this)[x][y] < 36;
  }
  bool do_op(int op) {
    int x = zx + dx[op];
    int y = zy + dy[op];
    tile t = (*this)[x][y];
    if(t > 35) return false;
    (*this)[zx][zy] = t;
    (*this)[x][y] = 0;
    zx = x;
    zy = y;
    return true;
  }
  
  // for debug
  void print() const {
    for(int iy = 1; iy <= my; ++iy) {
      for(int ix = 1; ix <= mx; ++ix) {
        std::cerr << "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ=*"[(*this)[ix][iy]];
      }
      std::cerr << std::endl;
    }
  }
};

// simple Manhattan dist with wall
struct Distance {
  Board tbl[37];
  
  // ctor
  Distance(Board const& b, int cut_pattern = 0) {
    // blank&wall: dist=0
    std::fill_n(tbl[0].b, 64, 0);
    int s = 1;
#ifdef X_WEIGHT
    int ss = 1;
#endif
    for(int iy = 1; iy <= b.my; ++iy) for(int ix = 1; ix <= b.mx; ++ix) {
      tile t = b[ix][iy];
      if(t < 36) {
        Board &cur = tbl[s];
        cur = b;
        calc_dist_table(cur, ix, iy);
#ifdef X_WEIGHT
	ss = s;
#endif
      }
      ++s;
    }

#ifdef X_WEIGHT
    // weight by '0' s distance
    s = 1;
    for(int iy = 1; iy <= b.my; ++iy) for(int ix = 1; ix <= b.mx; ++ix) {
      tile t = b[ix][iy];
      if(t < 36) {
	int w = tbl[ss][ix][iy];
	w = (w+X_WEIGHT-1)/X_WEIGHT;
	if(cut_pattern > 0) {
	  if((cut_pattern&0xf) < ix && (cut_pattern>>4) < iy) w = 0;
	}
	for(int jy = 1; jy <= b.my; ++jy) for(int jx = 1; jx <= b.mx; ++jx) {
	  tbl[s][jx][jy] *= w; // maybe over-estimate; not optimal
	}
      }
      ++s;
    }
#endif
    std::fill_n(tbl[36].b, 64, 0);
  }
  Distance(Board const& b, Board const& g) {
	// clear distance table
	for(int i = 0; i < 37; ++i) std::fill_n(tbl[i].b, 64, 0);
	for(int ix = 1; ix <= g.mx; ++ix) for(int iy = 1; iy <= g.my; ++iy) {
	  int t = g[ix][iy];
	  if(t == 0 || t > 35) continue;
	  tbl[t] = b;
	  calc_dist_table(tbl[t], ix, iy);
#ifdef D_WEIGHT
	  for(int jx = 1; jx < g.mx; ++jx) for(int jy = 1; jy < g.my; ++jy) {
		if(tbl[t][jx][jy] < 35) tbl[t][jx][jy] *= D_WEIGHT;
	  }
#endif
	}
  }

  static void calc_dist_table(Board& cur, int sx, int sy) {
    // fill 37
    for(int iy = 1; iy <= cur.my; ++iy) for(int ix = 1; ix <= cur.mx; ++ix) {
      if(cur[ix][iy] != 36) cur[ix][iy] = 37;
    }
    cur[sx][sy] = 0;
    // do naive meathod (because of small problem) to calc distance
    for(;;) {
      bool change = false;
      for(int iy = 1; iy <= cur.my; ++iy) for(int ix = 1; ix <= cur.mx; ++ix) {
        tile t = cur[ix][iy];
        if(t != 37) continue;
        for(int i = 0; i < 4; ++i) {
          tile tt = cur[ix + dx[i]][iy + dy[i]];
          if(tt < t) t = tt;
        }
        if(t == 37) continue;
        cur[ix][iy] = t+1;
        change = true;
      }
      if(!change) break;
    }
    // check
    for(int iy = 1; iy <= cur.my; ++iy) for(int ix = 1; ix <= cur.mx; ++ix) {
      if(cur[ix][iy] == 37) throw "invalid table";
    }
  }
  
  // calc
  int dist(int x, int y, tile n) const {
    return tbl[n][x][y];
  }
  int cost(Board const& b) const {
    int ret = 0;
    for(int iy = 1; iy <= b.my; ++iy) for(int ix = 1; ix <= b.mx; ++ix) {
      ret += dist(ix, iy, b[ix][iy]);
    }
    return ret;
  }
  
  // for debug
  void print() const {
    std::cerr << "*********" << std::endl;
    for(int s = 1; s <= 36; ++s) {
      Board const& b = tbl[s];
      if(b[1][1] == 36) continue;
	  std::cerr << "s = " << s << std::endl;
      b.print();
    }
    std::cerr << "*********" << std::endl;
  }
};


// simple IDA*
template<int MAXD>
struct IDAStar {
  Distance dist;
  
  Board path[MAXD+1];
  int cost[MAXD+1];
  int move[MAXD];
  int threshold;
  
  // ctor
  IDAStar(Board const&b, int cut_pattern = 0) : dist(b, cut_pattern) {
    path[0] = b;
  }
  
  IDAStar(Board const&b, Board const&g) : dist(b, g) {
    path[0] = b;
  }

  int *timeOut;
  
  int search() {
    HANDLE hF = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, L"timeOut");
    timeOut = (int*)MapViewOfFile(hF, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    for(threshold = dist.cost(path[0]); threshold < MAXD; ++threshold) {
      if(*timeOut) {
	std::cout << "timeout";
        return -1;
      }
      int ret = search_start();
      if(ret >= 0) return ret;
    }
    return -1;
  }

  int search_start() {
    cost[0] = dist.cost(path[0]);
    if(cost[0] == 0) return 0;

    for(int mv = 0; mv < 4; ++mv) {
      move[0] = mv;
      if(!path[0].can_op(move[0])) continue;
      path[1] = path[0];
      path[1].do_op(move[0]);
      int ret = search_core(1);
      if(ret > 0) return ret;
    }
    
    return -1;
  }
  
  int search_core(int depth) {
    if(*timeOut) return -1;
    // cost update
    //cost[depth] = dist.cost(path[depth]);
    Board const &cb = path[depth];
    Board const &pb = path[depth-1];
    cost[depth] = cost[depth-1]
                + dist.dist(pb.zx, pb.zy, cb[pb.zx][pb.zy])
                - dist.dist(cb.zx, cb.zy, pb[cb.zx][cb.zy]);
    
    if(cost[depth] == 0) return depth;
    if(depth + cost[depth] > threshold) return -1;
    for(int mv = 0; mv < 4; ++mv) {
      move[depth] = mv;
      if(move[depth] == (move[depth-1]^1)) continue; // back move
      if(!path[depth].can_op(move[depth])) continue;
      path[depth+1] = path[depth];
      path[depth+1].do_op(move[depth]);
      int ret = search_core(depth+1);
      if(ret > 0) return ret;
    }
    return -1;
  }
  
};


template<int MAXD, int MAXM>
struct FarFirst {
  int nMoves;
  int moves[MAXM];
  Board b;

  FarFirst(Board const& b_) : nMoves(0), b(b_) {
    // normalize board
    for(;;) {
      bool change = false;
      int s = 0;
      for(int ix = 1; ix <= b.mx; ++ix) for(int iy = 1; iy <= b.my; ++iy) {
	++s;
	if(36 == b[ix][iy]) continue;
        int c = 0;
        for(int mv = 0; mv < 4; ++mv) {
          if(b[ix+dx[mv]][iy+dy[mv]] == 36) ++c;
        }
        if(c > 2) {
          if(b[ix][iy] == s) b[ix][iy] = 36;
          else if(b[ix][iy] == 0) {
            int mv;
            for(mv = 0; mv < 4; ++mv) {
              if(b[ix+dx[mv]][iy+dy[mv]] == s) break;
            }
	    if(mv >= 4) throw; //cant reach here
	    b[ix+dx[mv]][iy+dy[mv]] = 0;
            b[ix][iy] = 36;
	    moves[nMoves++] = mv;
          } else throw; // cant reach here
          change = true;
        }
      }
      if(!change) break;
    }
  }

  int FillFar() {
    int mx, my;
    findFarthestFromSpaceHome(mx, my);
    return doFill(mx, my);
  }

  int FillFarWithHint(int h) {
    int mx = (h-1)%b.mx + 1, my = (h-1)/b.mx + 1;
    return doFill(mx, my); 
  }

  // find farthest position from 0's home position
  void findFarthestFromSpaceHome(int&mx, int&my) {
    // find 0's home pos
    int sx = b.mx, sy = b.my;
    for(;;) {
      while(b[sx][sy] == 36 && sx > 0) --sx;
      if(sx > 0) break;
      if(--sy == 0) throw; //can't reach here
      sx = b.mx;
    }
    
    // make home dist table and calc max dist
    int md = 0;
    mx = 1, my = 1;
    
    Board z = b;
    Distance::calc_dist_table(z, sx, sy);
    for(int ix = 1; ix <= z.mx; ++ix) for(int iy = 1; iy <= z.my; ++iy) {
      if(z[ix][iy] >= 36) continue;
      // choice
      //if(md < z[ix][iy]) {
      if(md <= z[ix][iy]) {
	md = z[ix][iy];
	mx = ix; my = iy;
      }
    }
  }


  int doFill(int mx, int my) {
    // mark fill positions
    Board m = b;
    int nMarked = 1;
    calcMarkedPositions(m, nMarked, mx, my);

    // choice
    //if(nMarked == 1) { // only one
    if(nMarked <= 2) { // one or two
      return do_IDAStarWithMark(m);
    }

    // find the moves to solve marked positions
    //   marked positions are like:
    //
    //   ****   1. find chain of '*'s (marked positions)
    //   *==*   2. find longest maching in the chain
    //   ####   3. lengthen the maching part
    //
    //   ops: UURRRDD s(1,2) t(4,2)
    {
      int ops[36], nums[36];
      int sx = mx, sy = my, tx, ty;
      chainMarkedPositions(m, sx, sy, tx, ty, ops, nums);
      int ix = 1;
      for(; ix < 36; ++ix) if(ops[ix] < 0) break;
      --ix;
      for(int i = 0; i < 36; ++i) if(nums[i] < 0) break;
      
      int ixb, ixe, ixp;
      findLongestMatching(b, nums, ixb, ixe, ixp);

      int gnums[36];
      while(ixe < ix) {
	// choice
	if((ixp+ixe)*2 < ix) {
	  std::fill_n(gnums, ix, 0);
	  gnums[ix] = -1;
	  std::copy(nums+ixb, nums+ixe, gnums+(ixp+ixb+ix-ixe)/2);
	  Board g=m;
	  makeMidGoal(g, nums, gnums);
	  if(IDAStarToMidGoal(g) < 0) return -1;
	}
	// make [ixb, ixe) --> [ixb+ix-ixe, ix)
	std::fill_n(gnums, ixb+ix-ixe, 0);
	std::copy(nums+ixb, nums+ixe, gnums+ixb+ix-ixe);
	gnums[ix] = -1;
	Board g=m;
	makeMidGoal(g, nums, gnums);
	g[tx+dx[ops[ix]]][ty+dy[ops[ix]]] = nums[ixe];
	if(IDAStarToMidGoal(g) < 0) return -1;
	++ixe;
      }
      
      while(ixb > 0) {
	// make [ixb, ix) --> [0, ix-ixb)
	std::copy(nums+ixb, nums+ix, gnums);
	std::fill_n(gnums+ix-ixb, ixb, 0);
	gnums[ix] = -1;
	Board g=m;
	makeMidGoal(g, nums, gnums);
	g[sx+dx[ops[0]^1]][sy+dy[ops[0]^1]] = nums[ixb-1];
	if(IDAStarToMidGoal(g) < 0) return -1;
	--ixb;
      }

      {
	Board g=m;
	makeMidGoal(g, nums, nums);
	if(IDAStarToMidGoal(g) < 0) return -1;
      }
      
    }

    // fill
    for(int ix = 1; ix <= m.mx; ++ix) for(int iy = 1; iy <= m.my; ++iy) {
      if(m[ix][iy] == 37) b[ix][iy] = 36;
    }

    return nMoves;
  }

  // do IDA* to mid goal
  int IDAStarToMidGoal(Board&g) {
    IDAStar<MAXD> idas(b, g);

    int ret = idas.search();
    if(ret < 0) return -1;
    for(int mv = 0; mv < ret; ++mv) {
      int cmv = idas.move[mv];
      moves[nMoves++] = cmv;
      b.do_op(cmv);
    }
    return ret;
  }

  // make mid goal
  static void makeMidGoal(Board& g, int nums[], int gnums[]) {
    for(int ix = 1; ix <= g.mx; ++ix) for(int iy = 1; iy <= g.my; ++iy) {
      tile& t = g[ix][iy];
      if(t < 36) t = 0; 
    }
    for(int i = 0; i < 36; ++i) {
      int s = nums[i];
      if(s < 0) break;
      int x = (s-1)%g.mx + 1, y = (s-1)/g.mx + 1;
      g[x][y] = gnums[i];
    }
  }

  // find longest match
  static void findLongestMatching(Board const&b, int nums[], int&ixb, int&ixe, int&ixp) {
    int n = 0;
    int rnums[36];
    std::fill_n(rnums, 36, -1);
    for(; n < 36; ++n) {
      int s = nums[n], xs = (s-1)%b.mx +1, ys = (s-1)/b.mx +1;
      if(s < 0) break;
      rnums[n] = b[xs][ys];
    }
    for(int i = 0; i < 36; ++i) if(rnums[i] < 0) break;
    
    for(int l = n; l > 0;--l) {
      for(ixb = 0; ixb < 1 + n - l; ++ixb) {
	ixe = ixb + l;
	for(ixp = 0; ixp < 1 + n - 1; ++ixp) {
	  int i = 0;
	  for(; i < l; ++i) if(nums[ixb+i] != rnums[ixp+i]) break;
	  if(i == l) return;
	}
      }
    }
  }

  //
  //  --(ops[0])-->(sx,sy)--(ops[1])--> ... -->(ops[nMarked-1])-->(tx,ty)--(ops[nMarked])-->
  //
  static void chainMarkedPositions(Board const&m, int&sx, int&sy, int&tx, int&ty, int ops[], int nums[]) {
    std::fill_n(ops, 36, -1);
    std::fill_n(nums, 36, -1);
    moveToTerminal(m, sx, sy);
    nums[0] = sx + (sy-1)*m.mx;
    for(int mv = 0; mv < 4; ++mv) {
      int px = sx + dx[mv], py = sy + dy[mv];
      if(m[px][py] == 36) continue;
      if(m[px][py] < 36) ops[0] = (mv^1);
      else ops[1] = mv;
    }
    tx = sx + dx[ops[1]]; ty = sy + dy[ops[1]];
    for(int ix = 1;; ++ix) {
      nums[ix] = tx + (ty-1)*m.mx;
      for(int mv = 0; mv < 4; ++mv) {
	if((ops[ix]^mv) == 1) continue;
	int px = tx + dx[mv], py = ty + dy[mv];
	if(m[px][py] == 36) continue;
	ops[ix+1] = mv;
	if(m[px][py] < 36) return;
	tx = px; ty = py;
	break;
      }
    }
  }

  static void moveToTerminal(Board const&m, int& sx, int& sy) {
    int mv = isTerminal(m, sx, sy);
    if(mv < 0) return;
    sx += dx[mv]; sy += dy[mv];
    for(;;) {
      for(int nmv = 0; nmv < 4; ++nmv) {
	if(1 ==(mv^nmv)) continue; // back move
	int px = sx + dx[nmv], py = sy + dy[nmv];
	if(m[px][py] == 36) continue;
	if(m[px][py] < 36) return; // terminal!!
	sx = px; sy = py;
	mv = nmv;
	break;
      }
    }
  }

  static int isTerminal(Board const&m, int sx, int sy) {
    int smv = -1;
    for(int mv = 0; mv < 4; ++mv) {
      int px = sx + dx[mv], py = sy + dy[mv];
      if(m[px][py] == 36) continue;
      if(m[px][py] < 36) return -1; // terminal!!
      smv = mv;
    }
    return smv;
  }

  void calcMarkedPositions(Board &m, int& nMarked, int mx, int my) {
    m[mx][my] = 37;
    nMarked = 1;
    for(;;) {
      bool change = false;
      for(int ix = 1; ix <= m.mx; ++ix) for(int iy = 1; iy <= m.my; ++iy) {
	if(m[ix][iy] >= 36) continue;
	int c = 0;
	for(int mv = 0; mv < 4; ++mv) {
	  int t = m[ix+dx[mv]][iy+dy[mv]];
	  if(t >= 36) ++c;
	}
	if(c < 3) continue;
	m[ix][iy] = 37;
	change = true;
	++nMarked;
      }
      if(!change) break;
    }
  }

  int do_IDAStarWithMark(Board &m) {
    // make IDA* and ignore non-marked in distance
    IDAStar<MAXD> idas(b);
    int s = 0;
    for(int iy = 1; iy <= m.my; ++iy) for(int ix = 1; ix <= m.mx; ++ix) {
      ++s;
      if(m[ix][iy] >= 36) continue;
      std::fill(idas.dist.tbl[s].b, idas.dist.tbl[s].b+64, 0);
    }
    
    // DO IDA*
    int ret = idas.search();
    if(ret < 0) return ret;
    for(int mv = 0; mv < ret; ++mv) {
      int cmv = idas.move[mv];
      moves[nMoves++] = cmv;
      b.do_op(cmv);
    }
    // fill
    for(int ix = 1; ix <= m.mx; ++ix) for(int iy = 1; iy <= m.my; ++iy) {
      if(m[ix][iy] == 37) b[ix][iy] = 36;
    }

    return ret;
  }

  int search(int numFill) {
    bool goal = false;
    for(int i = 0; i < numFill; ++i) {
      goal = b.is_goal();
      if(goal) break;
      if(FillFar() < 0) return -1;
    }
    if(!goal) {
      IDAStar<MAXD> idas(b);
      int ret = idas.search();
      if(ret < 0) return -1;
      for(int mv = 0; mv < ret; ++mv) moves[nMoves++] = idas.move[mv];
    }
    return display();
  }

  int searchWithHint(int hint[]) {
    bool goal = false;
    for(int i = 0;; ++i) {
      goal = b.is_goal();
      if(goal) break;
      int h = hint[i];
      if(h < 0) break;
      if(FillFarWithHint(h) < 0) return -1;
    }
    if(!goal) {
      IDAStar<MAXD> idas(b);
      int ret = idas.search();
      if(ret < 0) return -1;
      for(int mv = 0; mv < ret; ++mv) moves[nMoves++] = idas.move[mv];
    }
    return display();
  }

  int display() {
    // display
    int lrud[4];
    std::fill(lrud, lrud+4, 0);
    for(int m = 0; m < nMoves; ++m) {
      int op = moves[m];
      std::cout << "LRUD"[op];
      ++lrud[op];
    }
    for(int i = 0; i < 4; ++i) std::cout << ", " << lrud[i];
    return nMoves;
  }
};


#ifdef HINT1
struct {
  char const* problem;
  int hint[200];
} hints[15] = {
  { "5,5,24E9=1=83F6B=JNCHGOK=I0MA",		{8, 12, 1, -1} },  // (3,2)(2,3)(1,1) OK
  { "4,6,927=1=8BD53AN==CL0KGMHJI",		{1, 7, 9, -1} },  // (1,1)(3,2)(1,3) OK
  { "5,6,239F01==A56BC=4==D=KMIQSORLNPT",	{9, 1, -1} },  // (4,2)(1,1) OK
  { "6,4,293AH=1=740FLBIKGC=DE=5N",		{20, 1, -1} },  // (2,4)(1,1) OK
  { "6,6,=93450K72==6VPF8=NDJ=GMCXE===IWQYZUO", {6, 2, 7, 8, -1} },  // (6,1)(2,1)(1,2)(2,2) OK
  { "6,6,==345CFL9==68=M=N07=RIHYDJQ=OTVPWXZU", {6, 7, 31, 32, 33, -1} },  // (6,1)(1,2)(1,6)(2,6)(3,6) OK
  { "6,6,123A9=7=4BCHJ=5=IM0FXRL=DPQ=NUVWK=TZ", {26, 32, 1, -1} },  // (2,5)(2,6)(1,1) OK
  { "6,6,=A9BG=82L3457=KF=TJO0MNIP=D=CUVWXRYZ", {31, 33, 2, 8, -1} },  // (1,6)(3,6)(2,1)(2,2) OK
  { "4,6,13A=2=CEDF78M95B0L=GIHNK",		{1, 7, -1} },  // (1,1)(3,2) OK
  { "4,6,51279=40I==3KL=8JMDGHECN",		{7, 1, -1} },  // (3,2)(1,1) OK
  { "6,6,7KF56C3DG4=I1J2AH98LV=0OPQ=UTNWEXSZY", {31, 32, 25, 28, 19, 20, 1, 7, 2, 8, 3, 9, 4, -1} },  // (1,6)(2,6)(1,5)(4,5)(1,4)(2,4)(1,1)(1,2)(2,1)(2,2)(3,1)(3,2)(4,1) OK
  { "6,6,D8236BKE==5410PVGJ7W=F=CX=YHUIRSMTZO", {31, 19, 33, 1, 7, 8, -1} },// (1,6)(1,4)(3,6)(1,1)(1,2)(2,2) OK
  { "5,5,B234017==N6=KF5G=IDELMJOA",		{7, 1, -1} },  // (2,2)(1,1) OK
  { "6,4,9N10567=42IHEK=A3BJDLGMC",		{19, 14, -1} },  // (1,4)(2,3) OK
  { "6,6,40G9E31DJA56872F=BP===NZVWR=CUXQYTOI", {19,-1} }, // (1,4) OK
}
#ifdef EXTRA1
, extra[3] = {
  { "5,5,B234017==N6=KF5G=IDELMJOA",			
    {0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,
     0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,
     0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,
     0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,
     0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,
     0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,
     0,0,0,0,3,3,3,3,1,1,1,1,2,2,2,2,-1} },
  { "6,4,9N10567=42IHEK=A3BJDLGMC",
    {0,0,0,3,3,1,3,1,1,2,2,2,
     0,0,0,3,3,1,3,1,1,2,2,2,
     0,0,0,3,3,1,3,1,1,2,2,2,
     0,0,0,3,3,1,3,1,1,2,2,2,
     0,0,0,3,3,1,3,1,1,2,2,2,
     0,0,0,3,3,1,3,1,1,2,2,
	 0,2,1,
	 3,3,3,0,0,2,0,2,2,1,1,1,
     3,0,2,1,
	 3,3,3,0,0,2,0,2,2,1,1,1,-1} },
  { "6,6,40G9E31DJA56872F=BP===NZVWR=CUXQYTOI",
    {0,3,3,3,3,3,1,1,1,1,1,
     2,0,2,1,3,3,0,2,1,3,
     0,0,0,0,0,2,2,2,
     1,1,2,0,0,3,
     1,1,2,0,0,3,
     1,1,1,2,1,1,3,3,3,3,0,0,0,0,0,
     2,1,1,3,0,2,0,2,2,-1} },
}
#endif
;
#endif

unsigned int  WINAPI solve_one(LPVOID pParam) {
  char const* problem = (char const*)pParam;
  Board b(problem);

#ifdef HINT1
  int n;
#ifdef EXTRA1
  for(n = 0; n < 3; ++n) {
    if(!strcmp(extra[n].problem, problem)) break;
  }
  if(n < 3) {
    for(int i = 0; extra[n].hint[i] >= 0; ++i) {
      int m = extra[n].hint[i];
      b.do_op(m);
      std::cout << "LRUD"[m];
    }
  }
#endif
  for(n = 0; n < 15; ++n) {
    if(!strcmp(hints[n].problem, problem)) break;
  }
  if(n == 15) {
    std::cerr << "wrong hints table" << std::endl;
    return 0;
  }
  if(hints[n].hint[0] < 0) {
    std::cerr << "no hint" << std::endl;
    return 0;
  }
  FarFirst<MAX_DEPTH, 600> ff(b);
  ff.searchWithHint(hints[n].hint);
#elif defiend FAR_FIRST
  FarFirst<MAX_DEPTH, 600> ff(b);
  ff.search(FAR_FIRST);

#elif !defined(H_CUT) && !defined(V_CUT)
  IDAStar<MAX_DEPTH> idas(b);

  int ret = idas.search();
  if(ret >= 0) {
    int lrud[4];
    std::fill(lrud, lrud+4, 0);
    for(int m = 0; m < ret; ++m) {
      int op = idas.move[m];
      std::cout << "LRUD"[op];
      ++lrud[op];
    }
    for(int i = 0; i < 4; ++i) std::cout << ", " << lrud[i];
  }

#else
#ifndef H_CUT
#define H_CUT 0
#endif
#ifndef V_CUT
#define V_CUT 0
#endif
  IDAStar<MAX_DEPTH> idas(b, H_CUT*0x10 + V_CUT);
  int ret = idas.search();
  if(ret < 0) return 0;

  // make new problem
  for(int m = 0; m < ret; ++m) b.do_op(idas.move[m]);
  for(int iy = 1; iy <= b.my; ++iy) for(int ix = 1; ix <= b.mx; ++ix) {
      if(ix <= V_CUT || iy <= H_CUT) b[ix][iy] = 36; // wall
  }

  IDAStar<MAX_DEPTH> idas1(b);
  int ret1 = idas1.search();
  if(ret1 < 0) return 0;
  
  int lrud[4];
  std::fill(lrud, lrud+4, 0);
  for(int m = 0; m < ret; ++m) {
    int op = idas.move[m];
    std::cout << "LRUD"[op];
    ++lrud[op];
  }

  for(int m = 0; m < ret1; ++m) {
    int op = idas1.move[m];
    std::cout << "LRUD"[op];
    ++lrud[op];
  }
  for(int i = 0; i < 4; ++i) std::cout << ", " << lrud[i];
#endif

  return 0;
}

int
_tmain(int argc, _TCHAR* argv[]) {

  int r = ::wcstol(argv[1], 0, 10);
  int n;
  char buf[128];
  
  std::cin.getline(buf, sizeof buf);
  std::cin >> n;
  std::cin.getline(buf, sizeof buf);
  
  int ix = 0;
  for(int p = 0; p < n; ++p) {
    std::cin.getline(buf, sizeof buf);
    if(strlen(buf) < 5) {
      std::cout << std::endl;
      continue;
    }
    ++ix;
    if(r != (ix%NUM_PART)) {
      std::cout << std::endl;
    } else {
      unsigned int dwT;
      HANDLE hF = CreateFileMapping((HANDLE)0xffffffff, 0, PAGE_READWRITE,
		  0, sizeof(int), L"timeOut");
      int *timeOut = (int*)MapViewOfFile(hF, FILE_MAP_ALL_ACCESS, 0, 0, 0);
      *timeOut = 0;
      HANDLE hT = (HANDLE)::_beginthreadex(0, 0, solve_one, (LPVOID)buf, 0, &dwT);
      time_t t = time((time_t*)0);
      for(;;) {
	if(WAIT_OBJECT_0 == WaitForSingleObject(hT, 1000)) {
	  break;
	}
	if(time((time_t*)0) > t + TIMEOUT) {
	  *timeOut = 1;
	  WaitForSingleObject(hT, INFINITE);
	  break;
	}
      }
      CloseHandle(hF);
      CloseHandle(hT);
      std::cout << std::endl;
    }
  }
  
  return 0;
}
