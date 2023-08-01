/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2018      Siberian State University of Telecommunications
 *                         and Information Sciences. All rights reserved.
 * Copyright (c) 2018      Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 * Copyright (c) 2020      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "mpi.h"
#include "opal/util/bit_ops.h"
#include "ompi/constants.h"
#include "ompi/datatype/ompi_datatype.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/coll_base_topo.h"
#include "ompi/mca/coll/base/coll_tags.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/op/op.h"
#include "coll_tuned.h"

/* reduce_scatter_block algorithm variables */
static int coll_tuned_reduce_scatter_block_forced_algorithm = 0;
static int coll_tuned_reduce_scatter_block_segment_size = 0;
static int coll_tuned_reduce_scatter_block_tree_fanout;

/* valid values for coll_tuned_reduce_scatter_blokc_forced_algorithm */
static const mca_base_var_enum_value_t reduce_scatter_block_algorithms[] = {
    {0, "ignore"},
    {1, "basic_linear"},
    {2, "recursive_doubling"},
    {3, "recursive_halving"},
    {4, "butterfly"},
    {0, NULL}
};

/**
 * The following are used by dynamic and forced rules
 *
 * publish details of each algorithm and if its forced/fixed/locked in
 * as you add methods/algorithms you must update this and the query/map routines
 *
 * this routine is called by the component only
 * this makes sure that the mca parameters are set to their initial values and
 * perms module does not call this they call the forced_getvalues routine
 * instead
 */

int ompi_coll_tuned_reduce_scatter_block_intra_check_forced_init (coll_tuned_force_algorithm_mca_param_indices_t *mca_param_indices)
{
    mca_base_var_enum_t *new_enum;
    int cnt;

    for( cnt = 0; NULL != reduce_scatter_block_algorithms[cnt].string; cnt++ );
    ompi_coll_tuned_forced_max_algorithms[REDUCESCATTERBLOCK] = cnt;

    (void) mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                           "reduce_scatter_block_algorithm_count",
                                           "Number of reduce_scatter_block algorithms available",
                                           MCA_BASE_VAR_TYPE_INT, NULL, 0,
                                           MCA_BASE_VAR_FLAG_DEFAULT_ONLY,
                                           OPAL_INFO_LVL_5,
                                           MCA_BASE_VAR_SCOPE_CONSTANT,
                                           &ompi_coll_tuned_forced_max_algorithms[REDUCESCATTERBLOCK]);

    /* MPI_T: This variable should eventually be bound to a communicator */
    coll_tuned_reduce_scatter_block_forced_algorithm = 0;
    (void) mca_base_var_enum_create("coll_tuned_reduce_scatter_block_algorithms", reduce_scatter_block_algorithms, &new_enum);
    mca_param_indices->algorithm_param_index =
        mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                        "reduce_scatter_block_algorithm",
                                        "Which reduce reduce_scatter_block algorithm is used. "
                                        "Can be locked down to choice of: 0 ignore, 1 basic_linear, 2 recursive_doubling, "
                                        "3 recursive_halving, 4 butterfly. "
                                        "Only relevant if coll_tuned_use_dynamic_rules is true.",
                                        MCA_BASE_VAR_TYPE_INT, new_enum, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                        OPAL_INFO_LVL_5,
                                        MCA_BASE_VAR_SCOPE_ALL,
                                        &coll_tuned_reduce_scatter_block_forced_algorithm);
    OBJ_RELEASE(new_enum);
    if (mca_param_indices->algorithm_param_index < 0) {
        return mca_param_indices->algorithm_param_index;
    }

    coll_tuned_reduce_scatter_block_segment_size = 0;
    mca_param_indices->segsize_param_index =
        mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                        "reduce_scatter_block_algorithm_segmentsize",
                                        "Segment size in bytes used by default for reduce_scatter_block algorithms. Only has meaning if algorithm is forced and supports segmenting. 0 bytes means no segmentation.",
                                        MCA_BASE_VAR_TYPE_INT, NULL, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                        OPAL_INFO_LVL_5,
                                        MCA_BASE_VAR_SCOPE_ALL,
                                        &coll_tuned_reduce_scatter_block_segment_size);

    coll_tuned_reduce_scatter_block_tree_fanout = ompi_coll_tuned_init_tree_fanout; /* get system wide default */
    mca_param_indices->tree_fanout_param_index =
        mca_base_component_var_register(&mca_coll_tuned_component.super.collm_version,
                                        "reduce_scatter_block_algorithm_tree_fanout",
                                        "Fanout for n-tree used for reduce_scatter_block algorithms. Only has meaning if algorithm is forced and supports n-tree topo based operation.",
                                        MCA_BASE_VAR_TYPE_INT, NULL, 0, MCA_BASE_VAR_FLAG_SETTABLE,
                                        OPAL_INFO_LVL_5,
                                        MCA_BASE_VAR_SCOPE_ALL,
                                        &coll_tuned_reduce_scatter_block_tree_fanout);

    return (MPI_SUCCESS);
}

int ompi_coll_tuned_reduce_scatter_block_intra_do_this(const void *sbuf, void *rbuf,
                                                       int rcount,
                                                       struct ompi_datatype_t *dtype,
                                                       struct ompi_op_t *op,
                                                       struct ompi_communicator_t *comm,
                                                       mca_coll_base_module_t *module,
                                                       int algorithm, int faninout, int segsize
#ifdef ENABLE_ANALYSIS
                                                       , qentry **q
#endif
                                                       )
{
    OPAL_OUTPUT((ompi_coll_tuned_stream, "coll:tuned:reduce_scatter_block_intra_do_this selected algorithm %d topo faninout %d segsize %d",
                 algorithm, faninout, segsize));

#ifdef ENABLE_ANALYSIS
    qentry *item;
    if(q!=NULL){
        if(*q!=NULL){
            item = *q;
        } else item = NULL;
    } else item = NULL;
#endif

    switch (algorithm) {
#ifndef ENABLE_ANALYSIS
    case (0): return ompi_coll_tuned_reduce_scatter_block_intra_dec_fixed(sbuf, rbuf, rcount,
                                                                          dtype, op, comm, module);
    case (1): return ompi_coll_base_reduce_scatter_block_basic_linear(sbuf, rbuf, rcount,
                                                                      dtype, op, comm, module);
    case (2): return ompi_coll_base_reduce_scatter_block_intra_recursivedoubling(sbuf, rbuf, rcount,
                                                                                 dtype, op, comm, module);
    case (3): return ompi_coll_base_reduce_scatter_block_intra_recursivehalving(sbuf, rbuf, rcount,
                                                                                dtype, op, comm, module);
    case (4): return ompi_coll_base_reduce_scatter_block_intra_butterfly(sbuf, rbuf, rcount, dtype, op, comm,
                                                                         module);
#else
       case (0): return ompi_coll_tuned_reduce_scatter_block_intra_dec_fixed(sbuf, rbuf, rcount,
                                                                          dtype, op, comm, module, &item);
    case (1): return ompi_coll_base_reduce_scatter_block_basic_linear(sbuf, rbuf, rcount,
                                                                      dtype, op, comm, module, &item);
    case (2): return ompi_coll_base_reduce_scatter_block_intra_recursivedoubling(sbuf, rbuf, rcount,
                                                                                 dtype, op, comm, module, &item);
    case (3): return ompi_coll_base_reduce_scatter_block_intra_recursivehalving(sbuf, rbuf, rcount,
                                                                                dtype, op, comm, module, &item);
    case (4): return ompi_coll_base_reduce_scatter_block_intra_butterfly(sbuf, rbuf, rcount, dtype, op, comm,
                                                                         module, &item); 
#endif
    } /* switch */
    OPAL_OUTPUT((ompi_coll_tuned_stream, "coll:tuned:reduce_scatter_block_intra_do_this attempt to select algorithm %d when only 0-%d is valid?",
                 algorithm, ompi_coll_tuned_forced_max_algorithms[REDUCESCATTERBLOCK]));
    return (MPI_ERR_ARG);
}
