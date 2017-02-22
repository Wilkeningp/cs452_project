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
struct semaphore;
struct node;

typedef struct PCB {
    USLOSS_Context context;
    int priority;
    int pid;
    char *name;
    int (* start_func) (void *);
    void* stack;
    int stackSize;
    // 0 = running, 1 = ready, 2 = unusd, 3 = quit, 4 = waiting on semaphore
    int status;
    int CPU_uptime;
    int startTime;
    int parentID;
    void *startArg;
    int numOfChildren;
    struct semaphore *sem;
    struct semaphore *blockedSem;
    int blocked;
    struct node killedChildren;

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

queueNode *blockQueue;
queueNode *readyQueue;

/* the process table */
static PCB procTable[P1_MAXPROC];
static Semaphore semTable[P1_MAXSEM];
int usedSemaphores = 0;

/* current process ID */
int currentpid = -1;
int procHold = 0; 
/* number of processes */

int numProcs = 0;
//dispatcher context?
//int startUpTerminated;

static int sentinel(void *arg);
static void launch(void);
static void disableInterrupts(void);
static void enableInterrupts(void);
static void addToPriorityQueue(int pid, queueNode **queue, int priority);
static void addToQueue(int pid, queueNode **queue);
static PCB * PopAQueue(queueNode **queue);

int checkInvaildSemaphore (P1_Semaphore);

void clockIntHandler(int type, void *arg);
void countDownIntHandler(int type, void *arg);
void terminalIntHandler(int type, void *arg);
void diskIntHandler(int type, void *arg);
void MMUIntHandler(int type, void *arg);

Semaphore *clockDevSemaphore;
int clockDevStatus = 0;
Semaphore *alarmDevSemaphore;
int alarmDevStatus = 0;
Semaphore *terminalDevSemaphores[USLOSS_TERM_UNITS];
int terminalDevStatus[USLOSS_TERM_UNITS] = {0};
Semaphore *diskDevSemaphores[USLOSS_DISK_UNITS];
int diskDevStatus[USLOSS_DISK_UNITS] = {0};

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
  while (1) {
    disableInterrupts();
    PCB *currentProc, *nextProc;
    if (currentpid >=0) {
      currentProc =  procTable[currentpid];
      clockTicks = 0;
      if (procTable[currentpid].status == 0) {
        procTable[currentpid].status = 1;
      }
      if ( procHold && (procTable[currentpid].status != 3) && (procTable[currentpid].status != 4)) {
        prior = procTable[currentpid].priority;
        addToPriorityQueue(currentpid, &readyQueue, prior);
      }
    } else {
      currentProc = NULL;
    }
    nextProc = PopAQueue(&readyQueue);
    currentpid = nextProc->pid;
    if(nextProc->status == 1) {
      nextProc->status = 0;
    }
    clockTicks = 0;
    nextProc->startTime = USLOSS_Clock();
    enableInterrupts();
    if (currentProc == NULL) {
      USLOSS_ContextSwitch(&(currentProc->context), &(nextProc->context));
    } else {
      USLOSS_ContextSwitch(NULL, &(currentProc->context));
    }
    nextProc->CPU_uptime = P1_ReadTime();
  }
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
  CPU_uptime = -1;
  for(i = 0; i<P1_MAXPROC; i++){
    procTable[i].name = NULL;
    procTable[i].priority = -1;
    procTable[i].state = -1;
    procTable[i].pid = i;
    procTable[i].start_func = NULL;
    procTable[i].stack = NULL;
    procTable[i].stackSize = -1;
    procTable[i].status = -1;
    procTable[i].CPU_uptime = 0;
    procTable[i].startTime = 0;
    procTable[i].parentID = -1;
    procTable[i].numOfChildren = 0;
    procTable[i].killedChildren = NULL;
    procTable[i].sem = NULL;
  }

  /* Initialize the Ready list, Blocked list, etc. here */
  readyQueue = NULL;
  blockQueue = NULL;

  /* Initialize the interrupt vector here */
  //define prototype above, USLOSS_IntVec[USLOSS_CLOCK_INT] =, create function
  USLOSS_IntVec[USLOSS_CLOCK_INT] = clockIntHandler;
  USLOSS_IntVec[USLOSS_ALARM_INT] = countDownIntHandler;
  USLOSS_IntVec[USLOSS_TERM_INT] = terminalIntHandler;
  USLOSS_IntVec[USLOSS_DISK_INT] = diskIntHandler;
  USLOSS_IntVec[USLOSS_MMU_INT] = MMUIntHandler;

  /* Initialize the semaphores here */
  for(i = 0; i < P1_MAXSEM; i++) {
      semTable[i].id = i;
      semTable[i].count = 0;
      semTable[i].inUse = 0;
      semTable[i].name = NULL;
      semTable[i].queue = NULL;
  }

  // Semaphores for interrupt handlers
  P1_SemCreate("clockDevSemaphore", 0, (P1_Semaphore) clockDevSemaphore);
  P1_SemCreate("alarmDevSemaphore", 0, (P1_Semaphore) alarmDevSemaphore);
  for(i = 0; i < USLOSS_TERM_UNITS; i++){
    char str[30];
    sprintf(str, "terminalSemaphore%d", i);
    P1_SemCreate(str, 0, (P1_Semaphore) terminalDevSemaphores[i]);
  }
  for(i=0; i < USLOSS_DISK_UNITS; i++) {
    char str[30];
    sprintf(str, "diskSemaphore%d", i);
    P1_SemCreate(str, 0, (P1_Semaphore) diskDevSemaphores[i]);
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
  disableInterrupts();
    if (tag != 0 && tag != 1) {
        enableInterrupts();
        return -4;
    }
    if (priority < 0 || priority > 5) {
        if (priority == 6 && strcmp(name, "sentinel") == 0) {
          ;
        } else {
          enableInterrupts();
          return -3;
        }
    }
    if (stacksize < USLOSS_MIN_STACK) {
        enableInterrupts();
        return -2;
    }
    if(name == NULL || f == NULL){
      USLOSS_Console("Error, process name or function is not set");
      USLOSS_Halt(1);
    }
    /*
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
      enableInterrupts();
      return -1;
    }
    numProcs++;
    procTable[newPid].pid = newPid;
    procTable[newPid].priority = priority;
    procTable[newPid].state = 1;
    procTable[newPid].start_func = f;
    procTable[newPid].startArg = arg;
    procTable[newPid].name = strdup(name);
    
    stack0 = malloc(stacksize);
    procTable[newPid].stack = stack0;
    procTable[newPid].stacksize = stacksize;
    if (procTable[newPid].sem != NULL) {
      P1_SemFree(procTable[newPid].sem);
    }
    P1_SemCreate(name, 0, (P1_Semaphore) procTable[newPid].sem);


    // more stuff here, e.g. allocate stack, page table, initialize context, etc.
    USLOSS_ContextInit(&(procTable[newPid].context), procTable[newPid].stack, procTable[newPid].stacksize, P3_AllocatePageTable, launch);

    addToPriorityQueue(procTable[newPid].pid,&readyQueue,priority);
    procTable[newPid].status = 1;
    if(currentpid >= 0) {
      procTable[currentpid].numOfChildren++;
      procTable[newPid].parentID = currentpid;
    }
    if(priority < procTable[currentpid].priority) {
      procHold = 1;
      dispatcher();
    }
    enableInterrupts();
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
*/

int P1_Join (int tag, int *status) {
  if ((USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()) == 0 ){
      USLOSS_Console("Error, not in kernel mode.");
      USLOSS_Halt(1);
  }
  disableInterrupts();
  if (procTable[currentpid].numOfChildren <= 0) {
    enableInterrupts();
    return -1;
  }
  P1_P(procTable[currentpid].sem);

  //get killed child off queue
  PCB *dead = PopAQueue(&(procTable[currentpid].killedChildren));
  int deadPID = dead->pid;
  procTable[deadPID].status = -1;
  free(procTable[deadPID].name);
  enableInterrupts();
  return deadPID;
}


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
  disableInterrupts();
  numProcs--;
  procTable[currentpid].status = 3;
  procTable[currentpid].numOfChildren = 0;
  procTable[currentpid].CPU_uptime = 0;
  //update child count for parent
  if(procTable[currentpid].parentID >= 0) {
    procTable[procTable[currentpid].parentID].numOfChildren--;
  }
  //orphanate the children
  int i;
  for (i=0; i < P1_MAXPROC; i++) {
    if (procTable[i].parentID == currentpid) {
      procTable[i].parentID = -1;
      if(procTable[i].status = 3) {
        procTable[i].status = -1;
        free(procTable[i].name);
      }
    }
  }

  if (procTable[currentpid].parentID=-1){
    procTable[currentpid].status = -1;
    procTable[currentpid].parentID = -1;
    free(procTable(procTable[currentpid].name));
  } else {
    int pPID = procTable[currentpid].parentID;
    //add to queue of killed children
    addToQueue(currentpid, &(procTable[pPID].killedChildren));
    while (procTable[currentpid].killedChildren != NULL) {
      //clear the killed children
      PopAQueue(&(procTable[currentpid].killedChildren));
    }
    procTable[currentpid].parentID = -1;
    P1_V(procTable[pPID].sem);
  }
  enableInterrupts();
  currentpid = -1;
  dispatcher();
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
    if(procTable[i].pid == PID) {
      return procTable[i].status;
    }
  }
  return -1;
}

int P1_GetPID() {
  return currentpid;
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
          USLOSS_Console("State: %d\n", procTable[i].status);
          USLOSS_Console("ParentPID: %d\n", procTable[i].parentID);
          USLOSS_Console("Num of children: %d\n", procTable[i].numOfChildren);
          USLOSS_Console("CPU Time: %d\n", procTable[i].CPU_uptime);
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
  Enable/Disable the interrupts
*/
static void disableInterrupts() {
  USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}
static void enableInterrupts() {
  USLOSS_PsrSet(USLOSS_PsrGet() & ~(USLOSS_PSR_CURRENT_INT));
}

/*
    Add a process to a Priority Queue
*/
void addToPriorityQueue(int pid, queueNode **queue, int priority){
  disableInterrupts();
  queueNode *new = malloc(sizeof(queueNode));
  new->next = NULL;
  new->pcb = &procTable[pid];
  if(*queue == NULL){
    *queue = new;
    enableInterrupts();
    return;
  }
  
  if((*queue)->pcb->priority > priority){
    new.next = *queue;
    *queue = new;
    enableInterrupts();
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
  enableInterrupts();
}

void addToQueue (int pid, queueNode **queue) {
  disableInterrupts();
  queueNode *new = malloc(sizeof(queueNode));
  new->pcb = procTable[pid];
  new->next = NULL;
  if (*queue == NULL) {
    *queue = new;
    enableInterrupts();
    return;
  }
  queueNode *temp = *queue;
  while (temp->next != NULL) {
    temp = temp->NULL;
  }
  temp->next = new;
  enableInterrupts();
}

PCB * PopAQueue (queueNode **queue) {
  disableInterrupts();
  PCB *ret;
  queueNode *temp = *head;
  *head = (*head)->next;
  ret = temp->pcb;
  free temp;
  enableInterrupts();
  return ret;
}

int P1_SemCreate (char *name; unsigned int value, P1_Semaphore *sem) {
  disableInterrupts();
  if (name == NULL) {
    enableInterrupts();
    return -3;
  }
  int i;
  for(i=0; i < usedSemaphores; i++) {
    if(strcmp(name, semTable[i].name) == 0) {
      enableInterrupts();
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
      usedSemaphores++;
      sem = (P1_Semaphore) semTable[i];
      enableInterrupts();
      return 0;
    }
  }
  enableInterrupts();
  return -2
}

int P1_SemFree(P1_Semaphore sem) {
  disableInterrupts();
  if( checkInvaildSemaphore(sem) ) {
    enableInterrupts();
    return -1;
  }
  Semaphore *s = (Semaphore *) sem;
  if (s->queue != NULL) {
    enableInterrupts();
    return -2;
  }
  s->inUse = 0;
  s->count = 0;
  free(sem->name);
  enableInterrupts();
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
    disableInterrupts();
    if (s->count > 0) {
      s->count--;
      break;
    }
    // add to queue
    addToQueue(currentpid, &(s->queue));
    // block process
    procTable[currentpid].blocked = 1;
    procTable[currentpid].status = 4;
    // pass reference to sem to process
    procTable[currentpid].blockedSem = sem;
    enableInterrupts();
    procHold = 0;
    dispatcher();
  }
  //unblock process
  procTable[currentpid].blocked = 0;
  //dereference sem from process
  procTable[currentpid].blockedSem = NULL;
  enableInterrupts();
  return 0;
}

int P1_V(P1_Semaphore sem) {
  if ( checkInvaildSemaphore() ) {
      return -1;
  }
  Semaphore *s = (Semaphore *) sem;
  disableInterrupts();
  s->count++;
  if (s->queue != NULL) {
    //get process off semaphore queue
    PCB *p = PopAQueue(&(s->queue));
    //add process to ready queue;
    p->status = 1;
    addToPriorityQueue(p->pid, &readyQueue, p->priority);
    enableInterrupts();
    procHold = 1;
    dispatcher();
  } else {
    enableInterrupts();
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

/*
  --------------------------
  P1_ReadTime
  --------------------------
*/
int P1_ReadTime(void) {
  int finTime = USLOSS_Clock();
  return (procTable[currentpid].CPU_uptime + (finTime - procTable[currentpid].startTime));
}

/*
  P1_WaitDevice
*/
int P1_WaitDevice(int type, int unit, int *status) {
  
  disableInterrupts();

  /*Cannot wait on a device if this process is killed.*/
  if(P1_GetState(currentpid) == 3){
    enableInterrupts();
    return -3;
  }

  if(unit < 0){
    enableInterrupts();
    return -1;
  }

  if(type == USLOSS_CLOCK_DEV){
    if(unit == 0){
      P1_P(clockDevSemaphore);
      *status = clockDevStatus;
    }else{
      enableInterrupts();
      return -1;
    }
  }else if(type == USLOSS_ALARM_DEV){
    if(unit == 0){
      P1_P(alarmDevSemaphore);
      *status = alarmDevStatus;
    }else{
      enableInterrupts();
      return -1;
    }
  }else if(type == USLOSS_DISK_DEV){
    if(unit >= 0 && unit < USLOSS_DISK_UNITS){
      P1_P(diskDevSemaphores[unit]);
      *status = diskDevStatus[unit];
    }else{
      enableInterrupts();
      return -1;
    }
  }else if(type == USLOSS_TERM_DEV){
    if(unit >= 0 && unit < USLOSS_TERM_UNITS){
      P1_P(terminalDevSemaphores[unit]);
      *status = terminalDevStatus[unit];
    }else{
      enableInterrupts();
      return -1;
    }
  }else{
    enableInterrupts();
    return -2;
  }
  enableInterrupts();
  return 0;
}

/*
  --------------------------
  Interrupt handlers 
  --------------------------
*/
void clockIntHandler(int type, void *arg) {
  clockSum++;
  clockTicks++;
  if (clockSumTicks == 5) {
    USLOSS_DeviceInput(USLOSS_CLOCK_DEV,0,&clockDevStatus);
    P1_V(clockDevSemaphore);
    clockSumTicks = 0;
  }
  if (clockIntTicks == 4) {
    clockIntTicks = 0;
    procHold = 1;
    dispatcher();
  }
}

void countDownIntHandler(int type, void *arg) {
  USLOSS_DeviceInput(USLOSS_ALARM_DEV,0,&alarmDevStatus);
  P1_V(alarmDevSemaphore);
}
void terminalIntHandler(int type, void *arg) {
  int unit = (int) arg;
  USLOSS_DeviceInput(USLOSS_TERM_DEV,unit,&(terminalDevStatus[unit]));
  P1_V(terminalDevSemaphores[unit]);
}

void diskIntHandler(int type, void *arg) {
  int unit = (int) arg;
  USLOSS_DeviceInput(USLOSS_DISK_DEV,unit,&(diskDevStatus[unit]));
  P1_V(diskDevSemaphores[unit]);
}

void MMUIntHandler(int type, void *arg) {
  return;
}