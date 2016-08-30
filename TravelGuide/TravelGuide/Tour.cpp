#include <cstdio>
#include <fstream>
#include <vector>
#include <iostream>
using namespace std;


//in the tests the following cases are covered:
//1) when there is only 1 town
//2) the route with more passangers flow is longer than the shortest possible
//3) when there is only 1 road
//4) when only 1 trip is needed
//5) when source is not the first town



//a structure which represents the edge of the graph - two vertexes and the number of passengers it can hold
struct edge {
	int v1, v2, w;
	edge(int a, int b,int c): v1(a), v2(b), w(c) {};
	edge() {};
};

//print the matrix
void show(int** matr, int s1, int s2) {
	for (int** it=matr, i=0; i<s1; it++, i++) {
		for (int j=0; j<s2; j++) {
			cout << matr[i][j] << " ";
		}
		cout << endl;
	}
}

int main() {
	//the files we read and write to
	ifstream f("input.txt", ios::in);
	ofstream f2("output.txt", ios::out);
	//как учитывать что мой граф - ненаправленный?

	//number of scenarios
	int N = 0;

	int cityCount = 0; //number of towns
	int roads = 0; //number of edges (roads)
	f >> cityCount >> roads;
	f2 << "Tigunova Anna" << endl;

	while (cityCount != 0 || roads !=0) { //check whether we reached the end of input 
		N++;
		//check if there's only one town
		if (cityCount <= 1) {
			f2 << "Scenario #" << N << endl;
			f2 << "Minimum Number of Trips = " << 0 << endl;
			f2 << "Route = -" << endl << endl;
			int a;
			f >> a >> a >> a;
			int b = 12;
		}
		else {
			int** lengths = new int*[cityCount]; //matrix for passanger count that we use to store the data initially
			int** paths = new int*[cityCount]; //the matrix that we use for the algorithm
			vector<edge> edges; //a vector of edges
			int** precedors = new int*[cityCount]; //a mathix of previous vertexes which will determine a path to the destination

			//initializing the array of passangers with positive infinity (if there is no road)
			for (int** it=lengths, i=0; i<cityCount; it++, i++) {
				(*it) = new int[cityCount];
				for (int j=0; j<cityCount; j++) {
					lengths[i][j] = INT_MAX;
				}
			}

			//initializing supporting matrixes
			for (int **it2=paths, **it3=precedors, i=0; i<=roads; i++, it2++, it3++) {
				(*it2) = new int[roads];
				(*it3) = new int[roads];
				for (int j=0; j<cityCount; j++) {
					paths[i][j] = INT_MIN; //every path is still not determined
					precedors[i][j] = -1;
				}
			}

			//scanning input file for the values of edges and writing them into the initial matrix and edges array
			for (int i=0; i<roads; i++) {
				int s,d,t;
				f >> s >> d >> t;
				s--; d--; t--;
				lengths[s][d] = lengths[d][s] = t;
				edges.push_back(edge(s,d,t));
				edges.push_back(edge(d,s,t));
			}

			int source, destin, tourists;

			f >> source >> destin >> tourists;
			source--; destin--;

			//initialize the route to the source vertex 
			paths[0][source] = INT_MAX;
	
		
			/*I am going to use the analog for the ford-bellman algorithm with the exeption that in the supplementary matrix
			the value of the smallest capacity of the current route to that vertex is
			One index of this table will be the number of vertex, the other - number of edges which are contained in the current route
			on each step of the algorithm we take the next vertex and iteratively try to add all the edges so that the stream of passangers through this vertex is maximized
			First we determine if the capacity of the route through the opposite vertex should change if we add a new edge
			Second we look if the new route is better than the existing one through the vertex
			*/
	

			
			//show(lengths, cityCount, cityCount);
			
			//cout << endl;

			//ford-bellman, modified, so that in the table not minimum distance but the edge with the smallest value is stored
			for (int i=1; i<cityCount; i++) {//for each edge count
				for (int j=0; j<edges.size(); j++) {//try to add one more edge
					if (paths[i][edges[j].v1] < INT_MAX) 
					{
						int a = (edges[j].w > paths[i-1][edges[j].v1]) ? paths[i-1][edges[j].v1] : edges[j].w; //if adding the new edge will make the capacity less
						if (paths[i-1][edges[j].v2] < a) { //if the new route is better
							//first check if there is a loop
							bool loop = false;
							for (int k=0; k<i; k++)
								if (precedors[k][edges[j].v2] == edges[j].v1)
									loop = true;
							//if there is no loop or return to the source
							if ((!loop) && (i != source)) {
								paths[i][edges[j].v2] = a;
								precedors[i][edges[j].v2] = edges[j].v1;
							}
						}
					}
					//if we should retain the value we got on the previous iteration
					for (int k=0; k<cityCount; k++)
						if (paths[i][k] < paths[i-1][k] && paths[i-1][k] != INT_MAX && paths[i-1][k] != INT_MIN)
							paths[i][k] = paths[i-1][k];
				}
			}

			
			show(precedors, roads, cityCount);
			cout << endl;

			
			//restoring the shortest path
			int j=roads-1; //sarting from all the possible routes
			int* shortest = new int[roads]; //array for precedors of the destination vertex
			int k = destin; //here we look at the column with the destination vertex
			int cnt = 0;
			int minEdge = paths[j][destin]; //the capacity of the route
			while (j>0) {
				shortest[cnt] = k+1;
				cnt++;
				while (precedors[j][k] == -1 && j>0) {//if adding further edges is pointless
					j--;
					minEdge = paths[j][destin]; //because undefined distances can only be lower in the table than the last minimum edge
				}
				if (j>0)
					k = precedors[j][k];//stepping back
				if (paths[j][k] < minEdge && paths[j][k] > 0)
					minEdge = paths[j][k];
				j--;
			}


			//output
			int numTrips = (tourists % minEdge != 0) ? tourists / minEdge + 1 : tourists / minEdge; //counting the number of trips
			f2 << "Scenario #" << N << endl;
			f2 << "Minimum Number of Trips = " << numTrips << endl;
			f2 << "Route = " << source+1 << " ";
			for (int i=cnt - 1; i>0; i--)
				f2 << shortest[i] << " ";
			f2 << destin + 1 << endl << endl;

			delete(shortest);
			
		}
		f >> cityCount >> roads;

	}

	return 0;
}