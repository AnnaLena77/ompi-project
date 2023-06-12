/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mpi/fortran/mpif-h/bindings.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak PMPI_INFO_CREATE_ENV = ompi_info_create_env_f
#pragma weak pmpi_info_create_env = ompi_info_create_env_f
#pragma weak pmpi_info_create_env_ = ompi_info_create_env_f
#pragma weak pmpi_info_create_env__ = ompi_info_create_env_f

#pragma weak PMPI_Info_create_f = ompi_info_create_env_f
#pragma weak PMPI_Info_create_f08 = ompi_info_create_env_f
#else
OMPI_GENERATE_F77_BINDINGS (PMPI_INFO_CREATE_ENV,
                           pmpi_info_create_env,
                           pmpi_info_create_env_,
                           pmpi_info_create_env__,
                           pompi_info_create_env_f,
                           (MPI_Fint *info, MPI_Fint *ierr),
                           (info, ierr) )
#endif
#endif

#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_INFO_CREATE_ENV = ompi_info_create_env_f
#pragma weak mpi_info_create_env = ompi_info_create_env_f
#pragma weak mpi_info_create_env_ = ompi_info_create_env_f
#pragma weak mpi_info_create_env__ = ompi_info_create_env_f

#pragma weak MPI_Info_create_f = ompi_info_create_env_f
#pragma weak MPI_Info_create_f08 = ompi_info_create_env_f
#else
#if ! OMPI_BUILD_MPI_PROFILING
OMPI_GENERATE_F77_BINDINGS (MPI_INFO_CREATE_ENV,
                           mpi_info_create_env,
                           mpi_info_create_env_,
                           mpi_info_create_env__,
                           ompi_info_create_env_f,
                           (MPI_Fint *info, MPI_Fint *ierr),
                           (info, ierr) )
#else
#define ompi_info_create_env_f pompi_info_create_env_f
#endif
#endif


void ompi_info_create_env_f(MPI_Fint *info, MPI_Fint *ierr)
{
    int c_ierr;
    MPI_Info c_info;

    c_ierr = PMPI_Info_create_env(0, NULL, &c_info);
    if (NULL != ierr) *ierr = OMPI_INT_2_FINT(c_ierr);

    if (MPI_SUCCESS == c_ierr) {
        *info = PMPI_Info_c2f(c_info);
    }
}
