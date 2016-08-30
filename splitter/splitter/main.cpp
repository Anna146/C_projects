#include<dict/misspell/libs/splitters>
#include<fstream>
using namespace std;

int main() {

	ifstream f("input.txt", ios::in);
	ofstream f2("output.txt", ios::out);

	while (!f.eof) {
		char* buff1 = new char[100];
		f.getline(buff1, 100,'\n');
	}

	return 0;
}