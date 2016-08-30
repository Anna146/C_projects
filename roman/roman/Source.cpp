#include <iostream>
#include <string>
using namespace std;

int main() {
    string line;
    while (getline(cin, line)) {
        int n = atoi(&line[0]);
        int currsum = 0;
        int sum = (n-2)*(n-1)/2;
        for (int j=2; j< line.length(); j+=2)
        {
            currsum += atoi(&line[j]);
        }
        cout << currsum - sum << endl;
    }
}
