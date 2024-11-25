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
#include "ompi/mpi/c/pgcopy.h"

#ifdef ENABLE_ANALYSIS
#include "ompi/mpi/c/init.h"
#include "ompi/peruse/peruse.h"
#endif


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
static char *job_id;
static struct timespec start_runtime;
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

double timespec_diff(struct timespec start, struct timespec end){
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    return (double)elapsed_ns / 1e9;
}

const char* get_mpi_op_name(MPI_Op op) {
    if (op == MPI_SUM) {
        return "MPI_SUM";
    } else if (op == MPI_MAX) {
        return "MPI_MAX";
    } else if (op == MPI_MIN) {
        return "MPI_MIN";
    } else if (op == MPI_PROD) {
        return "MPI_PROD";
    } else if (op == MPI_LAND) {
        return "MPI_LAND";
    } else if (op == MPI_BAND) {
        return "MPI_BAND";
    } else if (op == MPI_LOR) {
        return "MPI_LOR";
    } else if (op == MPI_BOR) {
        return "MPI_BOR";
    } else if (op == MPI_LXOR) {
        return "MPI_LXOR";
    } else if (op == MPI_BXOR) {
        return "MPI_BXOR";
    } else {
        return "UNKNOWN_OP";
    }
}

void convert_MPI_datatype(MPI_Datatype type, char *datatype_field){
    if(type == MPI_INT) {
        memcpy(datatype_field, "MPI_INT", 7);
    } else if(type == MPI_CHAR){
        memcpy(datatype_field, "MPI_CHAR", 8);
    } else if (type == MPI_DOUBLE){
        memcpy(datatype_field, "MPI_DOUBLE", 10);
    } else if (type == MPI_LONG){
        memcpy(datatype_field, "MPI_LONG", 8);
    } else {
        //int type_name_length;
        //MPI_Type_get_name(type, datatype_field, &type_name_length);
        //datatype_field[type_name_length] = '\0'; 
    }
}

void initQentry(qentry **q, int dest, char *function, int function_len, int sendCount, int recvCount, char *commType, int commType_len, MPI_Datatype sendType, MPI_Datatype recvType, MPI_Comm comm, int blocking, MPI_Op op){
    if(q==NULL || *q==NULL){
    	return;
    } else {
        qentry *item = *q;
        item->id = 0;
        memcpy(item->function, function, function_len);
        item->blocking = blocking;
        
        if(sendType == NULL){
            memcpy(item->sendDatatype, "", 0);
        } else {
            convert_MPI_datatype(sendType, item->sendDatatype);
        }
        if(recvType == NULL){
            memcpy(item->recvDatatype, "", 0);
        } else {
            convert_MPI_datatype(recvType, item->recvDatatype);
        }
        
        memcpy(item->communicationType, commType, commType_len);
        
        item->sendcount = sendCount;
        item->sendDatasize = sendCount * sizeof(sendType);
        item->recvcount = recvCount;
        item->recvDatasize = recvCount * sizeof(recvType);
        
        if(op == NULL){
            memcpy(item->operation, "", 0);
        } else {
            memcpy(item->operation, get_mpi_op_name(op), 10);
        }
        
        if(comm == NULL){
            memcpy(item->communicationArea, "", 0);
        }
        else if(comm == MPI_COMM_WORLD){
            memcpy(item->communicationArea, "MPI_COMM_WORLD", 14);
        } else {
            int comm_name_length;
            MPI_Comm_get_name(comm, item->communicationArea, &comm_name_length);
        }
        memcpy(item->processorname, proc_name, proc_name_length);
        item->processrank = processrank;
        item->partnerrank = dest;
        
        //printf("Test Rank: %d, WriterPos: %d, Bcast: %p\n", processrank, writer_pos, item);
        
        item->recvWaitingTime = 0.0;
        item->sendWaitingTime = 0.0;

        item->lateSenderTime = 0.0;
        item->lateReceiverTime = 0.0;
        
        item->request = NULL;
        item->end.tv_sec = 0;
        item->end.tv_nsec = 0;
       
        memcpy(item->sendmode, "", 0);
        item->immediate = 0;
        memcpy(item->usedBtl, "", 0);
        memcpy(item->usedProtocol, "", 0);
        item->withinEagerLimit = -1;
        item->foundMatchWild = -1;
        memcpy(item->usedAlgorithm, "", 0);
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
    
    MPI_Comm comm = MPI_COMM_WORLD;
    int size, processrank;
    char processorname[30];
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &processrank);
    
    int proc_name_length;
    MPI_Get_processor_name(processorname, &proc_name_length);
}

/*static void writeToPostgres(PGconn *conn, char* buffer, int offset){
    counter ++;
    const char copyQuery[256];
    snprintf("COPY " + db_name + " FROM STDIN (FORMAT binary)";
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
    
}*/

void print_timespec(struct timespec ts) {
    // Zeit in Sekunden in eine tm-Struktur umwandeln
    struct tm *timeinfo;
    timeinfo = localtime(&ts.tv_sec);
    
    // Stunden, Minuten und Sekunden aus der tm-Struktur
    int hours = timeinfo->tm_hour;
    int minutes = timeinfo->tm_min;
    int seconds = timeinfo->tm_sec;
    
    // Ausgabe im Format yyyy-mm-dd hh:mm:ss.nnnnnnnnn
    printf(" %04d-%02d-%02d %02d:%02d:%02d.%09ld\n",
           timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1,
           timeinfo->tm_mday,
           hours, minutes, seconds, ts.tv_nsec);
}

void qentryToBinary(qentry q, char *buffer, int *off){
        counter ++;
        //printf("Counter: %d\n", counter);
        qentry *item = &q;
        int offset = *off;
        
        
        newRow(buffer, 15, &offset);
        
        //printf("%.9f Seconds\n", item->sendWaitingTime);
        
        
        int job = atoi(job_id);
        intToBinary(job, buffer, &offset);

        stringToBinary(item->function, buffer, &offset);
        
        stringToBinary(item->communicationType, buffer, &offset);
        
        intToBinary(item->sendDatasize, buffer, &offset);
        
        intToBinary(item->recvDatasize, buffer, &offset);
        
        stringToBinary(item->communicationArea, buffer, &offset);
        
        stringToBinary(item->processorname, buffer, &offset);
        
        intToBinary(item->processrank, buffer, &offset);
        
        intToBinary(item->partnerrank, buffer, &offset);
        
        stringToBinary(item->usedAlgorithm, buffer, &offset);

        timestampToBinary(item->start, buffer, &offset, 1);
        
        timestampToBinary(item->end, buffer, &offset, 0);
        
        doubleToBinary(item->lateSenderTime, buffer, &offset);
        
        doubleToBinary(item->lateReceiverTime, buffer, &offset);
        
        double time_diff = timespec_diff(item->start, item->end);
        doubleToBinary(time_diff, buffer, &offset);
        
        /*struct timespec time_end;
        clock_gettime(CLOCK_REALTIME, &time_end);
        
        timestampToBinary(time_end, buffer, &offset);*/
        
        *off = offset;
        //printf("Rank: %d, Function: %s, \n", item->processrank, item->function);
        item = NULL;
        //fwrite(buffer, offset, 1, file);
        /*if(item->function != "MPI_Bcast"){
            printf("Item: %s, BTL: %s, Datasize: UsedProtocol: %s\n", item->function, item->usedBtl, item->recvDatasize, item->usedAlgorithm); 
        }*/
        
        //printf("Start:");
        //print_timespec(item->start);
        //printf("End:");
        //print_timespec(item->end);
        //printf("LateSender: %.9f Seconds\n", item->lateSenderTime);
        //printf("LateReceiver: %.9f Seconds\n", item->lateReceiverTime);
        //printf("Test Request: %.9f Seconds\n", item->sendWaitingTime);
        //printf("\n");
        
}

#define TIME_TO_WAIT 0.5

//Monitor-Function for SQL-Connection
static void* SQLMonitorFunc(void* _arg){
    
    char *db_host = getenv("DB_HOST");
    char *db_port = getenv("DB_PORT");
    char *db_user = getenv("DB_USER");
    char *db_name = getenv("DB_NAME");
    char *db_pw = getenv("DB_PW");
    
    if (db_host == NULL || db_port == NULL || db_name == NULL || db_user == NULL || db_pw == NULL) {
        fprintf(stderr, "One or more environment variables are not set\n");
        return 1;
    }
    
    int conninfo_size=(36 + strlen(db_host) + strlen(db_port) + strlen(db_name) + strlen(db_user) + strlen(db_pw));
    char *conninfo = malloc(conninfo_size);
    
    strcpy(conninfo, "host=");
    strcat(conninfo, db_host);
    strcat(conninfo, " port=");
    strcat(conninfo, db_port);
    strcat(conninfo, " dbname=");
    strcat(conninfo, db_name);
    strcat(conninfo, " user=");
    strcat(conninfo, db_user);
    strcat(conninfo, " password=");
    strcat(conninfo, db_pw);
    

    PGconn *conn = PQconnectdb(conninfo);
    bool check_job_id=0;
       
    if(processrank==0){
        char *user_id = strrchr(getenv("HOME"), '/') + 1;
        char start_time_str[30]; // Platz für den Zeitstempel
        strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%d %H:%M:%S", localtime(&(start_runtime.tv_sec)));
        const char *query = "INSERT INTO edumpi_runs (edumpi_run_id, start_time, user_id) VALUES ($1, $2, $3)";
        const char *paramValues[3] = {job_id, start_time_str, user_id};
        int paramlengths[3] = {strlen(job_id), strlen(start_time_str), strlen(user_id)};
        PGresult *res = PQexecParams(conn, query, 3, NULL, paramValues, paramlengths, NULL, 0);  
    }
    else {     
       while(!check_job_id){
           const char *query = "SELECT edumpi_run_id FROM edumpi_runs WHERE edumpi_run_id = $1";
	  const char *paramValues[1] = {job_id};
           PGresult *res = PQexecParams(conn, query, 1, NULL, paramValues, strlen(job_id), NULL, 0);
           check_job_id = (PQntuples(res));
  
       }
   }

    
       char proc_rank[4];
       sprintf(proc_rank, "%d", processrank);
       const char *query = "INSERT INTO edumpi_cluster_info (edumpi_run_id, processorname, processrank) VALUES ($1, $2, $3)";
       const char *paramValues[3] = {job_id ,proc_name, proc_rank};
       PGresult *res_ = PQexecParams(conn, query, 3, NULL, paramValues, strlen(job_id)+strlen(proc_name)+strlen(proc_rank), NULL, 0);
       
    
    clock_t start = clock();
    
    const char *copyQuery = "COPY edumpi_running_data FROM STDIN (FORMAT binary)";
    PGresult *res;
    res = PQexec(conn, copyQuery);
    PQclear(res);
    PQputCopyData(conn, PGCOPY_HEADER, 19);
    
   // int copyRes = PQputCopyData(conn, buffer, offset);
    //int offset = 0;
    //char *buffer = (char*)malloc(sizeof(char)*20000000);

    while(run_thread){
        if(writer_pos == -1 || reader_pos==writer_pos || (reader_pos == MAX_RINGSIZE-1 && writer_pos == 0)){
            //printf("Reader sleeps at Position %d, %d\n", reader_pos, writer_pos);
            sleep(0.1);
            //printf("Prozess: %d haengt beim Lesen\n", processrank);
            if(!run_thread) break;
        }
        else {
            if(reader_pos == MAX_RINGSIZE-1){
                reader_pos = 0;
            }
            else{
                reader_pos ++;
            }
            
            char buffer[200];
            int offset = 0;
            qentry *item = &ringbuffer[reader_pos];
            if(reader_pos == writer_pos){
                while(item->end.tv_nsec == 0){
                    sleep(0.01);
                    //printf("Prozess: %d haengt beim Lesen\n", processrank);
                    if(!run_thread) break;
	   }
            }
            //printf("Datenbankvorbereitung fuer: %s, Rank: %d\n", item->function, processrank);
            MPI_Status status;
            MPI_Request *request = item->request;
            int flag = 0;
            if(request != MPI_REQUEST_NULL && request != NULL){
                while (!flag) {
	       MPI_Test(request, &flag, &status);
	   } 
            }
            qentryToBinary(ringbuffer[reader_pos], buffer, &offset);
            PQputCopyData(conn, buffer, offset);
        }
            
        clock_t current = clock();
        if(current>=(start + TIME_TO_WAIT * CLOCKS_PER_SEC)){
        res = PQputCopyEnd(conn, NULL);
            if(res != -1){
                PQflush(conn);
                res = PQexec(conn, copyQuery);
            }
            /*struct timespec endii;
            clock_gettime(CLOCK_MONOTONIC, &endii);
            
            long long startii_ns = startii.tv_sec * 1000000000LL + startii.tv_nsec;
            long long endii_ns = endii.tv_sec * 1000000000LL + endii.tv_nsec;
            double time_spentii = (endii_ns - startii_ns) / 1e9;
            
            // Zeit ausgeben
            printf("Process. %d, Time spent: %.6f seconds\n", processrank, time_spentii);		*/
            
            PQclear(res);
            PQputCopyData(conn, PGCOPY_HEADER, 19);
            start = current;
        }
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
    
    if(processrank == 0){
        struct timespec end_runtime;
        clock_gettime(CLOCK_REALTIME, &end_runtime);

        // Zeitstempel für end_time als String erstellen
        char end_time_str[30];
        strftime(end_time_str, sizeof(end_time_str), "%Y-%m-%d %H:%M:%S", localtime(&(end_runtime.tv_sec)));

        // SQL-Abfrage für UPDATE der end_time
        const char *update_query = "UPDATE edumpi_runs SET end_time = $1 WHERE edumpi_run_id = $2";
        const char *update_paramValues[2] = {end_time_str, job_id};
        int update_paramLengths[2] = {strlen(end_time_str), strlen(job_id)};
        int update_paramFormats[2] = {0, 0};

        // PostgreSQL Abfrage für das Update
        res = PQexecParams(conn, update_query, 2, NULL, update_paramValues, update_paramLengths, update_paramFormats, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "UPDATE failed: %s", PQerrorMessage(conn));
        } 
        PQclear(res);
    }
    
    PQflush(conn);
    PQfinish(conn);
    //free(ringbuffer);
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
    while(writer_pos == reader_pos-1 || (reader_pos == 0 && writer_pos == MAX_RINGSIZE)){
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
       qentry* item = &ringbuffer[writer_pos+1];
       item->end.tv_sec = 0;
       item->end.tv_nsec = 0;
       writer_pos++;
       //printf("writing\n");
    }
    //printf("Adressen-Test; %p\n", &ringbuffer[writer_pos]);
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
#ifdef ENABLE_ANALYSIS
    run_thread = 1;
    counter = 0;
    ringbuffer = (qentry*)malloc(sizeof(qentry)*MAX_RINGSIZE);
    writer_pos = -1;
    reader_pos = -1;
    q_qentry = (qentry*)malloc(sizeof(qentry));
    job_id = getenv("SLURM_JOBID");
    clock_gettime(CLOCK_REALTIME, &start_runtime); 
#endif

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
