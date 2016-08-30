/* 
  Assignment No. 4:  TSP 
  --------------------------------------

  Anna Tigunova
  30 November 2014

*/

/*

Problem Definition
Dmitry wants to buy something in all his favourite shops in MegaMegaMall but he is very
tired and wants to walk as little as possible. Given the distance between each shop, your task
is to produce an optimal route for Dmitry (i.e. the sequence in which he should visit the
shops). Dmitry always parks in the same place so we also know the distance from his car to
each shop.

Input
The first line of the input file will consist of an integer number k specifying the number of test
cases in the file. This is followed by the data specifying each test case, beginning on the next
line. There are no blank lines in the input file.

Output
The first line of your output file should contain your name.
For each test case in the input, first output the test case number, then print the total distance
that needs to be walked, followed the sequence of shops that Dmitry should use to minimize
his walking, starting at his car and returning to his car.

*/

/*
The following cases are tested:
- the number of shops from 3 to 5
- the ways between two shops are different in opposite directions
- all the weights are the same
- there is only one shop
- the shops are visited in the reverse order from the given one
- the shops are visited in random order from the given one
- on the first step a more expensive edge is chosen but it will lead to more optimal route
*/



#include<cstdio>
#include<fstream>
#include<vector>
#include<string>
#include<iostream>
#include<list>
using namespace std;

//global variables necessary for recursion
int bestw = INT_MAX; //the best weight we have been able to fnd so far
int last = 0; //the number of shops in the current sequence
vector<int> bestseq; //the sequence of shops which form the complete solution that has the best weight so far 
list<int> c; //the list of candidates for the next position in the resulting sequence of shops (acts as a queue)

//function for testing the result to be a solution
//vector<int> a - is the a current sequence of shops (maybe not complete)
bool is_a_solution(vector<int>& a, int k, int shopCount, bool* visited) {
	bool sol = true;
	//mark all the visited shops in te boolean array
	for (int i=0; i<k; i++)
		visited[a[i]] = true;
	//check if the last one is a car (which will make it a solution, may be partial)
	if ((a[k] != shopCount - 1) || (k == 0))
		sol = false;
	return sol;
}

//function for checking if the current solution is complete 
//and if it is chech wether it is better than the current best solution
void process_solution(vector<int>& a, int k, int currw, int shopCount, bool* visited) {
	bool sol = true;
	//check the array of visited shops so that each shop has been visited
	for (int i=0; i<shopCount; i++) {
		if (visited[i] == false)
			sol = false;
	}
	//if the last is the car and all the shops have been visited
	if ((a[k] == shopCount-1) && (sol)) {
		//if current sequence has less weight
		if (currw < bestw)
		{
			bestw = currw;
			bestseq = a;
			last = k;
		}
	}
	return;
}





//a backtrack function to find a solution
//recursively tries to add the next shop to the current sequence
//if the current sequence is complete returns
//also returns if the best weight in all the descendants stopped improving
bool backtrack(vector<int>& a, int k, int** input, int currw, int shopCount, bool* visited) {
	//try to extend the solution until it stops improving 
	 if (currw >= bestw)
		 return true; //the weight stopped improving
	/* for (int i=0; i<shopCount; i++)
		 cout << visited[i] << ' ';*/
	 //cout << endl;
	 bool finished = true; //the variable that will determine whether the cirrent weight > best weight in all the descendants 
	 //clear the visited array
	 for (int i=0; i<shopCount; i++)
		 visited[i] = false;
	 //if the sequence ends in the car - process it
	 //if not - continue adding shops
	 if (is_a_solution(a,k,shopCount, visited)) {
		 process_solution(a,k, currw, shopCount, visited);
		 return true;
	 }
	 else {
		k = k+1; //increase the length of the current sequence
		a.push_back(0); //reserving the space in the sequence vector for the new shop
		int j=0;
		for (auto it=c.begin(); j<shopCount-1; j++) {
			//if the current candidate has not been present we add it to the sequence
			//if it has been present just increase the iterator and move to the next candidate
			 if (visited[(*it)] != true || ((*it) == shopCount-1)) {
				 a[k] = (*it); //adding the next shop
				 /*
				 for (int l=0; l<k; l++)
					 cout << ' ';
				 cout << a[k] << endl;
				 */
				 //int tmp = (*(c.begin()));
				 auto tmp = it; //temporary iterator to store the next position
				 tmp++;
				 c.erase(it); //removing currently added candidate from the queue
				 c.push_back(a[k]); //adding it to the end
				 visited[a[k]] = true; //marking that we have been in this shop
				 //recursively backtrack
				 //if true is returned it means that by adding this shop the current weight became bigger than the best
				 if (!backtrack(a,k,input, currw+input[a[k-1]][a[k]], shopCount, visited))
					 finished = false;

				 //return the current candidate from the back of the queue to its initial position
				 //to move to the next candidate
				 c.pop_back();
				 tmp = c.begin();
				 for (int l=0; l<j; l++)
					 tmp++;
				 c.insert(tmp,a[k]);
				 //

				 visited[a[k]] = false; //remove the mark
				 it=tmp;
			 }
			 else
				 it++;
		}
		//return whether this path should not be explored further 
		return finished;
	}
}


int main() {

	//input and output files
	ifstream f("input.txt", ios::in);
	ofstream f2("output.txt", ios::out);
	
	//temporary char* for reading
	char* buff = new char[1000];
	int N; //number of tests

	//read the number of tests
	f >> N;

	//output the name
	f2 << "Tigunova Anna" << endl;


	for (int i=0; i<N; i++) {
		f2 << i+1 << endl; //output the number of test

		/* input section */
		int k;
		f >> k; //input number of shops
		k++; //increase so that it considers the car
		vector<string> shops; //stores the names of the shops
		f.getline(buff, 1,'\n');
		//reading shops' names (and car's)
		for (int j=0; j<k; j++)
		{
			f.getline(buff, 100,'\n');
			string name(buff);
			shops.push_back(name);
		}
		//matrix of distances between shops
		int** dist = new int*[k];
		//initialising  matrix
		for (int** it=dist, j=0; j<k; it++, j++) {
			(*it) = new int[k];
		}
		//reading the matrix of distances
		for (int** it=dist, j=0; j<k; it++, j++) {
			char* buff1 = new char[100];
			//(*it) = new int[k];
			f.getline(buff1, 100,'\n');
			string tmp(buff1);
			int poss=0, pose = 0;
			//parsing the string which contains the distance and maybe whitespaces which shold be removed
			for (int l=0; l<k; l++) {
				poss = pose;
				while (buff1[poss] == ' '){
					poss++;
				}
				pose = poss;
				while (buff1[pose] != ' ') 
					pose++;
				dist[j][l] = atoi(tmp.substr(poss, pose - poss).c_str());
			}
			delete[] buff1;
		}
		//end of input
		
		vector<int> a; //a vector to store the current sequence of shops visited so far
		a.push_back(k-1); //the first one is the car of course
		bool* visited = new bool[k]; //an array to check the visited shops
		//initialising the queue for the candidates for the next position
		//the car should be the last
		c.push_front(k-1);
		for (int j=0; j<k-1; j++) {
			visited[j] = false;
			c.push_front(j);
		}
		visited[k-1] = true;

		//computing the backtrack algorithm
		backtrack(a, 0, dist, 0, k, visited);

		/* output section */
		f2 << bestw << endl; //the best weight
		//output the best path
		for (int j=0; j<last+1; j++)
			f2 << shops[bestseq[j]] << endl; 
		f2 << endl;

		//restorint the global variables
		bestw = INT_MAX;
		last = 0;
		bestseq.clear();
		c.clear();
	}
	
	return 0;
}