#include <sys/queue.h>
#include <time.h>
#include <sys/time.h>
//#define ENABLE_ANALYSIS 1

extern void enqueue(char** operation, char** datatype, int count, int datasize, char** communicator, int processrank, int partnerrank, time_t ctime);
extern void initializeMongoDB(void);
extern void closeMongoDB(void);
extern pthread_t MONITOR_THREAD;
extern int run_thread;

#ifndef QENTRY_H_
#define QENTRY_H_
typedef struct qentry {
    int id;
    char function[30];
    char communicationType[30];
    int blocking;
    char datatype[30];
    int count;
    int sendcount;
    int recvcount;
    int datasize;
    char operation[30]; //MPI_Reduce, MPI_Accumulate
    char communicationArea[30];
    char processorname[30];
    int processrank;
    int partnerrank;
    char sendmode[30]; //later
    int immediate; //later
    char usedBtl[30];
    char usedProtocol[30];
    int withinEagerLimit;
    int foundMatchWild;
    char usedAlgorithm[30];
    struct timeval start;
    struct timeval initializeRequest;
    struct timeval startRequest;
    //Completion of the first fragment of a long message that requires an acknowledgement
    struct timeval requestCompletePmlLevel;
    //Warten auf Recv-Request
    struct timeval requestWaitCompletion;
    struct timeval requestFini;
    struct timeval sent;//later
    struct timeval bufferFree; //later
    struct timeval intoQueue;
    TAILQ_ENTRY(qentry) pointers;
} qentry;
#endif

extern void qentryIntoQueue(qentry **q);
extern void initQentry(qentry **q);




