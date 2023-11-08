/*
 * Copyright (c) 2011      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/memchecker.h"
#include "ompi/request/request.h"
#include "ompi/message/message.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Imrecv = PMPI_Imrecv
#endif
#define MPI_Imrecv PMPI_Imrecv
#endif

static const char FUNC_NAME[] = "MPI_Imrecv";


int MPI_Imrecv(void *buf, int count, MPI_Datatype type,
               MPI_Message *message, MPI_Request *request)
{
#ifdef ENABLE_ANALYSIS
    /*qentry *item = (qentry*)malloc(sizeof(qentry));
    //item->start
    gettimeofday(&item->start, NULL);
    //item->operation*/
    
    qentry *item = getWritingRingPos();
    initQentry(&item);
    //item->start
    clock_gettime(CLOCK_REALTIME, &item->start);
    
    strcpy(item->operation, "MPI_Imrecv");
    //item->blocking
    item->blocking = 0;
    //item->datatype
    char *type_name = (char*) malloc(MPI_MAX_OBJECT_NAME);
    int type_name_length;
    MPI_Type_get_name(type, type_name, &type_name_length);
    strcpy(item->datatype, type_name);
    free(type_name);

    //item->processrank
    int processrank;
    MPI_Comm_rank(MPI_COMM_WORLD, &processrank);
    item->processrank = processrank;
    
    //item->processorname
    char *proc_name = (char*)malloc(MPI_MAX_PROCESSOR_NAME);
    int proc_name_length;
    MPI_Get_processor_name(proc_name, &proc_name_length);
    strcpy(item->processorname, proc_name);
    free(proc_name);
    
#endif
    
    int rc = MPI_SUCCESS;
    ompi_communicator_t *comm;

    MEMCHECKER(
        memchecker_datatype(type);
        memchecker_message(message);
        memchecker_call(&opal_memchecker_base_isaddressable, buf, count, type);
        memchecker_comm(comm);
    );

    if ( MPI_PARAM_CHECK ) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        OMPI_CHECK_DATATYPE_FOR_RECV(rc, type, count);
        OMPI_CHECK_USER_BUFFER(rc, buf, type, count);

        if (NULL == message || MPI_MESSAGE_NULL == *message) {
            rc = MPI_ERR_REQUEST;
            comm = MPI_COMM_NULL;
        } else {
            comm = (*message)->comm;
        }

        OMPI_ERRHANDLER_CHECK(rc, comm, rc, FUNC_NAME);
    } else {
        comm = (*message)->comm;
    }

    if (&ompi_message_no_proc.message == *message) {
        *request = &ompi_request_empty;
        *message = MPI_MESSAGE_NULL;
        return MPI_SUCCESS;
     }

#if OPAL_ENABLE_FT_MPI
    /*
     * The message and associated request will be checked by the PML, and
     * handled appropriately. So no need to check here.
     */
#endif
#ifndef ENABLE_ANALYSIS
    rc = MCA_PML_CALL(imrecv(buf, count, type, message, request));
#else
    rc = MCA_PML_CALL(imrecv(buf, count, type, message, request, &item));
    //qentryIntoQueue(&item);
#endif
    OMPI_ERRHANDLER_RETURN(rc, comm, rc, FUNC_NAME);
}
