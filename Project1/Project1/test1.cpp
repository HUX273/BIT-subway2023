#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <queue>
using namespace std;

const int INF = 0x7f7f7f7f;

const int MAX_POINT = 500; //车站的个数上限
const int MAX_LINE = 50; //路线的个数上限

string line_name[MAX_LINE]; // 路线的名字
vector<int> line[MAX_LINE]; // 路线中有哪些车站
map<string, int> point_index;	//每个站的标号
string point_name[MAX_POINT]; // 每个站的名字
vector<int> neighbor[MAX_POINT];	// 点a的第i个邻居是neighbor[a][i]
vector<int> cost[MAX_POINT];        // 点a到neighbor[a][i]的距离为cost
vector<int> from_line[MAX_POINT];   // 点a到neighbor[a][i]的路线属于from_line路线
int dis[MAX_POINT];		//dis[a]表示起点到a点的最短距离
bool vis[MAX_POINT];	//标记
int pre[MAX_POINT];     //起点到a点的最短路径上，a点之前的一个点是pre[a] 
                        //因此到a点的最短路径为 start_point -> ... -> pre[pre[a]] -> pre[a] -> a
int pre_line[MAX_POINT];//a到pre[a]的路线属于pre_line路线

// 线路数、车站数
int line_count, point_count;

inline int to_point_index(string name) {
	return point_index[name];
}
inline string to_point_name(int index) {
	return point_name[index];
}

void add_edge(int x, int y, int l, int c = 1) {
	neighbor[x].push_back(y);
	cost[x].push_back(c);
	from_line[x].push_back(l);
	neighbor[y].push_back(x);
	cost[y].push_back(c);
	from_line[y].push_back(l);
}

void init()
{
	memset(dis, INF, sizeof(dis));
	memset(vis, false, sizeof(vis));
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
	fin.open("subway1.txt", ios::in);
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

string line_request(string name) {
	string res = "\n";
	int target_line = -1;
	for (int i = 0; i < line_count; ++i) {
		if (line_name[i] == name) {
			target_line = i;
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

int task_line_req() {
	while (true) {
		cout << "输入0退出，输入?显示所有路线。" << endl;
		cout << "请输入想要查询的路线：";
		cout.flush();
		string tmp;
		cin >> tmp;
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

void build_graph_normal() {
	for (int i = 0; i < line_count; ++i) {
		int pre_point = -1;
		for (int point : line[i]) {
			if (pre_point >= 0) {
				add_edge(pre_point, point, i);
			}
			pre_point = point;
		}
	}
}

string route(int s, int t) {
	string res = "\n";
	int last_line = -1;
	do {
		res += to_point_name(t);
		if (pre_line[t] != last_line && last_line != -1) {
			res += " 换乘" + line_name[pre_line[t]];
		}
		last_line = pre_line[t];
		res += "\n";
		t = pre[t];
	} while (t != s);
	res += to_point_name(t) + "\n";
	return res + "\n";
}

/// 计算两个车站之间的最短距离。换乘不计距离。返回需要输出的字符串。
/// 因为相邻车站之间的距离都为1，使用BFS算法。 
string dis_normal(string src_name, string tar_name) {
	int t = to_point_index(src_name), s = to_point_index(tar_name);
	if (s == t) {
		return "\n" + src_name + "\n\n";
	}
	build_graph_normal();
	queue<int> q;
	q.push(s);
	dis[s] = 0;
	vis[s] = true;
	while (!q.empty()) {
		int point = q.front();
		q.pop();
		for (int i = 0; i < neighbor[point].size(); ++i) {
			int nex_point = neighbor[point][i];
			if (!vis[nex_point]) {
				vis[nex_point] = 1;
				dis[nex_point] = dis[point] + 1;
				pre[nex_point] = point;
				pre_line[nex_point] = from_line[point][i];
				if (nex_point == t) {
					break;
				}
				q.push(nex_point);
			}
		}
		if (vis[t]) break;
	}
	return route(s, t);
}

int main(int argc, char *argv[])
{
	//输入 n m,初始化各点距离，相连的点距离为1，未相连的点距离为INF,
	init();
	input_map();

	if (argc == 1) {
		// 没有额外参数，查询路线，输出路线上所有站点
		return task_line_req();
	}

	if (strcmp(argv[1], "/b") == 0) {
		// 参数 /b : 计算所输入两个车站间的最短距离

		// 换乘没有额外距离的情况
		cout << dis_normal((string)argv[2], (string)argv[3]);
		
		// 换乘距离记为3（待完成）
		//以1为起点跑一次dij
		//dijkstra(1);
	}

	
	
	return 0;
	
}

