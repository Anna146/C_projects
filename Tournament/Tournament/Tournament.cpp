// Tournament.cpp : Defines the entry point for the console application.
//Author: Tigunova Anna

#include "stdafx.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <algorithm>
using namespace std;


//this class will provide properties of the team
class Team
{
	private:
		string teamName;
		int teamPoints;
		int teamWins;
		int teamDraws;
		int teamLoses;
		int teamGoalDiff;
		int teamScored;
		int teamMissed;
		int teamGameAm;

	public:
		Team(): teamPoints(0), teamWins(0), teamDraws(0), teamLoses(0), teamGoalDiff(0), teamScored(0), teamMissed(0), teamGameAm(0){};
		

		//getter methods
		string name() {
			return teamName;
		}
		int points() {
			return teamPoints;
		}
		int wins() {
			return teamWins;
		}
		int draws() {
			return teamDraws;
		}
		int loses() {
			return teamLoses;
		}
		int goalDiff() {
			return teamGoalDiff;
		}
		int scored() {
			return teamScored;
		}
		int missed() {
			return teamMissed;
		}
		int gameAm() {
			return teamGameAm;
		}



		//setter methods
		void name (string val) {
			teamName = val;
		}
		void points(int val) {
			teamPoints = val;
		}
		void wins(int val) {
			teamWins = val;
		}
		void draws(int val) {
			teamDraws = val;
		}
		void loses(int val) {
			teamLoses = val;
		}
		void goalDiff(int val) {
			teamGoalDiff = val;
		}
		void scored(int val) {
			teamScored = val;
		}
		void missed(int val) {
			teamMissed = val;
		}
		void gameAm(int val) {
			teamGameAm = val;
		}
};


//this function parses the input string in which the results of the single match are recorded
string* game(string s) {
	int curr1 = s.find('#');
	int curr2 = s.find('@');
	int curr3 = s.find_last_of('#');
	string* results = new string[4];
	results[0] = s.substr(0, curr1);
	results[1] = s.substr(curr1+1, curr2 - curr1-1);
	results[2] = s.substr(curr2+1, curr3 - curr2-1);
	results[3] = s.substr(curr3+1, s.size() - curr3-1);
	return results;
}



//this function transforms the string to lowercase to make case-insensitive sort of team names 
//test 4 to check whether this sort is really case-insensitive
string caseCheck(string s1) {
	for (int i=0; i<s1.length(); i++)
		if (!islower(s1[i]))
			s1[i] = tolower(s1[i]);
	return s1;
}



//a block of funtions which compare the teams by different characteristics
//test 3 to check if sorting by all these values works
bool byName(Team t1, Team t2) {
	if (strcmp(caseCheck(t1.name()).c_str(), caseCheck(t2.name()).c_str())>=0) return true;
	else return false;
}

bool byPoints(Team t1, Team t2) {
	return t1.points() >= t2.points();
}

bool byWins(Team t1, Team t2) {
	return t1.wins() >= t2.wins();
}

bool byGoalDiff(Team t1, Team t2) {
	return t1.goalDiff() >= t2.goalDiff();
}

bool byScored(Team t1, Team t2) {
	return t1.scored() >= t2.scored();
}

bool byGames(Team t1, Team t2) {
	return t1.gameAm() <= t2.gameAm();
}




//merge sorting function for the team class. This sort is known to be stable. Gets the pointer to the necessary compare function
Team* merge_sort(Team* arr, Team* tmp, int left, int right, bool (*compare)(Team, Team)) {
	if (left >= right)
		return arr+left; 
	else {
		int mid = (int)((left+right)/2);
		Team* leftArr = merge_sort(arr, tmp, left, mid, compare);
		Team* rightArr = merge_sort(arr, tmp, mid+1, right, compare);

		int i = left;
		int j = mid+1;
		for (int k = 0; k < right-left+1; k++) {
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
			else {
				if (i>mid)
					tmp[k] = arr[j++];
				else
					tmp[k] = arr[i++];
			}
		}
		for (int k = 0; k < right-left+1; k++)
			arr[k+left] = tmp[k];
		return arr+left;
	}
}


//functor for comparing to strings (because there's no < in a string class) so as to make map work with a string class as a key
struct my_strcmp {
	bool operator()(const string& str1, const string& str2) {
		return (strcmp(str1.c_str(), str2.c_str()) > 0) ? true : false;
	}
};



int main()
{
	ifstream f("input.txt", ios::in);
	ofstream f2("output.txt", ios::out);
	int N = 0;
	f >> N;
	char* buff = new char[1000];
	f.getline(buff, 1,'\n');
	for (int i=0; i<N; i++) {
		//inputing the name of the tournament
		f.getline(buff, 100,'\n');
		string tourn(buff);
		int K = 0;
		//creating map to quickly find values when we read the results of the matches
		map<string, Team, my_strcmp> teams;
		f >> K;
		f.getline(buff, 1,'\n');
		for (int j=0; j<K; j++)
		{
			f.getline(buff, 100,'\n');
			string name(buff);
			teams[name];
		}

		int G=0;
		f >> G;
		f.getline(buff, 1,'\n');

		//parcing the input values of the matches
		for (int j=0; j<G; j++)
		{
			f.getline(buff, 100,'\n');
			//parsing the string with the results of the match
			string* results = game(string(buff));
			int sc1 = atoi((*(results+1)).c_str());
			int sc2 = atoi((*(results+2)).c_str());
			teams[results[0]].scored(teams[results[0]].scored() + sc1);
			teams[results[0]].goalDiff(teams[results[0]].goalDiff() + sc1 - sc2);
			teams[results[0]].gameAm(teams[results[0]].gameAm() + 1);
			teams[results[0]].missed(teams[results[0]].missed() + sc2);
			teams[results[0]].name(*results);
			if (sc1 > sc2) {
					teams[results[0]].wins(teams[results[0]].wins()+1);
					teams[results[0]].points(teams[results[0]].points() + 3);
					teams[results[3]].loses(teams[results[3]].loses()+1);
				}
			else
				{
				if (sc2 > sc1) {
					teams[results[3]].wins(teams[results[3]].wins()+1);
					teams[results[0]].loses(teams[results[0]].loses()+1);
					teams[results[3]].points(teams[results[3]].points() + 3);
					}
				else {
					teams[results[0]].draws(teams[results[0]].draws()+1);
					teams[results[3]].draws(teams[results[3]].draws()+1);
					teams[results[0]].points(teams[results[0]].points()+1);
					teams[results[3]].points(teams[results[3]].points()+1);
					}
				}
			teams[results[3]].scored(teams[results[3]].scored() + sc2);
			teams[results[3]].goalDiff(teams[results[3]].goalDiff() + sc2 - sc1);
			teams[results[3]].gameAm(teams[results[3]].gameAm()+1);
			teams[results[3]].missed(teams[results[3]].missed() + sc1);
			teams[results[3]].name(*(results+3));
		}

		Team* arr = new Team[teams.size()];
		Team* tmp = new Team[teams.size()];
		int count=0;

		//putting the values from the map to the ordinary array of teams which will be easier to sort
		for (auto it = teams.begin(); it != teams.end(); it++, count++)
			arr[count] = (*it).second;

		//an array of compare functions
		bool (*comparators[])(Team, Team) = {
			byName, byGames, byScored, byGoalDiff, byWins, byPoints
		};

		//sorting the array with a stable sort function
		for (int j=0; j<6; j++)
			merge_sort(arr, tmp, 0, teams.size()-1, comparators[j]);

		//output
		f2 << tourn.c_str() << endl;
		for (int k=0; k<count; k++) {
			f2 << (k+1) << ") " << arr[k].name().c_str() << ' ' << arr[k].points() << "p, " << arr[k].gameAm() << "g (" << arr[k].wins() << '-' << arr[k].draws() << '-' << arr[k].loses() << "), " << arr[k].scored() - arr[k].missed() << "gd (" << arr[k].scored() << '-' << arr[k].missed() << ')' << endl;
		}
		f2 << endl; 
	}
	return 0;
}

