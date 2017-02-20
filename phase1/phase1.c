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

int clockTicks = 0;
int clockSum = 0;


typedef struct PCB {
    USLOSS_Context context;
    int priority;
    int pid;
    char name[MAXNAME];
    int (* start_func) (char *);
    char *stack;
    int stackSize;
    // 0 = running, 1 = ready, 2 = unusd, 3 = quit, 4 = waiting on semaphore
    int status;
    int timeCPU;
    int parentID;
    char startArg[MAXARG];
     PCB *childProcPtr;
     PCB *nextProcPtr;
     PCB *sibilingPtr;
    /*
    int             numOfChildren;


   procPtr         nextSiblingPtr;
   procPtr	       zombieProcPtr;
   procPtr	       nextZombieSiblingPtr;
   procPtr         zappedPtrList;
   procPtr         nextZappedSiblingPtr;
   int             quitStatus;
    */

} PCB;

PCB * curProc;

PCB *blockPtr;
PCB *readyPtr;



/* the process table */
static PCB procTable[P1_MAXPROC];
PCB cur;


/* current process ID */
int pid = -1;

/* number of processes */

int numProcs = 0;
//dispatcher context?
int startUpTerminated;

static int sentinel(void *arg);
static void launch(void);
static void interruptsOff(void);
static void interruptsOn(void);









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

  PCB *curProc, *nextProc;
  nextProc = readyPtr;
  curProc = current; //need to set current
  current = nextProc;
  int curProc_time = Current_CPUtime;
  readyPtr = readyPtr->nextProc;
  if(nextProc->status == 1 || nextProc->status == 0)
      addToQueue(nextProc->pid,&readyPtr,nextProc->priority);
  nextProc->status = 0;
  if(curProc->status != 2 && curProc_time != -1 && curProc->pid != nextProc->pid){
    if(curProc->timeCPU == -1)
      curProc->timeCPU = USLOSS_Clock() - curProc->timeCPU;
    else
      curProc->timeCPU = curProc->timeCPU + (USLOSS_Clock() - curProc->timeCPU);
  }
  if(curProc->priority == SENTINELPRIORITY)
    USLOSS_ContextSwitch(NULL,&nextProc->context);
  else
    USLOSS_ContextSwitch(&curProc->context,&nextProc->context)
  //enableInt();
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
  timeCPU = -1;
  for(i = 0; i<P1_MAXPROC; i++){
    procTable[i].name = (char *) malloc(sizeof(char) * MAXNAME);
    procTable[i].priority = -1;
    procTable[i].state = -1;
    procTable[i].pid = -1;
    procTable[i].start_func = NULL;
    procTable[i].stack = NULL;
    procTable[i].stackSize = -1;
    procTable[i].status = -1;
    procTable[i].timeCPU = -1;
    procTable[i].parentID = -1;
    procTable[i].startArg = (char *) malloc(sizeof(char) * MAXARG);
  }

  /* Initialize the Ready list, Blocked list, etc. here */
  readyPtr = NULL;
  blockPtr = NULL;

  /* Initialize the interrupt vector here */
  //define prototype above, USLOSS_IntVec[USLOSS_CLOCK_INT] =, create function

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
  //disableInterrupts()
    if (tag != 0 && tag != 1) {
        return -4;
    }
    if (priority < 0 || priority >= 6) {
        return -3;
    }
    if (stacksize < USLOSS_MIN_STACK) {
        return -2;
    }

    if(name == NULL || f == NULL){
      return -1;
    }

    int newPid = 0;
    if(strlen(name) >= MAXNAME-1){
      USLOSS_Console("Error, process name too long");
      USLOSS_Halt(1);
    }
    if(arg == NULL){
      arg = "";
    }else if(strlen(arg) >= MAXARG-1){
      USLOSS_Console("Error, argument too long");
      USLOSS_HAlt(1);
    }


    int i;
    for(i = 0; i < P1_MAXPROC; i++){
      if(procTable[i].state == -1){
        newPid = i;
        procTable[i].priority = priority;
        procTable[i].state = 1;
        procTable[i].pid = newPid;
        procTable[i].startFunc = f;
        procTable[i].startArg = strdup(arg);
        procTable[i].name = strdup(name);
        break;
      }
    }
    numProcs++;
    /*
    if(cur != NULL && f != &processStart)
      set parentid = currentpid
    else parentid = -2;
    */


    /* newPid = pid of empty PCB here */

    char *stack0 =(char *) malloc(stacksize * sizeof(char));
    // more stuff here, e.g. allocate stack, page table, initialize context, etc.
    USLOSS_ContextInit(&(procTable[i].context), stack0, stacksize, P3_AllocatePageTable, launch);

    addToQueue(procTable[i].pid,&readyPtr,priority);
    if(priority != SENTINELPRIORITY){
      procTable[i].status = 1;
      dispatcher();
    }else{
      //current = &proctable[i]

    }
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

/*

*/
void addToQueue(int pid, PCB **table, int priority){
  if(table == NULL){
    *table = &procTable[pid];
    return;
  }
  PCB *tmp = *table;
  if(tmp->priority > priority){
    procTable[pid].nextProcPtr = *table;
    *table = &procTable[pid];
    return;
  }
  PCB *prev = NULL;
  PCB *cur = *table;
  while(cur->priority <= priority){
    prev = cur;
    cur = cur->nextProcPtr;
  }
  prev->nextProcPtr = &procTable[pid];
  procTable[pid].nextProcPtr = cur;
}
void addChild(PCB  ins, PCB **table){
  if(*table == NULL){
    *table = ins;
  }
  PCB *tmp = *table;
  while(tmp->sibilingPtr != NULL){
    tmp = tmp->sibilingPtr;
  }
  cur->nextSiblingPtr = ins;
  node->nextSiblingPtr = NULL;
}
