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
#include <mongoc/mongoc.h>
#include <bson/bson.h>

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

//#define SQL 1
#define MongoDB 1

//samplerate pro Minute!
//#define SAMPLING 300
static struct timeval start;


static ID=0;
static count_q_entry = 0;
static int queue_lock;
static TAILQ_HEAD(tailhead, qentry) head;
//static TAILQ_HEAD(, qentry) helper_head;
static int queue_length=0;
static int lock = 0;
//sem_t ENSURE_INIT;

//Database Information
static MYSQL *conn;
static char *server = "10.35.8.10";
static char *user = "test";
static char *password = "test_pwd";
static char *database = "DataFromMPI";
static char *port = "3306";

static const char *uri_string = "mongodb://10.35.8.10:27017";
//static const char *uri_string = "mongodb://localhost:27017";
static mongoc_uri_t *uri;
static mongoc_client_t *client;
static mongoc_database_t *db;
static mongoc_collection_t *collection;
static bson_t *command, reply, *insert;
static bson_error_t error;
static char *str;
static bool retval;

static int count_qentries=0;

float timeDifference(struct timeval a, struct timeval b){
    float seconds = a.tv_sec-b.tv_sec;
    float microseconds = (a.tv_usec-b.tv_usec)*0.000001;
    return seconds+microseconds;
}

void initQentry(qentry **q){
    if(q==NULL || *q==NULL){
    	return;
    } else {
        qentry *item = *q;
        item->id = 0;
        strcpy(item->function, "");
        item->blocking = -1;
        strcpy(item->datatype, "");
        item->count = 0;
        item->sendcount = 0;
        item->recvcount = 0;
        item->datasize = 0;
        strcpy(item->operation, "");
        strcpy(item->communicationArea, "");
        strcpy(item->processorname, "");
        item->processrank = -1;
        item->partnerrank = -1;
        strcpy(item->sendmode, "");
        item->immediate = 0;
        strcpy(item->usedBtl, "");
        strcpy(item->usedProtocol, "");
        item->withinEagerLimit = -1;
        item->foundMatchWild = -1;
        strcpy(item->usedAlgorithm, "");
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

qentry* dequeue(){
    qentry *item;
    item = TAILQ_FIRST(&head);
    TAILQ_REMOVE(&head, item, pointers);
    return item;
}

//Needs to be global!
pthread_t MONITOR_THREAD = NULL;
int run_thread = 1;

//static const int LIMIT = 200;
//static int count = LIMIT;
static int last_one = 0;
static char *batchstring = "INSERT INTO MPI_Information(function, communicationType, blocking, datatype, count, sendcount, recvcount, datasize, operation, communicationArea, processorname, processrank, partnerrank, sendmode, immediate, usedBtl, usedProtocol, withinEagerLimit, foundMatchWild, usedAlgorithm, time_start, time_initializeRequest, time_startRequest, time_requestCompletePmlLevel, time_requestWaitCompletion, time_requestFini, time_sent, time_bufferFree, time_intoQueue)VALUES";

static void insertData(char **batchstr){
    printf("InsertData: n");
    //count = LIMIT;
    char *batch = *batchstr;
    batch[strlen(batch)-1]=';';
    //printf("%s\n", batch);
    if(mysql_query(conn, batch)){
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
}

static void createTimeString(struct timeval time, char* timeString){
    if(time.tv_usec==NULL){
    	sprintf(timeString, "NULL");
    }
    else {
    	struct tm tm = *localtime(&time.tv_sec);
    	sprintf(timeString, "'%d-%02i-%02i %02i:%02i:%02i.%06li'", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, time.tv_usec);   
    }
}

void qentryIntoQueue(qentry **q){
    //printf("IntoQueue: %d\n", count_q_entry++);
    //printf("Echtes Samplerandom: %d\n",samplerandom);
    //if(time(NULL)-counter_time>0.1){ }
    qentry *item = *q;
    gettimeofday(&item->intoQueue, NULL);
    item->id = ++ID;
    //printf("intoqueue: %d \n", item->id);
    if(item->id==2) lock = 1;
    TAILQ_INSERT_TAIL(&head, item, pointers);
    queue_length++;
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
    int datalen = strlen(item->function) + strlen(item->communicationType) + blockinglen + strlen(item->datatype) + countlen + sendcountlen + recvcountlen + datasizelen + strlen(item->operation) + strlen(item->communicationArea) + strlen(item->processorname) + processranklen + partnerranklen + strlen(item->sendmode) + immediatelen + strlen(item->usedBtl) + strlen(item->usedProtocol) + withinEagerLimitlen + foundMatchWildlen + strlen(item->usedAlgorithm)+ timestampslen*9 + 20*2 + 28*2 +2 +1;     
    //printf("Datalen: %d\n", datalen);
    char *data=(char*)malloc(datalen+1);
    
    sprintf(data, "('%s', '%s', %d, '%s', %d, %d, %d, %d, '%s', '%s', '%s', %d, %d, '%s', '%d', '%s', '%s', %d, %d, '%s', %s, %s, %s, %s, %s, %s, %s, %s, %s),", item->function, item->communicationType, item->blocking, item->datatype, item->count, item->sendcount, item->recvcount, item->datasize, item->operation, item->communicationArea, item->processorname, item->processrank, item->partnerrank, item->sendmode, item->immediate, item->usedBtl, item->usedProtocol, item->withinEagerLimit, item->foundMatchWild, item->usedAlgorithm, time_start, time_initializeRequest, time_startRequest, time_requestCompletePmlLevel , time_requestWaitCompletion, time_requestFini, time_sent, time_bufferFree, time_intoQueue);
    
    free(time_start);
    free(time_initializeRequest);
    free(time_startRequest);
    free(time_requestCompletePmlLevel);
    free(time_requestWaitCompletion);
    free(time_requestFini);
    free(time_sent);
    free(time_bufferFree);
    free(time_intoQueue);
    //free(*q);
    //*q  = NULL;
    //qentry *test = *q;
    //printf("%s\n", test->operation);
    
    *batchstr = realloc(*batchstr, strlen(*batchstr)+1 + strlen(data)+1);
    strcat(*batchstr, data);
    free(data);
   
    //printf("%s\n", *batchstr);
    /*if(time(NULL)-counter_time>0.1 || last_one){
	char *batch = *batchstr;
	insertData(&batch);
	time(&counter_time);
	*batchstr = realloc(*batchstr, strlen(batchstring)+1);
	strcpy(*batchstr, batchstring);
	last_one=0;
    }*/
}

static int generateRandom(int l, int r){
    return (rand() % (r - l + 1)) + l;

}

static bson_t * generateBson(qentry **q){
    qentry *item = *q;
    /*bson_t *document = BCON_NEW(
    	"function", 
    	"communicationType", 
    	"blocking",
    	"datatype", 
    	"count", 
    	"sendcount", 
    	"recvcount" , 
    	"datasize" , 
    	"operation" , 
    	"communicationArea" , 
    	"processorname" , 
    	"processrank", 
    	"partnerrank", 
    	"sendmode", 
    	"immediate", 
    	"usedBtl", 
    	"usedProtocol", 
    	"withinEagerLimit", 
    	"foundMatchWild", 
    	"usedAlgorithm", 
    	"time_start", 
    	"time_initializeRequest", 
    	"time_startRequest", 
    	"time_requestCompletePmlLevel", 
    	"time_requestWaitCompletion", 
    	"time_requestFini", 
    	"time_sent", 
    	"time_bufferFree", 
    	"time_intoQueue"
    );*/
    
    bson_t *document = bson_new();
    
    bson_append_utf8 (document, "function", -1, item->function, -1);
    bson_append_int32 (document, "ident", -1, item->id);
    bson_append_utf8 (document, "communicationType", -1, item->communicationType, -1);
    bson_append_bool (document, "blocking", -1, item->blocking);
    bson_append_utf8 (document, "datatype", -1, item->datatype, -1);
    bson_append_int32 (document, "count", -1, item->count);
    bson_append_int32 (document, "sendcount", -1, item->sendcount);
    bson_append_int32 (document, "recvcount", -1, item->recvcount);
    bson_append_int32 (document, "datasize", -1, item->datasize);
    bson_append_utf8 (document, "operation", -1, item->operation, -1);
    bson_append_utf8 (document, "communicationArea", -1, item->communicationArea, -1);
    bson_append_utf8 (document, "processorname", -1, item->processorname, -1);
    bson_append_int32 (document, "processrank", -1, item->processrank);
    bson_append_int32 (document, "partnerrank", -1, item->partnerrank);
    bson_append_utf8 (document, "sendmode", -1, item->sendmode, -1);
    bson_append_bool (document, "immediate", -1, item->immediate);
    bson_append_utf8 (document, "usedBTL", -1, item->usedBtl, -1);
    bson_append_utf8 (document, "usedProtocol", -1, item->usedProtocol, -1);
    bson_append_bool (document, "withinEagerLimit", -1, item->withinEagerLimit);
    bson_append_bool (document, "foundMatchWild", -1, item->foundMatchWild);
    bson_append_utf8 (document, "usedAlgorithm", -1, item->usedAlgorithm, -1);  
    bson_append_timeval(document, "start", -1, &item->start);
    bson_append_timeval(document, "initializeRequest", -1, &item->initializeRequest);
    bson_append_timeval(document, "startRequest", -1, &item->startRequest);
    bson_append_timeval(document, "requestCompletePmlLevel", -1, &item->requestCompletePmlLevel);
    bson_append_timeval(document, "requestWaitCompletion", -1, &item->requestWaitCompletion);
    bson_append_timeval(document, "requesrFini", -1, &item->requestFini);
    bson_append_timeval(document, "sent", -1, &item->sent);
    bson_append_timeval(document, "bufferFree", -1, &item->bufferFree);
    bson_append_timeval(document, "intoQueue", -1, &item->intoQueue);
    
    return document;
}

static void* MongoMonitorFunc(void* _arg){
    bson_oid_t oid;
    mongoc_bulk_operation_t *bulk;

    while(TAILQ_EMPTY(&head)){
    	sleep(0.1);
    }
    while ((TAILQ_LAST(&head, tailhead))->id < 5){
    	sleep(0.1);
    }
    
    qentry *queueiteration = TAILQ_FIRST(&head);
    qentry *prev;
    

    
    while(queueiteration != NULL){
    	//if(queueiteration->id == 6) break;
        bson_error_t error;
    	qentry *first = queueiteration;
    	qentry *last = TAILQ_LAST(&head, tailhead);
    	if(first->id == last->id){
            if(run_thread){
                continue;
            }
            else {
                bson_t *lastdoc = generateBson(&last);
                if (!mongoc_collection_insert_one (collection, (const bson_t*) lastdoc, NULL, NULL, &error)){ 
                    fprintf (stderr, "%s\n", error.message);
                }
            }
        }
    	
    	bson_t *doc;
    	bson_t reply;
    	
    	//printf("First-ID: %d, Last-ID: %d\n", first->id, last->id);
    	
    	int length = last->id-first->id + 1;
    	bson_t *documents[length];
    	int index = 0;
       	
    	while(index<length){
    	    documents[index] = generateBson(&first);
    	    //doc = generateBson(&first);
    	    //printf("Size: %d", sizeof(doc));
    	    //mongoc_bulk_operation_insert(bulk, doc);
    	    //bson_destroy(doc);
    	    first = TAILQ_NEXT(first, pointers);
    	    index++;
    	}
    	
    	/*if (!mongoc_bulk_operation_execute(bulk, &reply, &error)) {
            fprintf (stderr, "%s\n", error.message);
        }*/
        
        //mongoc_bulk_operation_destroy(bulk);
        //bulk = mongoc_collection_create_bulk_operation(collection, true, NULL);

       if (!mongoc_collection_insert_many (collection, (const bson_t**) documents, length, NULL, NULL, &error)) {
            fprintf (stderr, "%s\n", error.message);
        }
        
        //bson_destroy(documents);
        
        prev = last;
        queueiteration = TAILQ_NEXT(last, pointers);
        while(queueiteration == NULL && run_thread){
            sleep(0.5);
            //printf("PROBLEM!\n");
            qentry *lastone = TAILQ_LAST(&head, tailhead);
            if(prev->id != lastone->id){
                queueiteration = TAILQ_NEXT(prev, pointers);
            }
        }
    }
   // printf("Length: %d\n", queue_length);
}


//Monitor-Function for SQL-Connection
static void* SQLMonitorFunc(void* _arg){
    qentry *item;
    //counter_time = time(NULL);
    //#ifdef SQL
    char *batch = (char*) malloc(strlen(batchstring)+1);
    strcpy(batch, batchstring);
    int finish = 0;
    struct timeval current_time;
    while(!lock){
    	sleep(0.001);
    }
    
    qentry *firstOne = TAILQ_FIRST(&head);
    qentry *lastOne;
    
    //printf("into_thread\n");
    
    #ifdef SAMPLING
    while(firstOne != NULL){
         gettimeofday(&current_time, NULL);
	if(timeDifference(current_time, start)>0.1){
	    gettimeofday(&start, NULL);
	    lastOne = TAILQ_LAST(&head, tailhead);
	    //Anzahl der Items, die zu diesem Zeitpunkt in der Queue stehen
	    int quantity = lastOne->id - firstOne->id+1;
	    //aus allen "steps" items soll eines in die Datenbank
	    int steps = quantity/SAMPLING;
	    //Der nicht teilbare Rest, der untergebracht werden muss
	    int rest = lastOne->id - SAMPLING*steps - firstOne->id+1;
	    //Item, um durch die Items zu iterieren
	    qentry *q = firstOne;
	    int i = 1;
	    qentry *prev;
	    if(q->id==lastOne->id){
	        break;
	    }
	    while(q->id < lastOne->id){
	        int l = q->id;
	        int r;
	        if(rest>0){
	            r = q->id + steps;
	    	   rest --;
	        }
	        else {
	    	   r = q->id + steps-1;
	        }
	        int random = generateRandom(l,r);
	        while(q->id <= r){
	            if(q->id == random){
	    	       collectData(&q, &batch);
	            }
	            prev = q;
	            q = TAILQ_NEXT(q, pointers);
	            if(q==NULL) break;
	        }	
	        if(q==NULL) break;
	    }
	    if(strcmp(batch, batchstring) != 0){
	    	insertData(&batch);
	    	realloc(batch, strlen(batchstring)+1);
		strcpy(batch, batchstring);
	    }
	    firstOne = q;
	    if(firstOne == NULL){
	        if(TAILQ_NEXT(prev, pointers)!=NULL){
	            firstOne = TAILQ_NEXT(prev, pointers);
	        }
	        else {
	        	   closeMongoDB();
	        }
	    }
         }
    }
    #else
    int counter = 0;
    while(firstOne != NULL){
        qentry *previousq;
        qentry *iterateq = firstOne;
        lastOne = TAILQ_LAST(&head, tailhead);
        
        gettimeofday(&current_time, NULL);
        if(timeDifference(current_time, start)>0.2){
	   gettimeofday(&start, NULL);
	   while(iterateq->id <= lastOne->id){
	       counter++;
	       collectData(&iterateq, &batch);
	       iterateq = TAILQ_NEXT(iterateq, pointers);
	       if(counter>4000){
	           if(strcmp(batch, batchstring) != 0){
	               insertData(&batch);
	    	      realloc(batch, strlen(batchstring)+1);
		      strcpy(batch, batchstring);
                    }
                    counter = 0;
                }
                if(iterateq == NULL) break;   
	   }
        }
        if(iterateq == NULL){
        	   firstOne = TAILQ_NEXT(lastOne, pointers);
        	   if(firstOne==NULL){
        	   	printf("firstOne wurde NULL");
        	         /*if(strcmp(batch, batchstring) != 0){
            	    insertData(&batch);
	             realloc(batch, strlen(batchstring)+1);
	             strcpy(batch, batchstring);
                  }*/
            	break;
            }
        } else {
            firstOne = iterateq;
        }
    }
    #endif
    free(batch);
}

static const char FUNC_NAME[] = "MPI_Init";


void initializeSQL()
{
    conn = mysql_init(NULL);
    //if Proxy should be used: Port 6033 after database
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
    pthread_create(&MONITOR_THREAD, NULL, SQLMonitorFunc, NULL);
}


void initializeMongoDB()
{
    //Initialize libmongo's internals
    mongoc_init();
    
    uri = mongoc_uri_new_with_error (uri_string, &error);
    
    //Client-Instance
    client = mongoc_client_new_from_uri(uri);
    if (!client) {
        return EXIT_FAILURE;
    }
    
    //register application name for profile logs on the server
    mongoc_client_set_appname (client, "EduMPI-Connection");
    
    //handle on database and collection
    db = mongoc_client_get_database (client, "DataFromMPI");
    collection = mongoc_client_get_collection (client, "DataFromMPI", "MPI_Information");
	if(!mongoc_collection_drop (collection, &error)) {
	    //fprintf (stderr, "%s\n", error.message);
	    
    }
    
    TAILQ_INIT(&head);
    printf("Connected to MongoDB\n");
    pthread_create(&MONITOR_THREAD, NULL, MongoMonitorFunc, NULL);
}

void closeMongoDB()
{
    //Release handles
    mongoc_collection_destroy (collection);
    mongoc_database_destroy (db);
    mongoc_uri_destroy (uri);
    mongoc_client_destroy (client);
    mongoc_cleanup ();

}


int MPI_Init(int *argc, char ***argv)
{
    gettimeofday(&start, NULL);
    //printf("Test 1\n");
    
    #ifdef SQL
    	initializeSQL();
    #endif
    #ifdef ENABLE_ANALYSIS
    	initializeMongoDB();
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
    	printf("TEST\n");
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
