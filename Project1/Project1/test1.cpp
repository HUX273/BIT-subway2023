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

const int MAX_POINT = 500; //��վ�ĸ�������
const int MAX_LINE = 50; //·�ߵĸ�������

string line_name[MAX_LINE]; // ·�ߵ�����
vector<int> line[MAX_LINE]; // ·��������Щ��վ
map<string, int> point_index;	//ÿ��վ�ı��
string point_name[MAX_POINT]; // ÿ��վ������
vector<int> neighbor[MAX_POINT];	// ��a�ĵ�i���ھ���neighbor[a][i]
vector<int> cost[MAX_POINT];        // ��a��neighbor[a][i]�ľ���Ϊcost
vector<int> from_line[MAX_POINT];   // ��a��neighbor[a][i]��·������from_line·��
int dis[MAX_POINT];		//dis[a]��ʾ��㵽a�����̾���
bool vis[MAX_POINT];	//���
int pre[MAX_POINT];     //��㵽a������·���ϣ�a��֮ǰ��һ������pre[a] 
                        //��˵�a������·��Ϊ start_point -> ... -> pre[pre[a]] -> pre[a] -> a
int pre_line[MAX_POINT];//a��pre[a]��·������pre_line·��

// ��·������վ��
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
	dis[stp] = 0; //�Լ����Լ�����Ϊ0
	for (int i = 1; i <= n; i++)
	{
		//�������е㣬�ҵ��� stp ������̵ĵ� �����µ��õ��·��
		int minx = INF;
		int min_dis_point;
		for (int j = 1; j <= n; j++)
		{
			if (vis[j] == false && dis[j] <= minx)//������δ����ǵĵ�
			{
				minx = dis[j];
				min_dis_point = j;
				//route[min_dis_point] = route[i] + " " + to_string(min_dis_point);
			}
		}
		//�����
		vis[min_dis_point] = true;
		dis[min_dis_point] = minx;

		//�������к� min_dis_point ���ӵĵ�ľ��� �Լ� ·��
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
		// ��ʾ�����·����
		// cout << "line name : " << line_name[line_count - 1] << endl;
		string tmp;
		getline(fin, tmp);
		istringstream in(tmp);
		while (getline(in, tmp, ' ')) {
			// ��������վ�ǵ�һ�γ��֣�Ϊ������һ�����
			if (point_index.count(tmp) == 0) {
				point_index[tmp] = point_count;
				point_name[point_count++] = tmp;
			}
			int i = point_index[tmp];
			line[line_count - 1].push_back(i);

			// ��ʾ�����վ��
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
		res = "\n�Բ�������ѯ��·�߲����ڣ����������롣\n";
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
		cout << "����0�˳�������?��ʾ����·�ߡ�" << endl;
		cout << "��������Ҫ��ѯ��·�ߣ�";
		cout.flush();
		string tmp;
		cin >> tmp;
		if (tmp == "0") break;
		if (tmp == "?" || tmp == "��") {
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
			res += " ����" + line_name[pre_line[t]];
		}
		last_line = pre_line[t];
		res += "\n";
		t = pre[t];
	} while (t != s);
	res += to_point_name(t) + "\n";
	return res + "\n";
}

/// ����������վ֮�����̾��롣���˲��ƾ��롣������Ҫ������ַ�����
/// ��Ϊ���ڳ�վ֮��ľ��붼Ϊ1��ʹ��BFS�㷨�� 
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
	//���� n m,��ʼ��������룬�����ĵ����Ϊ1��δ�����ĵ����ΪINF,
	init();
	input_map();

	if (argc == 1) {
		// û�ж����������ѯ·�ߣ����·��������վ��
		return task_line_req();
	}

	if (strcmp(argv[1], "/b") == 0) {
		// ���� /b : ����������������վ�����̾���

		// ����û�ж����������
		cout << dis_normal((string)argv[2], (string)argv[3]);
		
		// ���˾����Ϊ3������ɣ�
		//��1Ϊ�����һ��dij
		//dijkstra(1);
	}

	
	
	return 0;
	
}

