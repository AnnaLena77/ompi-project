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
#include "ompi/request/request.h"
#include "ompi/mca/pml/ob1/pml_ob1_comm.h"
#include "opal/class/opal_free_list.h"
#include "ompi/mpi/c/init.h"
#include "ompi/mca/pml/base/pml_base_sendreq.h"


#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Init = PMPI_Init
#endif
#define MPI_Init PMPI_Init
#endif

//#define SAMPLING 10

typedef struct qentry {
    char* operation;
    char* datatype;
    int count;
    int datasize;
    char* communicator;
    int processrank;
    int partnerrank;
    time_t start;
    TAILQ_ENTRY(qentry) pointers;
} qentry;

static counter=0;
static TAILQ_HEAD(, qentry) head;
//sem_t ENSURE_INIT;

#ifdef SAMPLING
    static int samplecount = 1;
    static int samplerandom;
#endif

void testi(struct mca_pml_base_send_request_t* request){
    //ompi_mpi_object_t req_object = t->req_mpi_object;
    //ompi_communicator_t *comm = req_object.comm; 
    //mca_pml_ob1_comm_t *comm_ob1 = (mca_pml_ob1_comm_t *)comm->c_pml_comm;
    //opal_free_list_item_t list_item = (opal_free_list_item_t)t->super;
    //opal_list_item_t item = (opal_list_item_t)list_item.super;
    
    printf("%s\n", request->req_addr);
}

void enqueue(char** operation, char** datatype, int count, int datasize, char** communicator, int processrank, int partnerrank, time_t ctime){
    //printf("Operation: %s\n", *operation);
    //printf("Hier der Communicator: %s\n", *communicator);
    //printf("Hier der Type: %s\n", *datatype);
    //printf("Current Time: %ld \n", ctime);
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
    qentry *item = (qentry*)malloc(sizeof(qentry));
    item->operation = strdup(*operation);
    item->datatype = strdup(*datatype);
    item->count = count;
    item->datasize = datasize;
    item->communicator = strdup(*communicator);
    item->processrank = processrank;
    item->partnerrank = partnerrank;
    item->start = ctime;
    TAILQ_INSERT_TAIL(&head, item, pointers);
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

static const int LIMIT = 10;
static int last_one = 0;
static int count = LIMIT;
static char *batchstring = "INSERT INTO MPI_Data(operation, datatype, count, datasize, communicator, processrank, partnerrank, timestamp_into_queue)VALUES";

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

static void collectData(qentry **item, char **batchstr){
    qentry *q = *item;
    int countlen = (q->count==0)?1:(int)log10(q->count)+1;
    int datasizelen = (q->datasize==0)?1:(int)log10(q->datasize)+1;
    int processranklen = (q->processrank==0)?1:(int)log10(q->processrank)+1;
    int partnerranklen = (q->partnerrank==0)?1:(int)log10(q->partnerrank)+1;
    //Timestamp into right format
    char *timestamp=(char*)malloc(20);
    struct tm tm = *localtime(&q->start);
    sprintf(timestamp, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    //Speicherplatz für alle Einträge als Char + 3* '' für die Chars + 6* , und Leertaste + ()
    int datalen = strlen(q->operation) + strlen(q->datatype) + countlen + datasizelen + strlen(q->communicator) + processranklen + 
                  partnerranklen + strlen(timestamp) + 4*2 + 7*2 +2 +1;
    //printf("Datalen: %d\n", datalen);
    char *data=(char*)malloc(datalen+1);
    sprintf(data, "('%s', '%s', %d, %d, '%s', %d, %d, '%s'),", q->operation, q->datatype, q->count, q->datasize, q->communicator, 
            q->processrank, q->partnerrank, timestamp);
    *batchstr = realloc(*batchstr, strlen(*batchstr)+1 + strlen(data)+1);
    strcat(*batchstr, data);
    free(data);
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
    qentry *item = (qentry*)malloc(sizeof(qentry));
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
	    free(item);
            //printf("%d\n", item->data);
        }
    }
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
    	if(mysql_query(conn, "DELETE FROM MPI_Data")){
        	fprintf(stderr, "%s\n", mysql_error(conn));
        	exit(1);
    	}
    	if(mysql_query(conn, "ALTER TABLE MPI_Data AUTO_INCREMENT=1")){
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
    //There should only be the main thread in the process
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
    //Defined in ompi_mpi_init.c 
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
    	//Funktion aus \ompi\errhandler\errhandler_invoke.c
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
