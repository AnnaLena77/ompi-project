/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2018 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#include "coll_basic.h"

#include <stdio.h>

#include "mpi.h"
#include "ompi/constants.h"
#include "ompi/op/op.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/coll_tags.h"
#include "ompi/mca/pml/pml.h"
#include "coll_basic.h"


/*
 *	scan
 *
 *	Function:	- basic scan operation
 *	Accepts:	- same arguments as MPI_Scan()
 *	Returns:	- MPI_SUCCESS or error code
 */
int
mca_coll_basic_scan_intra(const void *sbuf, void *rbuf, int count,
                          struct ompi_datatype_t *dtype,
                          struct ompi_op_t *op,
                          struct ompi_communicator_t *comm,
                          mca_coll_base_module_t *module
#ifdef ENABLE_ANALYSIS
		        , qentry **q
#endif
                          )
{
#ifdef ENABLE_ANALYSIS
    qentry *item;
    if(q!=NULL){
        if(*q!=NULL){
            item = *q;
        } else item = NULL;
    } else item = NULL;
#endif

#ifndef ENABLE_ANALYSIS
    return ompi_coll_base_scan_intra_linear(sbuf, rbuf, count, dtype, op, comm, module);
#else
    return ompi_coll_base_scan_intra_linear(sbuf, rbuf, count, dtype, op, comm, module, &item);
#endif
}
