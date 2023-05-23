#include "Subway.h"

Subway::Subway(int change_line_cost) {
	ifstream fin;
	fin.open("subway2.txt", ios::in);//�˴��޸������ļ�
	string line_name;
	while (getline(fin, line_name)) {
		// ��ʾ�����·����
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
			// ��������վ�ǵ�һ�γ��֣�Ϊ������һ�����
			if (this->point_id.count(point_name) == 0) {
				point_id = this->points.size() + 1;
				this->points.push_back(Point(point_name, point_id));
				this->point_id[point_name] = point_id;
			}
			//���³�վ��·�ߵĴ�����ϵ
			this->lines[line_id].add_point(point_id);
			this->points[point_id].add_line(line_id);

			// ����
			// ��������ж������
			if (change_line_cost) {
				// ����Ϊ����վ��������㡢����
				if (this->points[point_id].belong_lines.size() == 2) {
					
				}
				// ����ǻ���վ������һ���µ��ߣ�������һ����
				if (this->points[point_id].belong_lines.size() > 1) {
					
				}
			}
			else {

			}

			// ��ʾ�����վ��
			// cout << "|" << i << tmp << "|";
		}
	}
	fin.close();
}

inline int Subway::line_name_to_id(string line_name) {
	return this->line_id[line_name];
}