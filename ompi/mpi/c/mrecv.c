/*
 * Copyright (c) 2011      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2012      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2012-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2018-2021 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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
#include "ompi/runtime/ompi_spc.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Mrecv = PMPI_Mrecv
#endif
#define MPI_Mrecv PMPI_Mrecv
#endif

static const char FUNC_NAME[] = "MPI_Mrecv";


int MPI_Mrecv(void *buf, int count, MPI_Datatype type,
              MPI_Message *message, MPI_Status *status)
{
#ifdef ENABLE_ANALYSIS
    /*qentry *item = (qentry*)malloc(sizeof(qentry));
    //item->start
    gettimeofday(&item->start, NULL);*/
    
    qentry *item = getWritingRingPos();
    initQentry(&item);
    //item->start
    clock_gettime(CLOCK_REALTIME, &item->start);
    
    //item->operation
    strcpy(item->operation, "MPI_Mrecv");
    //item->blocking
    item->blocking = 1;
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

    SPC_RECORD(OMPI_SPC_MRECV, 1);

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
        if (MPI_STATUS_IGNORE != status) {
            OMPI_COPY_STATUS(status, ompi_request_empty.req_status, false);
        }
        *message = MPI_MESSAGE_NULL;
        return MPI_SUCCESS;
     }

#if OPAL_ENABLE_FT_MPI
    /*
     * The message and associated request will be checked by the PML, and
     * handled appropriately. SO no need to check here.
     */
#endif

#ifndef ENABLE_ANALYSIS
    rc = MCA_PML_CALL(mrecv(buf, count, type, message, status));
#else
    rc = MCA_PML_CALL(mrecv(buf, count, type, message, status, &item));
    //qentryIntoQueue(&item);
#endif
    /* Per MPI-1, the MPI_ERROR field is not defined for
       single-completion calls */
    MEMCHECKER(
        opal_memchecker_base_mem_undefined(&status->MPI_ERROR, sizeof(int));
    );

    OMPI_ERRHANDLER_RETURN(rc, comm, rc, FUNC_NAME);
}
