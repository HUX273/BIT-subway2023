#pragma once
#include "head.h"

class Subway
{
public:
	// ���캯�����������ļ�����
	Subway(int change_line_cost);
	// ����s��t�ľ���;���վ��(����t�������ǵ�һ��Ԫ��)
	vector<int> route(string src_name, string tar_name);
	

private:
	struct Line;
	struct Point;
	
	vector<Line> lines;
	vector<Point> points;
	unordered_map<string, int> point_id;
	unordered_map<string, int> line_id;
	inline int point_name_to_id(string point_name);
	inline string point_id_to_name(int point_id);
	inline int line_name_to_id(string line_name);
	inline string line_id_to_name(int line_id);

	struct Point {
		string name;
		int id;
		bool have_run;
		int dis[MAX_POINT];
		int pre[MAX_POINT];
		set<int> belong_lines;
		set<int> contain_points; // �𿪺�ĵ�
		Point(string name, int id) : name(name), id(id), have_run(false) {
			memset(dis, 0, sizeof(dis));
			memset(pre, 0, sizeof(pre));
		}
		bool operator < (const Point &r) const { return this->id < r.id; }
		inline void add_line(int lid) { this->belong_lines.insert(lid); }
	};

	struct Line {
		string name;
		int id;
		set<int> contain_points;
		Line(string name, int id) : name(name), id(id) {}
		bool operator < (const Line &r) const { return this->id < r.id; }
		inline void add_point(int pid) { this->contain_points.insert(pid); }
	};
};
