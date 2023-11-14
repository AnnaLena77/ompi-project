/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2018 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2018 Cisco Systems, Inc.  All rights reserved
 * Copyright (c) 2007-2008 Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <math.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/mman.h>

#ifdef ENABLE_ANALYSIS
#include <postgresql/libpq-fe.h>
#endif

#include "opal/util/show_help.h"
#include "ompi/runtime/ompi_spc.h"
#include "ompi/mpi/c/bindings.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/constants.h"
#include "ompi/mpi/c/init.h"
#include "ompi/mpi/c/pgcopy.h"


#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Init = PMPI_Init
#endif
#define MPI_Init PMPI_Init
#endif

#ifdef ENABLE_ANALYSIS

static struct timeval start;
static struct timeval init_sql_finished;
static struct timeval init_sql_start;

static ID=0;
static count_q_entry = 0;
static int queue_lock;
static TAILQ_HEAD(tailhead, qentry) head;
static int queue_length=0;
static int lock = 0;
//sem_t ENSURE_INIT;

static int size, processrank;
static char processrank_arr[4];
char proc_name[MPI_MAX_PROCESSOR_NAME];
int proc_name_length;

static FILE *file;
static char filename[20];
static char filename[20];
static char* mapped_data;
static int file_size;
static int illi=1;

static int count_before = 0;
static int partner_before = 0;
static char count_before_arr[10];
static char partner_before_arr[10];

qentry *q_qentry;
qentry *ringbuffer;
int writer_pos;
int reader_pos;

float timeDifference(struct timeval a, struct timeval b){
    float seconds = a.tv_sec-b.tv_sec;
    float microseconds = (a.tv_usec-b.tv_usec)*0.000001;
    return seconds+microseconds;
}

static void createTimeString(struct timeval time, char* timeString){
    if(time.tv_usec==NULL){
        if(time.tv_sec==NULL){
            sprintf(timeString, "NULL");
        } else {
            struct tm tm = *localtime(&time.tv_sec);
           sprintf(timeString, "'%d-%02i-%02i %02i:%02i:%02i'", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
    }
    else {
        struct tm tm = *localtime(&time.tv_sec);
        sprintf(timeString, "'%d-%02i-%02i %02i:%02i:%02i.%06li'", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, time.tv_usec);
    }
}

static char *getTimeString(struct timespec time){
    time_t seconds = time.tv_sec;
    long nanoseconds = time.tv_nsec;
    if(seconds==0 && nanoseconds==0){
        return NULL;
    }
    
    struct tm local_time;
    localtime_r(&seconds, &local_time);
    char *timestamp = (char*)malloc(100);
    
    return timestamp;
}

void initQentry(qentry **q){
    if(q==NULL || *q==NULL){
    	return;
    } else {
        qentry *item = *q;
        item->id = 0;
        memcpy(item->function, "", 0);
        item->blocking = -1;
        memcpy(item->datatype, "", 0);
        item->sendcount = 0;
        item->sendDatasize = 0;
        item->recvcount = 0;
        item->recvDatasize = 0;
        memcpy(item->operation, "", 0);
        memcpy(item->communicationArea, "", 0);
        memcpy(item->processorname, proc_name, proc_name_length);
        item->processrank = processrank;
        item->partnerrank = -1;
        memcpy(item->sendmode, "", 0);
        item->immediate = 0;
        memcpy(item->usedBtl, "", 0);
        memcpy(item->usedProtocol, "", 0);
        item->withinEagerLimit = -1;
        item->foundMatchWild = -1;
        memcpy(item->usedAlgorithm, "", 0);
        //item->start = (struct timeval){0};
        item->initializeRequest = (struct timeval){0};
        item->startRequest = (struct timeval){0};
        item->requestCompletePmlLevel = (struct timeval){0};
        item->requestWaitCompletion = (struct timeval){0};
        item->requestFini = (struct timeval){0};
        item->sent = (struct timeval){0};        
        item->bufferFree = (struct timeval){0};
        item->intoQueue = (struct timeval){0};
    }
}

void qentryIntoQueue(qentry **q){
    //printf("IntoQueue: %d\n", count_q_entry++);
    //printf("Echtes Samplerandom: %d\n",samplerandom);
    //if(time(NULL)-counter_time>0.1){ }
    qentry *item = *q;
    //printf("%s\n", item->function);
    //gettimeofday(&item->intoQueue, NULL);
    item->id = ++ID;
    //printf("intoqueue: %d \n", item->id);
    TAILQ_INSERT_TAIL(&head, item, pointers);
    //printf("Es kam was rein\n");
    queue_length++;
    //printf("Queue Length: %d\n", queue_length);
    //printf("Leer? %d\n", TAILQ_EMPTY(&head));
}

qentry* dequeue(){
    qentry *item;
    item = TAILQ_FIRST(&head);
    if(item != NULL){
        TAILQ_REMOVE(&head, item, pointers);
    }
    return item;
}

//Needs to be global!
pthread_t MONITOR_THREAD = NULL;
int run_thread;
int counter;

static void registerCluster(){
    //Hier Slurm integrieren!
    
    MPI_Comm comm = MPI_COMM_WORLD;
    int size, processrank;
    char processorname[30];
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &processrank);
    
    int proc_name_length;
    MPI_Get_processor_name(processorname, &proc_name_length);
}

static void writeToPostgres(PGconn *conn, char* buffer, int offset){
    counter ++;
    const char *copyQuery = "COPY mpi_information FROM STDIN (FORMAT binary)";
    PGresult *res;
    res = PQexec(conn, copyQuery);
    if (PQresultStatus(res) != PGRES_COPY_IN) {
        printf("Fehler beim Starten des COPY-Befehls: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        PQfinish(conn);
        return;
    }
    PQclear(res);
    PQputCopyData(conn, PGCOPY_HEADER, 19);
    
    int copyRes = PQputCopyData(conn, buffer, offset);
    // Beende den COPY-Befehl
    if(copyRes == 1){
        if(PQputCopyEnd(conn, NULL)==1){
            res = PQgetResult(conn);
            if(PQresultStatus(res) == PGRES_COMMAND_OK){
                printf("Insert a record successfully\n");
            }
        }
    }
    PQflush(conn);
    
}

void qentryToBinary(qentry q, char *buffer, int *off){
        counter ++;
        //printf("Counter: %d\n", counter);
        qentry *item = &q;
        int offset = *off;
        
        newRow(buffer, 9, &offset);

        stringToBinary(item->function, buffer, &offset);
        
        stringToBinary(item->communicationType, buffer, &offset);
        
        intToBinary(item->sendDatasize, buffer, &offset);
        
        intToBinary(item->recvDatasize, buffer, &offset);
        
        stringToBinary(item->communicationArea, buffer, &offset);
        
        stringToBinary(item->processorname, buffer, &offset);
        
        intToBinary(item->processrank, buffer, &offset);
        
        intToBinary(item->partnerrank, buffer, &offset);

        timestampToBinary(item->start, buffer, &offset);
        
        *off = offset;
        //fwrite(buffer, offset, 1, file);
}

#define TIME_TO_WAIT 0.5

//Monitor-Function for SQL-Connection
static void* SQLMonitorFunc(void* _arg){
    
    char* conninfo = "host=10.35.8.10 port=5432 dbname=tsdb user=postgres password=postgres";
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK){
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    } else {
       char proc_rank[3];
       sprintf(proc_rank, "%d", processrank);
       const char *query = "INSERT INTO cluster_information (processorname, rank) VALUES ($1, $2)";
       const char *paramValues[2] = {proc_name, proc_rank};
       PGresult *res = PQexecParams(conn, query, 2, NULL, paramValues, strlen(proc_name)+strlen(proc_rank), NULL, 0);
    }
    
    clock_t start = clock();
    
    const char *copyQuery = "COPY mpi_information FROM STDIN (FORMAT binary)";
    PGresult *res;
    res = PQexec(conn, copyQuery);
    if (PQresultStatus(res) != PGRES_COPY_IN) {
        printf("Fehler beim Starten des COPY-Befehls: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        PQfinish(conn);
        return;
    }
    PQclear(res);
    PQputCopyData(conn, PGCOPY_HEADER, 19);
    
   // int copyRes = PQputCopyData(conn, buffer, offset);
    //int offset = 0;
    //char *buffer = (char*)malloc(sizeof(char)*20000000);

    while(run_thread){
        while(writer_pos == -1 || reader_pos==writer_pos-1 || (reader_pos == MAX_RINGSIZE-1 && writer_pos == 0)){
            //printf("Reader sleeps at Position %d, %d\n", reader_pos, writer_pos);
            sleep(0.01);
            //printf("Prozess: %d haengt beim Lesen\n", processrank);
            if(!run_thread) break;
        }
        
        if(reader_pos == MAX_RINGSIZE-1){
            reader_pos = 0;
        }
        else{
            reader_pos ++;
        }
        
        char buffer[200];
        int offset = 0;
        qentryToBinary(ringbuffer[reader_pos], buffer, &offset);
        PQputCopyData(conn, buffer, offset);
        
        clock_t current = clock();
        if(current>=(start + TIME_TO_WAIT * CLOCKS_PER_SEC)){
            PQputCopyEnd(conn, NULL);
            PQflush(conn);
            res = PQexec(conn, copyQuery);
            PQclear(res);
            PQputCopyData(conn, PGCOPY_HEADER, 19);
            
            start = current;
        }
        
        //printf("Reader: %d, Writer: %d\n", reader_pos, writer_pos);
        
        /*while(TAILQ_EMPTY(&head)){
            if(!run_thread){
                printf("finished\n");
                return;
            } else {
	       sleep(0.1);
                if(!TAILQ_EMPTY(&head)){
                    break;
                }
            }
        }
        
        qentry* test = TAILQ_FIRST(&head);
        
        while(test != NULL){
            //printf("%s\n", test->function);  
            char buffer[200];
            int offset = 0;
            qentryToBinary(&test, buffer, &offset);
        
            //PQputCopyData(conn, buffer, offset);
            while(TAILQ_NEXT(test, pointers)==NULL){
                if(!run_thread){
                    test = NULL;
                    break;
                } 
            }
            qentry *test2 = test;
            test = TAILQ_NEXT(test2, pointers);
            free(test2);
        }*/
        
        /*time_t dif = time(NULL) - start;
        if(offset>=19990000 || dif>500000){
            //fwrite(buffer, offset, 1, file);
            //writeToPostgres(conn, buffer, offset);
            offset = 0;
            memset(buffer, 0, 20000000);
            start = time(NULL);
        }*/
    }
    
    while(reader_pos != writer_pos){
        if(reader_pos == MAX_RINGSIZE-1){
            reader_pos = 0;
        } else {
            reader_pos ++;
        }
        char buffer[200];
        int offset = 0;
        qentryToBinary(ringbuffer[reader_pos], buffer, &offset);
        PQputCopyData(conn, buffer, offset);
        //PQputCopyData(conn, buffer, offset);
        //time_t dif = time(NULL)-start;
        /*if(offset>=19990000 || dif>500000){
            fwrite(buffer, offset, 1, file);
            //writeToPostgres(conn, buffer, offset);
            offset = 0;
            memset(buffer, 0, 20000000);
            start = time(NULL);
        }*/
        clock_t current = clock();
        if(current>=(start + TIME_TO_WAIT * CLOCKS_PER_SEC)){
            PQputCopyEnd(conn, NULL);
            PQflush(conn);
            
            res = PQexec(conn, copyQuery);
            PQclear(res);
            PQputCopyData(conn, PGCOPY_HEADER, 19);
            
            start = current;
        }
    }
    PQputCopyEnd(conn, NULL);
    PQflush(conn);
    PQfinish(conn);
    //fwrite(buffer, offset, 1, file);
    //writeToPostgres(conn, buffer, offset);
}

struct timeval start_time, end_time;
float total_time;

qentry* getWritingRingPos(){

    if(writer_pos == -1){
        writer_pos ++;
        return &ringbuffer[0];
    }
    
    //Ringbuffer ist voll, es müssen erst Items ausgelesen werden
    //gettimeofday(&start_time, NULL);
    while(writer_pos == reader_pos){
        printf("Writer sleeps at Position %d, %d\n", writer_pos, reader_pos);
        sleep(0.01);
    }
    //gettimeofday(&end_time, NULL);
    //total_time += timeDifference(end_time, start_time);
    //Letzte Position im Ring-Buffer erreicht
    
    if(writer_pos == MAX_RINGSIZE-1){
        writer_pos = 0;
    }
    //Solange Writer vor dem Reader kann nichts passieren
    else{
       writer_pos++;
       //printf("writing\n");
    }
    return &ringbuffer[writer_pos];
}

char* createIntArray(int count){
    char buffer_help[8];
    int i = 0;
    while(count!=0){
        int rem = count%10;
        buffer_help[i++] = (rem>9)?(rem-10)+'a':rem+'0';
        count = count / 10;
    }
    buffer_help[i] = '\0';
    char *buffer_help2 = (char*) malloc(i-1);
    int x = 0;
    while(i>0){
        i--;
        buffer_help2[x] = buffer_help[i];
        x++;
    }
    buffer_help2[x] = '\0';
    return buffer_help2;
}


void closeFile(){
    //fwrite(PGCOPY_TRAILER, 2, 1, file);  
    //close(file);
    printf("Time: %f\n", total_time);
    printf("Counter: %d\n", counter);
}

void openFile(){
    char filename[20];
    sprintf(filename, "./data_rank_%d.bin", processrank);
    file = fopen(filename, "wb");
    fwrite(PGCOPY_HEADER, 19, 1, file);
}

void initializeQueue()
{ 
    //gettimeofday(&init_sql_start, NULL);
    TAILQ_INIT(&head);
    
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &processrank);
    MPI_Get_processor_name(proc_name, &proc_name_length);
    
    
    //fprintf(file, "function,communicationType,count,datasize,communicationArea,processorname,processrank,partnerrank,time_start,time_db\n");
    
    //fd = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    //O_CREAT: Datei wird erstellt, wenn nicht vorhanden
    //O_WRONLY: Es darf nur in die Datei geschrieben werden
    //S_IRUSR | S_IWUSR: Eigentümer darf Datei lesen und schreiben (Permissions)
    
    /*if(fd == -1){
    	perror("open");
    	exit(EXIT_FAILURE);
    }*/
    
    //file_size = 6*20000000; // Größe der Datei (kann angepasst werden)
    
    // Ändere die Größe der Datei auf file_size Bytes
    /*if (ftruncate(fd, file_size) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    mapped_data = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }*/
    
    //fclose(file);
    pthread_create(&MONITOR_THREAD, NULL, SQLMonitorFunc, NULL);
    //gettimeofday(&init_sql_finished, NULL);
    //float dif = timeDifference(init_sql_finished, init_sql_start);
    //printf("Lost time for initializing sql: %f\n", dif);
}

#endif
static const char FUNC_NAME[] = "MPI_Init";
int MPI_Init(int *argc, char ***argv)
{
    run_thread = 1;
    counter = 0;
    ringbuffer = (qentry*)malloc(sizeof(qentry)*MAX_RINGSIZE);
    writer_pos = -1;
    reader_pos = -1;
    q_qentry = (qentry*)malloc(sizeof(qentry));
    //buffer = (char*)malloc(2000000);
    //offset = 0;
    #ifdef ENABLE_ANALYSIS
    gettimeofday(&start, NULL);
    #endif
    int err;
    int provided;
    char *env;
    int required = MPI_THREAD_SINGLE;

    /* check for environment overrides for required thread level.  If
       there is, check to see that it is a valid/supported thread level.
       If not, default to MPI_THREAD_MULTIPLE. mmap example c*/

    if (NULL != (env = getenv("OMPI_MPI_THREAD_LEVEL"))) {
        required = atoi(env);
        if (required < MPI_THREAD_SINGLE || required > MPI_THREAD_MULTIPLE) {
            required = MPI_THREAD_MULTIPLE;
        }
    }
    //printf("Test 2\n");

    /* Call the back-end initialization function (we need to put as
       little in this function as possible so that if it's profiled, we
       don't lose anything) */

    if (NULL != argc && NULL != argv) {
        err = ompi_mpi_init(*argc, *argv, required, &provided, false);
    } else {
        err = ompi_mpi_init(0, NULL, required, &provided, false);
    }
    //printf("Test 3\n");

    /* Since we don't have a communicator to invoke an errorhandler on
       here, don't use the fancy-schmancy ERRHANDLER macros; they're
       really designed for real communicator objects.  Just use the
       back-end function directly. */

    if (MPI_SUCCESS != err) {
        return ompi_errhandler_invoke(NULL, NULL,
                                      OMPI_ERRHANDLER_TYPE_COMM,
                                      err <
                                      0 ? ompi_errcode_get_mpi_code(err) :
                                      err, FUNC_NAME);
    }
    #ifdef ENABLE_ANALYSIS
    	initializeQueue();
    #endif
    //printf("hier gehts weiter\n");
    SPC_INIT();
    return MPI_SUCCESS;
}
