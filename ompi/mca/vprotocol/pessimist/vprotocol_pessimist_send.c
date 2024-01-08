/*
 * Copyright (c) 2004-2007 The Trustees of the University of Tennessee.
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
#include "vprotocol_pessimist.h"
#include "vprotocol_pessimist_sender_based.h"
//#include "ompi/mpi/c/init.h"

#ifndef ENABLE_ANALYSIS
int mca_vprotocol_pessimist_isend(const void *buf,
                       size_t count,
                       ompi_datatype_t* datatype,
                       int dst,
                       int tag,
                       mca_pml_base_send_mode_t sendmode,
                       ompi_communicator_t* comm,
                       ompi_request_t** request)
#else
int mca_vprotocol_pessimist_isend(const void *buf,
                       size_t count,
                       ompi_datatype_t* datatype,
                       int dst,
                       int tag,
                       mca_pml_base_send_mode_t sendmode,
                       ompi_communicator_t* comm,
                       ompi_request_t** request,
                       struct qentry **q)
#endif
{
    int ret;

    V_OUTPUT_VERBOSE(50, "pessimist:\tisend\tposted\t%"PRIpclock"\tto %d\ttag %d\tsize %lu",
                     mca_vprotocol_pessimist.clock, dst, tag, (unsigned long) count);

    vprotocol_pessimist_event_flush();
#ifndef ENABLE_ANALYSIS
    ret = mca_pml_v.host_pml.pml_isend(buf, count, datatype, dst, tag, sendmode,
                                       comm, request);
#else
    ret = mca_pml_v.host_pml.pml_isend(buf, count, datatype, dst, tag, sendmode,
                                       comm, request, NULL);
#endif
    VPESSIMIST_FTREQ_INIT(*request);
    vprotocol_pessimist_sender_based_copy_start(*request);
    return ret;
}

#ifndef ENABLE_ANALYSIS
int mca_vprotocol_pessimist_send(const void *buf,
                      size_t count,
                      ompi_datatype_t* datatype,
                      int dst,
                      int tag,
                      mca_pml_base_send_mode_t sendmode,
                      ompi_communicator_t* comm)
#else
int mca_vprotocol_pessimist_send(const void *buf,
                      size_t count,
                      ompi_datatype_t* datatype,
                      int dst,
                      int tag,
                      mca_pml_base_send_mode_t sendmode,
                      ompi_communicator_t* comm,
                      struct qentry **q)
#endif
{

    ompi_request_t *request = MPI_REQUEST_NULL;
    int rc;

    V_OUTPUT_VERBOSE(50, "pessimist:\tsend\tposted\t%"PRIpclock"\tto %d\ttag %d\tsize %lu",
                     mca_vprotocol_pessimist.clock, dst, tag, (unsigned long) count);

    vprotocol_pessimist_event_flush();
#ifndef ENABLE_ANALYSIS
    mca_pml_v.host_pml.pml_isend(buf, count, datatype, dst, tag, sendmode,
                                 comm, &request);
#else
    mca_pml_v.host_pml.pml_isend(buf, count, datatype, dst, tag, sendmode,
                                 comm, &request, NULL);
#endif
    VPESSIMIST_FTREQ_INIT(request);
    vprotocol_pessimist_sender_based_copy_start(request);
    VPROTOCOL_PESSIMIST_WAIT(&request, MPI_STATUS_IGNORE, rc);
    return rc;
}
