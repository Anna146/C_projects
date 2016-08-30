#include<cstdio>
#include<conio.h>
#include <ctype.h>
#include<string.h>
using namespace std;
//=========================================================
struct u_tab
{
   int no;
   char type[20];
   char name[20];
}u_tab[200];

struct lit
{
   int no;
   char name[20];
}lit_tab[100];

struct id
{
  int no;
  char name[20];
}id_tab[100];

FILE *fp1,*fp2;

char buff[80];

int lit_cnt=0,id_cnt=0,u_cnt=0;
//=========================================================
int search_idtab(char temp[20])
{
  int i;

  for(i=0;i<id_cnt;i++)
  {
	if(strcmp(temp,id_tab[i].name)==0)
	 return(i);
  }
  return(-1);
}
//=========================================================
int search_littab(char temp[20])
{
  int i;
  for(i=0;i<lit_cnt;i++)
  {
	if(strcmp(temp,lit_tab[i].name)==0)
	 return(i);
  }
  return(-1);
}
//=========================================================
void lex(char source[80])
{
  int i,id,lit=0,term=0,l=0,terminal_no,res;

  char temp[20],terminal[20],str[80];

  fp1=fopen(source,"r");
  if(fp1==NULL)
  {
	printf("\n\n source file not found::");
	getch();
	return;
  }

  while(fgets(str,80,fp1)!=NULL)
  {

  for(i=0;str[i]!='\0';i++)
  {
	  id=lit=term=l=0;

	  strcpy(temp,"");

	  if(str[i]=='{'||str[i]=='}'||str[i]=='['||str[i]==']')
	  {
		temp[l]=str[i];
		l++;
		temp[l]='\0';
	  }

	  if(str[i]=='+'||str[i]=='-'||str[i]=='*'||str[i]=='/')
	  {
		temp[l]=str[i];
		l++;
		temp[l]='\0';
	  }


	  if(str[i]==','||str[i]==':'||str[i]=='"')
	  {
		temp[l]=str[i];
		l++;
		temp[l]='\0';
	  }

	  if(str[i]=='#'||str[i]=='%'||str[i]=='('||str[i]==')')
	  {
		temp[l]=str[i];
		l++;
		temp[l]='\0';
	  }

	  if(str[i]=='<')
	  {
		   temp[l]=str[i];
		   l++;
		   if(str[i+1]=='=')
		   {
			 temp[l]=str[i+1];
			 i++;
			 l++;
		  }
		  temp[l]='\0';
	  }
	  else if(str[i]=='=')
	  {
		 temp[l]=str[i];
		 l++;
		 if(str[i+1]=='=')
		 {
		   temp[l]=str[i+1];
		   l++;
		   i++;
		 }
		 temp[l]='\0';
	  }
	  else if(str[i]=='!')
	  {
		 temp[l]=str[i];
		 l++;
		 if(str[i+1]=='=')
		 {
		   temp[l]=str[i+1];
		   l++;
		   i++;
		 }
		 temp[l]='\0';
	  }
	  else if(str[i]=='.')
	  {
		 temp[l]=str[i];
		 l++;
		 if(str[i+1]=='.')
		 {
		   temp[l]=str[i+1];
		   l++;
		   i++;
		 }
		 temp[l]='\0';
	  }
	  else if(str[i]=='>')
	  {
		temp[l]='>';
		l++;
		if(str[i+1]=='=')
		 {
		   temp[l]=str[i+1];
		   l++;
		   i++;
		 }
		temp[l]='\0';
	  }
	  else if(isalpha(str[i]))
	  {
		temp[l]=str[i];
		l++;
		i++;
	 while( isalpha(str[i]) || isdigit(str[i]) || str[i]=='_'||str[i]=='.')
	 {
			temp[l]=str[i];
			i++;
			l++;
		}
		i--;
		temp[l]='\0';
		id=1;
	 }
	 else if(isdigit(str[i]))
	 {
		temp[l]=str[i];
		l++;
		i++;
		while(isdigit(str[i]))
		{
		 temp[l]=str[i];
		 l++;
		 i++;
		}
		i--;
		temp[l]='\0';
		lit=1;
	 }
	fp2=fopen("term_tab.txt","r");

	if(fp2==NULL)
	{
	 printf("\n\n terminal file not found::");
	 getch();
	 return;
	}
	while(fscanf(fp2,"%s%d",terminal,&terminal_no)!=EOF)
	{
	  if(strcmp(terminal,temp)==0)
	  {
		strcpy(u_tab[u_cnt].name,temp);
		strcpy(u_tab[u_cnt].type,"term");
		u_tab[u_cnt].no=terminal_no;
		term=1;
		u_cnt++;
		break;
	   }
	}
	fclose(fp2);
	if(term!=1&&id==1)
	{
		 res=search_idtab(temp);

		 if(res==-1)//not found
		 {
			strcpy(u_tab[u_cnt].name,temp);
			strcpy(u_tab[u_cnt].type,"ID");
			u_tab[u_cnt].no=id_cnt;

			strcpy(id_tab[id_cnt].name,temp);
			id_tab[id_cnt].no=id_cnt;

			id_cnt++;
			u_cnt++;
		 }
		 else
		 {
			strcpy(u_tab[u_cnt].name,temp);
			strcpy(u_tab[u_cnt].type,"ID");
			u_tab[u_cnt].no=res;
			u_cnt++;
		 }
	}

	if(term!=1&&lit==1)
	{
		  res=search_littab(temp);
		 if(res==-1)//not found
		 {
			strcpy(u_tab[u_cnt].name,temp);
			strcpy(u_tab[u_cnt].type,"lit");
			u_tab[u_cnt].no=lit_cnt;

			strcpy(lit_tab[lit_cnt].name,temp);
			lit_tab[lit_cnt].no=lit_cnt;

			lit_cnt++;
			u_cnt++;
		 }
		 else
		 {
			strcpy(u_tab[u_cnt].name,temp);
			strcpy(u_tab[u_cnt].type,"lit");
			u_tab[u_cnt].no=res;
			u_cnt++;
		 }
	}
  }//for
 }
}
//=========================================================
void printUniver()
{ int i;

  printf("\n\n UNIVERSAL SYMB TABLE");
  printf("\nTERMINAL\tTYPE\tNO");
  for(i=0;i<u_cnt;i++)
  {
	printf("\n%s\t\t%s\t%d",u_tab[i].name,u_tab[i].type,u_tab[i].no);
	 getch();
  }
}
//=========================================================
void printLit()
{
	int i;

  printf("\n\n litral table");
  printf("\nlitral\tNO");
  for(i=0;i<lit_cnt;i++)
  {
	printf("\n%s\t%d",lit_tab[i].name,lit_tab[i].no);
	getch();
  }
}
//=========================================================
void main()
{
  char source[20];
  printf("\n\n enter source file name::");
  scanf("%s",source);
  lex(source);
  //clrscr();
   printUniver();
   //printLit();
	getch();
}
/*
=================================================================
		   Terminale
# 1
include 2
++ 3
+ 4
conio.h 5
< 6
void 7
main 8
( 9
) 10
{ 11
} 12
= 13
if 14
while 15
for 16
, 17
> 18
int 19
<= 20
== 21
printf 22
stdio.h 23
;  24
getch 25
=================================================================
	  Source File

#include<stdio.h>
#include<conio.h>

void main()
{
	int i,j=1,k=2,flag;
	for(i=0;i<=4;i++)
	{
		k=i+j;
	}
	flag=5;
	if(flag==10)
		printf("Hello Students");
	getch();
}
=================================================================
 UNIVERSAL SYMB TABLE
TERMINAL        TYPE    NO
#               term    1
include         term    2
<               term    6
stdio.h         term    23
>               term    18
#               term    1
include         term    2
<               term    6
conio.h         term    5
>               term    18
void            term    7
main            term    8
(               term    9
)               term    10
{               term    11
int             term    19
i               ID      0
,               term    17
j               ID      1
=               term    13
1               lit     0
,               term    17
k               ID      2
=               term    13
2               lit     1
,               term    17
flag            ID      3
for             term    16
(               term    9
i               ID      0
=               term    13
0               lit     2
i               ID      0
<=              term    20
4               lit     3
i               ID      0
+               term    4
+               term    4
)               term    10
{               term    11
k               ID      2
=               term    13
i               ID      0
+               term    4
j               ID      1
}               term    12
flag            ID      3
=               term    13
5               lit     4
if              term    14
(               term    9
flag            ID      3
==              term    21
10              lit     5
)               term    10
printf          term    22
(               term    9
Hello           ID      4
Students        ID      5
)               term    10
getch           term    25
(               term    9
)               term    10
}               term    12
=========================================================
 litral table
litral  NO
1       0
2       1
0       2
4       3
5       4
10      5
*/