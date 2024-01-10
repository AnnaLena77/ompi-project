#include <sys/queue.h>
#include <time.h>
#include <sys/time.h>

extern void enqueue(char** operation, char** datatype, int count, int datasize, char** communicator, int processrank, int partnerrank, time_t ctime);
extern void initializeMongoDB(void);
extern void closeMongoDB(void);
extern pthread_t MONITOR_THREAD;
extern int run_thread;
extern int counter;

#ifndef QENTRY_H_
#define QENTRY_H_
typedef struct qentry {
    int id;
    char function[30];
    char communicationType[30];
    int blocking;
    char datatype[64];
    int sendcount;
    int sendDatasize;
    int recvcount;
    int recvDatasize;
    char operation[30]; //MPI_Reduce, MPI_Accumulate
    char communicationArea[64];
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
    struct timespec start;
    struct timespec end;
    //struct timeval start;
    struct timespec initializeRequest;
    struct timespec startRequest;
    //Completion of the first fragment of a long message that requires an acknowledgement
    struct timespec requestCompletePmlLevel;
    //Warten auf Recv-Request
    struct timespec requestWaitCompletion;
    struct timespec requestFini;
    struct timespec sent;//later
    struct timespec bufferFree; //later
    struct timespec intoQueue;
    //struct collective_p2p collectives;
    TAILQ_ENTRY(qentry) pointers;
} qentry;
#endif

#ifndef collective_p2p_H
#define collective_p2p_H
typedef struct {
    char function[30];
    int count;
    int datasize;
    int processrank;
    int partnerrank;
    struct timespec start;
} collective_p2p;
#endif

extern void qentryIntoQueue(qentry **q);
extern void initQentry(qentry **q);
//extern void writeIntoFile(qentry q);

extern qentry *q_qentry;

extern qentry* getWritingRingPos(void);

#ifndef INIT_H
#define INIT_H
#define MAX_RINGSIZE 1000000
extern qentry *ringbuffer;
extern int writer_pos;
extern int reader_pos;
#endif

extern void closeFile(void);



