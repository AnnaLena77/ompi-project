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
 * Copyright (c)      2012 Oak Rigde National Laboratory. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
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
#include "ompi/memchecker.h"
#include "ompi/runtime/ompi_spc.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Ibarrier = PMPI_Ibarrier
#endif
#define MPI_Ibarrier PMPI_Ibarrier
#endif

static const char FUNC_NAME[] = "MPI_Ibarrier";


int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
#ifdef ENABLE_ANALYSIS
    qentry *item = (qentry*)malloc(sizeof(qentry));
    initQentry(&item);
    item->start = time(NULL);
    strcpy(item->function, "MPI_Barrier");
    strcpy(item->communicationType, "collective");
#endif 
    int err = MPI_SUCCESS;

    SPC_RECORD(OMPI_SPC_IBARRIER, 1);

    MEMCHECKER(
            memchecker_comm(comm);
            );

    /* Error checking */

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (ompi_comm_invalid(comm)) {
            return OMPI_ERRHANDLER_NOHANDLE_INVOKE(MPI_ERR_COMM, FUNC_NAME);
        }
    }
#ifndef ENABLE_ANALYSIS
    err = comm->c_coll->coll_ibarrier(comm, request, comm->c_coll->coll_ibarrier_module);
#else
    err = comm->c_coll->coll_ibarrier(comm, request, comm->c_coll->coll_ibarrier_module, &item);
    qentryIntoQueue(&item);
#endif

    /* All done */

    OMPI_ERRHANDLER_RETURN(err, comm, err, FUNC_NAME);
}
