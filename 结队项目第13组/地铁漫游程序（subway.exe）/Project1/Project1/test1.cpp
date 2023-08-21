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
#include<math.h>

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
bool consider_transfer_cost = 0;//���ǻ��˿���


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
	fin.open("subway3.txt", ios::in);//�˴��޸������ļ�
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

// ����·��վ������func3����ʱ����·��վ���vis_func3��ʣ�������
int count_step(int t, int s) {
	int temp = t;
	int count = 1;
	int last_line = -1;
	while (temp != s) {
		count++;

		if (consider_transfer_cost == 1) {// ���㻻�˿���
			if (pre_line[temp] != last_line && last_line != -1) {// ������ϴ������߲�ͬ�˾�Ҫ�����count
				count += 3;
			}
			last_line = pre_line[temp];
		}

		temp = pre[temp];

		//func3��Ǹ��£���func3���ô˺���ʱ��ִ��
		if (vis_func3[temp] == false) {
			vis_func3[temp] = true;
			remaining_point--;
		}

	}

	return count;
}

// ##��������##����·��վ������������·��վ��
int count_step(int t, int s, bool global) {
	int temp = t;
	int count = 1;
	int last_line = -1;
	while (temp != s) {
		count++;
		
		if (consider_transfer_cost == 1) {// ���㻻�˿���
			if (pre_line[temp] != last_line && last_line != -1) {// ������ϴ������߲�ͬ�˾�Ҫ�����count
				count += 3;
			}
			last_line = pre_line[temp];
		}

		temp = pre[temp];
	}
	

	return count;
}



//��������յ��������·��string�����øú�����ǰ���ǽ��й�bfs��
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
		for (unsigned int i = 0; i < neighbor[point].size(); ++i) {
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

	string res = "\n";
	int count_point = count_step(t, s);
	res = res + to_string(count_point) + "\n" + src_name + route(s, t);
	return res;

}

//bfs_func3������func3�ڵ����ݣ���count_step�и���func3�ڵķ��ʱ�Ǻ�ʣ�������
string bfs_func3(int point_now, int next_point) {
	int t = point_now, s = next_point;
	queue<int> q;//�Ƚ��ȳ�
	q.push(s);//�յ�վ������
	dis[s] = 0;//���Լ����Լ��ľ������Ϊ0
	vis[s] = true;//��Ǵ˵㣬˵���Ѿ��߹�
	while (!q.empty()) {
		int point = q.front();//ȡ���е�һ������ֵ
		q.pop();//ɾ�����е�һ����
		for (unsigned int i = 0; i < neighbor[point].size(); ++i) {
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


int quick_sort(int* dis_global, int left, int right, int* point_rank) {

	if (left > right) {
		return -1;
	}

	int pivot = dis_global[left];//����������ߵľ���
	int point_trank_temp = point_rank[left];

	int i = left;//i��
	int j = right;//j��

	while (i != j) {
		while (i < j && dis_global[j] >= pivot) {
			j--;
		}
		dis_global[i] = dis_global[j];
		point_rank[i] = point_rank[j];

		while (i < j && dis_global[i] <= pivot) {
			i++;
		}
		dis_global[j] = dis_global[i];
		point_rank[j] = point_rank[i];
	}

	dis_global[i] = pivot;
	point_rank[i] = point_trank_temp;

	quick_sort(dis_global, left, i - 1, point_rank);
	quick_sort(dis_global, i + 1, right, point_rank);
	return 0;
}

//ȡ���������
double getRandData(int min, int max)
{
	double m1 = (double)(rand() % 101) / 101;                        // ���� 0��1֮������С��,�õ���ֵ�����Ϊ(0,1)
	min++;                                                                             //�� �����Ϊ(min+1,max),
	double m2 = (double)((rand() % (max - min + 1)) + min);    //���� min+1,max ֮�������������õ���ֵ��Ϊ[min+1,max]
	m2 = m2 - 1;                                                                        //��ֵ��Ϊ[min,max-1]
	return m1 + m2;                                                                //����ֵ��Ϊ(min,max),Ϊ�������������
}


//�����������㾡���ܿ�ر������������г�վ��·��
string display_func3(string src_name) {
	
	//��������ǻ��˿�����ע�͵�����
	consider_transfer_cost = 1;
	//

	fun = 3;
	string res;//����·�߽��
	string res_temp;//��¼����վ��·��
	int step = INF;//���ղ������
	int step_temp = INF;//��¼����վ����
	int st = to_point_index(src_name);//���
	build_graph_normal();//��ͼ

	//�ظ�nn��ȡ���·�������·����С��һ��
	for (int nn = 5; nn > 0; nn--) {


		res_temp = src_name + "\n";
		step_temp = 1;
		st = to_point_index(src_name);
		int point_now = st;//��ʱ���ڵ�
		int next_point = MAX_POINT - 1;//bfsĿ���
		remaining_point = point_count;


		remaining_point--;		//
		vis_func3[st] = true;	//�ų����
		//

		while (remaining_point != 0) {	// ���ȴ� point_now �� neighbor �����ѡһ�� vis_func3 != true �ĵ���Ϊ��һ��
			int neighbor_size = neighbor[point_now].size();//�ڽӵ����
			int* point_unreached_adjacency = new int[neighbor_size];
			int j = 0;	//point_unreached���±�
			for (int i = 0; i < neighbor_size; i++) {
				if (vis_func3[neighbor[point_now][i]] != true) {
					point_unreached_adjacency[j] = neighbor[point_now][i];
					j++;
				}
			}


			if (j != 0) {	//j != 0 ˵��point_unreached_adjacency��Ϊ��
				//ȡ�����
				srand((unsigned int)time(NULL));
				int temp = rand() % j;
				next_point = point_unreached_adjacency[temp];

				res_temp += to_point_name(next_point) + "\n";

				vis_func3[next_point] = true;
				point_now = next_point;
				remaining_point--;

				step_temp++;

				delete[] point_unreached_adjacency;
			}
			else {		//point_unreached_adjacencyΪ�գ�˵���ڽӵ㶼������ˣ���ȫ���� next_point
				int* point_unreached_global = new int[point_count];
				int k = 0;//point_unreached_2���±�
				for (int i = 0; i < point_count; i++) {
					if (vis_func3[i] != true) {
						point_unreached_global[k] = i;
						k++;
					}
				}

				// �㷨1��ȫ������ȫ�����ѡȡ��һ��
				/*
				srand((unsigned int)time(NULL));
				int temp = rand() % k;
				next_point = point_unreached_global[temp];
				*/
				
				

				// �㷨2���˻��㷨������Ȩ�����ѡȡȫ����δ�߹���һ����Ϊ��һ��������Խ��Ȩ��Խ��
				// ĳ��ȫ�ֵ㣺point_unreached_global[i]��i��[0,k)
				// ����ȫ�ֵ���Ϊ��һ�㣬������̾��룬����¼��dis_global[i]��
				// ��dis_global���ҵ���С���룬ȡ��Ӧi���ö�Ӧ��point_unreached_global[i]
				int* dis_global = new int[k];
				for (int i = 0; i < k; i++) {
					int next_point_global = point_unreached_global[i];

					bfs_func3(point_now, next_point_global);

					dis_global[i] = count_step(point_now, next_point_global, true);

					memset(vis, false, sizeof(vis));
					memset(dis, INF, sizeof(dis));
					memset(pre, 0, sizeof(pre));
					memset(pre_line, 0, sizeof(pre_line));
				}
				// ��dis_global�е����ݰ��մ�С������������ǰ��Ȩ�ش����ں��Ȩ��С
				// ����һ��point_rank���ٵ���i����ʼ��point_rank[i] = i
				// ���Ž���dis_global�еľ����ͬʱ����point_rank�еı��
				int* point_rank = new int[k];
				for (int i = 0; i < k; i++) {
					point_rank[i] = i;
				}
				quick_sort(dis_global, 0, k - 1, point_rank);
				// ͨ����point_rank[i]��Ϊ����ϵ�����ɰ���·�������С�������point_unreached_global
				// ����point_rankǰ���Ȩ�ش󣬰�������ϵ��i����Ȩ�غ����ѡȡһ��
				// ��ѡ��point_rank[i]�ĸ���P= e^(-i)/[e^(-0)+e^(-1)+...+e^(-k+1)]
				// �������������0 ~ e^(-0)ʱȡ0������e^(-0) ~ e^(-0)+e^(-1)ʱȡ1��������
				double* P = new double[k];
				double Psum = 0;
				int const_num = 100; // �˻��㷨����ȡֵ������Ҫ������Լ��������µ��㷨����
				for (int i = 0; i < k; i++) {
					P[i] = pow(2.17828, -const_num * dis_global[i]);
					Psum += P[i];
				}

				srand((unsigned int)time(NULL));
				double random_num = getRandData(0, const_num);
				random_num = random_num * Psum / const_num;

				for (int i = 0; i < k; i++) {
					random_num -= P[i];
					if (random_num <= 0) {
						next_point = point_unreached_global[point_rank[i]];
						break;
					}
				}



				// ͨ���㷨ѡ����һ��վ������bfs
				res_temp += bfs_func3(point_now, next_point);

				step_temp += (count_step(point_now, next_point) - 1);
				remaining_point;
				vis_func3[next_point] = true;
				point_now = next_point;

				delete[] point_unreached_global;
				delete[] dis_global;
				delete[] point_rank;
				delete[] P;
				// ��ʼ������func2��ʹ�õ�����ȫ�ֱ���
				memset(vis, false, sizeof(vis));
				memset(dis, INF, sizeof(dis));
				memset(pre, 0, sizeof(pre));
				memset(pre_line, 0, sizeof(pre_line));
			}
		}

		// ���е������ˣ������
		if (remaining_point == 0) {

			next_point = st;
			// ����bfs
			bfs_func3(point_now, next_point);
			res_temp += bfs_func3(point_now, next_point);

			step_temp += (count_step(point_now, next_point) - 1);
			remaining_point;
			vis_func3[next_point] = true;
			point_now = next_point;
		}


		if (step_temp < step) {
			res.clear();
			res = res_temp;
			step = step_temp;
		}

		res_temp.clear();

		//memset(neighbor, NULL, sizeof(neighbor));
		//memset(cost, NULL, sizeof(cost));
		//memset(from_line, NULL, sizeof(from_line));
		memset(vis, false, sizeof(vis));
		memset(dis, INF, sizeof(dis));
		memset(pre, 0, sizeof(pre));
		memset(pre_line, 0, sizeof(pre_line));
		memset(vis_func3, false, sizeof(vis_func3));
	}


	return to_string(step) + "\n" + res;
	//return to_string(step) + "\n";
}




int main(int argc, char* argv[])
{

	consider_transfer_cost = 0;

	//���Թ�������
	argc = 4;
	argv[1] = const_cast<char*>("/a");
	argv[2] = const_cast<char*>("�����ѧ�Ǳ�");//���
	argv[3] = const_cast<char*>("����վ");//Ŀ�ĵ�



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

