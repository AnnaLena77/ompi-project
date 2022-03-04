#include <sys/queue.h>
#define ENABLE_ANALYSIS 1;

extern void enqueue(char** operation, char** datatype, int count, int datasize, char** communicator, int processrank, int partnerrank, time_t ctime);
extern void initialize(void);
extern pthread_t MONITOR_THREAD;

#ifndef QENTRY_H_
#define QENTRY_H_
typedef struct qentry {
    char* operation;
    char* sendmode; //later
    int blocking;
    int immediate; //later
    char* datatype;
    int count;
    int datasize;
    char* communicator;
    int processrank;
    int partnerrank;
    time_t start;
    time_t initializeRequest;
    time_t startRequest;
    time_t requestWaitCompletion;
    time_t requestFini;
    time_t sent;//later
    time_t bufferFree; //later
    TAILQ_ENTRY(qentry) pointers;
} qentry;
#endif

extern void qentryIntoQueue(qentry **q);

