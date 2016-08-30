// sortTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdio>
#include <string>
#include <iostream>
using namespace std;

class Team {
	public:
		string name;
		int goals;
		int wins;
};

bool compare(Team t1, Team t2) {
	if (t1.goals < t2.goals)
		return true;
	else
		return false;
}




Team* merge_sort(Team* arr, Team* tmp, int left, int right) {
	if (left >= right)
		return arr+left; 
	else {
		int mid = (int)((left+right)/2);
		Team* leftArr = merge_sort(arr, tmp, left, mid);
		Team* rightArr = merge_sort(arr, tmp, mid+1, right);

		int i = left;
		int j = mid;
		for (int k = 0; k < right-left; k++) {
			if ((i <= mid) && (j <= right)) {
				if (compare(arr[i],arr[j])) {
					tmp[k] = arr[i];
					i++;
				}
				else {
					tmp[k] = arr[j];
					j++;
				}
			}
			if (i>mid)
				tmp[k] = arr[j++];
			if (j>right)
				tmp[k] = arr[i++];
		}
		for (int k = 0; k < right-left; k++)
			arr[k+left] = tmp[k];
		return arr+left;
	}
}


int main()
{
	int n = 3;
	Team* tms = new Team[n];
	Team* buf = new Team[n];
	tms[0].name = "alpha";
	tms[0].goals = 4;
	tms[0].wins = 2;
	tms[1].name = "beta";
	tms[1].goals = 3;
	tms[1].wins = 0;
	tms[2].name = "gamma";
	tms[2].goals = 4;
	tms[2].wins = 3;
	merge_sort(tms, buf, 0, n);
	for (int i=0; i<n; i++)
		cout << tms[i].name << endl;
	cout << endl;
	return 0;
}

