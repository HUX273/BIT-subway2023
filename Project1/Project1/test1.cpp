#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include<ctime>

using namespace std;

const int INF = 0x7f7f7f7f;

const int MAX_POINT = 500; //��վ�ĸ�������
const int MAX_LINE = 50; //·�ߵĸ�������


string line_name[MAX_LINE]; // ·�ߵ�����
vector<int> line[MAX_LINE]; // ·��������Щ��վ
map<string, int> point_index;	//ÿ��վ�ı��
string point_name[MAX_POINT];	// ÿ��վ������
vector<int> neighbor[MAX_POINT];	// ��a�ĵ�i���ھӵ�վ�����Ϊ��neighbor[a][i]				//
vector<int> cost[MAX_POINT];        // ��a����i���ھӵľ���Ϊ��cost[a][i]						// �ھӵ�˳�����������ļ�������
vector<int> from_line[MAX_POINT];   // ��a����i���ھӵĵ���·�����Ϊ��from_line[a][i]			//
int dis[MAX_POINT];		//dis[a]��ʾ��㵽a�����̾���
bool vis[MAX_POINT];	//���
int pre[MAX_POINT];     //��㵽a������·���ϣ�a��֮ǰ��һ������pre[a] 
                        //��˵�a������·��Ϊ start_point -> ... -> pre[pre[a]] -> pre[a] -> a
int pre_line[MAX_POINT];//a��pre[a]��·������pre_line·��

bool vis_func3[MAX_POINT];	//����3ר�ñ��
int remaining_point = -1;	//������ʣ�����


//��·��
int line_count;
//��վ��
int point_count;
//��ʶ���ĸ�fun���ô�ģ��
int fun = 0;

//����վ�����ַ��� ���
inline int to_point_index(string name) {
	return point_index[name];
}
//���ݱ�ŷ��� վ������
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
	fin.open("subway2.txt", ios::in);//�˴��޸������ļ�
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

//��������� ������·���� ��� ����·�ϵ�����վ������
string line_request(string name) {
	string res = "\n";
	int target_line = -1;
	for (int i = 0; i < line_count; ++i) {
		if (line_name[i] == name) {
			target_line = i;//ȡ����·���ƶ�Ӧ��index��
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

//����վ����ڽӹ�ϵ
void add_edge(int x, int y, int l, int c = 1) {
	neighbor[x].push_back(y);	//��y�ǵ�x���ھ�
	cost[x].push_back(c);		//Ŀǰ�����ǻ��߿����������е����ߵ�cost��Ĭ��Ϊ1
	from_line[x].push_back(l);	//x��y��·������������������
	neighbor[y].push_back(x);	//��x�ǵ�y���ھ�
	cost[y].push_back(c);		
	from_line[y].push_back(l);	//y��x��·�����������������ϣ���Ȼ������һ�£�
}

//��ʼ������վ����ڽӹ�ϵ��վ���������ڵĵ�����·�������ǻ��˿�����
void build_graph_normal() {
	for (int i = 0; i < line_count; ++i) {//���Ų�ͬ�ĵ�����·����ʼ���ڵ������ϵĸ���վ����ڽӹ�ϵ������¼վ�������Ǽ����ߣ���i��
		int pre_point = -1;//��Чվ���indexֵ��0��ʼ
		for (int point : line[i]) {//��line[i]��վ���˳�򣬳�ʼ��i�����������ϸ�վ�����Ϣ
			if (pre_point >= 0) {//��� pre_point ����Чվ��
				add_edge(pre_point, point, i);//��ʼ��
			}
			pre_point = point;//�����ѭ����ʼ����վ����Ϊǰվ��
		}
	}
}

//����·��վ����
int count_step(int t, int s) {
	int temp = t;
	int count = 1;
	while (temp != s) {
		count++;
		temp = pre[temp];
		//func3��Ǹ���
		if (vis_func3[temp] == false) {
			vis_func3[temp] = true;
			remaining_point--;
		}
	}

	return count;
}


//�������·��
string route(int s, int t) {
	string res = "";
	int temp = t;
	int last_line = -1;
	do {
		if (temp != t) {
			res += to_point_name(t);//string�ϼ�վ������
		}
		if (fun == 2) {
			if (pre_line[t] != last_line && last_line != -1) {//������ϴ������߲�ͬ�˾�Ҫ���������Ϣ
				res += " ����" + line_name[pre_line[t]];
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

//����һ����ѯ�����·���ϵ�վ��
int task_line_req() {
	while (true) {
		cout << "����0�˳�������?��ʾ����·�ߡ�" << endl;
		cout << "��������Ҫ��ѯ��·�ߣ�";
		cout.flush();
		string tmp;
		cin >> tmp;//�������ѯ����·����
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

//���ܶ�������������վ֮�����̾��루���˲�����ƾ��룩��������Ҫ������ַ�������Ϊ���ڳ�վ֮��ľ��붼Ϊ1��ʹ��BFS�㷨�� 
string display_normal(string src_name, string tar_name) {
	fun = 2;
	int t = to_point_index(src_name), s = to_point_index(tar_name);//������t����㣬s���յ㣨���������������ͻ�������޸ģ�
	if (s == t) {//����������յ�
		return "\n" + (string)"1" + "\n" + src_name + "\n\n";
	}
	build_graph_normal();//��ͼ
	queue<int> q;//�Ƚ��ȳ�
	q.push(s);//�յ�վ������
	dis[s] = 0;//���Լ����Լ��ľ������Ϊ0
	vis[s] = true;//��Ǵ˵㣬˵���Ѿ��߹�
	while (!q.empty()) {
		int point = q.front();//ȡ���е�һ������ֵ
		q.pop();//ɾ�����е�һ����
		for (int i = 0; i < neighbor[point].size(); ++i) {
			int nex_point = neighbor[point][i];
			if (!vis[nex_point]) {//û�߹��õ�
				vis[nex_point] = 1;//��Ǵ˵�
				dis[nex_point] = dis[point] + 1;
				pre[nex_point] = point;
				pre_line[nex_point] = from_line[point][i];
				if (nex_point == t) {
					break;//�����յ�break
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
	queue<int> q;//�Ƚ��ȳ�
	q.push(s);//�յ�վ������
	dis[s] = 0;//���Լ����Լ��ľ������Ϊ0
	vis[s] = true;//��Ǵ˵㣬˵���Ѿ��߹�
	while (!q.empty()) {
		int point = q.front();//ȡ���е�һ������ֵ
		q.pop();//ɾ�����е�һ����
		for (int i = 0; i < neighbor[point].size(); ++i) {
			int nex_point = neighbor[point][i];
			if (!vis[nex_point]) {//û�߹��õ�
				vis[nex_point] = 1;//��Ǵ˵�
				dis[nex_point] = dis[point] + 1;
				pre[nex_point] = point;
				pre_line[nex_point] = from_line[point][i];
				if (nex_point == t) {
					break;//�����յ�break
				}
				q.push(nex_point);
			}
		}
		if (vis[t]) break;
	}
	return route(s, t);
}

//�����������㾡���ܿ�ر������������г�վ��·��
string display_func3(string src_name) {
	fun = 3;
	string res = src_name + "\n";
	int step = 1;	//��¼����վ����
	int st = to_point_index(src_name);//���
	int point_now = st;//��ʱ���ڵ�
	int next_point = -1;
	remaining_point = point_count;
	build_graph_normal();//��ͼ
	
	remaining_point--;		//
	vis_func3[st] = true;	//�ų����
							//

	while (remaining_point != 0) {	// point_now �� neighbor �����ѡһ�� vis_func3 != true �ĵ���Ϊ��һ��
		int neighbor_size = neighbor[point_now].size();
		int *point_unreached = new int[neighbor_size];
		int j = 0;	//point_unreached���±�
		for (int i = 0; i < neighbor_size; i++) {
			if (vis_func3[neighbor[point_now][i]] != true) {
				point_unreached[j] = neighbor[point_now][i];
				j++;
			}
		}

		if (j != 0) {	//j != 0 ˵��point_unreached��Ϊ��
			//ȡ�����
			srand((unsigned int)time(NULL));
			int temp = rand() % j;
			next_point = point_unreached[temp];

			res += to_point_name(next_point) + "\n";

			vis_func3[next_point] = true;
			point_now = next_point;
			remaining_point--;
			step++;

			delete point_unreached;
		} else {		//point_unreachedΪ�գ�˵���ھӶ�������ˣ���ȫ���� next_point
			int *point_unreached_2 = new int[point_count];
			int k = 0;//point_unreached_2���±�
			for (int i = 0; i < point_count; i++) {
				if (vis_func3[i] != true) {
					point_unreached_2[k] = i;
					k++;
				}
			}

			srand((unsigned int)time(NULL));
			int temp = rand() % k;
			next_point = point_unreached_2[temp];


			//����bfs
			bfs_func3(point_now, next_point);
			res += bfs_func3(point_now, next_point);


			step += (count_step(point_now,next_point) - 1);
			remaining_point;
			vis_func3[next_point] = true;
			point_now = next_point;

			delete point_unreached_2;

			//��ʼ������func2��ʹ�õ�����ȫ�ֱ���
			memset(vis, false, sizeof(vis));
			memset(dis, INF, sizeof(dis));
			memset(pre, 0, sizeof(pre));
			memset(pre_line, 0, sizeof(pre_line));
		}
	}

	//���е������ˣ������
	if (remaining_point == 0) {	

		next_point = st;
		//����bfs
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
	//���Թ��ܶ���
	argc = 4;
	argv[1] = const_cast<char*>("/a");
	argv[2] = const_cast<char*>("ʮ�ŵ�");
	argv[3] = const_cast<char*>("ʮ���ŵ�");


	//���� n m,��ʼ��������룬�����ĵ����Ϊ1��δ�����ĵ����ΪINF,
	init();
	input_map();

	// û�ж�����������빦��һ����ѯ·�ߣ����·��������վ��
	if (argc == 1) {
		
		return task_line_req();
	}

	//�����е��ó�������˲��� /b �����빦�ܶ�������������������վ������·��
	if (strcmp(argv[1], "/b") == 0) {
		
		// ����û�ж����������
		cout << display_normal((string)argv[2], (string)argv[3]);
		
		// ���˿�����Ϊ3������ɣ�

	}

	//�����е��ó�������˲��� /a �����빦���������㾡���ܿ�ر������������г�վ��·��(�Ȳ����ǻ��˿�������ͼΪ��Ȩ����ͼ��
	if (strcmp(argv[1], "/a") == 0) {
		
		// ����û�ж����������
		cout << display_func3((string)argv[2]);


	}
	
	
	return 0;
	
}

