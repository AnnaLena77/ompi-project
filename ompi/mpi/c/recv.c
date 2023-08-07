/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2018 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
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
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/memchecker.h"
#include "ompi/request/request.h"
#include "ompi/runtime/ompi_spc.h"
#include <time.h>

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Recv = PMPI_Recv
#endif
#define MPI_Recv PMPI_Recv
#endif

static const char FUNC_NAME[] = "MPI_Recv";


int MPI_Recv(void *buf, int count, MPI_Datatype type, int source,
             int tag, MPI_Comm comm, MPI_Status *status)
{
    #ifdef ENABLE_ANALYSIS
    qentry *item = q_qentry;
    initQentry(&item);
    //item->start
    //gettimeofday(&item->start, NULL);
    //item->operation
    /*memcpy(item->function, "MPI_Recv", 8);
    memcpy(item->communicationType, "p2p", 3);
    //item->blocking
    item->blocking = 1;
    //item->datatype
    char type_name[MPI_MAX_OBJECT_NAME];
    int type_name_length;
    MPI_Type_get_name(type, type_name, &type_name_length);
    memcpy(item->datatype, type_name, type_name_length);

    //item->communicator
    char comm_name[MPI_MAX_OBJECT_NAME];
    int comm_name_length;
    MPI_Comm_get_name(comm, comm_name, &comm_name_length);
    memcpy(item->communicationArea, comm_name, comm_name_length);
    
    //item->processrank
    int processrank;
    MPI_Comm_rank(comm, &processrank);
    item->processrank = processrank;
    //item->partnerrank
    item->partnerrank = source;
    
    //item->processorname
    char proc_name[MPI_MAX_PROCESSOR_NAME];
    int proc_name_length;
    MPI_Get_processor_name(proc_name, &proc_name_length);
    memcpy(item->processorname, proc_name, proc_name_length);*/
    
    //#endif
    int rc = MPI_SUCCESS;

    SPC_RECORD(OMPI_SPC_RECV, 1);

    MEMCHECKER(
        memchecker_datatype(type);
        memchecker_call(&opal_memchecker_base_isaddressable, buf, count, type);
        memchecker_comm(comm);
    );

    if ( MPI_PARAM_CHECK ) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        OMPI_CHECK_DATATYPE_FOR_RECV(rc, type, count);
        OMPI_CHECK_USER_BUFFER(rc, buf, type, count);

        if (ompi_comm_invalid(comm)) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COMM, FUNC_NAME);
        } else if (((tag < 0) && (tag != MPI_ANY_TAG)) || (tag > mca_pml.pml_max_tag)) {
            rc = MPI_ERR_TAG;
        } else if ((source != MPI_ANY_SOURCE) &&
                   (MPI_PROC_NULL != source) &&
                   ompi_comm_peer_invalid(comm, source)) {
            rc = MPI_ERR_RANK;
        }

        OMPI_ERRHANDLER_CHECK(rc, comm, rc, FUNC_NAME);
    }

    if (MPI_PROC_NULL == source) {
        if (MPI_STATUS_IGNORE != status) {
            *status = ompi_request_empty.req_status;
        }
        return MPI_SUCCESS;
    }
#ifndef ENABLE_ANALYSIS
    rc = MCA_PML_CALL(recv(buf, count, type, source, tag, comm, status));
#else
    rc = MCA_PML_CALL(recv(buf, count, type, source, tag, comm, status, &item));
    //writeIntoFile(&item);
    //free(item);
    //qentryIntoQueue(&item);
#endif
    OMPI_ERRHANDLER_RETURN(rc, comm, rc, FUNC_NAME);
}
