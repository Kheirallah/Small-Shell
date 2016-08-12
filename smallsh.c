#include "smallsh.h"

//Structure for the process table
struct processTable pTable[MAXPROCESS];

/*program buffers and work pointers */
static char inpbuf[MAXBUF], tokbuf[2*MAXBUF], *ptr = inpbuf, *tok = tokbuf;

static char special [] = {' ', '\t', '&', ';', '\n', '\0'};

int processCounter = 0; /* counter for the number of processes */

//Handler for catching an interrupt signal
void intHandler()
{
        for(int i = 0; i < processCounter; i++)
               //Checks the process table for the foreground process and sends
               //kill signal to that process
               if(pTable[i].type == FOREGROUND)
               {
                   pTable[i].status = TERMINATED;
                   kill(pTable[i].pid, SIGKILL);
                   break;
               }
}

//Handler for catching a stop signal
void stopHandler()
{
        for(int i = 0; i < processCounter; i++)
               //checks the process table for the foreground process and sends 
               //stop signal to that process
               if(pTable[i].type == FOREGROUND)
               {
                   pTable[i].type = BACKGROUND;
                   pTable[i].status = STOPPED;
                   kill(pTable[i].pid, SIGSTOP);
                   break;
                }
}


//Handler for catching the child signal
void childHandler()
{
int status, value;
        //checks every process in the processTable and if the value returned from waitpid is -1
        //then the process has died and status will be terminated
        for(int i = 0; i < processCounter; i++)
               {
               value = waitpid(pTable[i].pid, &status, WNOHANG);
               if(value < 0)
               pTable[i].status = TERMINATED;
               }
}

//Displays a job table for current processes
void jobs()
{
  printf("Status Indicators\n");
  printf("Stopped: 0 \nRunning: 1\n");

  printf("\n============================\n");
  printf("Job #      PID        Status\n");
  printf("============================\n");
  
  for(int i = 0; i < processCounter; i++)
    {
         //Only processes which were not terminated are displayed
         if(pTable[i].status != TERMINATED)
         {   
             printf(" %d         %d         %d\n", i, pTable[i].pid, pTable[i].status);
             printf("----------------------------\n");
         }
    }
}

/*print prompt and read a line */
int userin(char *p)
{
 int c, count;
 
 /* initialization for later routines */
 ptr = inpbuf;
 tok = tokbuf;
 
 /* display prompt */
 printf("%s", p);
 
 count = 0;
 
 while(1)
 {
   if((c = getchar()) == EOF)
         return(EOF);
      
   if(count < MAXBUF)
         inpbuf[count++] = c;
         
   if (c == '\n' && count < MAXBUF)
   {
         inpbuf[count] = '\0';
         return count;
   }
   
   /* if line too long restart */
   if(c == '\n')
   {
         printf("smallsh: input line too long\n");
         count = 0;
         printf("%s ", p);
   }
  }
}

int inarg(char c)
{
 char *wrk;
 
 for(wrk = special; *wrk; wrk++)
 {
      if(c == *wrk)
            return (0);
 }
 
 return (1);
} 

int gettok(char **outptr)
{

 int type;
 
 /*set the outptr string to tok */
 *outptr = tok;
 
 /*strip white space from the buffer containing the tokens */
 while ( *ptr == ' ' || *ptr == '\t')
      ptr++;
      
 /*set the token pointer to the first token in the buffer */
 *tok++ = *ptr;
 
 /* set the type variable depending 
  * on the token in the buffer */
  
  switch(*ptr++)
  {
  case '\n':
               type = EOL;
               break;
  case '&':
               type = AMPERSAND;
               break;
  case ';': 
               type = SEMICOLON;
               break;
  default:
               type = ARG;
               /* keep reading valid ordinary characters */
               while(inarg(*ptr))
                     *tok++ = *ptr++;
  }
  
  *tok++ = '\0';
  return type;
}

int procline(void)   /*process input line */
{
 char *arg[MAXARG + 1];    /*pointer array for runcommand */
 int toktype;              /*type of token in command */
 int narg;                 /*number of arguments so far */
 int type;                 /*FOREGROUND or BACKGROUND */
 
 narg = 0;
 
 for(;;)                   /*loop forever */
 {
      /* take action according to token type */
      switch(toktype = gettok(&arg[narg]))
      {
      case ARG:  if(narg < MAXARG)
                     narg++;
                 break;
      case EOL:
      case SEMICOLON:
      case AMPERSAND:
                  if (toktype == AMPERSAND)
                        type = BACKGROUND;
                  else
                        type = FOREGROUND;
                  
                  if(narg != 0)
                  {
                        arg[narg] = NULL;
                        runcommand(arg, type);
                  }
                  
                  if(toktype == EOL)
                        return 0;
                        
                  narg = 0;
                  break;
       }
 }
 
}

int runcommand(char **cline, int where)
{

 pid_t pid;
 int status;
 int i = 0;
 
 //if the command read in is quit then shell exits
 if(strcmp(*cline, "quit") == 0)
 exit(0);
 
 //Clears the screen of previous output
 else if(strcmp(*cline, "clear") == 0)
 printf("\033[2J\033[1;1H");

 
 //if the command read in is jobs then the job table is shown
 else if(strcmp(*cline,"jobs") == 0)
 jobs();
              
 //if the command read in is kill   
 else if(strcmp(*cline,"kill") == 0)
 {
 
 //Get the job number
 printf("Input job number: ");
 scanf("%d", &i);
 
      //find the process in the process table and send the kill signal to it
      if(pTable[i].status == RUNNING || pTable[i].status == STOPPED)
      {
        pTable[i].status = TERMINATED;
        kill(pTable[i].pid, SIGKILL);
      }
      else
      printf("Process has already terminated.\n");
      
 }
 
 //If the command read in is bg
 else if (strcmp(*cline, "bg") == 0)
 {
 
 //Gets the job number
 printf("Input job number: ");
 scanf("%d", &i);
 
      //Checks the given job # to ensure it is a background job then 
      //sends the continue signal and changes the status to running
      if(pTable[i].type == BACKGROUND) 
      {
      kill(pTable[i].pid, SIGCONT);
      pTable[i].status = RUNNING;
      }
      
 }     

 //if the command given is fg
 else if (strcmp(*cline, "fg") == 0)
 {
 
 //Get the job number
 printf("Input job number: ");
 scanf("%d", &i);
      
      //Checks if the given job is a background job 
      //then changes it to the foreground 
      if(pTable[i].type == BACKGROUND)
      {
      pTable[i].type = FOREGROUND;
      
         //If the process is stopped send the continue signal
         //if(pTable[i].status == STOPPED)
            kill(pTable[i].pid, SIGCONT);
      
      //Updates the status and waitpid ensures the shell waits until the process 
      //executes 
      pTable[i].status = RUNNING;
      waitpid(pTable[i].pid, &status, 0);
      }
 }


 else
 {
 
 //Create a new process and add it to the job table
 processCounter++;
 
 pTable[processCounter - 1].type = where;
 pTable[processCounter - 1].status = RUNNING;
 
 switch(pid = fork())
 {
 case -1:
      perror("smallsh");
      return(-1);
 case 0:
      execvp(*cline, cline);
      perror(*cline);
      exit(1);
 default:
   pTable[processCounter - 1].pid = pid;
 }
 
 /*code for parent */
 /* if background process print pid and exit */
 if(where == BACKGROUND)
 {
      printf("[Process id %d]\n", pTable[processCounter - 1].pid);
      return (0);
 }
 
 
 /*wait until process pid exits */
 if(waitpid(pid, &status, 0) == -1)
      return (-1);
 else
 {
 
 for(i = 0; i < processCounter; i++)
      if(pTable[i].pid == pid)
      {
      pTable[i].status = TERMINATED;
      break;
      }
      
    return (status);
    
 }

 }
 
}

char *prompt = "Command> "; /* prompt */

int main()
{
        //Signal structures for the shell
        static struct sigaction act, actStop, actChild;

        void intHandler();
        void stopHandler();
        void childHandler();

        act.sa_handler = intHandler;
        actStop.sa_handler = stopHandler;
        actChild.sa_handler = childHandler;

        sigfillset(&(act.sa_mask));
        sigfillset(&(actStop.sa_mask));
        sigfillset(&(actChild.sa_mask));

        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTSTP, &actStop, NULL);
        sigaction(SIGCHLD, &actChild, NULL);
        
        //Runs forever
        for(;;)
            while(userin(prompt) != EOF)
               procline();
               
        return 0;
}