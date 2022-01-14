//#define ENABLE_ANALYSIS 1;
extern void enqueue(char** operation, char** datatype, int count, int datasize, char** communicator, int processrank, int partnerrank, time_t ctime);
extern void initialize(void);
extern pthread_t MONITOR_THREAD;


