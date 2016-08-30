#include<cstdio>
#include<string>
#include<iostream>
#include<fstream>
using namespace std;

int numArrLen = 13;
int numbers[] = { 1000,  900,  500,  400,  100,   90, 50,   40,   10,    9,    5,    4,    1 };
string letters[] = { "M",  "CM",  "D",  "CD", "C",  "XC", "L",  "XL",  "X",  "IX", "V",  "IV", "I" };


int letterToNumber(char letter) {
            switch (letter) {
               case 'I':  return 1;
               case 'V':  return 5;
               case 'X':  return 10;
               case 'L':  return 50;
               case 'C':  return 100;
               case 'D':  return 500;
               case 'M':  return 1000;
            }
         }


int toArabic(string roman) {

            int i = 0;       
            int arabic = 0;  
            while (i < roman.length()) {
               char letter = roman[i];        
               int number = letterToNumber(letter); 

               i++; 

               if (i == roman.length()) {
				   arabic += number;
               }
               else {
                  int nextNumber = letterToNumber(roman[i]);
                  if (nextNumber > number) {
                     arabic += (nextNumber - number);
                     i++;
                  }
                  else {
                     arabic += number;
                  }
               }

            } 

			return arabic;
         }




string toRoman(int num) {
            string roman = "";  
            int N = num;       
			for (int i = 0; i < numArrLen; i++) {
               while (N >= numbers[i]) {
                  roman += letters[i];
                  N -= numbers[i];
               }
            }
            return roman;
         }

int main() {

	ifstream f("input.txt", ios::in);
	char* buff = new char[1000];
	int diffCount = 0;

	while (!f.eof()) {
		f >> buff;
		string romanOld(buff);
		string romanNew(toRoman(toArabic(romanOld)));
		diffCount += romanOld.length() - romanNew.length();
	}

	cout << diffCount;
	return 0;
}