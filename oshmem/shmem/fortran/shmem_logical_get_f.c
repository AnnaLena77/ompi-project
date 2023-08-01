  /*
 * Copyright (c) 2013      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2013 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "oshmem_config.h"
#include "oshmem/shmem/fortran/bindings.h"
#include "oshmem/include/shmem.h"
#include "oshmem/shmem/shmem_api_logger.h"
#include "oshmem/runtime/runtime.h"
#include "oshmem/mca/spml/spml.h"
#include "ompi/datatype/ompi_datatype.h"
#include "stdio.h"

#if OSHMEM_PROFILING
#include "oshmem/shmem/fortran/pbindings.h"
SHMEM_GENERATE_WEAK_BINDINGS(SHMEM_LOGICAL_GET, shmem_logical_get)
#include "oshmem/shmem/fortran/profile-defines.h"
#endif

SHMEM_GENERATE_FORTRAN_BINDINGS_SUB (void,
        SHMEM_LOGICAL_GET,
        shmem_logical_get_,
        shmem_logical_get__,
        shmem_logical_get_f,
        (FORTRAN_POINTER_T target, FORTRAN_POINTER_T source, MPI_Fint *len, MPI_Fint *pe),
        (target,source,len,pe) )

void shmem_logical_get_f(FORTRAN_POINTER_T target, FORTRAN_POINTER_T source, MPI_Fint *len, MPI_Fint *pe)
{
    size_t logical_type_size = 0;
    ompi_datatype_type_size(&ompi_mpi_logical.dt, &logical_type_size);

    MCA_SPML_CALL(get(oshmem_ctx_default, FPTR_2_VOID_PTR(source),
        OMPI_FINT_2_INT(*len) * logical_type_size,
        FPTR_2_VOID_PTR(target),
        OMPI_FINT_2_INT(*pe)));
}

