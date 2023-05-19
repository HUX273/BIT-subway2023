#include <iostream>
#include <cstring>
#include <cstdio>
#include<string>
using namespace std;
#define INF 0x7ffffff

const int N = 10; //��ĸ�������

int maze[N][N];	//maze[a][b]��ʾab�������
int dis[N];		//dis[a]��ʾ��㵽a��ľ���
bool vis[N];	//���
string route[N];//��¼��㵽����������·��

//��ĸ����ͱߵ�����
int n, m;



void init()
{
	memset(maze, INF, sizeof(maze));
	memset(dis, INF, sizeof(dis));
	memset(vis, false, sizeof(vis));
	for(int i = 1; i <= N; i++)
	{
		route[i] = to_string(i);
	}
}

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
				route[min_dis_point] = route[i] + " " + to_string(min_dis_point);
			}
		}
		//�����
		vis[min_dis_point] = true;

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
}

void input_nm()
{
	while (scanf_s("%d %d", &n, &m) != EOF)
	{
		if (n == 0 && m == 0) break;
		//ÿ�����ݶ�Ҫ��ʼ��
		init();
		for (int i = 1; i <= m; i++)
		{
			int x, y, len;
			scanf_s("%d %d %d", &x, &y, &len);
			if (x != y && maze[x][y] > len)
			{
				maze[y][x] = len;
				maze[x][y] = len;
			}
		}
	}
}


int main()
{
	//���� n m,��ʼ��������룬�����ĵ����Ϊ1��δ�����ĵ����ΪINF,
	input_nm();

	//��1Ϊ�����һ��dij
	dijkstra(1);

	//�����n�ľ���
	printf("%d\n", dis[n]);
	//cout << route[n] << endl;
	
	return 0;
	
}

