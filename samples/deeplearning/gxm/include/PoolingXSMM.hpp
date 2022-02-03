/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/libxsmm/libxsmm/                    *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Sasikanth Avancha, Dhiraj Kalamkar (Intel Corp.)
******************************************************************************/


#pragma once
#include "PoolingImpl.hpp"
#include "libxsmm.h"
#include "check.hpp"

#define CHKERR_LIBXSMM_DNN(A) if ( A != LIBXSMM_DNN_SUCCESS )\
{\
  fprintf(stdout, "%s, %s\n", gp->node_name.c_str(), libxsmm_dnn_get_error(A) );\
  fflush(stdout);\
}

class PoolXSMM : public PoolImpl
{
  protected:
    PoolImpl *gp_;
    libxsmm_dnn_pooling_desc pooling_desc;
    libxsmm_dnn_pooling* libxsmm_handle[NUM_NUMA_NODES];
    libxsmm_dnn_tensor*  libxsmm_input[NUM_NUMA_NODES] = {NULL};
    libxsmm_dnn_tensor*  libxsmm_delinput[NUM_NUMA_NODES]={NULL};
    libxsmm_dnn_tensor*  libxsmm_output[NUM_NUMA_NODES]={NULL};
    libxsmm_dnn_tensor*  libxsmm_deloutput[NUM_NUMA_NODES]={NULL};
    libxsmm_dnn_tensor*  libxsmm_mask[NUM_NUMA_NODES]={NULL};
    libxsmm_dnn_tensor_datalayout* libxsmm_layout;
    libxsmm_dnn_err_t status;
    libxsmm_dnn_err_t global_status = LIBXSMM_DNN_SUCCESS;
    bool updated_scratch_fwd=false, updated_scratch_bwd=false;
    void *scratch=NULL;
    int prev_scratch_size = 0;
  public:
    PoolXSMM(PoolImplParams* gp, int engine);
    virtual ~PoolXSMM(void) {}

    // Assume external threading, e.g., #pragma omp
    void forwardPropagate(TensorBuf *inp, TensorBuf *outp, int *maskp, int tid);
    void backPropagate(TensorBuf *deloutp, int *maskp, TensorBuf *delinp, int tid);
};
