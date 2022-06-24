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
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <math.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <mysql/mysql.h>

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

//#define SAMPLING 15

static counter=0;
static TAILQ_HEAD(, qentry) head;
//sem_t ENSURE_INIT;

#ifdef SAMPLING
    static int samplecount = 1;
    static int samplerandom;
#endif

void initQentry(qentry **q){
    if(q==NULL || *q==NULL){
    	return 0;
    } else {
        qentry *item = *q;
        strcpy(item->function, "");
        item->blocking = -1;
        strcpy(item->datatype, "");
        item->count = 0;
        item->sendcount = 0;
        item->recvcount = 0;
        item->datasize = 0;
        strcpy(item->operation, "");
        strcpy(item->communicationArea, "");
        item->processrank = -1;
        item->partnerrank = -1;
        strcpy(item->sendmode, "");
        item->immediate = 0;
        strcpy(item->usedBtl, "");
        strcpy(item->usedProtocol, "");
        item->withinEagerLimit = -1;
        item->foundMatchWild = -1;
        strcpy(item->usedAlgorithm, "");
        item->start = 0;
        item->initializeRequest = 0;
        item->startRequest = 0;
        item->requestCompletePmlLevel = 0;
        item->requestWaitCompletion = 0;
        item->requestFini = 0;
        item->sent = 0;
        item->bufferFree = 0;
        item->intoQueue = 0;
    }
}

qentry* dequeue(){
    qentry *item;
    item = TAILQ_FIRST(&head);
    TAILQ_REMOVE(&head, item, pointers);
    return item;
}

//Needs to be global!
pthread_t MONITOR_THREAD = NULL;

//Database Information
static MYSQL *conn;
static char *server = "192.168.42.9";
static char *user = "AnnaLena";
static char *password = "annalena";
static char *database = "DataFromMPI";

static const int LIMIT = 200;
static int last_one = 0;
static int count = LIMIT;
static char *batchstring = "INSERT INTO MPI_Information(function, communicationType, blocking, datatype, count, sendcount, recvcount, datasize, operation, communicationArea, processrank, partnerrank, sendmode, immediate, usedBtl, usedProtocol, withinEagerLimit, foundMatchWild, usedAlgorithm, time_start, time_initializeRequest, time_startRequest, time_requestCompletePmlLevel, time_requestWaitCompletion, time_requestFini, time_sent, time_bufferFree, time_intoQueue)VALUES";

static void insertData(char **batchstr){
    count = LIMIT;
    char *batch = *batchstr;
    batch[strlen(batch)-1]=';';
    //printf("%s\n", batch);
    if(mysql_query(conn, batch)){
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
}

static void createTimeString(time_t time, char* timeString){
    if(time==NULL){
    	sprintf(timeString, "NULL");
    }
    else {
    	struct tm tm = *localtime(&time);
    	struct timeval timestamp;
    	gettimeofday(&timestamp, &time);
    	sprintf(timeString, "'%d-%02i-%02i %02i:%02i:%02i.%06li'", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, timestamp.tv_usec);   
    }
}

void qentryIntoQueue(qentry **q){
#ifdef SAMPLING
    //printf("%d\n", samplerandom);
    if(samplecount==1){
        samplecount = SAMPLING;
        samplerandom=rand()%SAMPLING+1;
        //printf("Berechnetes Samplerandom: %d\n",samplerandom);
    }
    else{
        samplecount--;
    }
    if(samplecount!=samplerandom){
    	 //printf("Samplecount = %d, Samplerandom = %d\n", samplecount, samplerandom);
    	 return;
    }
#endif
//printf("Echtes Samplerandom: %d\n",samplerandom);
    qentry *item = *q;
    item->intoQueue = time(NULL);
    TAILQ_INSERT_TAIL(&head, item, pointers);
}

static void collectData(qentry **q, char **batchstr){
    qentry *item = *q;
    int countlen = (item->count==0 || item->count == -1)?1:(int)log10(item->count)+1;
    int sendcountlen = (item->sendcount==0 || item->sendcount == -1)?1:(int)log10(item->sendcount)+1;
    int recvcountlen = (item->recvcount==0 || item->recvcount == -1)?1:(int)log10(item->recvcount)+1;
    int datasizelen = (item->datasize==0 || item->datasize==-1)?1:(int)log10(item->datasize)+1;
    int processranklen = (item->processrank==0 || item->processrank==-1)?1:(int)log10(item->processrank)+1;
    int partnerranklen = (item->partnerrank==0 || item->partnerrank==-1)?1:(int)log10(item->partnerrank)+1;
    int immediatelen = 1;
    int blockinglen = 1;
    int withinEagerLimitlen =1;
    int foundMatchWildlen=1;
    int timestampslen = 29;
  
    //Timestamps into right format
    char *time_start=(char*)malloc(29);
    char *time_initializeRequest=(char*)malloc(29);
    char *time_startRequest=(char*)malloc(29);
    char *time_requestCompletePmlLevel=(char*)malloc(29);
    char *time_requestWaitCompletion=(char*)malloc(29);
    char *time_requestFini=(char*)malloc(29);
    char *time_sent=(char*)malloc(29);
    char *time_bufferFree=(char*)malloc(29);
    char *time_intoQueue=(char*)malloc(29);
  
    createTimeString(item->start, time_start);
    createTimeString(item->initializeRequest, time_initializeRequest);
    createTimeString(item->startRequest, time_startRequest);
    createTimeString(item->requestCompletePmlLevel, time_requestCompletePmlLevel);
    createTimeString(item->requestWaitCompletion, time_requestWaitCompletion);
    createTimeString(item->requestFini, time_requestFini);
    createTimeString(item->sent, time_sent);
    createTimeString(item->bufferFree, time_bufferFree);
    createTimeString(item->intoQueue, time_intoQueue);

    //Speicherplatz für alle Einträge als Char + 3* '' für die Chars + 6* , und Leertaste + ()
    int datalen = strlen(item->function) + strlen(item->communicationType) + blockinglen + strlen(item->datatype) + countlen + sendcountlen + recvcountlen + datasizelen + strlen(item->operation) + strlen(item->communicationArea) + processranklen + partnerranklen + strlen(item->sendmode) + immediatelen + strlen(item->usedBtl) + strlen(item->usedProtocol) + withinEagerLimitlen + foundMatchWildlen + strlen(item->usedAlgorithm)+ timestampslen*9 + 19*2 + 28*2 +2 +1;     
    //printf("Datalen: %d\n", datalen);
    char *data=(char*)malloc(datalen+1);
    
    sprintf(data, "('%s', '%s', %d, '%s', %d, %d, %d, %d, '%s', '%s', %d, %d, '%s', '%d', '%s', '%s', %d, %d, '%s', %s, %s, %s, %s, %s, %s, %s, %s, %s),", item->function, item->communicationType, item->blocking, item->datatype, item->count, item->sendcount, item->recvcount, item->datasize, item->operation, item->communicationArea, item->processrank, item->partnerrank, item->sendmode, item->immediate, item->usedBtl, item->usedProtocol, item->withinEagerLimit, item->foundMatchWild, item->usedAlgorithm, time_start, time_initializeRequest, time_startRequest, time_requestCompletePmlLevel , time_requestWaitCompletion, time_requestFini, time_sent, time_bufferFree, time_intoQueue);
    
    free(time_start);
    free(time_initializeRequest);
    free(time_startRequest);
    free(time_requestCompletePmlLevel);
    free(time_requestWaitCompletion);
    free(time_requestFini);
    free(time_sent);
    free(time_bufferFree);
    free(time_intoQueue);
    free(*q);
    *q  = NULL;
    //qentry *test = *q;
    //printf("%s\n", test->operation);
    
    *batchstr = realloc(*batchstr, strlen(*batchstr)+1 + strlen(data)+1);
    strcat(*batchstr, data);
    free(data);
    
    //printf("%s\n", *batchstr);
    count--;
    if(count==0 || last_one){
	char *batch = *batchstr;
	insertData(&batch);
	*batchstr = realloc(*batchstr, strlen(batchstring)+1);
	strcpy(*batchstr, batchstring);
	last_one=0;
    }
}

static void* MonitorFunc(void* _arg){
    qentry *item;
    char *batch=(char*) malloc(strlen(batchstring)+1);
    strcpy(batch, batchstring);
    int finish = 0;
    while(!finish){
        if(TAILQ_EMPTY(&head)){
            sleep(1);
            if(TAILQ_EMPTY(&head)){
                finish = 1; 
            }
        }
        else {
            item = dequeue();
            if(TAILQ_EMPTY(&head)){
            	last_one=1;
            }
            collectData(&item, &batch);
            //printf("%d\n", item->data);
        }
    }
    //printf("out of loop\n");
    free(batch);
}

static const char FUNC_NAME[] = "MPI_Init";

void initialize()
{
    conn = mysql_init(NULL);
    if(!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)){
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    else {
    	if(mysql_query(conn, "DELETE FROM MPI_Information")){
        		fprintf(stderr, "%s\n", mysql_error(conn));
        		exit(1);
    	}
    	if(mysql_query(conn, "ALTER TABLE MPI_Information AUTO_INCREMENT=1")){
    		fprintf(stderr, "%s\n", mysql_error(conn));
        		exit(1);
    	}
    }
    
    TAILQ_INIT(&head);
    pthread_create(&MONITOR_THREAD, NULL, MonitorFunc, NULL);
}


int MPI_Init(int *argc, char ***argv)
{

    //printf("Test 1\n");
    #ifdef SAMPLING
        srand(time(NULL));
        samplerandom=rand()%100+1;
    #endif
    
    #ifdef ENABLE_ANALYSIS
    	initialize();
    #endif
    int err;
    int provided;
    char *env;
    int required = MPI_THREAD_SINGLE;

    /* check for environment overrides for required thread level.  If
       there is, check to see that it is a valid/supported thread level.
       If not, default to MPI_THREAD_MULTIPLE. */

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
    //printf("Test 4\n");

    SPC_INIT();
    //printf("Test 5\n");
    return MPI_SUCCESS;
}
