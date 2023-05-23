#include "head.h"


string line_name[MAX_LINE]; // 路线的名字
vector<int> line[MAX_LINE]; // 路线中有哪些车站
map<string, int> point_index;	//每个站的标号
string point_name[MAX_POINT];	// 每个站的名字
vector<int> neighbor[MAX_POINT];	// 点a的第i个邻居的站点序号为：neighbor[a][i]				//
vector<int> cost[MAX_POINT];        // 点a到第i个邻居的距离为：cost[a][i]						// 邻居的顺序是由输入文件决定的
vector<int> from_line[MAX_POINT];   // 点a到第i个邻居的地铁路线序号为：from_line[a][i]			//
int dis[MAX_POINT];		//dis[a]表示起点到a点的最短距离
bool vis[MAX_POINT];	//标记
int pre[MAX_POINT];     //起点到a点的最短路径上，a点之前的一个点是pre[a] 
                        //因此到a点的最短路径为 start_point -> ... -> pre[pre[a]] -> pre[a] -> a
int pre_line[MAX_POINT];//a到pre[a]的路线属于pre_line路线

bool vis_func3[MAX_POINT];	//功能3专用标记
int remaining_point = -1;	//功能三剩余点数


//线路数
int line_count;
//车站数
int point_count;
//标识是哪个fun调用此模块
int fun = 0;

//根据站点名字返回 标号
inline int to_point_index(string name) {
	return point_index[name];
}
//根据标号返回 站点名字
inline string to_point_name(int index) {
	return point_name[index];
}

void init()
{
	memset(dis, INF, sizeof(dis));
	memset(vis, false, sizeof(vis));
	memset(vis_func3, false, sizeof(vis_func3));
	line_count = point_count = 0;
}

/*
void dijkstra(int stp)//start point
{
	dis[stp] = 0; //自己到自己距离为0
	for (int i = 1; i <= n; i++)
	{
		//遍历所有点，找到和 stp 距离最短的点 并更新到该点的路线
		int minx = INF;
		int min_dis_point;
		for (int j = 1; j <= n; j++)
		{
			if (vis[j] == false && dis[j] <= minx)//必须是未被标记的点
			{
				minx = dis[j];
				min_dis_point = j;
				//route[min_dis_point] = route[i] + " " + to_string(min_dis_point);
			}
		}
		//并标记
		vis[min_dis_point] = true;
		dis[min_dis_point] = minx;

		//更新所有和 min_dis_point 连接的点的距离 以及 路线
		for (int j = 1; j <= n; j++)
		{
			if (vis[j] == false && dis[j] > dis[min_dis_point] + maze[min_dis_point][j])
			{
				dis[j] = dis[min_dis_point] + maze[min_dis_point][j];
				route[j] = route[min_dis_point] + " " + to_string(j);
			}
		}
	}
}*/

void input_map()
{
	ifstream fin;
	fin.open("subway2.txt", ios::in);//此处修改输入文件
	while (getline(fin, line_name[line_count++])) {
		// 显示读入的路线名
		// cout << "line name : " << line_name[line_count - 1] << endl;
		string tmp;
		getline(fin, tmp);
		istringstream in(tmp);
		while (getline(in, tmp, ' ')) {
			// 如果这个车站是第一次出现，为它分配一个标号
			if (point_index.count(tmp) == 0) {
				point_index[tmp] = point_count;
				point_name[point_count++] = tmp;
			}
			int i = point_index[tmp];
			line[line_count - 1].push_back(i);

			// 显示读入的站名
			// cout << "|" << i << tmp << "|";
		}
	}
	line_count--;
	fin.close();
}

//根据输入的 地铁线路名称 输出 该线路上的所有站点名称
string line_request(string name) {
	string res = "\n";
	int target_line = -1;
	for (int i = 0; i < line_count; ++i) {
		if (line_name[i] == name) {
			target_line = i;//取出线路名称对应的index号
			break;
		}
	}
	if (target_line == -1) {
		res = "\n对不起，您查询的路线不存在，请重新输入。\n";
	}
	else {
		for (int point : line[target_line]) {
			res += to_point_name(point) + "\n";
		}
	}
	
	return res;
}

string line_list() {
	string res = "\n" + line_name[0];
	for (int i = 1; i < line_count; ++i) {
		res += ", " + line_name[i];
	}
	return res + "\n";
}

//更新站点的邻接关系
void add_edge(int x, int y, int l, int c = 1) {
	neighbor[x].push_back(y);	//点y是点x的邻居
	cost[x].push_back(c);		//目前不考虑换线开销，故所有点连线的cost都默认为1
	from_line[x].push_back(l);	//x到y的路线是在哪条地铁线上
	neighbor[y].push_back(x);	//点x是点y的邻居
	cost[y].push_back(c);		
	from_line[y].push_back(l);	//y到x的路线是在哪条地铁线上（当然与上面一致）
}

//初始化所有站点的邻接关系、站点连线所在的地铁线路（不考虑换乘开销）
void build_graph_normal() {
	for (int i = 0; i < line_count; ++i) {//沿着不同的地铁线路来初始化在地铁线上的各个站点的邻接关系，并记录站点连线是几号线（即i）
		int pre_point = -1;//有效站点的index值从0开始
		for (int point : line[i]) {//按line[i]内站点的顺序，初始化i这条地铁线上各站点的信息
			if (pre_point >= 0) {//如果 pre_point 是有效站点
				add_edge(pre_point, point, i);//初始化
			}
			pre_point = point;//把这次循环初始化的站点置为前站点
		}
	}
}

//计算路过站点数，并更新路过站点的vis_func3
int count_step(int t, int s) {
	int temp = t;
	int count = 1;
	while (temp != s) {
		count++;
		temp = pre[temp];
		//func3标记更新
		if (vis_func3[temp] == false) {
			vis_func3[temp] = true;
			remaining_point--;
		}
	}

	return count;
}

//生成输出路线
string route(int s, int t) {
	string res = "";
	int temp = t;
	int last_line = -1;
	do {
		if (temp != t) {
			res += to_point_name(t);//string上加站点名称
		}
		if (fun == 2) {
			if (pre_line[t] != last_line && last_line != -1) {//如果和上次坐的线不同了就要输出换乘信息
				res += " 换乘" + line_name[pre_line[t]];
			}
		}
		
		last_line = pre_line[t];
		if (temp != t) {
			res += "\n";
		}
		t = pre[t];
	} while (t != s);
	res += to_point_name(t);

	return res + "\n";
}

//功能一：查询并输出路线上的站点
int task_line_req() {
	while (true) {
		cout << "输入0退出，输入?显示所有路线。" << endl;
		cout << "请输入想要查询的路线：";
		cout.flush();
		string tmp;
		cin >> tmp;//输入想查询的线路名称
		if (tmp == "0") break;
		if (tmp == "?" || tmp == "？") {
			cout << line_list() << endl;
		}
		else {
			cout << line_request(tmp) << endl;
		}
	}
	return 0;
}

//功能二：计算两个车站之间的最短距离（换乘不额外计距离）。返回需要输出的字符串。因为相邻车站之间的距离都为1，使用BFS算法。 
string display_normal(string src_name, string tar_name) {
	fun = 2;
	int t = to_point_index(src_name), s = to_point_index(tar_name);//！！！t是起点，s是终点（与后续程序命名冲突，后续修改）
	if (s == t) {//如果起点就是终点
		return "\n" + (string)"1" + "\n" + src_name + "\n\n";
	}
	build_graph_normal();//建图
	queue<int> q;//先进先出
	q.push(s);//终点站序号入队
	dis[s] = 0;//把自己到自己的距离更新为0
	vis[s] = true;//标记此点，说明已经走过
	while (!q.empty()) {
		int point = q.front();//取队列第一个数的值
		q.pop();//删除队列第一个数
		for (int i = 0; i < neighbor[point].size(); ++i) {
			int nex_point = neighbor[point][i];
			if (!vis[nex_point]) {//没走过该点
				vis[nex_point] = 1;//标记此点
				dis[nex_point] = dis[point] + 1;
				pre[nex_point] = point;
				pre_line[nex_point] = from_line[point][i];
				if (nex_point == t) {
					break;//到达终点break
				}
				q.push(nex_point);
			}
		}
		if (vis[t]) break;
	}

	string res= "\n";
	int count_point = count_step(t, s);
	res = res + to_string(count_point) + "\n" + src_name + route(s, t);
	return res;

}


string bfs_func3(int point_now, int next_point) {
	int t = point_now, s = next_point;
	queue<int> q;//先进先出
	q.push(s);//终点站序号入队
	dis[s] = 0;//把自己到自己的距离更新为0
	vis[s] = true;//标记此点，说明已经走过
	while (!q.empty()) {
		int point = q.front();//取队列第一个数的值
		q.pop();//删除队列第一个数
		for (int i = 0; i < neighbor[point].size(); ++i) {
			int nex_point = neighbor[point][i];
			if (!vis[nex_point]) {//没走过该点
				vis[nex_point] = 1;//标记此点
				dis[nex_point] = dis[point] + 1;
				pre[nex_point] = point;
				pre_line[nex_point] = from_line[point][i];
				if (nex_point == t) {
					break;//到达终点break
				}
				q.push(nex_point);
			}
		}
		if (vis[t]) break;
	}
	return route(s, t);
}

//功能三：计算尽可能快地遍历地铁的所有车站的路线
string display_func3(string src_name) {
	fun = 3;
	string res = src_name + "\n";
	int step = 1;	//记录经过站点数
	int st = to_point_index(src_name);//起点
	int point_now = st;//此时所在点
	int next_point = -1;
	remaining_point = point_count;
	build_graph_normal();//建图
	
	remaining_point--;		//
	vis_func3[st] = true;	//排除起点
							//

	while (remaining_point != 0) {	// point_now 的 neighbor 中随机选一个 vis_func3 != true 的点作为下一点
		int neighbor_size = neighbor[point_now].size();
		int *point_unreached = new int[neighbor_size];
		int j = 0;	//point_unreached的下标
		for (int i = 0; i < neighbor_size; i++) {
			if (vis_func3[neighbor[point_now][i]] != true) {
				point_unreached[j] = neighbor[point_now][i];
				j++;
			}
		}

		if (j != 0) {	//j != 0 说明point_unreached不为空
			//取随机数
			srand((unsigned int)time(NULL));
			int temp = rand() % j;
			next_point = point_unreached[temp];

			res += to_point_name(next_point) + "\n";

			vis_func3[next_point] = true;
			point_now = next_point;
			remaining_point--;
			step++;

			delete point_unreached;
		} else {		//point_unreached为空，说明邻居都被标记了，从全局找 next_point
			int *point_unreached_2 = new int[point_count];
			int k = 0;//point_unreached_2的下标
			for (int i = 0; i < point_count; i++) {
				if (vis_func3[i] != true) {
					point_unreached_2[k] = i;
					k++;
				}
			}

			srand((unsigned int)time(NULL));
			int temp = rand() % k;
			next_point = point_unreached_2[temp];


			//调用bfs
			bfs_func3(point_now, next_point);
			res += bfs_func3(point_now, next_point);


			step += (count_step(point_now,next_point) - 1);
			remaining_point;
			vis_func3[next_point] = true;
			point_now = next_point;

			delete point_unreached_2;

			//初始化所有func2中使用的所有全局变量
			memset(vis, false, sizeof(vis));
			memset(dis, INF, sizeof(dis));
			memset(pre, 0, sizeof(pre));
			memset(pre_line, 0, sizeof(pre_line));
		}
	}

	//所有点走完了，回起点
	if (remaining_point == 0) {	

		next_point = st;
		//调用bfs
		bfs_func3(point_now, next_point);
		res += bfs_func3(point_now, next_point);

		step += (count_step(point_now, next_point) - 1);
		remaining_point;
		vis_func3[next_point] = true;
		point_now = next_point;
	}



	return to_string(step) + "\n" + res;
}




int main(int argc, char *argv[])
{
	//调试功能二：
	argc = 4;
	argv[1] = const_cast<char*>("/a");
	argv[2] = const_cast<char*>("十号点");
	argv[3] = const_cast<char*>("十二号点");


	//输入 n m,初始化各点距离，相连的点距离为1，未相连的点距离为INF,
	init();
	input_map();

	// 没有额外参数，进入功能一：查询路线，输出路线上所有站点
	if (argc == 1) {
		
		return task_line_req();
	}

	//命令行调用程序加上了参数 /b ，进入功能二：计算所输入两个车站间的最短路线
	if (strcmp(argv[1], "/b") == 0) {
		
		// 换乘没有额外距离的情况
		cout << display_normal((string)argv[2], (string)argv[3]);
		
		// 换乘开销记为3（待完成）

	}

	//命令行调用程序加上了参数 /a ，进入功能三：计算尽可能快地遍历地铁的所有车站的路线(先不考虑换乘开销，地图为等权无向图）
	if (strcmp(argv[1], "/a") == 0) {
		
		// 换乘没有额外距离的情况
		cout << display_func3((string)argv[2]);


	}
	
	
	return 0;
	
}

