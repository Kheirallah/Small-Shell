#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>

#define EOL 1 /*end of line */
#define ARG 2 /*normal arguments */
#define AMPERSAND 3
#define SEMICOLON 4

#define MAXARG 512 /* max. no. command args */
#define MAXBUF 512 /* max. length input line */
#define MAXPROCESS 50 /* 50 process maximum */
#define FOREGROUND 0
#define BACKGROUND 1
#define RUNNING 1
#define STOPPED 0
#define TERMINATED -1

//Process table structure
struct processTable 
{
  pid_t pid;
  int type;
  int status;
};

struct processTable pTable[MAXPROCESS];