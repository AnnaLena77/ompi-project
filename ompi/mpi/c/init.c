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
#include <libpq-fe.h>
#endif

#include "opal/util/show_help.h"
#include "ompi/runtime/ompi_spc.h"
#include "ompi/mpi/c/bindings.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/constants.h"
//#include "ompi/mpi/c/init.h"


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
char proc_name[MPI_MAX_PROCESSOR_NAME];
int proc_name_length;

static FILE *file;
static char filename[20];
static int fd;
static char* mapped_data;
static int file_size;
static int illi=1;

qentry *q_qentry;

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

void initQentry(qentry **q){
    if(q==NULL || *q==NULL){
    	return;
    } else {
        qentry *item = *q;
        item->id = 0;
        memcpy(item->function, "", 0);
        item->blocking = -1;
        memcpy(item->datatype, "", 0);
        item->count = 0;
        item->sendcount = 0;
        item->recvcount = 0;
        item->datasize = 0;
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
        item->start = (struct timeval){0};
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
    //gettimeofday(&item->intoQueue, NULL);
    item->id = ++ID;
    //printf("intoqueue: %d \n", item->id);
    if(item->id==2) lock = 1;
    TAILQ_INSERT_TAIL(&head, item, pointers);
    //printf("Es kam was rein\n");
    queue_length++;
    //printf("Queue Length: %d\n", queue_length);
}

qentry* dequeue(){
    qentry *item;
    item = TAILQ_FIRST(&head);
    TAILQ_REMOVE(&head, item, pointers);
    return item;
}

//Needs to be global!
pthread_t MONITOR_THREAD = NULL;
int run_thread = 1;

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

static void writeToPostgres(PGconn *conn, int numberOfEntries){
    PGresult *res;
    int i;
    int totalWritten = 0;
    printf("Funktionsaufruf Test writeToPostgres, NumberOfEntries: %d\n", numberOfEntries);

    // Erzeuge den COPY-Befehl
    const char *copyQuery = "COPY MPI_Information(function, communicationType, count, datasize, communicationArea, processorname, processrank, partnerrank, time_start, time_db) FROM STDIN (FORMAT text)";
    res = PQexec(conn, copyQuery);
    if (PQresultStatus(res) != PGRES_COPY_IN) {
        printf("Fehler beim Starten des COPY-Befehls: %s\n", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }
    PQclear(res);

    // Schreibe die Daten in das COPY-Stream
    for (i = 0; i < numberOfEntries; i++) {
        //printf("DEQUEUE\n");
        qentry *q = dequeue();
        //printf("DEQUEUE FERTIG\n");
        char buffer[1000]; // Puffer für den Text        
        char buffer2[30];
        createTimeString(q->start, buffer2);
        snprintf(buffer, sizeof(buffer), "%s\t%s\t%d\t%d\t%s\t%s\t%d\t%d\t%s\tNOW()\n",
                 q->function, q->communicationType, q->count, q->datasize, q->communicationArea,
                 q->processorname, q->processrank, q->partnerrank, buffer2);
        //printf("%s\n", buffer);
        PQputCopyData(conn, buffer, strlen(buffer));
        free(q);
    }
    // Beende den COPY-Befehl
    PQputCopyEnd(conn, NULL);
    PQflush(conn);

    // Warte auf das Ergebnis der COPY-Operation
    /*PGresult *copyResult = NULL;
    while ((copyResult = PQgetResult(conn)) != NULL) {
        ExecStatusType status = PQresultStatus(copyResult);
        if (status == PGRES_COMMAND_OK) {
        // INSERT erfolgreich
            printf("Daten erfolgreich in die Datenbank geschrieben.\n");
        } else {
        // Fehler beim INSERT
            printf("Fehler beim Schreiben der Daten in die Datenbank: %s\n", PQresultErrorMessage(copyResult));
        }
    PQclear(copyResult);
    }*/
}

//Monitor-Function for SQL-Connection
static void* SQLMonitorFunc(void* _arg){
    //Batchstring für den ersten Eintrag vorbereiten
    char* conninfo = "host=10.35.8.10 port=5432 dbname=tsdb user=postgres password=postgres";
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK){
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    } else {
        //printf("Database connected\n");
        if(!PQexec(conn, "DELETE FROM MPI_Information")){
            fprintf(stderr, "Remove Data failed: %s\n", PQerrorMessage(conn));
            //PQfinish(conn);
            //exit(1);
        }
        /*if(PQexec(conn, "ALTER TABLE MPI_Information AUTO_INCREMENT=1")){
            fprintf(stderr, "Auto-Inocrement failed: %s\n", PQerrorMessage(conn));
            //PQfinish(conn);
            //exit(1);
        }*/
    } 
    
    qentry *item;
    int finish = 0;
    struct timeval start_timestamp;
    struct timeval now;
    gettimeofday(&start_timestamp, NULL);

    while(TAILQ_EMPTY(&head)){
        if(!run_thread){
            printf("finished\n");
            return;
        } else {
	   sleep(0.5);
            if(!TAILQ_EMPTY(&head)){
                break;
            }
        }
    }

    while ((TAILQ_LAST(&head, tailhead))->id < 5){
    	gettimeofday(&now, NULL);
    	sleep(0.5);
    	if(timeDifference(now, start_timestamp)>2.0){
    		break;
    	}
    }
    
    qentry *queueiteration = TAILQ_FIRST(&head);
    qentry *prev;

    while(queueiteration != NULL){
        gettimeofday(&now, NULL);
        //printf("in der Queueiteration-While\n");
        if(timeDifference(now, start_timestamp)>1){
    	     qentry *first = queueiteration;
    	     qentry *last = TAILQ_LAST(&head, tailhead);
    	     //printf("First ID: %d\n", first->id);
    	     //printf("Last ID: %d\n", last->id);
    	     if(first->id == last->id){
                  if(run_thread){
                     continue;
                  }
                  else {
                      //Es ist noch genau ein Eintrag übrig und das Programm ist fertig
                      //printf("letzter Eintrag, thread ist durch\n");
                      last = TAILQ_LAST(&head, tailhead);
                      if(first->id == last->id){
                         writeToPostgres(conn, 1);
                         break;
                      }
                 }
             }
    	
    	    //printf("First-ID: %d, Last-ID: %d\n", first->id, last->id);
    	    int length = last->id-first->id;
    	    //printf("Length: %d\n", length);
    	    if(length<100000){
    	        writeToPostgres(conn, length);
    	        gettimeofday(&start_timestamp, NULL);
    		
            } else {
                while(length>100000){
                    length = length-100000;
                    writeToPostgres(conn, 100000);
                    gettimeofday(&start_timestamp, NULL);
                }
                writeToPostgres(conn, length);
                gettimeofday(&start_timestamp, NULL);
           } 
           
         }
         
         while(TAILQ_EMPTY(&head)){
             printf("empty am ende\n");
             if(run_thread){
                 continue;
             } else {
                 queueiteration = TAILQ_FIRST(&head);
                 break;
             }
         }
         queueiteration = TAILQ_FIRST(&head);
    }
   // printf("Length: %d\n", queue_length);
}

void writeIntoFile(qentry **q){
    if(q==NULL || *q==NULL){
    	return;
    } else {
        qentry *item = *q;
        
        int offset = 0;
        char buffer[500];
        
        int func_len = strlen(item->function);
        memcpy(buffer, item->function, func_len);
        offset += func_len;
        buffer[offset] = ',';
        offset ++;
        
        int comm_type_len = strlen(item->communicationType);
        memcpy(buffer + offset, item->communicationType, comm_type_len);
        offset += comm_type_len;
        buffer[offset] = ',';
        offset ++;
        
        memcpy(buffer+offset, (char *)&item->count, sizeof(item->count));
        offset ++;
        buffer[offset] = ',';
        offset ++;
        
        memcpy(buffer+offset, (char *)&item->datasize, sizeof(item->datasize));
        offset ++;
        buffer[offset] = ',';
        offset ++;
        
        printf("test4\n");
        
        int comm_area_len = strlen(item->communicationArea);
        memcpy(buffer + offset, item->communicationArea, comm_area_len);
        offset += comm_area_len;
        buffer[offset] = ',';
        offset ++;
        
        printf("test5\n");
        
        int procname_len = strlen(item->processorname);
        memcpy(buffer + offset, item->processorname, procname_len);
        offset += procname_len;
        buffer[offset] = ',';
        offset ++;
        
        printf("test6\n");
        
        memcpy(buffer+offset, (char *)&item->processrank, sizeof(item->processrank));
        offset ++;
        buffer[offset] = ',';
        offset ++;
        
        printf("test7\n");
        memcpy(buffer+offset, (char *)&item->partnerrank, sizeof(item->partnerrank));
        offset ++;
        buffer[offset] = '\n';
        
        printf("test8\n");
        
        printf("%s", buffer);
        
        
        
        
        
        //char buffer2[30];
        //createTimeString(item->start, buffer2);
        /*snprintf(buffer, sizeof(buffer), "%s\t%s\t%d\t%d\t%s\t%s\t%d\t%d\tNOW()\n",
                 item->function, item->communicationType, item->count, item->datasize, item->communicationArea,
                 item->processorname, item->processrank, item->partnerrank);*/
        //memcpy(mapped_data, test, strlen(test));
        //mapped_data += strlen(test);
        //fwrite(buffer, 1, strlen(buffer), file);
    }
}

void closeFile(){
    // Synchronisiere die Daten auf die Festplatte
    /*if (msync(mapped_data, file_size, MS_SYNC) == -1) {
        perror("msync");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Schließe die Abbildung und die Datei
    if (munmap(mapped_data, file_size) == -1) {
        perror("munmap");
        close(fd);
        exit(EXIT_FAILURE);
    }*/
    free(q_qentry);
    close(fd);
}

void initializeQueue()
{ 
    //gettimeofday(&init_sql_start, NULL);
    TAILQ_INIT(&head);
    
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &processrank);
    MPI_Get_processor_name(proc_name, &proc_name_length);
    
    sprintf(filename, "./data_rank_%d.csv", processrank);
    
    file = fopen(filename, "w");
    
    fprintf(file, "function,communicationType,count,datasize,communicationArea,processorname,processrank,partnerrank,time_start,time_db\n");
    
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
    //pthread_create(&MONITOR_THREAD, NULL, SQLMonitorFunc, NULL);
    //gettimeofday(&init_sql_finished, NULL);
    //float dif = timeDifference(init_sql_finished, init_sql_start);
    //printf("Lost time for initializing sql: %f\n", dif);
}

#endif
static const char FUNC_NAME[] = "MPI_Init";
int MPI_Init(int *argc, char ***argv)
{
    q_qentry = (qentry*)malloc(sizeof(qentry));
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
