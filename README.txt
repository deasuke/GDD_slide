作成日：2011年9月17日(土)
週末になってようやくコードの説明を書く時間ができたのでアップします。

プログラム全体は NUM_PART (例えば16)個に問題を分割し、slide.exe 0 ~ slide.exe 15 までを起動することで並列に解きます。1つのプログラムは最大でも約500KBほどしかメモリを使いません(通常は30KB以下、たぶん5000題解いた人のなかでは最もメモリに優しい？)のでcoreの数(HTならもっと)だけ分割してOKです。すでに解いた問題については空行にしておけば、空行は無視した上でNUM_PART等分するようになっています。



■アルゴリズム(解法)の解説

解法の方針ですがsimpleなIDA* + Manhattan距離(壁考慮なのでManhattan道のり?)です。以降、Manhattan距離と言えば壁を考慮したものを指すこととします。IDA*は答が整数の場合は閾値を1ずつ増やせば必ず最適解が得られます。この方法だけで、制限時間40分/問までで4525題の最適解を得ています。制限時間を徐々に増やした結果は下記のようでした。

 1分/問 約3300題
 5分/問 約4000題
10分/問 約4200題
20分/問 約4400題
40分/問 　4525題

次に評価関数に対して次のようなweightを掛けました。すなわし、数字nのホームポジションと空白のホームポジションのManhattan距離をwとするとき、数字nのManhattan距離に(w/x)の小数部分を切り上げたものをweightとします。ただし、x=5,4,3,2,1の順に実験しました。
もちろん、評価関数がadmissibleでなくなるので最適解を得ない可能性が出てきます。しかしこの時点で移動の余りが十分だったので問題ないと判断しました。結果は制限時間40分/問までで428題を解き、のこり47題となりました。

次に試したのは、部分解を求める方法です。手順は、
1. 揃えたい数字の集合に属さない数字に関してはManhattan距離を足し込まないでIDA*を動かす。すなわち、揃えたい数字以外が揃っていなくても揃ったものと見做して解を出力します。
2. 1.の解の盤面で揃えた数字(偶然揃ったものは含まない)を壁に変更する。
3. IDA*を動かす。
というものです。

この方法で、まず上から1行/2行、左から1列/2列を揃える対象として実験しました。結果は制限時間5分/問で13題解け、残りは34問になりました。

次に、揃えたい数字の集合の求め方を単に行や列ではなく後で揃えやすくなるように考えました。たとえば下記で1を揃える場合は、2と7も揃えておかないと詰まります。アルゴリズム的には、まず1の場所を*にしたら、3方が=または*になっているところも*にします。これを変化がなくなるまで繰り返します。下記の6x3の盤面ですと、127CDの5つの位置が*になります。

123456
7=9ABC
DEFGH0

さらに、こうやって*が3つ以上になった場合には高い確率で*になった部分が一筆書になっています。このときManhattan距離を使っていると、この部分を揃えるには一旦距離を増さないと揃えられないことが多々あります。そこで、順々に中間ゴールを設定しました。仮に上記の形で127DEの5つの位置が*だとします。手順は、

<1> まず*の位置を一筆書にする。端は*=以外と隣接すること。答の例：ED712
<2> 実際の盤面で1.の答と最も長く一致している部分列を探す。下記の図だと D71 (?のところは気にしない)

GB????
1=????
7D????

<3> まずはお尻を伸ばすように中間ゴールを設定：このとき新しく述ばす数字は*の外にくるようにする。
 　 このゴール(?は気にしない--127D以外の数字に関してはManhattan距離を足し込まない)に対してIDA*を起動。
    この例ではこれで終りだが、お尻が届かない場合は届くまで繰り返す。

712???
D=????
??????

<4> 次は頭に向って伸ばすように中間ゴールを設定し、IDA*を起動。今回の例だと下記のようになります。

2?????
1=????
7DE???

<5> 次に最終の中間ゴールに向かってIDA*を起動。

12????
7=????
DE????

この方法で、最初の*で塗る最初の位置の選び方を空白のホームポジションから最も遠いところ、として14題ときました。
また、この部分解の手順1.2.を2度繰替えすことでさらに5題解きました。これで残り15題となります。

残りの15題中12題については、この方法の「最初の*で塗る位置」を人手で与えることで解き、
さらに残りの3題については、さらに、最初の何手かを解きやすくするように人手で与えて解きました。




■ソースの解説
言語はC++です。好みの問題でテンプレート利用。国外で空いているマシンがWindowsのためにVC++で実装。Console Applicationを選択。
タイムアウトの実装だが、WindowsでSIGALRMが使えないことに気付いてthread + WaitForSIngleObject()で代用したけどこれでいいのか良く分からない。最初はthreadをCloseHandle()したらthreadを強制終了できると勘違いしてWindowsって便利!と感動していたら、そんなことはなく大量スレッドでsystemの反応を鈍らせてしまった。WaitForSIngleObject()の第2引数に大きい値を入れると指定の時間より早く終ったよ、と言われガンガンタイムアウトになるので1000msにしてtime()で本当にそれだけの秒数経ったか確認することにした。


◎#define定数
TIMEOUT     タイムアウトする秒数
MAX_DEPTH   IDA*の閾値を増やす最大
NUM_PART    問題を何分割するか
FAR_FIRST   空白から遠い順に*で塗って中間ゴールを生成する方法を利用する、またそのときの繰り返し数
HINT1	    最初の*で塗る位置、のヒント利用
EXTRA1      最初の*で塗る位置、のヒント利用のとき、さらに最初の何手かのヒントを利用
H_CUT	    最初に上の行を揃える、そのときの行数 
V_CUT	    最初に左の列を揃える、そのときの列数
X_WEIGHT    Manhattanにweightを掛ける、またそのときのxの値
D_WIEGHT    中間ゴールにWEIGHTを掛ける、そのときの倍率


◎グローバル変数
static int dx[4], dy[4];
L:0, R:1, U:2, D:3 でLRUD方向に進むためのx,yの増分が入る。
struct{ char const* problem; int hint[200]; } hints[15], extra[3];
問題と対応するヒント。ヒントの内容はhintsの方では*で塗る位置の配列、extraの方では最初のオペレーション(LRUD)。デリミタは-1。


◎クラスなど
◇ tile -- unsigned char タイルの数字または壁、マークを示す
  0:空白、1~35:1~Zのタイル、36:壁、37:マーク(アルゴリズム中でマークしたいときに利用)

◇struct Board  --  盤面を表わすクラス(6x6まで)
 *盤面の状態
 tile b[64]; ボードを表現。一行8tileとし範囲外は壁とすることで範囲外かどうかの判定を不要とする
 tile (const)* operator[] ボード上の各タイルに[1..6][1..6]の形でアクセス可能とする
 int mx, my; ボードのサイズ;3以上6以下
 int zx, zy; ボードの空白の位置
 例えば、3,4,1327A40=5B96の場合は
 ========
 =132====
 =7A4====
 =0=5====
 =B96====
 ========
 ========
 ========
 と考え、b[64] = {
 36, 36, 36, 36, 36, 36, 36, 36, 36,  1,  3,  2, 36, 36, 36, 36, 
 36,  7, 10,  4, 36, 36, 36, 36, 36,  0, 36,  5, 36, 36, 36, 36, 
 36, 11,  9,  6, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, };
 mx = 3; my = 4; zx = 1; zy = 3;
 となります。
 
 *コンストラクタ
 Board() 全部が壁のボードを生成
 Board(char const *str) 問題のstrからBoardを生成
 static tile char2tile(char c) ヘルパ関数；問題文の文字からtileに変換。
  
 *オペレーション
 bool is_goal() const ゴール状態ならtrue (評価関数を使っていないことに注意!)
 bool can_op(int op) const オペレーションop(0:L 1:R 2:U 3:D)が可能ならtrue
 bool do_op(int op) オペレーションop(0:L 1:R 2:U 3:D)が可能なら行なう。返り値はcan_op()と同じ
 
 *デバッグ用
 void print() const 標準エラー出力にBoardの状態を出力
 
◇struct Distance  --  Manhattan距離を表わすクラス
 *距離のテーブル
 Board tbl[37]; 距離のテーブル。数字nが位置(x,y)に居るときの距離はtbl[n][x][y]で表わされる。
 int dist(int x, int y, tile n) const 数字nが位置(x,y)に居るときの距離
 int cost(Board const& b) const 指定したBoardのgoalまでのcostを計算
 
 *コンストラクタ
 Distance(Board const&b, int cut_pattern = 0)
 盤面bの距離テーブルを構成; cut_patternはH_CUT/V_CUTのときに利用
 処理：1. tbl[0] はすべて0とする
      2. 数字sのホームポジション(ix,iy)が壁やマークでないものすべてに関して
         calc_dist_table(tbl[s], ix, iy)を呼んでManhattan距離を計算する
	 問題中最大の数字+1のホームポジションは空白のホームポジションとする
      3. X_WEIGHTが有効なときには変数ssには問題中最大の数字+1が入っている
         すなわちtbl[ss][x][y]は(x,y)と空白のホームポジションとのManhattan距離
      4. X_WEIGHTが有効なときは、各タイルに関してそのホームポジションが(ix, iy)
         のとき、ウェイトint w = (tbl[ss][ix][iy] + X_WEIGHT-1)/X_WEIGHT;
	 を計算してテーブルの要素に掛け算する
      5. tbl[36] はすべて0とする
 Distance(Board const& b, Board const&g)
 盤面bの、中間ゴールgに対する距離テーブルを構成
 処理: 1. 全tbl[]をすべて0とする
      2. g[ix][iy]をtとするとき tbl[t] = b として calc_dist_table(tbl[t], ix, iy)
         でManhattan距離を計算
      3. D_WIEGHTがonのときはそのtbl[t]をD_WEIGHT倍する
      
 *ヘルパ
 static void calc_dist_table(Board& cur, int sx, int sy) 
 curに(sx, xy)からのManhattan距離を計算
 処理: 事実上のBFSです;検索範囲が小さいのでqueueを持たずに毎回全範囲見ています。
      1. curの壁でないところをマークで埋める
      2. cur[sx][xy]を0とする
      3. curを全部見てcur[ix][iy]がマークなら
         そのLRUDのどれかが壁でもマークでもなければそのうち最も小さい値をtとして、
	 cur[ix][iy] = t+1とする
      4. 変化がなくなるまで3.を繰り返す。
      
 *デバッグ用
 void print() const 標準エラー出力にDistanceの状態を出力
 
 
◇template<int MAXD> struct IDAStar 単純なIDA*を行なう
 *状態など
 Distance dist; このIDA*で使う距離
 Board path[MAXD+1] 現在探索中のパス path[0]が問題の盤面、path[n]は問題からn手動かした盤面
 int cost[MAXD+1] ゴールまでの予測コスト。cost[n]はpath[n]に対するもの
 int move[MAXD] オペレーションの列 path[n] に move[n] を適用すると path[n+1] になる
 int threashold 現在のIDA*の閾値
 int *timeOut これが非零になるとタイムアウト
 
 *コンストラクタ
 IDAStar(Board const&b, int cut_pattern=0) 距離を(b, cut_pattern)で定義したIDAStarを準備
 IDAStar(Board const&b, Board const& g) 距離を(b, g)で定義したIDAStarを準備
 
 *アルゴリズム
 int search() 探索をIDA*で行なう;返り値は解の手数
 int search_start() IDA*のルートノードでの処理
 int search_core(int depth) IDA*の深さdepth(>=1)での処理
 内容は単純なIDA*で特筆することもないですが、敢えて書くなら下記の点である
 1. IDA*は解が整数のときthresholdを1ずつ増やせば特にノードのソートなどは不要
 2. *timeOutにshared memoryのアドレス取得して*timeOutが非零ならタイムアウトして-1を返している
 3. オペレーションをL:0 R:1 U:2 D:3 とするとき2つのオペレーションop1とop2が (op1^op2) == 1
    ならばop1とop2は逆操作である。前の操作と逆の操作については探索しない
 4. 盤面のcostは各タイルのManhattan距離の和である(空白の位置はは数えない)。したがって、
    前のcostから動いたタイルのManhattan距離の差分を計算するだけで新しいcostが計算可能。
      
◇template<int MAXD, int MAXM> struct FarFirst 中間ゴールをみつけてIDA*を起動しつつ解くsolver
 *状態など
 int nMoves;       記録している操作の数
 int moves[MAXM];  記録している操作
 Board b;          現在の盤面
 
 *コンストラクタ
 FarFirst(Board const&b_) 問題盤面b_を持つsolverを構成;
 例えば下記の1のように3方(以上)が壁の場合は1のところを壁にする。

 1=???      
 ?????  
 ?????
 
 また、下記の0のような場合は操作Dを生成して、1を上に移動したあと1のところを壁とする。
 
 0=???
 1????
 ?????
 
 こうして3方が壁となっている部分をなくす。
 
 *アルゴリズム
 int FillFar() 空白のホームポジションから最も遠い部分を揃える
 int FillFarWithHint(int h) ヒントhで指定された部分を揃える
 void findFarthestFromSpaceHome(int&mx, int&my) 空白のホームポジションから最も遠い位置を(mx,my)に求める
 int IDAStarToMidGoal(Board&g) 中間ゴールgに向けてIDA*を起動 
 
 static void makeMidGoal(Board&g, int nums[], int gnums[]) 中間ゴールをgに作成する
 配列nums[]には、一筆書上の位置に入る正しい数字
 配列gnums[]は対応する位置上の中間ゴールの数字
 たとえば最終的な中間ゴールが
 12????
 7=????
 DE????
 のような場合 nums[] = {14, 13, 7, 1, 2, -1} となっており、現在の中間ゴールを
 71????
 D=????
 ??????
 としたい場合 gnums[] = {0, 0, 13, 7, 1, -1}  のようにする。
 1の右隣を2にする場合は結果のgに対してさらに操作が必要。

 static void findLongestMatching(Board const&b, int nums[], int&ixb, int&ixe, int&ixp)
 配列[]の中で最大長で一致している部分を求める
 上記の例で
 71????
 D=????
 GH????
 となっている場合最大の一致はD71という列でありこれはnums[]の中でindexは[1,4)であるから
 ixb = 1, ixe = 4となる。D71の位置(Dの位置)のindexはは2であるからixp=2となる。
  
 int doFill(int mx, my) (mx, my)を含む一帯を揃える
 処理：1. mに問題局面を入れ、calcMarkedPositions(m, nMarked, mx, my)を呼んで(mx, my)
         を含む一帯をマークする
      2. nMarkedが2以下ならdo_IDAStarWithMark(m)に処理を移す
      3. chainMarkedPositions()を呼んでmのマークの一筆書きを計算
      4. findLongestMatching()を呼んで最も長く一致した部分を見付ける
      5. お尻に向かって順に揃えるようにmakeMidGoal(), IDAStarToMidGoal()を呼ぶ
      6. 頭に向かって順に揃えるようにmakeMidGoal(), IDAStarToMidGoal()を呼ぶ
      7. 一つずれているので最後に揃える
      8. 揃った部分を壁にする
 
 static void chainMarkedPositions(Board const&m, int&sx, int&sy, int&tx, int&ty, int ops[], int nums[]) 盤面mでマークされた位置の一筆書きを求める
 入力：盤面mと検索開始位置(sx, sy)
 出力：一筆書きの始点(sx, sy)、終点(tx, ty)、一筆書き上の値nums[]、一筆書き上の移動操作ops[]
 例：下記の図の127DEの位置がmarkされていて(sx,sy)=(1,1)のとき出力は、
 (sx,sy) = (2,3) (Eの位置) (tx, ty) = (2, 1) (2の位置)
 nums[] = { 14, 13, 7, 1, 2, -1 } (ED712)
 ops[] = { 0, 0, 2, 2, 1, 1, -1 } (LLUURR -- 始点Eに入る操作、終点2から出る操作も含む)
  12???? 
 7=????
 DE????
 ※この例で(sx, sy)と(tx, ty)は逆でも良い。その場合nums[]とops[]も217DE、LLDDRRのようになる。

 static void moveToTerminal(Board const&m, int& sx, int& sy) (sx, sy)をマークの端点に移動
 上記の例だと(sx, sy)が(2,3)(または(2,1))になる
 
 static int isTerminal((Board const&m, int& sx, int& sy) 
 点(sx, sy)がマークの端点なら-1でなければどちらかのマークに移動する動作を返す
 
 void calcMarkedPositions(Board &m, int& nMarked, int mx, int my) 
 点(mx, my)から始めて3方がマークまたは壁となるところはすべてマークする
 
 int do_IDAStarWithMark(Board &m) 盤面mでマークされた場所をホームとする数字以外の距離は0としてIDA*を起動
 
 int search(int numFill) 部分解答式solver; 部分解答FillFar()をnumFill回呼んでからIDA*で解く。
 
 int searchWithHint(int hint[]) ヒント付き部分解答式solver;
 配列hint[]の値が-1になるまでの各値でFillFarWithHint()をくり返し呼んでからIDA*で解く。
 
 int display() 解答を標準出力に出力する
 
 
◎クラス外の関数
unsigned int  WINAPI solve_one(LPVOID pParam) パラメータpParamの問題の解答を標準出力に出力
pParamはchar const*にcastすると問題の文字列を得ます。
 
int _tmain(int argc, _TCHAR* argv[]) 問題の解答出力
問題(2行目の値は問題数；3行目以降を問題と見なす)のうち空行でない中で問題番号を1から順に付けたとき、問題番号%NUM_PART が argv[1]の値と一致した問題について解き、その解答を出力。出力形式は、行数は必ず2行目の問題数と一致、空行の問題や解かなかった問題、解けなかった問題については空行を出力。
解いた問題については、<解答となるLURDの列>, <Lの使用回数>, <Rの使用回数>, <Uの使用回数>, <Dの使用回数>という行を出力。

以上
