/* ------------------------------------------------------------------------
   phase1.c

   Skeleton file for Phase 1. These routines are very incomplete and are
   intended to give you a starting point. Feel free to use this or not.
   Authors:
   Andrew Tarr
   Patrick Wilkening
   ------------------------------------------------------------------------ */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "usloss.h"
#include "phase1.h"

/* -------------------------- Globals ------------------------------------- */




/* the process table */
static PCB procTable[P1_MAXPROC];

/* current process ID */
int pid = -1;

/* number of processes */

int numProcs = 0;

static int sentinel(void *arg);
static void launch(void);

/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - runs the highest priority runnable process
   Parameters - none
   Returns - nothing
   Side Effects - runs a process
   ----------------------------------------------------------------------- */

void dispatcher()
{
  USLOSS_ContextSwitch(NULL,&procTable[0].context);
  /*
  int i;
  for(i = P1_MAXPROC-1; i>=0; i--){
    if(procTable[i].state == 1){
      procTable[i].state = 3;//?
      USLOSS_ContextSwitch(&procTable[i].context, &procTable[i].context);
    }
  }
*/
  /*
   * Run the highest priority runnable process. There is guaranteed to be one
   * because the sentinel is always runnable.
   */
}
/* ------------------------------------------------------------------------
   Name - startup
   Purpose - Initializes semaphores, process lists and interrupt vector.
             Start up sentinel process and the P2_Startup process.
   Parameters - none, called by USLOSS
   Returns - nothing
   Side Effects - lots, starts the whole thing
   ----------------------------------------------------------------------- */
void startup(int argc, char **argv)
{
  /* initialize the process table here */
  int i;
  for(i = 0; i<P1_MAXPROC; i++){
    procTable[i].name = "";
    procTable[i].priority = -1;
    procTable[i].tag = -1;
    procTable[i].state = -1;
    procTable[i].pid = -1;
  }


  /* Initialize the Ready list, Blocked list, etc. here */


  /* Initialize the interrupt vector here */

  /* Initialize the semaphores here */


  /* startup a sentinel process */
  /* HINT: you don't want any forked processes to run until startup is finished.
   * You'll need to do something  to prevent that from happening.
   * Otherwise your sentinel will start running as soon as you fork it and
   * it will call P1_Halt because it is the only running process.
   */

  int rc = P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6, 0);
  /* start the P2_Startup process */
  rc = P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 1, 0);

  dispatcher();
  /* Should never get here (sentinel will call USLOSS_Halt) */
  return;
} /* End of startup */

/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void finish(int argc, char **argv)
{
  USLOSS_Console("Goodbye.\n");
} /* End of finish */


/* ------------------------------------------------------------------------
   Name - P1_Fork
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or an error code.
   Side Effects - ReadyList is changed, procTable is changed, Current
                  process information changed



                  USLOSS_Context      context;
                  int                 (*startFunc)(void *);    Starting function
                  void                 *startArg;              Arg to starting function
                  int                 priority;
                  int                 tag;
   ------------------------------------------------------------------------ */
int P1_Fork(char *name, int (*f)(void *), void *arg, int stacksize, int priority, int tag)
{
  //error message for not being in kernel mode
  if ((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ){
      USLOSS_Console("Error, not in kernel mode.");
      USLOSS_Halt(1);
  }
    if (tag != 0 && tag != 1) {
        return -4;
    }
    if (priority < 0) {
        return -3;
    }
    if (stacksize < USLOSS_MIN_STACK) {
        return -2;
    }
    int newPid = 0;

    int i;
    for(i = 0; i < P1_MAXPROC; i++){
      if(procTable[i].state == -1){
        newPid = i;
        procTable[i].priority = priority;
        procTable[i].state = 1;
        procTable[i].pid = newPid;
        procTable[i].startFunc = f;
        procTable[i].startArg = arg;
        procTable[i].name = strdup(name);
        break;
      }
    }
    numProcs++;
    /* newPid = pid of empty PCB here */
    /*
    procTable[newPid].priority = priority;
    procTable[newPid].state = 1;
    USLOSS_Console("%d\n",procTable[newPid].state);
    procTable[newPid].pid = newPid;
    procTable[newPid].startFunc = f;
    procTable[newPid].startArg = arg;
    procTable[newPid].name = strdup(name);
    */
    void *stack0 = malloc(stacksize);
    // more stuff here, e.g. allocate stack, page table, initialize context, etc.
    USLOSS_ContextInit(&(procTable[i].context), stack0, stacksize, NULL, launch);
    return i;
} /* End of fork */




/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */
void launch(void)
{
  int  rc;
  int  status;
  status = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
  if (status != 0) {
      USLOSS_Console("USLOSS_PsrSet failed: %d\n", status);
      USLOSS_Halt(1);
  }
  rc = procTable[pid].startFunc(procTable[pid].startArg);
  /* quit if we ever come back */
  P1_Quit(rc);
} /* End of launch */

/* ------------------------------------------------------------------------
   Name - P1_Quit
   Purpose - Causes the process to quit and wait for its parent to call P1_Join.
   Parameters - quit status
   Returns - nothing
   Side Effects - the currently running process quits
   ------------------------------------------------------------------------ */
void P1_Quit(int status) {
  if ((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ){
      USLOSS_Console("Error, not in kernel mode.");
      USLOSS_Halt(1);
  }
  procTable[pid].state= 3;
  numProcs--;
  // Do something here.
}

/* ------------------------------------------------------------------------
   Name - P1_GetState
   Purpose - gets the state of the process
   Parameters - process PID
   Returns - process state
   Side Effects - none
   ------------------------------------------------------------------------ */
int P1_GetState(int PID) {
  if ((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ){
      USLOSS_Console("Error, not in kernel mode.");
      USLOSS_Halt(1);
  }
  int i;
  for(i =0; i<P1_MAXPROC; i++){
    if(procTable[i].pid == PID) return procTable[i].state;
  }
  return 0;
}

/*
 DumpProcesses
 prints information about each process for debuging purposes


 int                 state;
 int                 pid;
 char                * name;
 */

void P1_DumpProcesses(void) {
        int i;
        for(i = 0; i < numProcs; i++){
          USLOSS_Console("Process Name: %s\n", procTable[i].name);
          USLOSS_Console("PID: %d\n", procTable[i].pid);
          USLOSS_Console("State: %d\n", procTable[i].state);

        }
}


/* ------------------------------------------------------------------------
   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
             processes are blocked.  The other is to detect and report
             simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
                   and halt.
   ----------------------------------------------------------------------- */
int sentinel (void *notused)
{
    while (numProcs > 1)
    {
        /* Check for deadlock here */
        USLOSS_WaitInt();
    }
    USLOSS_Halt(0);
    /* Never gets here. */
    return 0;
} /* End of sentinel */
