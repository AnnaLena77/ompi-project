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
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/constants.h"
#include "coll_sm.h"


/*
 *	alltoallw_intra
 *
 *	Function:	- MPI_Alltoallw
 *	Accepts:	- same as MPI_Alltoallw()
 *	Returns:	- MPI_SUCCESS or an MPI error code
 */
int mca_coll_sm_alltoallw_intra(const void *sbuf, const int *scounts, const int *sdisps,
                                struct ompi_datatype_t * const *sdtypes,
                                void *rbuf, const int *rcounts, const int *rdisps,
                                struct ompi_datatype_t * const *rdtypes,
                                struct ompi_communicator_t *comm,
                                mca_coll_base_module_t *module
#ifdef ENABLE_ANALYSIS
			     , qentry **q
#endif
                                )
{
    return OMPI_ERR_NOT_IMPLEMENTED;
}
