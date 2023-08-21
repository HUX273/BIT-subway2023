#include "Subway.h"

Subway::Subway(int change_line_cost) {
	ifstream fin;
	fin.open("subway2.txt", ios::in);//此处修改输入文件
	string line_name;
	while (getline(fin, line_name)) {
		// 显示读入的路线名
		// cout << "line name : " << line_name[line_count - 1] << endl;
		bool line_first_appear = this->line_id.count(line_name) == 0;
		int line_id;
		if (line_first_appear) {
			line_id = this->lines.size() + 1;
			this->lines.push_back(Line(line_name, line_id));
			this->line_id[line_name] = line_id;
		}
		else {
			line_id = this->line_name_to_id(line_name);
		}
		string point_name;
		getline(fin, point_name);
		istringstream in(point_name);
		int last_point_id = 0;
		while (getline(in, point_name, ' ')) {
			int point_id;
			// 如果这个车站是第一次出现，为它分配一个标号
			if (this->point_id.count(point_name) == 0) {
				point_id = this->points.size() + 1;
				this->points.push_back(Point(point_name, point_id));
				this->point_id[point_name] = point_id;
			}
			//更新车站和路线的从属关系
			this->lines[line_id].add_point(point_id);
			this->points[point_id].add_line(line_id);

			// 连边
			// 如果换乘有额外距离
			if (change_line_cost) {
				// 升级为换乘站，增加入点、出点
				if (this->points[point_id].belong_lines.size() == 2) {
					
				}
				// 如果是换乘站，且是一条新的线，则增加一个点
				if (this->points[point_id].belong_lines.size() > 1) {
					
				}
			}
			else {

			}

			// 显示读入的站名
			// cout << "|" << i << tmp << "|";
		}
	}
	fin.close();
}

inline int Subway::line_name_to_id(string line_name) {
	return this->line_id[line_name];
}