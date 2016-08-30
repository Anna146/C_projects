/* 

  travellingSalesmanApplication.cpp - application file for the solution of the travelling saleman problem by exhaustive search using backtracking

  CS-CO-412 Algorithms and Data Structures Assignment No. 4

  See travellingSalesman.h for details of the problem

  David Vernon
  17 November 2014

*/
 
#include "travellingSalesman.h"

int main() {

   struct record_type record[NUMBER_OF_STOPS];

   int i, j, k;                                      // general purpose counters
   int n;                                            // number of shops
   int number_of_test_cases;                         //
   char shop_name[STRING_SIZE];                      // general purpose string
   int distances[NUMBER_OF_STOPS][NUMBER_OF_STOPS];  // distances between shop i (row) and shop j (column)
   
   int debug = TRUE;                                 // flag: if TRUE print information to assist with debugging

   FILE *fp_in, *fp_out;                             // input and output file pointers


   /* open input and output files */

   if ((fp_in = fopen("input.txt","r")) == 0) {
	   printf("Error can't open input input.txt\n");
      exit(0);
   }

   if ((fp_out = fopen("output.txt","w")) == 0) {
	   printf("Error can't open output output.txt\n");
      exit(0);
   }

   /* read the number of test cases */

   fscanf(fp_in, "%d", &number_of_test_cases);
   fgetc(fp_in); // move past end of line for subsequent fgets; fgets fails unless we do this

   if (debug) printf ("%d\n", number_of_test_cases);


   /* main processing loop, one iteration for each test case */

   for (k=0; k<number_of_test_cases; k++) {

      /* read the data for each test case  */
      /* --------------------------------  */
      
      /* number of shops */

      fscanf(fp_in, "%d", &n);
      fgetc(fp_in);  // move past end of line for subsequent fgets

      if (debug) printf ("%d\n",n);

      /* get the shop names and the car name */

      for (i = 0; i < n+1; i++) {

         fgets(shop_name, STRING_SIZE, fp_in);
         remove_new_line(shop_name);

         record[i].key = i+1;
         strcpy(record[i].string,shop_name);

         if (debug) printf ("%s\n",record[i].string);
      }

      /* get the matrix of distances */

      for (i = 0; i < n+1; i++) {
         for (j = 0; j < n+1; j++) {
            fscanf(fp_in, "%d", &(distances[i][j]));
         }
      }

      if (debug) {
         for (i = 0; i < n+1; i++) {
            for (j = 0; j < n+1; j++) {
               printf("%3d ", distances[i][j]);
            }
            printf("\n");
         }
      }


      /* main processing begins here */
      /* --------------------------- */

 
      if (debug) getchar();

   }

   fclose(fp_in);
   fclose(fp_out);                                                         
}