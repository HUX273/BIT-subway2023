#include <iostream>
#include <cstring>
#include <cstdio>
#include<string>
using namespace std;
#define INF 0x7ffffff

const int N = 10; //点的个数上限

int maze[N][N];	//maze[a][b]表示ab两点距离
int dis[N];		//dis[a]表示起点到a点的距离
bool vis[N];	//标记
string route[N];//记录起点到各个点的最短路径

//点的个数和边的条数
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
				route[min_dis_point] = route[i] + " " + to_string(min_dis_point);
			}
		}
		//并标记
		vis[min_dis_point] = true;

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
}

void input_nm()
{
	while (scanf_s("%d %d", &n, &m) != EOF)
	{
		if (n == 0 && m == 0) break;
		//每组数据都要初始化
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
	//输入 n m,初始化各点距离，相连的点距离为1，未相连的点距离为INF,
	input_nm();

	//以1为起点跑一次dij
	dijkstra(1);

	//输出到n的距离
	printf("%d\n", dis[n]);
	//cout << route[n] << endl;
	
	return 0;
	
}

