/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2013 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015-2019 Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/datatype/ompi_datatype.h"
#include "ompi/mca/coll/base/coll_base_util.h"
#include "ompi/memchecker.h"
#include "ompi/runtime/ompi_spc.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Igatherv = PMPI_Igatherv
#endif
#define MPI_Igatherv PMPI_Igatherv
#endif

static const char FUNC_NAME[] = "MPI_Igatherv";


int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int recvcounts[], const int displs[],
                 MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
#ifdef ENABLE_ANALYSIS
    qentry *item = (qentry*)malloc(sizeof(qentry));
    initQentry(&item);
    gettimeofday(&item->start, NULL);
    strcpy(item->function, "MPI_Igatherv");
    strcpy(item->communicationType, "collective");
    int processrank;
    MPI_Comm_rank(comm, &processrank);
    item->processrank = processrank;
    //item->partnerrank
    if(processrank==root){
    	item->partnerrank = -1;
    	//item->datatype
         char *type_name = (char*) malloc(MPI_MAX_OBJECT_NAME);
         int type_name_length;
         MPI_Type_get_name(recvtype, type_name, &type_name_length);
         strcpy(item->datatype, type_name);
         free(type_name);
         //item->sendcount
         //item->count = recvcount;
         //item->datasize
         //item->datasize = recvcount * sizeof(recvtype);
    }
    else{
    	item->partnerrank = root;
    	//item->datatype
         char *type_name = (char*) malloc(MPI_MAX_OBJECT_NAME);
         int type_name_length;
         MPI_Type_get_name(sendtype, type_name, &type_name_length);
         strcpy(item->datatype, type_name);
         free(type_name);
         //item->count
         //item->count = sendcount;
         //item->datasize
         //item->datasize = sendcount * sizeof(sendtype);
    }
    
    //item->communicator
    char *comm_name = (char*) malloc(MPI_MAX_OBJECT_NAME);
    int comm_name_length;
    MPI_Comm_get_name(comm, comm_name, &comm_name_length);
    strcpy(item->communicationArea, comm_name);
    free(comm_name);
    item->blocking = 0;
    
    //item->processorname
    char *proc_name = (char*)malloc(MPI_MAX_PROCESSOR_NAME);
    int proc_name_length;
    MPI_Get_processor_name(proc_name, &proc_name_length);
    strcpy(item->processorname, proc_name);
    free(proc_name);
    
#endif 
    int i, size, err;

    SPC_RECORD(OMPI_SPC_IGATHERV, 1);

    MEMCHECKER(
        ptrdiff_t ext;

        size = ompi_comm_remote_size(comm);
        ompi_datatype_type_extent(recvtype, &ext);

        memchecker_comm(comm);
        if(OMPI_COMM_IS_INTRA(comm)) {
            if(ompi_comm_rank(comm) == root) {
                /* check whether root's send buffer is defined. */
                if (MPI_IN_PLACE == sendbuf) {
                    for (i = 0; i < size; i++) {
                        memchecker_call(&opal_memchecker_base_isdefined,
                                        (char *)(recvbuf)+displs[i]*ext,
                                        recvcounts[i], recvtype);
                    }
                } else {
                    memchecker_datatype(sendtype);
                    memchecker_call(&opal_memchecker_base_isdefined, sendbuf, sendcount, sendtype);
                }

                memchecker_datatype(recvtype);
                /* check whether root's receive buffer is addressable. */
                for (i = 0; i < size; i++) {
                    memchecker_call(&opal_memchecker_base_isaddressable,
                                    (char *)(recvbuf)+displs[i]*ext,
                                    recvcounts[i], recvtype);
                }
            } else {
                memchecker_datatype(sendtype);
                /* check whether send buffer is defined on other processes. */
                memchecker_call(&opal_memchecker_base_isdefined, sendbuf, sendcount, sendtype);
            }
        } else {
            if (MPI_ROOT == root) {
                memchecker_datatype(recvtype);
                /* check whether root's receive buffer is addressable. */
                for (i = 0; i < size; i++) {
                    memchecker_call(&opal_memchecker_base_isaddressable,
                                    (char *)(recvbuf)+displs[i]*ext,
                                    recvcounts[i], recvtype);
                }
            } else if (MPI_PROC_NULL != root) {
                memchecker_datatype(sendtype);
                /* check whether send buffer is defined. */
                memchecker_call(&opal_memchecker_base_isdefined, sendbuf, sendcount, sendtype);
            }
        }
    );

    if (MPI_PARAM_CHECK) {
        err = MPI_SUCCESS;
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (ompi_comm_invalid(comm)) {
            return OMPI_ERRHANDLER_NOHANDLE_INVOKE(MPI_ERR_COMM,
                                          FUNC_NAME);
        } else if ((ompi_comm_rank(comm) != root && MPI_IN_PLACE == sendbuf) ||
                   (ompi_comm_rank(comm) == root && MPI_IN_PLACE == recvbuf)) {
            return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_ARG, FUNC_NAME);
        }

        /* Errors for intracommunicators */

        if (OMPI_COMM_IS_INTRA(comm)) {

            /* Errors for all ranks */

            if ((root >= ompi_comm_size(comm)) || (root < 0)) {
                err = MPI_ERR_ROOT;
            } else if (MPI_IN_PLACE != sendbuf) {
                OMPI_CHECK_DATATYPE_FOR_SEND(err, sendtype, sendcount);
            }
            OMPI_ERRHANDLER_CHECK(err, comm, err, FUNC_NAME);

            /* Errors for the root.  Some of these could have been
               combined into compound if statements above, but since
               this whole section can be compiled out (or turned off at
               run time) for efficiency, it's more clear to separate
               them out into individual tests. */

            if (ompi_comm_rank(comm) == root) {
                if (NULL == displs) {
                    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_ARG, FUNC_NAME);
                }

                if (NULL == recvcounts) {
                    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COUNT, FUNC_NAME);
                }

                size = ompi_comm_size(comm);
                for (i = 0; i < size; ++i) {
                    if (recvcounts[i] < 0) {
                        return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COUNT, FUNC_NAME);
                    } else if (MPI_DATATYPE_NULL == recvtype || NULL == recvtype) {
                        return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_TYPE, FUNC_NAME);
                    }
                }
            }
        }

        /* Errors for intercommunicators */

        else {
            if (! ((root >= 0 && root < ompi_comm_remote_size(comm)) ||
                   MPI_ROOT == root || MPI_PROC_NULL == root)) {
                return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_ROOT, FUNC_NAME);
            }

            /* Errors for the senders */

            if (MPI_ROOT != root && MPI_PROC_NULL != root) {
                OMPI_CHECK_DATATYPE_FOR_SEND(err, sendtype, sendcount);
                OMPI_ERRHANDLER_CHECK(err, comm, err, FUNC_NAME);
            }

            /* Errors for the root.  Ditto on the comment above -- these
               error checks could have been combined above, but let's
               make the code easier to read. */

            else if (MPI_ROOT == root) {
                if (NULL == displs) {
                    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_ARG, FUNC_NAME);
                }

                if (NULL == recvcounts) {
                    return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COUNT, FUNC_NAME);
                }

                size = ompi_comm_remote_size(comm);
                for (i = 0; i < size; ++i) {
                    if (recvcounts[i] < 0) {
                        return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COUNT, FUNC_NAME);
                    } else if (MPI_DATATYPE_NULL == recvtype || NULL == recvtype) {
                        return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_TYPE, FUNC_NAME);
                    }
                }
            }
        }
    }

    /* Invoke the coll component to perform the back-end operation */
#ifndef ENABLE_ANALYSIS
    err = comm->c_coll->coll_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcounts, displs, recvtype,
                                     root, comm, request, comm->c_coll->coll_igatherv_module);
#else
    err = comm->c_coll->coll_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcounts, displs, recvtype,
                                     root, comm, request, comm->c_coll->coll_igatherv_module, &item);
#endif
    if (OPAL_LIKELY(OMPI_SUCCESS == err)) {
        if (OMPI_COMM_IS_INTRA(comm)) {
            if (MPI_IN_PLACE == sendbuf) {
                sendtype = NULL;
            } else if (ompi_comm_rank(comm) != root) {
                recvtype = NULL;
            }
        } else {
            if (MPI_ROOT == root) {
                sendtype = NULL;
            } else if (MPI_PROC_NULL == root) {
                sendtype = NULL;
                recvtype = NULL;
            } else {
                recvtype = NULL;
            }
        }
        ompi_coll_base_retain_datatypes(*request, sendtype, recvtype);
    }
#ifdef ENABLE_ANALYSIS
    qentryIntoQueue(&item);
#endif  
    OMPI_ERRHANDLER_RETURN(err, comm, err, FUNC_NAME);
}
