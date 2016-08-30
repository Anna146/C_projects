/* 

   travellingSalesman.h - interface file for the solution of the travelling saleman problem by exhaustive search using backtracking

   CS-CO-412 Algorithms and Data Structures Assignment No. 4

   The assignment is defined as follows.
   
   Dmitry wants to buy something in all his favourite shops in MegaMegaMall but he is very tired and wants to walk as little as possible.  
   Given the distance between each shop, the task is to produce an optimal route for Dmitry (i.e. the sequence in which he should visit the shops).  
   Dmitry always parks in the same place so we also know the distance from his car to each shop.  

   Input data for several test cases is provided in an input file input.txt
   Output for each test case is written to an output file output.txt


   Input
   -----

   - The first line of the input file will consist of an integer number k specifying the number of test cases in the file.  
   - This is followed by the data specifying each test case, beginning on the next line.  
   - There are no blank lines in the input file.
   - All lines contain 80 characters or less.
   - Each test case will begin with a line containing an integer number n giving the number of shops to be visited. 
   - This will be followed by n+1 lines containing 
   
     -- the n names of each shop 
     -- followed the name of the manufacturer of Dmitry’s car
     -- It is assumed that n < 10 and that each name is on a separate line

   - This is followed by n+1 lines each containing n+1 integers specifying the distances between each shop (including itself).  
   - The jth number on line i gives the distance from shop i to shop j. 
   - The j+1st number on line i gives the distance from shop i to the car.  
   - Note that the distance from shop i to shop j may not be the same as the distance from shop j to shop i 
     since you may not be able to take the same path.
   - Line n+1 gives the distance of the car to each shop

   Output
   ------

   - The first line of the output file  contains the name of the author of the software.
   - For each test case in the input
   
     -- The test case number is printed on a separate line
     -- The total distance that needs to be walked is printed on the next line
     -- This is followed the sequence of shops that Dmitry should use to minimize his walking, 
        starting at his car and returning to his car.   
        Each shop name and car name is printed on a separate line
     
   - If there is more than one tour that gives the same minimum distance, the first one encounted is the one selected.
 
   Sample Input
   ------------

   1
   4
   Top Bike Shop
   Cars r Us
   Everything Electronic
   The Hot Chocolate Emporium
   Morgan 
   0   30  100  50 120 
   30   0  100  80  60 
   100 110   0  30  60 
   50   80  40   0  60 
   120  60  90  70   0 

   Sample Output
   -------------

   David Vernon
   1
   240
   Morgan 
   Cars r Us
   Top Bike Shop
   The Hot Chocolate Emporium
   Everything Electronic 
   Morgan 

   Solution Strategy
   -----------------

   The solution uses combinatorial search with backtracking so we seek the permutation of the shops that provided the minimum length tour 
   Instead of enumerating permutations of the shop names we associate an integer key with each shop name and form permutations of these keys.


   David Vernon
   17 November 2014
*/
 
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <limits.h>
#define TRUE 1
#define FALSE 0
#define STRING_SIZE 80
#define NUMBER_OF_STOPS 11


/* data structure to represent a shop or car */

struct record_type {
   int  key;                  // an integer that will be used in the permutation instead of the string
   char string[STRING_SIZE];  // the string representing the shop name or car name
};


/* function prototypes */
/* ------------------- */

/* utility function to remove a newline character that may have been included when parsing the input data */

void remove_new_line(char string[]);

