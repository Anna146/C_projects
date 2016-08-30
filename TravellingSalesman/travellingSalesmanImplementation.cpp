/* 

  travellingSalesmanImplementation.cpp - application file for the solution of the travelling saleman problem by exhaustive search using backtracking

  CS-CO-412 Algorithms and Data Structures Assignment No. 4

  See travellingSalesman.h for details of the problem

  David Vernon
  17 November 2014

*/
 
#include "travellingSalesman.h"

void remove_new_line(char string[]) {
   int i;

   i=0;
   while(string[i] != '\0' && string[i] != '\n')
      i++;
   string[i] = '\0';
}


