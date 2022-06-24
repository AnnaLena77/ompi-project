#include <sys/queue.h>
//#define ENABLE_ANALYSIS 1

extern void enqueue(char** operation, char** datatype, int count, int datasize, char** communicator, int processrank, int partnerrank, time_t ctime);
extern void initialize(void);
extern pthread_t MONITOR_THREAD;

#ifndef QENTRY_H_
#define QENTRY_H_
typedef struct qentry {
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
    int processrank;
    int partnerrank;
    char sendmode[30]; //later
    int immediate; //later
    char usedBtl[30];
    char usedProtocol[30];
    int withinEagerLimit;
    int foundMatchWild;
    char usedAlgorithm[30];
    time_t start;
    time_t initializeRequest;
    time_t startRequest;
    //Completion of the first fragment of a long message that requires an acknowledgement
    time_t requestCompletePmlLevel;
    //Warten auf Recv-Request
    time_t requestWaitCompletion;
    time_t requestFini;
    time_t sent;//later
    time_t bufferFree; //later
    time_t intoQueue;
    TAILQ_ENTRY(qentry) pointers;
} qentry;
#endif

extern void qentryIntoQueue(qentry **q);
extern void initQentry(qentry **q);




