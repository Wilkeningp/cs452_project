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

/* -------------------------- Structs ------------------------------------- */
struct node;

typedef struct PCB {
    USLOSS_Context context;
    int priority;
    int pid;
    char name[MAXNAME];
    int (* start_func) (void *);
    char *stack;
    int stackSize;
    // 0 = running, 1 = ready, 2 = unusd, 3 = quit, 4 = waiting on semaphore
    int status;
    int timeCPU;
    int parentID;
    void *startArg;
    // PCB *childProcPtr;
    int numOfChildren;
    // PCB *nextProcPtr;
    // PCB *sibilingPtr;
    // PCB *zombieProcPtr;
    // PCB *nextZombieSiblingPtr
    /*
   procPtr         nextSiblingPtr;
   procPtr         zappedPtrList;
   procPtr         nextZappedSiblingPtr;
   int             quitStatus;
    */

} PCB;

typedef struct node {
  PCB *pcb;
  struct node *next;
} queueNode;

typedef struct {
  int id;
  unsigned int count;
  char *name;
  int inUse;
  queueNode *queue;
} Semaphore;

/* -------------------------- Globals ------------------------------------- */

int clockTicks = 0;
int clockSum = 0;

//PCB * curProc;

PCB *blockPtr;
queueNode *readyQueue;

/* the process table */
static PCB procTable[P1_MAXPROC];
static Semaphore semTable[P1_MAXSEM];
int usedSemaphores = 0;
PCB cur;

/* current process ID */
int currentpid = -1;

/* number of processes */

int numProcs = 0;
//dispatcher context?
int startUpTerminated;

static int sentinel(void *arg);
static void launch(void);
static void interruptsOff(void);
static void interruptsOn(void);
static void addToPriorityQueue(int pid, PCB **table, int priority);

int checkInvaildSemaphore (P1_Semaphore);






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
  nextProc = readyQueue;
  curProc = current; //need to set current
  current = nextProc;
  int curProc_time = Current_CPUtime;
  readyQueue = readyQueue->nextProc;
  if(nextProc->status == 1 || nextProc->status == 0)
      addToPriorityQueue(nextProc->pid,&readyQueue,nextProc->priority);
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
    procTable[i].name = NULL;
    procTable[i].priority = -1;
    procTable[i].state = -1;
    procTable[i].pid = -1;
    procTable[i].start_func = NULL;
    procTable[i].stack = NULL;
    procTable[i].stackSize = -1;
    procTable[i].status = -1;
    procTable[i].timeCPU = -1;
    procTable[i].parentID = -1;
  }

  /* Initialize the Ready list, Blocked list, etc. here */
  readyQueue = NULL;
  blockPtr = NULL;

  /* Initialize the interrupt vector here */
  //define prototype above, USLOSS_IntVec[USLOSS_CLOCK_INT] =, create function

  /* Initialize the semaphores here */
  for(i = 0; i < P1_MAXSEM; i++) {
      semTable[i].id = i;
      semTable[i].count = 0;
      semTable[i].inUse = 0;
      semTable[i].name = NULL;
      semTable[i].queue = NULL;
  }


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
  //disable Interrupts()
    if (tag != 0 && tag != 1) {
        //enable interrupts()
        return -4;
    }
    if (priority < 0 || priority > 5) {
        if (priority == 6 && strcmp(name, "sentinel") == 0) {
          ;
        } else {
          //enable interrupts
          return -3;
        }
    }
    if (stacksize < USLOSS_MIN_STACK) {
        //enable interrupts
        return -2;
    }
    if(name == NULL || f == NULL){
      USLOSS_Console("Error, process name or function is not set");
      USLOSS_Halt(1);
    }
    if(strlen(name) >= MAXNAME-1){
      USLOSS_Console("Error, process name too long");
      USLOSS_Halt(1);
    }
    /*
    else if(strlen(arg) >= MAXARG-1){
      USLOSS_Console("Error, argument too long");
      USLOSS_Halt(1);
    }
    */

    int newPid = -1;
    int i = 0;
    
    while (newPid == -1 && i < 50) {
      if (procTable[i].state = -1) {
        newPid = i;
      }
      i++;
    }
    if (newPid == -1) {
      //enable interrupts
      return -1;
    }
    numProcs++;
    procTable[newPid].pid = newPid;
    procTable[newPid].priority = priority;
    procTable[newPid].state = 1;
    procTable[newPid].start_func = f;
    procTable[newPid].startArg = arg;
    procTable[newPid].name = strdup(name);
    
    char *stack0 =(char *) malloc(stacksize * sizeof(char));
    strcpy(procTable[newPid].stack, stack0);
    procTable[newPid].stacksize = stacksize;
    
    // more stuff here, e.g. allocate stack, page table, initialize context, etc.
    USLOSS_ContextInit(&(procTable[newPid].context), procTable[newPid].stack, procTable[newPid].stacksize, P3_AllocatePageTable, launch);

    addToPriorityQueue(procTable[newPid].pid,&readyQueue,priority);
    procTable[newPid].status = 1;
    /*
    if(priority != SENTINELPRIORITY){
      procTable[newPid].status = 1;
    }else{
      //current = &proctable[i]

    }
    */
    return newPid;
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
   Name - P1_Join
   Purpose - Wait for a child process (if one exists) to quit.
   Parameters - A int tag of the parent process, 

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
  //procTable[pid].state= 3;
  //TODO:: disable interrupts
  numProcs--;
  // Do something here.

  // Check for Children Still Active
  if (Current->childProcPtr != NULL) {
    USLOSS_Console("Error, Childern still active.");
    USLOSS_Halt(1);
  }

  /*
  remove Zombie processes
  can leak memory though.
  */

  PCB tempProc = Current->zombieProcPtr;
  while (tempProc != NULL) {
    free((*tempProc).name);
    free((*tempProc).arg);
    (*tempProc).pid = -1;
    (*tempProc).status = -1;
    (*tempProc).numOfChildren = 0;
    (*tempProc).nextProcPtr = NULL;
    (*tempProc).childProcPtr = NULL;
    (*tempProc).zombieProcPtr = NULL;
    (*tempProc).nextZombieSiblingPtr = NULL;
    (*tempProc).zappedPtrList = NULL;
    (*tempProc).nextZappedSiblingPtr = NULL;
    (*tempProc).context = (USLOSS_Context) NULL;
    (*tempProc).priority = -1;
    (*tempProc).start_func = NULL;
    (*tempProc).stack = NULL;
    (*tempProc).stacksize = NULL;
    (*tempProc).status = -1;
    (*tempProc).parentID = -1;
    (*tempProc).timeCPU = -1;
    tempProc = tempProc->nextZombieSiblingPtr;
  }

  tempProc = Current->zappedPtrList;
  while (tempProc != NULL) {
    tempProc->status = 1;
    deleteFromQueue(tempProc->pid, &blockPtr);
    addToPriorityQueue(tempProc->pid, &readyQueue, procTable[tempProc->pid].priority);
    tempProc = tempProc->nextZappedSiblingPtr;
  }



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
    Add a process to a Priority Queue
*/
void addToPriorityQueue(int pid, queueNode **queue, int priority){
  //disable interrupts
  queueNode *new = malloc(sizeof(queueNode));
  new->next = NULL;
  new->pcb = &procTable[pid];
  if(*queue == NULL){
    *queue = new;
    //enable interrupts
    return;
  }
  
  if((*queue)->pcb->priority > priority){
    new.next = *queue;
    *queue = new;
    //enable interrupts
    return;
  }

  queueNode *prev = *queue;
  queueNode *cur = (*queue)->next;
  while(cur->priority <= priority){
    prev = cur;
    cur = cur->next;
  }
  new->next = prev->next;
  prev->next = new;
  //enable interrupts
}

/*
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
*/

/*
void deleteFromQueue(int pid, PCB* list) {
  if (list == NULL) {
    return;
  }
  if ((*list)->pid == pid) {
    PCB temp = (*list)->nextProc;
    (*list)->nextProcPtr = NULL;
    (*list) = temp; 
    return;
  } else {
    PCB prevProc = *list;
    PCB curProc = *list;
    while (curProc != NULL) {
      if ((curProc)->pid == pid) {
        (prevProc)->nextProcPtr = (curProc)->nextProcPtr;
        (curProc)->nextProcPtr = NULL;
        return;
      }
      prevProc = curProc;
      curProc = (curProc->nextProcPtr);
    }
  }
}
*/

int P1_SemCreate (char *name; unsigned int value, P1_Semaphore *sem) {
  //disable interrupts
  if (name == NULL) {
    //enable interrupts
    return -3;
  }
  int i;
  for(i=0; i < usedSemaphores; i++) {
    if(strcmp(name, semTable[i].name) == 0) {
      //enable interrupts
      return -1;
    }
  }
  for(i=0; i < P1_MAXSEM; i++) {
    if (semTable[i].inUse == 0) {
      semTable[i].count = value;
      semTable[i].queue = NULL;
      semTable[i].inUse = 1;
      semTable[i].name = malloc(strlen(name)+1);
      strcpy(semTable[i].name, name);
      //enable interrupts
      return 0;
    }
  }
  //enable interrupts
  return -2
}

int P1_SemFree(P1_Semaphore sem) {
  //disable interrupts
  if( checkInvaildSemaphore(sem) ) {
    //enable interrupts
    return -1;
  }
  Semaphore *s = (Semaphore *) sem;
  if (s->queue != NULL) {
    //enable interrupts
    return -2;
  }
  s->inUse = 0;
  s->count = 0;
  free(sem->name);
  //enable interrupts
  return 0;
}

/* 
  ------------------------------------
  Name - P1_P
  Purpose - P function for semaphores
  Parameters - a semaphore
  Returns - function status
  ------------------------------------
*/
int P1_P(P1_Semaphore sem) {
  if ( checkInvaildSemaphore(sem) ){
    return -1;
  }
  Semaphore *s = (Semaphore *) sem;
  while (1) {
    //disable interrupts
    if (s->count > 0) {
      s->count--;
      break;
    }
    // add to queue
    // block process
    // pass reference to sem to process
    // enable interrupts
    dispatcher();
  }
  //unblock process
  //dereference sem from process
  //enable interrupts
  return 0;
}

int P1_V(P1_Semaphore sem) {
  if ( checkInvaildSemaphore() ) {
      return -1;
  }
  Semaphore *s = (Semaphore *) sem;
  //disable interrupts
  s->count++;
  if (s->queue != NULL) {
    //get process off semaphore queue
    //add process to ready queue;
    //enable interrupts
    dispatcher();
  } else {
    //enable interrupts
  }
  return 0;
}

int checkInvaildSemaphore (P1_Semaphore) {
  if (sem == NULL) {
    return 1;
  }
  int i;
  for (i=0; i<P1_MAXSEM; i++) {
    if (&(semTable[i]) == sem) {
      if(semTable[i].inUse == 1) {
        return 0;
      } else { 
        return 1;
      }
    }
  }
  return 1;
}