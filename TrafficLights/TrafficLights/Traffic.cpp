#include <cstdio>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <map>
#include <string>
using namespace std;

//Created by Anna Tigunova


//in my tests the following cases are handled:
/*
1 case-insensitive key-value pairs
2 key-value pairs in random order
3 the period is longer than a minute
4 if many cars leave during one period
5 if it takes some periods for a car to leave
6 if red light shine all the time
7 if each light shines longer than one period
8 the cars entering the queue at the begining are the only ones (considering the remains of the queue)
9 red doesn't shine at all
10 many spaces in the input
*/

static bool first_call = true; //for randomizer to give each time different values

   
//the function to give the amount of cars that come every minute using the poisson distribution
int samplePoisson(double lambda) {
/* Generate a random sample from a Poisson distribution */
/* with a given mean, lambda */
/* Use the function rand to generate a random number */
	/* Seed the random-number generator with current time so */
   /* that the numbers will be different every time we run */

   if (first_call) {
      srand( (unsigned)time( NULL ));
      first_call = false;
   }
	int count;
	double product;
	double zero_probability;
	/* Seed the random-number generator with current time so */
	/* that the numbers will be different every time we run */
	srand( (unsigned)time( NULL ) );
	count = 0;
	product = (double) rand() / (double) RAND_MAX;
	zero_probability = exp(-lambda);
	while (product > zero_probability) {
		count++;
		product = product * ((double) rand() / (double) RAND_MAX);
	}
	return count;
};

//a class which model a car in the queue
class node {
	public:
		node* next;
		//stores the time the car entered the queue
		int arrTime;

		//a constructor for node
		node(int time): arrTime(time) {}
};


//a class for the queue of cars
class queue {
public:
	node* head;
	node* tail;
	int length;

//puts the 'count' amount of cars to the queue with the same time that they actually arrived 
	void enqueue(int count, int time) {
		for (int i=0; i<count; i++) {
			if (length == 0) {
			//if the queue is empty we make a new head
				node* newNode = new node(time);
				head = tail = newNode;
				int a = 6;
			}
			else {
				// putting new value to the tail
				node* newNode = new node(time);
				tail->next = newNode;
				tail = newNode;
			}
			length++;
		}
	}


	//removes one car from the head of the queue, returns the difference between the time the car
	//arrived and the depature time specified by 'time' parametr
	int dequeue(int time) {
		int timeDiff = time - head->arrTime;
		//make sure the queue is not empty
			if (length > 1) {
				head = head->next;
				length--;
			}
			else {
				//if there is only 1 car in the queue we empty it
				if (length == 1) {
					head = NULL;
					tail = NULL;
					length--;
				}
			}
		return timeDiff;
	}

	queue(): length(0) {};
};

//a function to make all symbols in the sting lowcase so as to make reading case-insensitive
void toLower(char* str) {
	while ((*str) != '\0') {
		(*str) = tolower(*str);
		str++;
	}
	return;
}




int main() {
	//the files we read and write to
	ifstream f("input.txt", ios::in);
	ofstream f2("output.txt", ios::out);

	//number of experiments
	int N = 0;
	f >> N;
	char* buff = new char[1000];
	char* buff2 = new char[1000];

	//output name
	f2 << "Tigunova Anna" << endl;


	for (int i=0; i<N; i++) {
		int features[6]; //array of input values for each simulation
		
		//where I store pairs key-value for features
		map <string, int, less<string>> inpVals;

		//input values for each simulation
		for (int j=0; j<6; j++) {
			f >> buff; //read key
			f >> buff2; //read number
			toLower(buff); //making case-insensitive
			inpVals[buff] = atoi(buff2);
		}



		//important values
		long periods = inpVals["runtime"] * 60 * 1000 / inpVals["increment"]; //the number of periods during the runtime determined by increment value
		int perPerMinute = 60 * 1000 / inpVals["increment"]; //the number of periods in a minute
		int currLight = 0; //0 - for red, 1 - for green
		int countLight = 0; //how long the current light shone
		int depPer = inpVals["departure"] * 1000 / inpVals["increment"]; //how many periods it takes the car to leave the queue
		int depCarsNum = inpVals["increment"] / inpVals["departure"] / 1000; //how many cars leave in one period
		if (depPer == 0) //if the car leaves sooner than the period finishes
			depPer = 1;
		if (depCarsNum == 0) //if a car leaves in more than 1 period
			depCarsNum = 1;
		int depCount = 0; //current number of periods since the last car departed
		int redInc = inpVals["red"] * 1000 / inpVals["increment"]; //number of periods red light goes
		redInc =  redInc > 0 ? redInc : 1; //if the time the light goes is less than a period we assume that it goes for 1 period
		int greenInc = inpVals["green"] * 1000 / inpVals["increment"]; //number of periods green light goes
		greenInc =  greenInc > 0 ? greenInc : 1; //if the time the light goes is less than a period we assume that it goes for 1 period

		int maxL = 0; //max length
		int avgL = 0; //average length
		int maxT = 0; // max time
		int avgT = 0; //average time
		int carCount = 0; //all the cars that have entered the queue
		int allTime = 0; //the sum of the time that all cars spent in the queue
		int totLen = 0; //the sum of lengths of the queue for all the periods
		
		int currDiff = 0; //time difference for currently dequeued car

		long k = 0; //periods iterator
		queue q;

		//at the begining some cars enter the queue
		if (perPerMinute == 0) {
			int cars = samplePoisson(inpVals["arrival"]);
			q.enqueue(cars, k);
			carCount += cars;
		}

		while (k < periods) {

			//if a minute passed since last arrival a new portion of cars arrive
			if (perPerMinute !=0 && k % perPerMinute == 0) {
				int cars = samplePoisson(inpVals["arrival"]);
				q.enqueue(cars, k);
				carCount += cars;
			}

			//deals with avg and max length
			totLen += q.length;
			if (q.length > maxL)
				maxL = q.length;
			//


			// light check: what light is currently on
			if (currLight == 0) //if red
			{
				//increase the time the current light shines
				if (countLight < redInc) { //if it is still supposed to be shining
					countLight++;
				}
				else { //if it's time the light swithced
					countLight = 0; //null the light timer
					currLight = 1; //change the light
					//so the green light turns on and cars leave the queue
					for (int j=0; j<depCarsNum; j++) {
						if (q.length != 0) {
							currDiff = q.dequeue(k); //the first cars depart
							allTime += currDiff;
							if (currDiff > maxT)
								maxT = currDiff;
						}
					}
					depCount = -1;
				}
			}

			//if green light is on
			if (currLight == 1)
			{
				if (countLight < greenInc) {//if it still should shine
					countLight++;
					//cars leave the queue
					if (depCount >= depPer){//we want to determine wether the previous car still leaves
						for (int j=0; j<depCarsNum; j++) {//the next portion of cars leave (or one car)
							if (q.length != 0) {
								currDiff = q.dequeue(k); //dequeue the car
								allTime += currDiff; //increase the whole time
								if (currDiff > maxT)
									maxT = currDiff;
							}
						}
						depCount = 0;//the car has just left
					}
					depCount++; //increase the time since the car left queue counter
				}
				else {
					//if we now should switch the light
					countLight = 0;
					currLight = 0;
					depCount = 0;
					countLight++; //as during the red light cars don't leave the queue
				}
			}
			k++;
		}

		//regarding the remains of the queue
		if (q.length != 0) {
			for (int k=0; k<q.length; k++) {
				currDiff = q.dequeue(periods);
				allTime += currDiff;
				if (currDiff > maxT)
					maxT = currDiff;
			}
		}
		//


		//output
		f2 << "Arrival rate:   " << inpVals["arrival"]  << " cars per minute" << endl;
		f2 << "Departure:      " << inpVals["departure"]  << " seconds per car" << endl;
		f2 << "Runime:         " << inpVals["runtime"] << " minutes" << endl;
		f2 << "Time increment: " << inpVals["increment"] << " milliseconds" << endl;
		f2 << "Light sequence: Red " << inpVals["red"] << " seconds; Green " << inpVals["green"] << " seconds" <<  endl;
		if (totLen/periods != 0)
			f2 << "Average length: " << totLen/periods << " cars" << endl;
		else
			f2 << "Average length: " << maxL/2 << " cars" << endl;
		f2 << "Maximum length: " << maxL << " cars" << endl;

		if (allTime/carCount == maxT) { //if all the cars arrived and departed simultaneously
			f2 << "Average wait:   " << allTime*inpVals["increment"]/500/carCount << " seconds" << endl;
			f2 << "Maximum wait:   " << allTime*inpVals["increment"]/1000/carCount << " seconds" << endl;
		}
		else
		{
			f2 << "Average wait:   " << allTime*inpVals["increment"]/1000/carCount << " seconds" << endl;
			f2 << "Maximum wait:   " << maxT*inpVals["increment"]/1000 << " seconds" << endl;
		}
		f2 << endl;

		
	}
	return 0;
}