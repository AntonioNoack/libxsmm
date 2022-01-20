/******************************************************************************
* Copyright (c) Intel Corporation - All rights reserved.                      *
* This file is part of the LIBXSMM library.                                   *
*                                                                             *
* For information on the license, see the LICENSE file.                       *
* Further information: https://github.com/hfp/libxsmm/                        *
* SPDX-License-Identifier: BSD-3-Clause                                       *
******************************************************************************/
/* Deepti Aggarwal, Alexander Heinecke (Intel Corp.), Antonio Noack (FSU Jena)
******************************************************************************/

#include "generator_aarch64_instructions.h"
#include "generator_mateltwise_aarch64.h"
#include "generator_mateltwise_aarch64_sve.h"
#include "generator_common_aarch64.h"
#include "generator_common.h"
#include "generator_mateltwise_unary_binary_aarch64.h"
#include "libxsmm_main.h"

#define MN_LOOP_ORDER 0
#define NM_LOOP_ORDER 1
#define LOOP_TYPE_M 0
#define LOOP_TYPE_N 1

LIBXSMM_API_INTERN
void adjust_after_microkernel_addr_aarch64_gp_reg( libxsmm_generated_code*                 io_generated_code,
                                                   libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                   libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                   const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                   unsigned int                            i_gp_reg,
                                                   unsigned int                            i_adjust_instr,
                                                   unsigned int                            m_microkernel,
                                                   unsigned int                            n_microkernel,
                                                   unsigned int                            i_loop_type ) {
  unsigned int is_inp_gp_reg = ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) || ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) )) ? 1 : 0;
  unsigned int is_out_gp_reg = (i_gp_reg == i_gp_reg_mapping->gp_reg_out) ? 1 : 0;
  unsigned int bcast_row = (((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_ROW) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_ROW_IN_0) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_ROW_IN_1) > 0))) ? 1 : 0;
  unsigned int bcast_col = (((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_COL) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_COL_IN_0) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_COL_IN_1) > 0))) ? 1 : 0;
  unsigned int bcast_scalar = (((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_SCALAR) > 0)) ||
                               ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_SCALAR_IN_0) > 0)) ||
                               ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_SCALAR_IN_1) > 0))) ? 1 : 0;
  unsigned int bcast_input = ( bcast_row == 1 || bcast_col == 1 || bcast_scalar == 1 ) ? 1 : 0;

  if ((is_inp_gp_reg > 0) || (is_out_gp_reg > 0)) {
    unsigned int tsize  = (is_inp_gp_reg > 0) ? i_micro_kernel_config->datatype_size_in : i_micro_kernel_config->datatype_size_out;
    unsigned int ld     = (is_inp_gp_reg > 0) ? (((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY)) ? i_mateltwise_desc->ldi2 : i_mateltwise_desc->ldi) : i_mateltwise_desc->ldo;

    if (bcast_input == 0) {
      if (i_loop_type == LOOP_TYPE_M) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel * tsize );
      } else {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, ld * n_microkernel * tsize);
      }
    } else {
      if (bcast_row > 0) {
        if (i_loop_type == LOOP_TYPE_N) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, n_microkernel * tsize);
        }
      }
      if (bcast_col > 0) {
        if (i_loop_type == LOOP_TYPE_M) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel * tsize);
        }
      }
    }
  } else {
    /* Advance relumasks if need be */
    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY ) {
      if ( ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) )
           && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0)) {
        if (i_loop_type == LOOP_TYPE_M) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel/8);
        } else {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->ldo_mask * n_microkernel)/8);
        }
      }

      if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV)       ||
           (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
           (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV)           ) {
        if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
          if (i_loop_type == LOOP_TYPE_M) {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel/8);
          } else {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->ldi_mask * n_microkernel)/8);
          }
        } else {
          if (i_loop_type == LOOP_TYPE_M) {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel * i_micro_kernel_config->datatype_size_in);
          } else {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, i_mateltwise_desc->ldi * n_microkernel * i_micro_kernel_config->datatype_size_in );
          }
        }
      }

      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0)) {
        if (i_loop_type == LOOP_TYPE_M) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel/8);
        } else {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->ldo_mask * n_microkernel)/8);
        }
      }

      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
        if ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) {
          if (i_loop_type == LOOP_TYPE_M) {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, m_microkernel/8);
          } else {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->ldi_mask * n_microkernel)/8);
          }
        } else {
          LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_BITMASK_REQUIRED );
          return;
        }
      }
    }
  }
}

LIBXSMM_API_INTERN
void adjust_in_microkernel_addr_aarch64_gp_reg( libxsmm_generated_code*                 io_generated_code,
                                                libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                unsigned int                            i_gp_reg,
                                                unsigned int                            i_adjust_instr,
                                                unsigned int                            i_adjust_param,
                                                unsigned int                            i_loop_type ) {
  unsigned int is_inp_gp_reg = ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) || ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) )) ? 1 : 0;
  unsigned int is_out_gp_reg = (i_gp_reg == i_gp_reg_mapping->gp_reg_out) ? 1 : 0;
  unsigned int bcast_row = (((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_ROW) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_ROW_IN_0) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_ROW_IN_1) > 0))) ? 1 : 0;
  unsigned int bcast_col = (((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_COL) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_COL_IN_0) > 0)) ||
                            ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_COL_IN_1) > 0))) ? 1 : 0;
  unsigned int bcast_scalar = (((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_SCALAR) > 0)) ||
                               ((i_gp_reg == i_gp_reg_mapping->gp_reg_in) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_SCALAR_IN_0) > 0)) ||
                               ((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_SCALAR_IN_1) > 0))) ? 1 : 0;
  unsigned int bcast_input = ( bcast_row == 1 || bcast_col == 1 || bcast_scalar == 1 ) ? 1 : 0;

  if ((is_inp_gp_reg > 0) || (is_out_gp_reg > 0)) {
    unsigned int vlen   = (is_inp_gp_reg > 0) ? i_micro_kernel_config->vlen_in : i_micro_kernel_config->vlen_out;
    unsigned int tsize  = (is_inp_gp_reg > 0) ? i_micro_kernel_config->datatype_size_in : i_micro_kernel_config->datatype_size_out;
    unsigned int ld     = (is_inp_gp_reg > 0) ? (((i_gp_reg == i_gp_reg_mapping->gp_reg_in2) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) ) ? i_mateltwise_desc->ldi2 : i_mateltwise_desc->ldi) : i_mateltwise_desc->ldo;

    if (bcast_input == 0) {
      if (i_loop_type == LOOP_TYPE_M) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, vlen * i_adjust_param * tsize  );
      } else {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, i_adjust_param * tsize * ld );
      }
    } else {
      if (bcast_row > 0) {
        if (i_loop_type == LOOP_TYPE_N) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, i_adjust_param * tsize );
        }
      }
      if (bcast_col > 0) {
        if (i_loop_type == LOOP_TYPE_M) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, vlen * i_adjust_param * tsize );
        }
      }
    }
  } else {
    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY ) {
      if (((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU)) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0)) {
        /* @TODO Evangelos: why is here i_gp_reg_mapping->gp_reg_relumask used? */
        if (i_loop_type == LOOP_TYPE_M) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask, (i_micro_kernel_config->vlen_out * i_adjust_param)/8 );
        } else {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask, (i_micro_kernel_config->ldo_mask * i_adjust_param)/8 );
        }
      }

      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV)) {
        if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
          if (i_loop_type == LOOP_TYPE_M) {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->vlen_in * i_adjust_param)/8);
          } else {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->ldi_mask * i_adjust_param)/8);
          }
        } else {
          if (i_loop_type == LOOP_TYPE_M) {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, i_micro_kernel_config->vlen_in * i_adjust_param * i_micro_kernel_config->datatype_size_in);
          } else {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, i_mateltwise_desc->ldi * i_adjust_param * i_micro_kernel_config->datatype_size_in );
          }
        }
      }

      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0)) {
        /* @TODO Evangelos: copied from ReLU.... */
        if (i_loop_type == LOOP_TYPE_M) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg_mapping->gp_reg_dropoutmask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_dropoutmask, (i_micro_kernel_config->vlen_out * i_adjust_param)/8 );
        } else {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg_mapping->gp_reg_dropoutmask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_dropoutmask, (i_micro_kernel_config->ldo_mask * i_adjust_param)/8 );
        }
      }

      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
        if ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) {
          if (i_loop_type == LOOP_TYPE_M) {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->vlen_in * i_adjust_param)/8);
          } else {
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, i_adjust_instr, i_gp_reg, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg, (i_micro_kernel_config->ldi_mask * i_adjust_param)/8);
          }
        } else {
          LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_BITMASK_REQUIRED );
          return;
        }
      }
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_generator_configure_aarch64_vlens(const libxsmm_meltw_descriptor* i_mateltwise_desc, libxsmm_mateltwise_kernel_config* i_micro_kernel_config) {

  /* First, determine the vlen compute based on the architecture; there may be architectures with different widths for different types */
  /* At the moment, all types are assumed to be of the same length */
  unsigned int l_asimd_bytes_per_register = libxsmm_cpuid_vlen32(i_micro_kernel_config->instruction_set) * 4;

  unsigned char l_inp_type = LIBXSMM_GETENUM_INP(i_mateltwise_desc->datatype2);
  unsigned int  l_inp_type_size = LIBXSMM_TYPESIZE(l_inp_type);/* like libxsmm_typesize; returns 0 if type is unknown */
  if(l_inp_type_size > 0) i_micro_kernel_config->vlen_comp = l_asimd_bytes_per_register / l_inp_type_size;

  /* The vlen_in is the same as vlen compute */
  i_micro_kernel_config->vlen_in = i_micro_kernel_config->vlen_comp;

  /* The vlen_out depends on the output datatype */
  unsigned char l_out_type = LIBXSMM_GETENUM_OUT(i_mateltwise_desc->datatype);
  unsigned int  l_out_type_size = LIBXSMM_TYPESIZE(l_out_type);
  if(l_out_type_size > 0) i_micro_kernel_config->vlen_out = l_asimd_bytes_per_register / l_out_type_size;

  /* if the computation is done in F32 or the input is in F32, then set vlen_out to 16 */
  if ( LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype ) ||
       LIBXSMM_DATATYPE_I16 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype ) ||
       LIBXSMM_DATATYPE_F16 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype )) {
    if ( LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype2 ) ||
        LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype ) )
    {
      i_micro_kernel_config->vlen_out= 4;
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_generator_configure_aarch64_M_N_blocking( libxsmm_generated_code*         io_generated_code,
                                                       const libxsmm_meltw_descriptor* i_mateltwise_desc,
                                                       unsigned int m,
                                                       unsigned int n,
                                                       unsigned int vlen,
                                                       unsigned int *m_blocking,
                                                       unsigned int *n_blocking,
                                                       unsigned int available_vregs ) {
  /* The m blocking is done in chunks of vlen */
  unsigned int m_chunks = 0;
  /* TODO: Make m chunk remainder depend on number of available vector registers */
  unsigned int m_chunk_remainder = 4;/* how many registers are required for other "functions" */
  unsigned int m_chunk_boundary = 32;
  unsigned int m_range, m_block_size, foo1, foo2;

  /* in order to work with bitmasks we need at least 8 entries, on ASIMD, that means 2 registers */
  /* TODO: for SVE, we maybe could use the predicate registers, so we don't need to half the count; however, we only have 15 predicate registers, so this still may be correct*/
  if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (vlen == 4) ) {
    vlen *= 2;
    if ( available_vregs < 3 ) {
      LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_BITMASK_REQUIRED );
      return;
    }
    available_vregs /= 2;
    m_chunk_boundary /= 2;
  }

  m_chunks = LIBXSMM_UPDIV(m, vlen); /* (m+vlen-1)/vlen; */
  m_chunks &= ~1; /* round down to even numbers, (m_chunks/2)*2 */

  if (m % vlen == 0) {
    /* If there is not remainder in M, then we block M in order to limit block size */
    if (m_chunks > m_chunk_boundary) {
      libxsmm_compute_equalized_blocking(m_chunks, (m_chunks+1)/2, &m_range, &m_block_size, &foo1, &foo2);
      *m_blocking = m_range * vlen;
    } else {
      *m_blocking = m;
    }
  } else {
    /* If there is remainder we make sure we can fully unroll the kernel with masks */
    if (m_chunks > (m_chunk_boundary/2)) {
      *m_blocking = (m_chunks - m_chunk_remainder) * vlen;
    } else {
      if (available_vregs * vlen >= m) {
        *m_blocking = m;
      } else {
        *m_blocking = (m_chunks - 1) * vlen;
      }
    }
  }
  /* For now not any additional blocking in N */
  *n_blocking = n;
}

LIBXSMM_API_INTERN
void libxsmm_generator_configure_aarch64_loop_order(const libxsmm_meltw_descriptor* i_mateltwise_desc, unsigned int *loop_order, unsigned int *m_blocking, unsigned int *n_blocking, unsigned int *out_blocking, unsigned int *inner_blocking, unsigned int *out_bound, unsigned int *inner_bound) {
  unsigned int _loop_order = NM_LOOP_ORDER;

  /* TODO: Potentially reorder loops given the kernel type */
  *loop_order = _loop_order;

  if (_loop_order == NM_LOOP_ORDER) {
    *out_blocking = *n_blocking;
    *out_bound = i_mateltwise_desc->n;
    *inner_blocking = *m_blocking;
    *inner_bound = i_mateltwise_desc->m;
  } else {
    *out_blocking = *m_blocking;
    *out_bound = i_mateltwise_desc->m;
    *inner_blocking = *n_blocking;
    *inner_bound = i_mateltwise_desc->n;
  }
}

LIBXSMM_API_INTERN
void libxsmm_load_aarch64_2d_reg_block( libxsmm_generated_code*                 io_generated_code,
                                        libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                        const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                        const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                        unsigned int                            i_vlen,
                                        unsigned int                            i_start_vreg,
                                        unsigned int                            i_m_blocking,
                                        unsigned int                            i_n_blocking,
                                        unsigned int                            i_mask_last_m_chunk,
                                        unsigned int                            i_mask_reg) {
  unsigned int im, in, cur_vreg;
  unsigned int bcast_row = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_ROW) > 0)) ||
                            ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_ROW_IN_0) > 0))) ? 1 : 0;
  unsigned int bcast_col = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_COL) > 0)) ||
                            ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_COL_IN_0) > 0))) ? 1 : 0;
  unsigned int bcast_scalar = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_SCALAR) > 0)) ||
                               ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_SCALAR_IN_0) > 0))) ? 1 : 0;
  unsigned int bcast_input = ( bcast_row == 1 || bcast_col == 1 || bcast_scalar == 1 ) ? 1 : 0;
  unsigned int l_ld_bytes = i_mateltwise_desc->ldi * i_micro_kernel_config->datatype_size_in;
  unsigned int l_m_adjust = ( i_mask_last_m_chunk == 0 ) ? i_micro_kernel_config->datatype_size_in * i_vlen * i_m_blocking : i_micro_kernel_config->datatype_size_in * ( (i_vlen * (i_m_blocking-1)) + i_mask_last_m_chunk );
  unsigned int offset = 0;
  LIBXSMM_UNUSED(i_mask_reg);

  unsigned char l_is_sve = io_generated_code->arch == LIBXSMM_AARCH64_A64FX;
  libxsmm_aarch64_sve_type l_sve_type = libxsmm_generator_aarch64_get_sve_type(i_micro_kernel_config->datatype_size_in);

  /* In this case we don't have to load any data  */
  if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_XOR)) return;
#if 0 /* todo: code from X86, that is still missing in ASIMD/SVE */
  if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_PACK)) {
    for (in = 0; in < i_n_blocking; in++) {
      for (im = 0; im < i_m_blocking; im++) {
        cur_vreg = i_start_vreg + in * i_m_blocking + im;
        libxsmm_x86_instruction_vec_move( io_generated_code,
            io_generated_code->arch,
            LIBXSMM_X86_INSTR_VPMOVZXWD,
            i_gp_reg_mapping->gp_reg_in,
            LIBXSMM_X86_GP_REG_UNDEF,
            0,
            (im * i_vlen + in * i_mateltwise_desc->ldi) * i_micro_kernel_config->datatype_size_in,
            'z',
            cur_vreg,
            ((i_mask_last_m_chunk == 1) && (im == i_m_blocking - 1)) ? i_mask_reg : 0,
            0, 0);
      }
    }
    return;
  }
#endif

  for (in = 0; in < i_n_blocking; in++) {
    for (im = 0; im < i_m_blocking; im++) {
      cur_vreg = i_start_vreg + in * i_m_blocking + im;
      if ( bcast_input == 0) {
        offset = (l_ld_bytes*i_n_blocking);
        libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_in, i_gp_reg_mapping->gp_reg_scratch_1, cur_vreg,
                                                                i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 0 );
        /* If compute is in F32 and input is BF16 (or input is BF16 and output is F32), then upconvert BF16 -> FP32 */
#if 0 /* todo: code from X86, that is still missing in ASIMD/SVE */
        if ( (LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype2 ) && LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype)) ||
             (LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype  ) && LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype)) ) {
          libxsmm_generator_cvtbf16ps_avx512( io_generated_code, 'z', cur_vreg, cur_vreg );
        }
#endif
      } else {
        if ( (bcast_row == 1) || (bcast_scalar == 1) ) {
          offset = (bcast_scalar == 1) ?  i_micro_kernel_config->datatype_size_in:i_micro_kernel_config->datatype_size_in*i_n_blocking;
          if (im == 0) {
            if ((bcast_row == 1) || ((bcast_scalar == 1) && (in == 0))) {
              libxsmm_generator_bcastload_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_in, i_gp_reg_mapping->gp_reg_scratch_1, cur_vreg,
                                                                     i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1 );
            } else if ((bcast_scalar == 1) && (in > 0) && (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) ) {
              if(l_is_sve){
                libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ORR_V,
                                                         i_start_vreg, i_start_vreg, 0, cur_vreg, 0, l_sve_type);
              } else {
                libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V,
                                                           i_start_vreg, i_start_vreg, 0, cur_vreg,
                                                           (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D );
              }
            }
          }

           if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY)) {
             /* Copy the register to the rest of the "M-registers" in this case....  */
             if (im > 0) {
               if(l_is_sve){
                 libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ORR_V,
                                                          i_start_vreg + in * i_m_blocking, i_start_vreg + in * i_m_blocking, 0, cur_vreg, 0, l_sve_type);
               } else {
                 libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V,
                                                            i_start_vreg + in * i_m_blocking, i_start_vreg + in * i_m_blocking, 0, cur_vreg,
                                                            (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D );
               }
            }
          }

        }

        if ( bcast_col == 1 ) {
          offset = ( i_mask_last_m_chunk == 0 ) ? i_micro_kernel_config->datatype_size_in * i_vlen * i_m_blocking : i_micro_kernel_config->datatype_size_in * ( (i_vlen * (i_m_blocking-1)) + i_mask_last_m_chunk );
          if (in == 0) {
            libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_in, i_gp_reg_mapping->gp_reg_scratch_1, cur_vreg,
                                                                    i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 0 );
          }
#if 0 /* todo: code from X86 that is missing in ASIMD/SVE */
            /* If compute is in F32 and input is BF16 (or input is BF16 and output is F32), then upconvert BF16 -> FP32 */
            if ( (LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype2 ) && LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype)) ||
                 (LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype  ) && LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype)) ) {
              libxsmm_generator_cvtbf16ps_avx512( io_generated_code, 'z', cur_vreg, cur_vreg );
            }
          }
#endif
          if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY)) {
            /* Copy the register to the rest of the "N-REGISTERS" in this case....  */
            if (in > 0) {
              if(l_is_sve){
                libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ORR_V,
                                                         i_start_vreg + im, i_start_vreg + im, 0, cur_vreg, 0, l_sve_type );
              } else {
                libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V,
                                                           i_start_vreg + im, i_start_vreg + im, 0, cur_vreg,
                                                           (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D );

              }
            }
          }
        }
      }
    }
    if ( bcast_input == 0 ) {
      if ( l_ld_bytes != l_m_adjust ) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                      i_gp_reg_mapping->gp_reg_in, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_in,
                                                      (unsigned long long)((unsigned long long)(l_ld_bytes-l_m_adjust)) );
      }
    }
  }
  libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                  i_gp_reg_mapping->gp_reg_in, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_in,
                                                  (unsigned long long)((unsigned long long)(offset)) );
}

LIBXSMM_API_INTERN
void libxsmm_store_aarch64_2d_reg_block( libxsmm_generated_code*                 io_generated_code,
                                         libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                         const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                         const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                         unsigned int                            i_vlen,
                                         unsigned int                            i_start_vreg,
                                         unsigned int                            i_m_blocking,
                                         unsigned int                            i_n_blocking,
                                         unsigned int                            i_mask_last_m_chunk,
                                         unsigned int                            i_mask_reg) {
  unsigned int im, in, cur_vreg;
  unsigned int bcast_row = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_ROW) > 0))) ? 1 : 0;
  unsigned int bcast_col = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_COL) > 0))) ? 1 : 0;
  unsigned int bcast_scalar = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_SCALAR) > 0))) ? 1 : 0;
  unsigned int l_ld_bytes = i_mateltwise_desc->ldo * i_micro_kernel_config->datatype_size_out;
  unsigned int l_m_adjust = ( i_mask_last_m_chunk == 0 ) ? i_micro_kernel_config->datatype_size_in * i_vlen * i_m_blocking : i_micro_kernel_config->datatype_size_in * ( (i_vlen * (i_m_blocking-1)) + i_mask_last_m_chunk );
  LIBXSMM_UNUSED(i_mask_reg);
#if 0 /* todo: code from X86, that is still missing in ASIMD/SVE */
  if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_UNPACK_TO_BLOCKS)) {
    for (in = 0; in < i_n_blocking; in++) {
      for (im = 0; im < i_m_blocking; im++) {
        cur_vreg = i_start_vreg + in * i_m_blocking + im;
        libxsmm_x86_instruction_vec_move( io_generated_code,
            io_generated_code->arch,
            LIBXSMM_X86_INSTR_VPMOVDW,
            i_gp_reg_mapping->gp_reg_out,
            LIBXSMM_X86_GP_REG_UNDEF,
            0,
            (im * i_vlen + in * i_mateltwise_desc->ldo) * i_micro_kernel_config->datatype_size_out,
            'z',
            cur_vreg,
            ((i_mask_last_m_chunk == 1) && (im == i_m_blocking - 1)) ? i_mask_reg : 0,
            0, 1);
        libxsmm_x86_instruction_vec_compute_2reg_imm8( io_generated_code, LIBXSMM_X86_INSTR_VPSRAD_I, 'z', cur_vreg, cur_vreg, 16 );
        libxsmm_x86_instruction_vec_move( io_generated_code,
            io_generated_code->arch,
            LIBXSMM_X86_INSTR_VPMOVDW,
            i_gp_reg_mapping->gp_reg_out,
            i_gp_reg_mapping->gp_reg_offset,
            1,
            (im * i_vlen + in * i_mateltwise_desc->ldo) * i_micro_kernel_config->datatype_size_out,
            'z',
            cur_vreg,
            ((i_mask_last_m_chunk == 1) && (im == i_m_blocking - 1)) ? i_mask_reg : 0,
            0, 1);
      }
    }
  } else if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) && (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_PACK)) {
    for (in = 0; in < i_n_blocking; in++) {
      for (im = 0; im < i_m_blocking; im++) {
        cur_vreg = i_start_vreg + in * i_m_blocking + im;
        libxsmm_x86_instruction_unified_vec_move( io_generated_code,
            i_micro_kernel_config->vmove_instruction_out,
            i_gp_reg_mapping->gp_reg_out,
            LIBXSMM_X86_GP_REG_UNDEF, 0,
            (im * i_vlen + in * i_mateltwise_desc->ldo) * i_micro_kernel_config->datatype_size_out,
            'z',
            cur_vreg, ((i_mask_last_m_chunk == 1) && (im == i_m_blocking - 1)) ? 1 : 0, ((i_mask_last_m_chunk == 1) && (im == i_m_blocking - 1)) ? i_mask_reg : 0, 1 );
      }
    }
  }  else {
#endif
  for (in = 0; in < i_n_blocking; in++) {
    for (im = 0; im < i_m_blocking; im++) {
      cur_vreg = i_start_vreg + in * i_m_blocking + im;
#if 0 /* todo: code from X86, that might be missing for ASIMD/SVE */
        cur_vreg_real = i_start_vreg + in * i_m_blocking + im;
#endif
      /* In the XOR case we have a constant vreg  */
      if ((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_XOR)) {
        cur_vreg = i_micro_kernel_config->zero_vreg;
      } else {
        if (bcast_row == 1) {
          cur_vreg = i_start_vreg + in * i_m_blocking;
        }
#if 0 /* todo: code from X86, that still is missing in ASIMD/SVE */
          /* If compute is in F32 and output is BF16 (or input is F32 and output is BF16), then downconvert BF16 -> FP32 */
          if ( (cur_vreg == cur_vreg_real) &&
           ((LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype2 ) || LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype )) &&
             LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype )) ) {
          if ( io_generated_code->arch >= LIBXSMM_X86_AVX512_CPX ) {
            libxsmm_x86_instruction_vec_compute_2reg( io_generated_code, LIBXSMM_X86_INSTR_VCVTNEPS2BF16,
                                                      i_micro_kernel_config->vector_name,
                                                      cur_vreg, cur_vreg );
          } else {
            libxsmm_generator_vcvtneps2bf16_avx512_preppedstack( io_generated_code, i_micro_kernel_config->vector_name,
                                                                 cur_vreg, cur_vreg,
                                                                 i_micro_kernel_config->dcvt_zmm_aux0, i_micro_kernel_config->dcvt_zmm_aux1, i_micro_kernel_config->dcvt_mask_aux0, i_micro_kernel_config->dcvt_mask_aux1);
          }
#endif
        if (bcast_scalar == 1) {
          cur_vreg = i_start_vreg;
        }
        if (bcast_col == 1) {
          cur_vreg = i_start_vreg + im;
        }
      }
      libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_out, i_gp_reg_mapping->gp_reg_scratch_1, cur_vreg,
                                                              i_micro_kernel_config->datatype_size_out, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 1 );
    }
    if ( l_ld_bytes != l_m_adjust ) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                      i_gp_reg_mapping->gp_reg_out, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_out,
                                                     (unsigned long long)((unsigned long long)(l_ld_bytes-l_m_adjust)) );
    }
  }
  libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                 i_gp_reg_mapping->gp_reg_out, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_out,
                                                 (unsigned long long)((unsigned long long)(l_ld_bytes*i_n_blocking)) );
}

LIBXSMM_API_INTERN
void libxsmm_compute_unary_aarch64_2d_reg_block_op( libxsmm_generated_code*                 io_generated_code,
                                                    libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                    const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                    const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                    unsigned int                            i_vlen,
                                                    unsigned int                            i_start_vreg,
                                                    unsigned int                            i_m_blocking,
                                                    unsigned int                            i_n_blocking,
                                                    unsigned int                            i_mask_last_m_chunk,
                                                    unsigned int                            i_mask_reg) {
  unsigned int im, in, cur_vreg;
  unsigned int bcast_row = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_ROW) > 0))) ? 1 : 0;
  unsigned int bcast_col = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_COL) > 0))) ? 1 : 0;
  unsigned int bcast_scalar = (((i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BCAST_SCALAR) > 0))) ? 1 : 0;

  unsigned char l_is_sve = io_generated_code->arch == LIBXSMM_AARCH64_A64FX;

  LIBXSMM_UNUSED(i_gp_reg_mapping);
  LIBXSMM_UNUSED(i_vlen);
  LIBXSMM_UNUSED(i_mask_last_m_chunk);

  unsigned char l_pred_reg = i_mask_reg;
  libxsmm_aarch64_sve_type l_sve_type = libxsmm_generator_aarch64_get_sve_type(i_micro_kernel_config->datatype_size_in);
  libxsmm_aarch64_asimd_tupletype l_tupletype = (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D;

  unsigned char l_op_needs_predicates =
    i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_X2 &&
    i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL &&
    i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL_SQRT;

  if(l_op_needs_predicates && l_is_sve){
    libxsmm_generator_set_p_register_aarch64_sve( io_generated_code, l_pred_reg, -1, 0 );
  }

  for (in = 0; in < i_n_blocking; in++) {
    if ((bcast_col == 1) && (in > 0)) {
      continue;
    }
    for (im = 0; im < i_m_blocking; im++) {
      if ((bcast_row == 1) && (im > 0)) {
        continue;
      }
      if ((bcast_scalar == 1) && ((im > 0) || (in > 0))) {
        continue;
      }

      cur_vreg = i_start_vreg + in * i_m_blocking + im;
      switch(i_mateltwise_desc->param){
        case LIBXSMM_MELTW_TYPE_UNARY_X2:
          if( l_is_sve ) {
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                     cur_vreg, cur_vreg, 0, cur_vreg, l_pred_reg, l_sve_type );
          } else {/* ASIMD */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V,
                                                       cur_vreg, cur_vreg, 0, cur_vreg,
                                                       l_tupletype );
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_NEGATE:
          if( l_is_sve ) {
            /* fneg only exists predicated */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FNEG_V_P,
                                                     cur_vreg, cur_vreg, 0, cur_vreg, l_pred_reg, l_sve_type );
          } else {
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FNEG_V,
                                                       cur_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, cur_vreg,
                                                       l_tupletype );
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_INC:
          if( l_is_sve ) {
            /* either create a 1-register like asimd, or add immediate value */
            /* todo test which one is faster (if it matters somehow) */
            unsigned char l_immediate_enum = 1; /* 0 = 0.5, 1 = 1.0 */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FADD_I_P,
                                                     l_immediate_enum, 0, 0, cur_vreg, l_pred_reg, l_sve_type );
          } else {
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FADD_V,
                                                       cur_vreg, i_micro_kernel_config->vec_ones, 0, cur_vreg,
                                                       l_tupletype );
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL:
          if( l_is_sve ) {
            /* can we improve the performance by using multiple temporary registers? no,still 2.60x faster than ASIMD on 64x64  */
            /* one iteration step is close to perfect with 1/[1..50] */
            if(libxsmm_get_ulp_precision() != LIBXSMM_ULP_PRECISION_ESTIMATE){
              unsigned char tmp_vreg = i_micro_kernel_config->tmp_vreg;
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FRECPE_V, /* save the estimate in tmp */
                                                       cur_vreg, cur_vreg, 0, tmp_vreg, l_pred_reg, l_sve_type );
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FRECPS_V, /* compute the improvement by tmp,cur into cur */
                                                       cur_vreg, tmp_vreg, 0, cur_vreg, l_pred_reg, l_sve_type);
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V, /* apply the improvement on tmp, and write result into cur */
                                                       cur_vreg, tmp_vreg, 0, cur_vreg, l_pred_reg, l_sve_type);
            } else {/* if we don't really care about precision, we can skip the extra iteration */
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FRECPE_V,
                                                       cur_vreg, cur_vreg, 0, cur_vreg, l_pred_reg, l_sve_type );
            }
          } else {
            /* todo: this is only an approximation */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FRECPE_V,
                                                       cur_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, cur_vreg,
                                                       l_tupletype );
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL_SQRT:
          /* typical relative error in tests (iterations = 0, fp32): 5-8% */
          /* typical relative error in tests (iterations = 1, fp32): 0.06% */
          /* typical relative error in tests (iterations = 2, fp32): 0.001% */
          /* typical relative error in tests (iterations = 3, fp32): 0.00002% */
          /* typical relative error in tests (iterations = 4, fp32): 0.0002% */
          {
            /* number needs to be adjusted, if the type is fp64 or bf16 */
            /* fp32 is type 0x02; number of iterations for bytes: 0, bf16: 1, fp32: 3, fp64: 7 */
            unsigned char max_num_iterations = (1 << (unsigned char) l_sve_type) - 1;
            unsigned char num_iterations = libxsmm_get_ulp_precision() == LIBXSMM_ULP_PRECISION_ESTIMATE ? 0 : max_num_iterations;
            if( l_is_sve ) {
              unsigned char tmp_guess = i_micro_kernel_config->tmp_vreg;
              unsigned char tmp_guess_squared = i_micro_kernel_config->tmp_vreg2;
              /* Newton iteration: guess *= (3-guess*guess*x)/2 */
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FRSQRTE_V,
                                                      cur_vreg, cur_vreg, 0, num_iterations > 0 ? tmp_guess : cur_vreg, l_pred_reg, l_sve_type);
              unsigned char i;
              for(i=0;i<num_iterations;i++){
                unsigned char dst_reg = i == num_iterations-1 ? cur_vreg : tmp_guess;/* improve the guess; then save it */
                libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                         tmp_guess, tmp_guess, 0, tmp_guess_squared, l_pred_reg, l_sve_type);
                libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FRSQRTS_V, /* dst = (3-s0*s1)/2 */
                                                         cur_vreg, tmp_guess_squared, 0, tmp_guess_squared, l_pred_reg, l_sve_type);
                libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                         tmp_guess, tmp_guess_squared, 0, dst_reg, l_pred_reg, l_sve_type);
              }
            } else {
              /* todo: this only is an estimate as well, apply Newton iterations to improve the results */
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FRSQRTE_V,
                                                      cur_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, cur_vreg,
                                                      l_tupletype );
            }
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_SQRT:
          if( l_is_sve ) {
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FSQRT_V_P,
                                                     cur_vreg, cur_vreg, 0, cur_vreg, l_pred_reg, l_sve_type);
          } else {
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FSQRT_V,
                                                     cur_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, cur_vreg,
                                                     l_tupletype );
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_EXP:
          libxsmm_generator_exp_ps_3dts_aarch64(
            io_generated_code,
            cur_vreg,
            i_micro_kernel_config->vec_y,
            i_micro_kernel_config->vec_z,
            i_micro_kernel_config->vec_c0,
            i_micro_kernel_config->vec_c1,
            i_micro_kernel_config->vec_c2,
            i_micro_kernel_config->vec_c3,
            i_micro_kernel_config->vec_halves,
            i_micro_kernel_config->vec_log2e,
            i_micro_kernel_config->vec_expmask,
            i_micro_kernel_config->vec_hi_bound,
            i_micro_kernel_config->vec_lo_bound,
            l_tupletype, l_sve_type, l_pred_reg );
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_TANH:
        case LIBXSMM_MELTW_TYPE_UNARY_TANH_INV:
          libxsmm_generator_tanh_ps_rational_78_aarch64(
            io_generated_code,
            cur_vreg,
            i_micro_kernel_config->vec_x2,
            i_micro_kernel_config->vec_nom,
            i_micro_kernel_config->vec_denom,
            i_micro_kernel_config->mask_hi,
            i_micro_kernel_config->mask_lo,
            i_micro_kernel_config->vec_c0,
            i_micro_kernel_config->vec_c1,
            i_micro_kernel_config->vec_c2,
            i_micro_kernel_config->vec_c3,
            i_micro_kernel_config->vec_c1_d,
            i_micro_kernel_config->vec_c2_d,
            i_micro_kernel_config->vec_c3_d,
            i_micro_kernel_config->vec_hi_bound,
            i_micro_kernel_config->vec_lo_bound,
            i_micro_kernel_config->vec_ones,
            i_micro_kernel_config->vec_neg_ones,
            i_micro_kernel_config->vec_tmp0,
            l_tupletype, l_sve_type, l_pred_reg );

          if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_TANH_INV) {/* 1st derivative of tanh(x) = 1-tanh(x)^2 */
            if(l_is_sve){
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                         cur_vreg, cur_vreg, 0, i_micro_kernel_config->vec_tmp0,
                                                         l_pred_reg, l_sve_type );
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FNEG_V_P,
                                                         i_micro_kernel_config->vec_tmp0, LIBXSMM_AARCH64_SVE_REG_UNDEF, 0,  i_micro_kernel_config->vec_tmp0,
                                                         l_pred_reg, l_sve_type );
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FSUB_V,
                                                         i_micro_kernel_config->vec_tmp0, i_micro_kernel_config->vec_neg_ones, 0, cur_vreg,
                                                         l_pred_reg, l_sve_type );
            } else {
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V,
                                                         cur_vreg, cur_vreg, 0, i_micro_kernel_config->vec_tmp0,
                                                         l_tupletype );
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FNEG_V,
                                                         i_micro_kernel_config->vec_tmp0, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0,  i_micro_kernel_config->vec_tmp0,
                                                         l_tupletype );
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FSUB_V,
                                                         i_micro_kernel_config->vec_tmp0, i_micro_kernel_config->vec_neg_ones, 0, cur_vreg,
                                                         l_tupletype );
            }
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_SIGMOID:
        case LIBXSMM_MELTW_TYPE_UNARY_SIGMOID_INV:
          libxsmm_generator_sigmoid_ps_rational_78_aarch64(
            io_generated_code,
            cur_vreg,
            i_micro_kernel_config->vec_x2,
            i_micro_kernel_config->vec_nom,
            i_micro_kernel_config->vec_denom,
            i_micro_kernel_config->mask_hi,
            i_micro_kernel_config->mask_lo,
            i_micro_kernel_config->vec_c0,
            i_micro_kernel_config->vec_c1,
            i_micro_kernel_config->vec_c2,
            i_micro_kernel_config->vec_c3,
            i_micro_kernel_config->vec_c1_d,
            i_micro_kernel_config->vec_c2_d,
            i_micro_kernel_config->vec_c3_d,
            i_micro_kernel_config->vec_hi_bound,
            i_micro_kernel_config->vec_lo_bound,
            i_micro_kernel_config->vec_ones,
            i_micro_kernel_config->vec_neg_ones,
            i_micro_kernel_config->vec_halves,
            i_micro_kernel_config->vec_tmp0,
            l_tupletype, l_sve_type, l_pred_reg );

          if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_SIGMOID_INV) {
            if(l_is_sve){
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FSUB_V,
                                                       i_micro_kernel_config->vec_ones, cur_vreg, 0, i_micro_kernel_config->vec_x2,
                                                       l_pred_reg, l_sve_type );
              libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                       i_micro_kernel_config->vec_x2, cur_vreg, 0, cur_vreg,
                                                       l_pred_reg, l_sve_type );
            } else {
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FSUB_V,
                                                         i_micro_kernel_config->vec_ones, cur_vreg, 0, i_micro_kernel_config->vec_x2,
                                                         l_tupletype );
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V,
                                                         i_micro_kernel_config->vec_x2, cur_vreg, 0, cur_vreg,
                                                         l_tupletype );
            }
          }
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_GELU:
          libxsmm_generator_gelu_ps_minimax3_aarch64( io_generated_code,
              cur_vreg,
              i_micro_kernel_config->vec_xr,
              i_micro_kernel_config->vec_xa,
              i_micro_kernel_config->vec_index,
              i_micro_kernel_config->vec_C0,
              i_micro_kernel_config->vec_C1,
              i_micro_kernel_config->vec_C2,
              i_micro_kernel_config->vec_thres,
              i_micro_kernel_config->vec_absmask,
              i_micro_kernel_config->vec_scale,
              i_micro_kernel_config->vec_shifter,
              i_micro_kernel_config->vec_halves,
              i_micro_kernel_config->vec_c0,
              i_micro_kernel_config->vec_c1,
              i_micro_kernel_config->vec_c2,
              i_micro_kernel_config->vec_tmp0,
              i_micro_kernel_config->vec_tmp1,
              l_tupletype, l_sve_type, l_pred_reg );
          break;
        case LIBXSMM_MELTW_TYPE_UNARY_GELU_INV:
          libxsmm_generator_gelu_inv_ps_minimax3_aarch64( io_generated_code,
              cur_vreg,
              i_micro_kernel_config->vec_xr,
              i_micro_kernel_config->vec_xa,
              i_micro_kernel_config->vec_index,
              i_micro_kernel_config->vec_C0,
              i_micro_kernel_config->vec_C1,
              i_micro_kernel_config->vec_C2,
              i_micro_kernel_config->vec_thres,
              i_micro_kernel_config->vec_absmask,
              i_micro_kernel_config->vec_scale,
              i_micro_kernel_config->vec_shifter,
              i_micro_kernel_config->vec_halves,
              i_micro_kernel_config->vec_c0,
              i_micro_kernel_config->vec_c1,
              i_micro_kernel_config->vec_c2,
              i_micro_kernel_config->vec_tmp0,
              i_micro_kernel_config->vec_tmp1,
              l_tupletype, l_sve_type, l_pred_reg );
          break;
      }
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_compute_unary_aarch64_2d_reg_block_relu( libxsmm_generated_code*                 io_generated_code,
                                                      libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                      const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                      const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                      unsigned int                            i_vlen,
                                                      unsigned int                            i_start_vreg,
                                                      unsigned int                            i_m_blocking,
                                                      unsigned int                            i_n_blocking,
                                                      unsigned int                            i_mask_last_m_chunk,
                                                      unsigned int                            i_mask_reg) {
  unsigned int im, in;
  LIBXSMM_UNUSED(i_mask_last_m_chunk);
  LIBXSMM_UNUSED(i_vlen);

  if(io_generated_code->arch == LIBXSMM_AARCH64_A64FX){

    unsigned char l_pred_reg = i_mask_reg;
    unsigned char l_blend_reg = 7;/* sve predicate register for blending; todo should be a function input / part of the config */
    unsigned char l_tmp_pred_reg = 6;/* tmp sve predicate register for blending; todo should be a function input / part of the config */
    libxsmm_aarch64_sve_type l_sve_type = libxsmm_generator_aarch64_get_sve_type(i_micro_kernel_config->datatype_size_in);

    for (in = 0; in < i_n_blocking; in++) {
      unsigned int l_mask_adv = 0;
      for (im = 0; im < i_m_blocking; im++) {
        /*unsigned int l_vlen = ( l_bf16_compute > 0 ) ? 32 : 16;*/
        unsigned int cur_vreg = i_start_vreg + in * i_m_blocking + im;

        if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU) ) {
          if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) ) {
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FCMGT_Z_V,
                                                     cur_vreg, LIBXSMM_AARCH64_SVE_REG_UNDEF, 0, l_blend_reg,
                                                     l_pred_reg, l_sve_type );
          }

          if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {

            /* warning! the current implementation assumes SVE with 512 vector bits (A64FX), 64 bit predicates, so 16 flags for fp32 */
            /* which perfectly fit into two bytes */

            /* store bitflags (l_blend_reg) to bitflag array in memory */

            /* mv l_blend_reg into l_tmp_pred_reg */
            /* UZP_P_E, 16 bits */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_UZP_P_E,
                                                     l_blend_reg, l_blend_reg, 0, l_tmp_pred_reg,
                                                     0, LIBXSMM_AARCH64_SVE_TYPE_H );
            /* UZP_P_E, 8 bits */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_UZP_P_E,
                                                     l_tmp_pred_reg, l_tmp_pred_reg, 0, l_tmp_pred_reg,
                                                     0, LIBXSMM_AARCH64_SVE_TYPE_B );

            /* ideal: store predicate into register, store register into memory (only 2 bytes) */
            /* Antonio can't find an instruction for that -> ensure the buffer has padding, and write over the end :/ */
            /* a hacky but less illegal way to do that would be to write to the stack, load from it, and store to the correct location */
            libxsmm_aarch64_instruction_sve_move( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_STR_P_I_OFF,
                                                  i_gp_reg_mapping->gp_reg_relumask, 0, 0, l_tmp_pred_reg, 0 );

            /* increment gp_reg_relumask by 2 */
            libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                           i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                           2 );
            l_mask_adv += 2;/* we wrote two bytes */

          }

          if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU ) {
            /* ReLU */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMAX_V_P, /* only exists predicated */
                                                     cur_vreg, i_micro_kernel_config->zero_vreg, 0, cur_vreg,
                                                     l_pred_reg, l_sve_type );
          } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU  ) {
            /* we need to multiply with alpha */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                     cur_vreg,  i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                     l_pred_reg, l_sve_type);

            /* now we blend both together */
            libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_SEL_V_P,
                                                     i_micro_kernel_config->tmp_vreg2, cur_vreg, 0, cur_vreg,
                                                     l_blend_reg, l_sve_type );
          } else {
            /* should not happen */
          }
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU )  {
          /* Compute exp */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ORR_V,
                                                   cur_vreg, cur_vreg, 0, i_micro_kernel_config->tmp_vreg2,
                                                   l_pred_reg, l_sve_type );
          libxsmm_generator_exp_ps_3dts_aarch64( io_generated_code,
              i_micro_kernel_config->tmp_vreg2, /* x */
              i_micro_kernel_config->tmp_vreg,  /* y */
              i_micro_kernel_config->tmp_vreg3, /* z */
              i_micro_kernel_config->vec_c0,
              i_micro_kernel_config->vec_c1,
              i_micro_kernel_config->vec_c2,
              i_micro_kernel_config->vec_c3,
              i_micro_kernel_config->vec_halves,
              i_micro_kernel_config->vec_log2e,
              i_micro_kernel_config->vec_expmask,
              i_micro_kernel_config->vec_hi_bound,
              i_micro_kernel_config->vec_lo_bound,
              LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S, l_sve_type, l_pred_reg );

          /* compute mask */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FCMGT_Z_V,
                                                   cur_vreg, LIBXSMM_AARCH64_SVE_REG_UNDEF, 0, l_blend_reg,
                                                   l_pred_reg, l_sve_type );

          /* compute ELU for < 0 */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                   i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                   l_pred_reg, l_sve_type );
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FSUB_V,
                                                   i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                   l_pred_reg, l_sve_type );

          /* blend exp-fma result with input reg based on elu mask */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_SEL_V_P,
                                                   i_micro_kernel_config->tmp_vreg2, cur_vreg, 0, cur_vreg,
                                                   l_blend_reg, l_sve_type );
        } else {
          /* shouldn't happen */
        }
      }
      if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU) ) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                       i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                       (i_micro_kernel_config->ldo_mask - (l_mask_adv*8))/8 );
      }
    }
    if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU) ) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                     i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                     i_n_blocking * (i_micro_kernel_config->ldo_mask/8) );
    }

  } else {

    libxsmm_aarch64_asimd_tupletype l_tupletype = (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D;

    for (in = 0; in < i_n_blocking; in++) {
      unsigned int l_mask_adv = 0;
      for (im = 0; im < i_m_blocking; im++) {
        /* unsigned int l_vlen = ( l_bf16_compute > 0 ) ? 32 : 16; removed because bf16 is not supported for ASIMD */
        unsigned int cur_vreg = i_start_vreg + in * i_m_blocking + im;

        if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU) ) {
          if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) ) {
            /* tmp_vreg = (cur_vreg > 0) */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FCMGT_Z_V,
                                                       cur_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, i_micro_kernel_config->tmp_vreg,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
          }

          if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {
            /* vreg = helper0/1 && tmp_vreg */
            if ( im % 2 == 0 ) {
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V,
                                                         i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->mask_helper0_vreg, 0, i_micro_kernel_config->tmp_vreg2,
                                                         LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
            } else {
              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V,
                                                         i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->mask_helper1_vreg, 0, i_micro_kernel_config->tmp_vreg2,
                                                         LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
            }
            /* reduce tmp_vreg2 via addition into tmp_vreg2, element size: int32 */
            /* this converts the 8-bit-strided bits of the elements into a bitmask within a single int32 */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ADDV_V,
                                                       i_micro_kernel_config->tmp_vreg2, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, i_micro_kernel_config->tmp_vreg2,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

            /* store tmp_vreg2 into *gp_reg_relumask[...] */
            if ( im % 2 == 0 ) {
              if ( im == i_m_blocking - 1 ) {
                libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_STR_I_POST,
                                                      i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_UNDEF, 1, i_micro_kernel_config->tmp_vreg2,
                                                      LIBXSMM_AARCH64_ASIMD_WIDTH_B );
                l_mask_adv++;
              } else {
                libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_STR_I_OFF,
                                                      i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_micro_kernel_config->tmp_vreg2,
                                                      LIBXSMM_AARCH64_ASIMD_WIDTH_B );
              }
            } else {
              libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF,
                                                      i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_micro_kernel_config->tmp_vreg3,
                                                      LIBXSMM_AARCH64_ASIMD_WIDTH_B );

              libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V,
                                                         i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->tmp_vreg3, 0, i_micro_kernel_config->tmp_vreg2,
                                                         LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

              libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_STR_I_POST,
                                                      i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_UNDEF, 1, i_micro_kernel_config->tmp_vreg2,
                                                      LIBXSMM_AARCH64_ASIMD_WIDTH_B );
              l_mask_adv++;
            }
          }

          if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU ) {
            /* ReLU */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMAX_V, cur_vreg, i_micro_kernel_config->zero_vreg, 0, cur_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
          } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU  ) {
            /* we need to multiply with alpha */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V, cur_vreg,  i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

            /* now we blend both together */
            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V, i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->tmp_vreg, 0, cur_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
          } else {
            /* should not happen */
          }
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU )  {
          /* compute exp */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V, cur_vreg, cur_vreg, 0, i_micro_kernel_config->tmp_vreg2, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
          libxsmm_generator_exp_ps_3dts_aarch64( io_generated_code,
              i_micro_kernel_config->tmp_vreg2,
              i_micro_kernel_config->tmp_vreg,
              i_micro_kernel_config->tmp_vreg3,
              i_micro_kernel_config->vec_c0,
              i_micro_kernel_config->vec_c1,
              i_micro_kernel_config->vec_c2,
              i_micro_kernel_config->vec_c3,
              i_micro_kernel_config->vec_halves,
              i_micro_kernel_config->vec_log2e,
              i_micro_kernel_config->vec_expmask,
              i_micro_kernel_config->vec_hi_bound,
              i_micro_kernel_config->vec_lo_bound,
              l_tupletype, LIBXSMM_AARCH64_SVE_TYPE_S, 0 );

          /* compute mask */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FCMGT_Z_V, cur_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, i_micro_kernel_config->tmp_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

          /* compute ELU for < 0 */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V,
                                                     i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FSUB_V,
                                                     i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );

          /* blend exp-fma result with input reg based on elu mask */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V,
                                                     i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->tmp_vreg, 0, cur_vreg,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
        } else {
          /* shouldn't happen */
        }
      }
      if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU) ) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                       i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                       (i_micro_kernel_config->ldo_mask - (l_mask_adv*8))/8 );
      }
    }
    if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU) ) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                     i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                     i_n_blocking * (i_micro_kernel_config->ldo_mask/8) );
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_compute_unary_aarch64_2d_reg_block_relu_inv( libxsmm_generated_code*                 io_generated_code,
                                                          libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                          const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                          const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                          unsigned int                            i_vlen,
                                                          unsigned int                            i_start_vreg,
                                                          unsigned int                            i_m_blocking,
                                                          unsigned int                            i_n_blocking,
                                                          unsigned int                            i_mask_last_m_chunk,
                                                          unsigned int                            i_mask_reg) {
  unsigned int im, in;
  unsigned int l_ld_bytes = i_mateltwise_desc->ldi * i_micro_kernel_config->datatype_size_in;
  unsigned int l_m_adjust = ( i_mask_last_m_chunk == 0 ) ? i_micro_kernel_config->datatype_size_in * i_vlen * i_m_blocking : i_micro_kernel_config->datatype_size_in * ( (i_vlen * (i_m_blocking-1)) + i_mask_last_m_chunk );

  if( io_generated_code->arch == LIBXSMM_AARCH64_A64FX ){

    unsigned char l_blend_reg = 7;/* vector register for blending; todo should be a function input / part of the reg mapping */
    libxsmm_aarch64_sve_type l_sve_type = libxsmm_generator_aarch64_get_sve_type(i_micro_kernel_config->datatype_size_in);

    for (in = 0; in < i_n_blocking; in++) {
      unsigned int l_mask_adv = 0;
      for (im = 0; im < i_m_blocking; im++) {
        unsigned int cur_vreg = i_start_vreg + in * i_m_blocking + im;

        if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {

          /* warning! this currently is specific to a sve vector length of 512 bits (A64FX) */
          /* load 16 bits into the predicate register fp32 flags */

          /* load 2 bytes from memory into first 16 bits of predicate register */
          /* warning! here we load too much, so the memory region needs padding */
          /* less hacky way: load into stack, load from stack */
          libxsmm_aarch64_instruction_sve_move( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_LDR_P_I_OFF,
                                                i_gp_reg_mapping->gp_reg_relumask, 0, 0, l_blend_reg, 0 );

          /* zip, 8 bits, low */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ZIP_P_L,
                                                   l_blend_reg, l_blend_reg, 0, l_blend_reg,
                                                   0, LIBXSMM_AARCH64_SVE_TYPE_B );

          /* zip, 16 bits, low */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ZIP_P_L,
                                                   l_blend_reg, l_blend_reg, 0, l_blend_reg,
                                                   0, LIBXSMM_AARCH64_SVE_TYPE_H );

          /* increment gp_reg_relumask by 2 */
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                         i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                         2 );
          l_mask_adv += 2;/* we read two bytes */

        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) {
          libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_relumask,
                                                                  i_gp_reg_mapping->gp_reg_scratch_1, i_micro_kernel_config->tmp_vreg,
                                                                  i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 0 );

          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FCMGT_Z_V,
                                                   i_micro_kernel_config->tmp_vreg, LIBXSMM_AARCH64_SVE_REG_UNDEF, 0, l_blend_reg, /* dst was i_micro_kernel_config->tmp_vreg2 */
                                                   i_mask_reg, l_sve_type );
        } else {
          /* shouldn't happen */
        }

        if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV ) {
          /* we need to multiply with alpha */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                   cur_vreg, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                   i_mask_reg, l_sve_type );

          /* now we blend both together */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_SEL_V_P,
                                                   cur_vreg, i_micro_kernel_config->tmp_vreg2, 0, cur_vreg, /* mask was i_micro_kernel_config->tmp_vreg */
                                                   l_blend_reg, l_sve_type );
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV ) {
          /* now we blend both together */
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_SEL_V_P,
                                                   cur_vreg, i_micro_kernel_config->zero_vreg, 0, cur_vreg, /* mask was i_micro_kernel_config->tmp_vreg */
                                                   l_blend_reg, l_sve_type );
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) {
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FADD_V,
                                                   i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg,
                                                   i_mask_reg, l_sve_type );

          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_FMUL_V,
                                                   i_micro_kernel_config->tmp_vreg, cur_vreg, 0, i_micro_kernel_config->tmp_vreg,
                                                   i_mask_reg, l_sve_type );

          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_SEL_V_P,
                                                   cur_vreg, i_micro_kernel_config->tmp_vreg, 0, cur_vreg,
                                                   l_blend_reg, l_sve_type );
        } else {
          /* shouldn't happen */
        }
      }
      if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && ( i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) ) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                       i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                       (i_micro_kernel_config->ldi_mask - (l_mask_adv*8))/8 );
      } else {
        if ( l_ld_bytes != l_m_adjust ) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                        i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                        (unsigned long long)((unsigned long long)(l_ld_bytes-l_m_adjust)) );
        }
      }
    }

  } else {

    for (in = 0; in < i_n_blocking; in++) {
      unsigned int l_mask_adv = 0;
      for (im = 0; im < i_m_blocking; im++) {
        unsigned int cur_vreg = i_start_vreg + in * i_m_blocking + im;

        if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
          if ( im % 2 == 0 ) {
            if ( im == i_m_blocking - 1 ) {
              libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R_R_POST,
                                                               i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_XZR, i_micro_kernel_config->tmp_vreg,
                                                               LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
              l_mask_adv++;
            } else {
              libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R,
                                                               i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_UNDEF, i_micro_kernel_config->tmp_vreg,
                                                               LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
            }

            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V,
                                                       i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->mask_helper0_vreg, 0, i_micro_kernel_config->tmp_vreg,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );

            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_CMEQ_R_V,
                                                       i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->mask_helper0_vreg, 0, i_micro_kernel_config->tmp_vreg,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
          } else {
            libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R_R_POST,
                                                            i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_GP_REG_XZR, i_micro_kernel_config->tmp_vreg,
                                                            LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
            l_mask_adv++;

            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V,
                                                       i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->mask_helper1_vreg, 0, i_micro_kernel_config->tmp_vreg,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );

            libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_CMEQ_R_V,
                                                       i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->mask_helper1_vreg, 0, i_micro_kernel_config->tmp_vreg,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
          }
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) {
          libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_1, i_micro_kernel_config->tmp_vreg,
                                                                  i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 0 );

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FCMGT_Z_V,
                                                     i_micro_kernel_config->tmp_vreg, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, i_micro_kernel_config->tmp_vreg2,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
        } else {
          /* shouldn't happen */
        }

        if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV ) {
          /* we need to multiply with alpha */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V,
                                                     cur_vreg, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg2,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

          /* now we blend both together */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V,
                                                     i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->tmp_vreg, 0, cur_vreg,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV ) {
          /* now we blend both together */
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V,
                                                     i_micro_kernel_config->zero_vreg, i_micro_kernel_config->tmp_vreg, 0, cur_vreg,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
        } else if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) {
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FADD_V,
                                                     i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->fam_lu_vreg_alpha, 0, i_micro_kernel_config->tmp_vreg,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V,
                                                     i_micro_kernel_config->tmp_vreg, cur_vreg, 0, i_micro_kernel_config->tmp_vreg,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V,
                                                     i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->tmp_vreg2, 0, cur_vreg,
                                                     LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
        } else {
          /* shouldn't happen */
        }
      }
      if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && ( i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) ) {
        /* adjust reg_relumask for stride */
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                       i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                       (i_micro_kernel_config->ldi_mask - (l_mask_adv*8))/8 );
      } else {
        if ( l_ld_bytes != l_m_adjust ) {
          libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                        i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                        (unsigned long long)((unsigned long long)(l_ld_bytes-l_m_adjust)) );
        }
      }
    }

  }

  if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && ( i_mateltwise_desc->param != LIBXSMM_MELTW_TYPE_UNARY_ELU_INV ) ) {
    libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                   i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                   i_n_blocking * (i_micro_kernel_config->ldi_mask/8) );
  } else {
    libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                   i_gp_reg_mapping->gp_reg_relumask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_relumask,
                                                   (unsigned long long)((unsigned long long)(l_ld_bytes*i_n_blocking)) );
  }
}

LIBXSMM_API_INTERN
void libxsmm_compute_unary_aarch64_2d_reg_block_dropout( libxsmm_generated_code*                 io_generated_code,
                                                         libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                         const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                         const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                         unsigned int                            i_vlen,
                                                         unsigned int                            i_start_vreg,
                                                         unsigned int                            i_m_blocking,
                                                         unsigned int                            i_n_blocking,
                                                         unsigned int                            i_mask_last_m_chunk,
                                                         unsigned int                            i_mask_reg) {
  unsigned int im, in;
  LIBXSMM_UNUSED(i_mask_last_m_chunk);
  LIBXSMM_UNUSED(i_mask_reg);
  LIBXSMM_UNUSED(i_vlen);

  for (in = 0; in < i_n_blocking; in++) {
    unsigned int l_mask_adv = 0;
    for (im = 0; im < i_m_blocking; im++) {
      /*unsigned int l_vlen = ( l_bf16_compute > 0 ) ? 32 : 16;*/
      unsigned int cur_vreg = i_start_vreg + in * i_m_blocking + im;

      /* draw a random number */
      libxsmm_generator_xoshiro128p_f32_aarch64_asimd( io_generated_code, i_micro_kernel_config->prng_state0_vreg, i_micro_kernel_config->prng_state1_vreg, i_micro_kernel_config->prng_state2_vreg, i_micro_kernel_config->prng_state3_vreg,
                                                       i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->dropout_vreg_tmp1, i_micro_kernel_config->dropout_vreg_one, i_micro_kernel_config->dropout_vreg_tmp2 );

      /* compare with p */
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FCMGT_R_V, i_micro_kernel_config->dropout_prob_vreg, i_micro_kernel_config->dropout_vreg_tmp2, 0, i_micro_kernel_config->dropout_vreg_tmp0, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
      if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {
        if ( im % 2 == 0 ) {
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V, i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->mask_helper0_vreg, 0, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
        } else {
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V, i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->mask_helper1_vreg, 0, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
        }
        libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ADDV_V, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_REG_UNDEF, 0, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
        if ( im % 2 == 0 ) {
          if ( im == i_m_blocking - 1 ) {
            libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_STR_I_POST,
                                                    i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_UNDEF, 1, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_WIDTH_B );
            l_mask_adv++;
          } else {
            libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_STR_I_OFF,
                                                    i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_WIDTH_B );
          }
        } else {
          libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF,
                                                  i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_micro_kernel_config->dropout_vreg_tmp2, LIBXSMM_AARCH64_ASIMD_WIDTH_B );

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V, i_micro_kernel_config->dropout_vreg_tmp1, i_micro_kernel_config->dropout_vreg_tmp2, 0, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

          libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_STR_I_POST,
                                                    i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_UNDEF, 1, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_WIDTH_B );
          l_mask_adv++;
        }
      }

      /* weight */
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V, cur_vreg, i_micro_kernel_config->dropout_invprob_vreg, 0, cur_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

      /* blend zero and multiplication result together */
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_EOR_V, i_micro_kernel_config->dropout_vreg_tmp1, i_micro_kernel_config->dropout_vreg_tmp1, 0, i_micro_kernel_config->dropout_vreg_tmp1, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V, i_micro_kernel_config->dropout_vreg_tmp1, i_micro_kernel_config->dropout_vreg_tmp0, 0, cur_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
    }
    if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD, i_gp_reg_mapping->gp_reg_dropoutmask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_dropoutmask, (i_micro_kernel_config->ldo_mask - (l_mask_adv*8))/8 );
    }
  }
  if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {
    libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB, i_gp_reg_mapping->gp_reg_dropoutmask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_dropoutmask, i_n_blocking * (i_micro_kernel_config->ldo_mask/8) );
  }
}

LIBXSMM_API_INTERN
void libxsmm_compute_unary_aarch64_2d_reg_block_dropout_inv( libxsmm_generated_code*                 io_generated_code,
                                                             libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                             const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                             const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                             unsigned int                            i_vlen,
                                                             unsigned int                            i_start_vreg,
                                                             unsigned int                            i_m_blocking,
                                                             unsigned int                            i_n_blocking,
                                                             unsigned int                            i_mask_last_m_chunk,
                                                             unsigned int                            i_mask_reg) {
  unsigned int im, in;
  LIBXSMM_UNUSED(i_mask_last_m_chunk);
  LIBXSMM_UNUSED(i_mask_reg);
  LIBXSMM_UNUSED(i_vlen);

  for (in = 0; in < i_n_blocking; in++) {
    unsigned int l_mask_adv = 0;
    for (im = 0; im < i_m_blocking; im++) {
      unsigned int cur_vreg = i_start_vreg + in * i_m_blocking + im;

      if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) ) {
        if ( im % 2 == 0 ) {
          if ( im == i_m_blocking - 1 ) {
            libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R_R_POST,
                                                             i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_XZR, i_micro_kernel_config->dropout_vreg_tmp0,
                                                             LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
            l_mask_adv++;
          } else {
            libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R,
                                                             i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_UNDEF, i_micro_kernel_config->dropout_vreg_tmp0,
                                                             LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
          }

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V, i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->mask_helper0_vreg, 0, i_micro_kernel_config->dropout_vreg_tmp0, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_CMEQ_R_V, i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->mask_helper0_vreg, 0, i_micro_kernel_config->dropout_vreg_tmp0, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
        } else {
          libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R_R_POST,
                                                           i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_GP_REG_XZR, i_micro_kernel_config->dropout_vreg_tmp0,
                                                           LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
          l_mask_adv++;

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_AND_V, i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->mask_helper1_vreg, 0, i_micro_kernel_config->dropout_vreg_tmp0, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );

          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_CMEQ_R_V, i_micro_kernel_config->dropout_vreg_tmp0, i_micro_kernel_config->mask_helper1_vreg, 0, i_micro_kernel_config->dropout_vreg_tmp0, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
        }
      } else {
        LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_BITMASK_REQUIRED );
        return;
      }

      /* weight */
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V, cur_vreg, i_micro_kernel_config->dropout_prob_vreg, 0, cur_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );

      /* select which value is set to 0 */
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_BIF_V, i_micro_kernel_config->dropout_vreg_zero, i_micro_kernel_config->dropout_vreg_tmp0, 0, cur_vreg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
    }
    if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD, i_gp_reg_mapping->gp_reg_dropoutmask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_dropoutmask, (i_micro_kernel_config->ldi_mask - (l_mask_adv*8))/8 );
    }
  }
  if ( (i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ) {
    libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB, i_gp_reg_mapping->gp_reg_dropoutmask, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_dropoutmask, i_n_blocking * (i_micro_kernel_config->ldi_mask/8) );
  }
}


LIBXSMM_API_INTERN
void libxsmm_compute_binary_aarch64_2d_reg_block( libxsmm_generated_code*                 io_generated_code,
                                                  libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                  const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                  const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                  unsigned int                            i_vlen,
                                                  unsigned int                            i_start_vreg,
                                                  unsigned int                            i_m_blocking,
                                                  unsigned int                            i_n_blocking,
                                                  unsigned int                            i_mask_last_m_chunk,
                                                  unsigned int                            i_mask_reg) {
  unsigned int im, in, cur_vreg;
  unsigned int binary_op_instr = 0;
  unsigned int bcast_row = (((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_ROW_IN_1) > 0)) ? 1 : 0;
  unsigned int bcast_col = (((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_COL_IN_1) > 0)) ? 1 : 0;
  unsigned int bcast_scalar = (((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_BINARY_BCAST_SCALAR_IN_1) > 0)) ? 1 : 0;
  unsigned int bcast_input = ( bcast_row == 1 || bcast_col == 1 || bcast_scalar == 1 ) ? 1 : 0;
  unsigned int l_ld_bytes = i_mateltwise_desc->ldo * i_micro_kernel_config->datatype_size_out;
  unsigned int l_ld_bytes_in2 = i_mateltwise_desc->ldi2 * i_micro_kernel_config->datatype_size_in;
  unsigned int l_m_adjust = ( i_mask_last_m_chunk == 0 ) ? i_micro_kernel_config->datatype_size_out * i_vlen * i_m_blocking : i_micro_kernel_config->datatype_size_out * ( (i_vlen * (i_m_blocking-1)) + i_mask_last_m_chunk );
  unsigned int l_m_adjust_in2 = ( i_mask_last_m_chunk == 0 ) ? i_micro_kernel_config->datatype_size_in * i_vlen * i_m_blocking : i_micro_kernel_config->datatype_size_in * ( (i_vlen * (i_m_blocking-1)) + i_mask_last_m_chunk );
  unsigned int offset = 0, offset2 = 0;
  unsigned int _in_blocking = (bcast_col == 1) ? 1 : i_n_blocking;

  unsigned char l_is_sve = io_generated_code->arch == LIBXSMM_AARCH64_A64FX;
  unsigned char l_is_predicated = (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_DIV) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_MULADD);
  unsigned char l_pred_reg = i_mask_reg;
  libxsmm_aarch64_sve_type l_sve_type = libxsmm_generator_aarch64_get_sve_type(i_micro_kernel_config->datatype_size_in);

  switch (i_mateltwise_desc->param) {
    case LIBXSMM_MELTW_TYPE_BINARY_ADD: {
      binary_op_instr = l_is_sve ? LIBXSMM_AARCH64_INSTR_SVE_FADD_V : LIBXSMM_AARCH64_INSTR_ASIMD_FADD_V;
    } break;
    case LIBXSMM_MELTW_TYPE_BINARY_MUL: {
      binary_op_instr = l_is_sve ? LIBXSMM_AARCH64_INSTR_SVE_FMUL_V : LIBXSMM_AARCH64_INSTR_ASIMD_FMUL_V;
    } break;
    case LIBXSMM_MELTW_TYPE_BINARY_SUB: {
      binary_op_instr = l_is_sve ? LIBXSMM_AARCH64_INSTR_SVE_FSUB_V : LIBXSMM_AARCH64_INSTR_ASIMD_FSUB_V;
    } break;
    case LIBXSMM_MELTW_TYPE_BINARY_DIV: {
      binary_op_instr = l_is_sve ? LIBXSMM_AARCH64_INSTR_SVE_FDIV_V_P : LIBXSMM_AARCH64_INSTR_ASIMD_FDIV_V;
    } break;
    case LIBXSMM_MELTW_TYPE_BINARY_MULADD: {
      binary_op_instr = l_is_sve ? LIBXSMM_AARCH64_INSTR_SVE_FMLA_V_P : LIBXSMM_AARCH64_INSTR_ASIMD_FMLA_V;
    } break;
    default:;
  }
#if 0 /* todo: code from X86, that is missing from ASIMD/SVE */
  if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_PACK) {
    for (in = 0; in < i_n_blocking; in++) {
      for (im = 0; im < i_m_blocking; im++) {
        cur_vreg = i_start_vreg + in * i_m_blocking + im;

        libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_POST,
                                                i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_GP_REG_UNDEF, 16,
                                                cur_vreg,
                                                LIBXSMM_AARCH64_ASIMD_WIDTH_Q );


        libxsmm_x86_instruction_vec_move( io_generated_code,
            io_generated_code->arch,
            LIBXSMM_X86_INSTR_VPMOVZXWD,
            i_gp_reg_mapping->gp_reg_in2,
            LIBXSMM_X86_GP_REG_UNDEF,
            0,
            (im * i_vlen + in * i_mateltwise_desc->ldi2) * i_micro_kernel_config->datatype_size_in,
            'z',
            i_micro_kernel_config->tmp_vreg,
            ((i_mask_last_m_chunk == 1) && (im == i_m_blocking - 1)) ? i_mask_reg : 0,
            0, 0);

        libxsmm_x86_instruction_vec_compute_2reg_imm8( io_generated_code, LIBXSMM_X86_INSTR_VPSLLD_I, 'z', i_micro_kernel_config->tmp_vreg, i_micro_kernel_config->tmp_vreg, 16 );
        libxsmm_x86_instruction_vec_compute_3reg( io_generated_code, LIBXSMM_X86_INSTR_VPORD, 'z', cur_vreg, i_micro_kernel_config->tmp_vreg, cur_vreg );
      }
    }
    return;
  }
#endif

  if( l_is_sve && l_is_predicated ) {/* set the whole predicate register to true */
    libxsmm_generator_set_p_register_aarch64_sve( io_generated_code, l_pred_reg, -1, 0 );
  }

  for (in = 0; in < i_n_blocking; in++) {
    for (im = 0; im < i_m_blocking; im++) {
      cur_vreg = i_start_vreg + in * i_m_blocking + im;

      if ( (bcast_row == 1) || (bcast_scalar == 1) ) {
        offset2 = (bcast_scalar == 1) ?  0:i_micro_kernel_config->datatype_size_in*i_n_blocking;
        offset = (l_ld_bytes*i_n_blocking);
        libxsmm_generator_bcastload_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_in2, i_gp_reg_mapping->gp_reg_scratch_1, i_micro_kernel_config->tmp_vreg,
                                                               i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 0 );

      }
      if ( bcast_input == 0 || bcast_col == 1) {
        offset = (l_ld_bytes*i_n_blocking);
        offset2 = (bcast_col == 1) ? l_m_adjust:(l_ld_bytes_in2*_in_blocking);
        libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_in2, i_gp_reg_mapping->gp_reg_scratch_1, i_micro_kernel_config->tmp_vreg,
                                                                i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 0 );
      }

      if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_MULADD) {
        libxsmm_generator_vloadstore_masked_vreg_aarch64_asimd( io_generated_code, i_gp_reg_mapping->gp_reg_out, i_gp_reg_mapping->gp_reg_scratch_1, i_micro_kernel_config->tmp_vreg2,
                                                                i_micro_kernel_config->datatype_size_in, (im == i_m_blocking - 1) ? i_mask_last_m_chunk : 0, 1, 0 );
        /* the result is temporarily stored in tmp_vreg2, because the instruction computes dst += src0*src1 */
        if( l_is_sve ){
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, binary_op_instr,
                                                   cur_vreg, i_micro_kernel_config->tmp_vreg, 0, i_micro_kernel_config->tmp_vreg2,
                                                   l_pred_reg, l_sve_type );
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_ORR_V,
                                                   i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->tmp_vreg2, 0, cur_vreg,
                                                   l_pred_reg, l_sve_type );
        } else {
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, binary_op_instr,
                                                    cur_vreg, i_micro_kernel_config->tmp_vreg, 0, i_micro_kernel_config->tmp_vreg2,
                                                    (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D );
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_ORR_V,
                                                    i_micro_kernel_config->tmp_vreg2, i_micro_kernel_config->tmp_vreg2, 0, cur_vreg,
                                                    (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D );
        }
      } else {
        if( l_is_sve ){
          libxsmm_aarch64_instruction_sve_compute( io_generated_code, binary_op_instr,
                                                   cur_vreg, i_micro_kernel_config->tmp_vreg, 0, cur_vreg,
                                                   l_pred_reg, l_sve_type );
        } else {
          libxsmm_aarch64_instruction_asimd_compute( io_generated_code, binary_op_instr,
                                                     cur_vreg, i_micro_kernel_config->tmp_vreg, 0, cur_vreg,
                                                     (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D );
        }
      }
    }
    if ( (l_ld_bytes != l_m_adjust) && (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_MULADD) ) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                     i_gp_reg_mapping->gp_reg_out,i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_out,
                                                     (unsigned long long)((unsigned long long)(l_ld_bytes-l_m_adjust)) );
    }
    if ( bcast_input == 0 ) {
      if ( l_ld_bytes_in2 != l_m_adjust ) {
        libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                       i_gp_reg_mapping->gp_reg_in2, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_in2,
                                                       (unsigned long long)((unsigned long long)(l_ld_bytes_in2-l_m_adjust_in2)) );
      }
    }
    if (bcast_col == 1 && in < (i_n_blocking-1)){
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                     i_gp_reg_mapping->gp_reg_in2, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_in2,
                                                     (unsigned long long)((unsigned long long)(offset2)) );
    }

    if (bcast_row == 1) {
      libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_ADD,
                                                     i_gp_reg_mapping->gp_reg_in2, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_in2,
                                                     (unsigned long long)((unsigned long long)(i_micro_kernel_config->datatype_size_in)) );
    }
  }
  libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                 i_gp_reg_mapping->gp_reg_in2, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_in2,
                                                 (unsigned long long)((unsigned long long)(offset2)) );
  if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_MULADD) {
    libxsmm_aarch64_instruction_alu_compute_imm64( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_META_SUB,
                                                   i_gp_reg_mapping->gp_reg_out, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_out,
                                                   (unsigned long long)((unsigned long long)(offset)) );
  }
}

LIBXSMM_API_INTERN
void libxsmm_compute_unary_binary_aarch64_2d_reg_block( libxsmm_generated_code*                 io_generated_code,
                                                        libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                        const libxsmm_mateltwise_kernel_config* i_micro_kernel_config,
                                                        const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                        unsigned int                            i_vlen,
                                                        unsigned int                            i_start_vreg,
                                                        unsigned int                            i_m_blocking,
                                                        unsigned int                            i_n_blocking,
                                                        unsigned int                            i_mask_last_m_chunk,
                                                        unsigned int                            i_mask_reg) {
  if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
    switch (i_mateltwise_desc->param) {
      case LIBXSMM_MELTW_TYPE_UNARY_TANH:
      case LIBXSMM_MELTW_TYPE_UNARY_EXP:
      case LIBXSMM_MELTW_TYPE_UNARY_SIGMOID:
      case LIBXSMM_MELTW_TYPE_UNARY_TANH_INV:
      case LIBXSMM_MELTW_TYPE_UNARY_SIGMOID_INV:
      case LIBXSMM_MELTW_TYPE_UNARY_GELU:
      case LIBXSMM_MELTW_TYPE_UNARY_GELU_INV:
      case LIBXSMM_MELTW_TYPE_UNARY_SQRT:
      case LIBXSMM_MELTW_TYPE_UNARY_NEGATE:
      case LIBXSMM_MELTW_TYPE_UNARY_INC:
      case LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL:
      case LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL_SQRT:
      case LIBXSMM_MELTW_TYPE_UNARY_X2: {
        libxsmm_compute_unary_aarch64_2d_reg_block_op( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_vlen, i_start_vreg, i_m_blocking, i_n_blocking, i_mask_last_m_chunk, i_mask_reg);
      } break;
      case LIBXSMM_MELTW_TYPE_UNARY_RELU:
      case LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU:
      case LIBXSMM_MELTW_TYPE_UNARY_ELU: {
        libxsmm_compute_unary_aarch64_2d_reg_block_relu( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_vlen, i_start_vreg, i_m_blocking, i_n_blocking, i_mask_last_m_chunk, i_mask_reg);
      } break;
      case LIBXSMM_MELTW_TYPE_UNARY_RELU_INV:
      case LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV:
      case LIBXSMM_MELTW_TYPE_UNARY_ELU_INV: {
        libxsmm_compute_unary_aarch64_2d_reg_block_relu_inv( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_vlen, i_start_vreg, i_m_blocking, i_n_blocking, i_mask_last_m_chunk, i_mask_reg);
      } break;
      case LIBXSMM_MELTW_TYPE_UNARY_DROPOUT: {
        libxsmm_compute_unary_aarch64_2d_reg_block_dropout( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_vlen, i_start_vreg, i_m_blocking, i_n_blocking, i_mask_last_m_chunk, i_mask_reg);
      } break;
      case LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV: {
        libxsmm_compute_unary_aarch64_2d_reg_block_dropout_inv( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_vlen, i_start_vreg, i_m_blocking, i_n_blocking, i_mask_last_m_chunk, i_mask_reg);
      } break;
    }
  } else if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
    libxsmm_compute_binary_aarch64_2d_reg_block( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_vlen, i_start_vreg, i_m_blocking, i_n_blocking, i_mask_last_m_chunk, i_mask_reg);
  }
}

LIBXSMM_API_INTERN
void libxsmm_setup_input_output_aarch64_masks( libxsmm_generated_code*                 io_generated_code,
                                               libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                               const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                               unsigned int                            i_tmp_reg,
                                               unsigned int                            i_m,
                                               unsigned int*                           i_use_m_input_masking,
                                               unsigned int*                           i_mask_reg_in,
                                               unsigned int*                           i_use_m_output_masking,
                                               unsigned int*                           i_mask_reg_out) {
  unsigned int i_vlen_in = i_micro_kernel_config->vlen_in;
  unsigned int i_vlen_out = i_micro_kernel_config->vlen_out;
  LIBXSMM_UNUSED(io_generated_code);
  LIBXSMM_UNUSED(i_micro_kernel_config);
  LIBXSMM_UNUSED(i_mateltwise_desc);
  LIBXSMM_UNUSED(i_tmp_reg);

  *i_mask_reg_in = 0;
  *i_use_m_input_masking = i_m % i_vlen_in;
  *i_mask_reg_out = 0;
  *i_use_m_output_masking = i_m % i_vlen_out;
}

LIBXSMM_API_INTERN
void libxsmm_configure_microkernel_aarch64_loops( libxsmm_generated_code*                 io_generated_code,
                                                  libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                  libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                  const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                  unsigned int                            i_m,
                                                  unsigned int                            i_n,
                                                  unsigned int                            i_use_m_input_masking,
                                                  unsigned int*                           i_m_trips,
                                                  unsigned int*                           i_n_trips,
                                                  unsigned int*                           i_m_unroll_factor,
                                                  unsigned int*                           i_n_unroll_factor,
                                                  unsigned int*                           i_m_assm_trips,
                                                  unsigned int*                           i_n_assm_trips,
                                                  unsigned int*                           i_out_loop_trips,
                                                  unsigned int*                           i_inner_loop_trips,
                                                  unsigned int*                           i_out_loop_bound,
                                                  unsigned int*                           i_inner_loop_bound,
                                                  unsigned int*                           i_out_loop_reg,
                                                  unsigned int*                           i_inner_loop_reg,
                                                  unsigned int*                           i_out_unroll_factor,
                                                  unsigned int*                           i_inner_unroll_factor) {
  unsigned int m_trips, n_trips, m_unroll_factor, n_unroll_factor, m_assm_trips, n_assm_trips, out_loop_trips, inner_loop_trips, out_loop_bound, inner_loop_bound, out_loop_reg, inner_loop_reg, out_unroll_factor, inner_unroll_factor;
  unsigned int max_nm_unrolling = 32;
  unsigned int i_loop_order = i_micro_kernel_config->loop_order;
  unsigned int reserved_zmms = i_micro_kernel_config->reserved_zmms;
  unsigned int i_vlen_in = i_micro_kernel_config->vlen_in;
  LIBXSMM_UNUSED(i_mateltwise_desc);
  LIBXSMM_UNUSED(io_generated_code);

  m_trips               = (i_m + i_vlen_in - 1) / i_vlen_in;
  n_trips               = i_n;

  max_nm_unrolling  = max_nm_unrolling - reserved_zmms;

  if ( ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) && (i_vlen_in == 4) ) {
    m_unroll_factor = m_trips;
  } else {
    if (i_use_m_input_masking != 0) {
      m_unroll_factor = m_trips;
    } else {
      m_unroll_factor = LIBXSMM_MIN(m_trips,16);
    }
  }

  if (m_unroll_factor > max_nm_unrolling) {
    m_unroll_factor = max_nm_unrolling;
  }

  while (m_trips % m_unroll_factor != 0) {
    m_unroll_factor--;
  }

  n_unroll_factor = n_trips;
  while (m_unroll_factor * n_unroll_factor > max_nm_unrolling) {
    n_unroll_factor--;
  }

  while (n_trips % n_unroll_factor != 0) {
    n_unroll_factor--;
  }

  m_assm_trips = m_trips/m_unroll_factor;
  n_assm_trips = n_trips/n_unroll_factor;

  out_loop_trips      = (i_loop_order == NM_LOOP_ORDER) ? n_assm_trips : m_assm_trips;
  out_loop_bound      = (i_loop_order == NM_LOOP_ORDER) ? n_trips : m_trips;
  out_loop_reg        = (i_loop_order == NM_LOOP_ORDER) ? i_gp_reg_mapping->gp_reg_n_loop : i_gp_reg_mapping->gp_reg_m_loop;
  out_unroll_factor   = (i_loop_order == NM_LOOP_ORDER) ? n_unroll_factor : m_unroll_factor;

  inner_loop_trips    = (i_loop_order == MN_LOOP_ORDER) ? n_assm_trips : m_assm_trips;
  inner_loop_bound    = (i_loop_order == MN_LOOP_ORDER) ? n_trips : m_trips;
  inner_loop_reg      = (i_loop_order == MN_LOOP_ORDER) ? i_gp_reg_mapping->gp_reg_n_loop : i_gp_reg_mapping->gp_reg_m_loop;
  inner_unroll_factor = (i_loop_order == MN_LOOP_ORDER) ? n_unroll_factor : m_unroll_factor;

  *i_m_trips = m_trips;
  *i_n_trips = n_trips;
  *i_m_unroll_factor = m_unroll_factor;
  *i_n_unroll_factor = n_unroll_factor;
  *i_m_assm_trips = m_assm_trips;
  *i_n_assm_trips = n_assm_trips;
  *i_out_loop_trips = out_loop_trips;
  *i_inner_loop_trips = inner_loop_trips;
  *i_out_loop_bound = out_loop_bound;
  *i_inner_loop_bound = inner_loop_bound;
  *i_out_loop_reg = out_loop_reg;
  *i_inner_loop_reg = inner_loop_reg;
  *i_out_unroll_factor = out_unroll_factor;
  *i_inner_unroll_factor = inner_unroll_factor;
}

LIBXSMM_API_INTERN
void libxsmm_configure_unary_aarch64_kernel_vregs_masks(  libxsmm_generated_code*                 io_generated_code,
                                                          libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                          unsigned int                            op,
                                                          unsigned int                            flags,
                                                          unsigned int                            i_gp_reg_tmp0,
                                                          unsigned int                            i_gp_reg_tmp1,
                                                          const unsigned int                      i_gp_reg_aux0,
                                                          const unsigned int                      i_gp_reg_aux1 ) {

  LIBXSMM_UNUSED(flags);
  LIBXSMM_UNUSED(i_gp_reg_aux0);
  LIBXSMM_UNUSED(i_gp_reg_aux1);

  unsigned char l_is_sve = io_generated_code->arch == LIBXSMM_AARCH64_A64FX;
  unsigned char l_pred_reg = 0;/* todo decide which predicate register to use */
  libxsmm_aarch64_sve_type l_sve_type = libxsmm_generator_aarch64_get_sve_type(i_micro_kernel_config->datatype_size_in);
  libxsmm_aarch64_asimd_tupletype l_tupletype = (i_micro_kernel_config->datatype_size_in == 4) ? LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S : LIBXSMM_AARCH64_ASIMD_TUPLETYPE_2D;


  if ((op == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (op == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV)   ||
      (op == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (op == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV)) {
    i_micro_kernel_config->zero_vreg = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->tmp_vreg  = i_micro_kernel_config->reserved_zmms + 1;
    i_micro_kernel_config->tmp_vreg2 = i_micro_kernel_config->reserved_zmms + 2;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 3;

    if ( (flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0 ){
      i_micro_kernel_config->tmp_vreg3 = i_micro_kernel_config->reserved_zmms;
      i_micro_kernel_config->mask_helper0_vreg =  i_micro_kernel_config->reserved_zmms + 1;
      i_micro_kernel_config->mask_helper1_vreg =  i_micro_kernel_config->reserved_zmms + 2;
      i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 3;

      if( l_is_sve ){

        /* not masks are needed for sve */

      } else {

        /* load 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 into mask_helper0/1_vreg */

        /* stack pointer -= 32, so prepare to use 32 bytes = 8 floats of stack memory */
        /* while LIBXSMM_AARCH64_INSTR_GP_STP_I_OFF supports a signed offset, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF does not */
        libxsmm_aarch64_instruction_alu_compute_imm12( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_SUB_I,
                                                       LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_XSP, 32, 0 );

        libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp0, 0x200000001 );/* int32 0x02, 0x01, little endian -> 0x01, 0x02 */
        libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp1, 0x800000004 );/* int32 0x08, 0x04, little endian -> 0x04, 0x08 */

        /* store a pair of fp registers to memory: 1,2,4,8 is being loaded into the stack memory */
        libxsmm_aarch64_instruction_alu_pair_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_STP_I_OFF,
                                                   LIBXSMM_AARCH64_GP_REG_XSP, 16,
                                                   i_gp_reg_tmp0, i_gp_reg_tmp1 );

        /* now those 32 bytes are stored into mask_helper0_vreg */
        libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF,
                                                LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_UNDEF, 16,
                                                i_micro_kernel_config->mask_helper0_vreg, LIBXSMM_AARCH64_ASIMD_WIDTH_Q );

        libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp0, 0x2000000010 );/* int32 0x20, 0x10 */
        libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp1, 0x8000000040 );/* int32 0x80, 0x40 */

        libxsmm_aarch64_instruction_alu_pair_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_STP_I_OFF,
                                                   LIBXSMM_AARCH64_GP_REG_XSP, 0,
                                                   i_gp_reg_tmp0, i_gp_reg_tmp1 );

        libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF,
                                                LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_UNDEF, 0,
                                                i_micro_kernel_config->mask_helper1_vreg, LIBXSMM_AARCH64_ASIMD_WIDTH_Q );

        /* reset stack pointer to its original position */
        libxsmm_aarch64_instruction_alu_compute_imm12( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_ADD_I,
                                                       LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_XSP, 32, 0 );

      }
    }

    if ( (op == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (op == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ) {
      i_micro_kernel_config->fam_lu_vreg_alpha = i_micro_kernel_config->reserved_zmms;
      i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 1;

      /* load alpha */
      if( l_is_sve ){
        libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
        libxsmm_aarch64_instruction_sve_move( io_generated_code, /* load 1 value and broadcast it to all elements */
          l_sve_type == LIBXSMM_AARCH64_SVE_TYPE_B ? LIBXSMM_AARCH64_INSTR_SVE_LD1RB_I_OFF :
          l_sve_type == LIBXSMM_AARCH64_SVE_TYPE_H ? LIBXSMM_AARCH64_INSTR_SVE_LD1RH_I_OFF :
          l_sve_type == LIBXSMM_AARCH64_SVE_TYPE_S ? LIBXSMM_AARCH64_INSTR_SVE_LD1RW_I_OFF :
                                                     LIBXSMM_AARCH64_INSTR_SVE_LD1RD_I_OFF,
                                              i_gp_reg_aux1, LIBXSMM_AARCH64_GP_REG_UNDEF, i_micro_kernel_config->fam_lu_vreg_alpha,
                                              l_pred_reg, l_sve_type );
      } else {
        libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R,
                                                         i_gp_reg_aux1, LIBXSMM_AARCH64_GP_REG_UNDEF, i_micro_kernel_config->fam_lu_vreg_alpha,
                                                         LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
      }
    }

    /* Set zero register needed for relu  */
    if( l_is_sve ){
      libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
      libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_EOR_V,
                                               i_micro_kernel_config->zero_vreg, i_micro_kernel_config->zero_vreg, 0, i_micro_kernel_config->zero_vreg,
                                               l_pred_reg, l_sve_type );
    } else {
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_EOR_V,
                                                 i_micro_kernel_config->zero_vreg, i_micro_kernel_config->zero_vreg, 0, i_micro_kernel_config->zero_vreg,
                                                 LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
    }
  }

  if( op == LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL ){
    i_micro_kernel_config->tmp_vreg = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 1;
  }

  if( op == LIBXSMM_MELTW_TYPE_UNARY_RECIPROCAL_SQRT ){
    /* two temporary registers are required, 3 in total: original, guess, guess squared */
    i_micro_kernel_config->tmp_vreg = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->tmp_vreg = i_micro_kernel_config->reserved_zmms + 1;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 2;
  }

  if ((op == LIBXSMM_MELTW_TYPE_UNARY_ELU) || (op == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV)) {
    i_micro_kernel_config->tmp_vreg = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->tmp_vreg2 = i_micro_kernel_config->reserved_zmms + 1;
    i_micro_kernel_config->tmp_vreg3 = i_micro_kernel_config->reserved_zmms + 2;
    i_micro_kernel_config->fam_lu_vreg_alpha = i_micro_kernel_config->reserved_zmms + 3;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 4;

    /* load alpha */
    if( l_is_sve ){
      libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
      libxsmm_aarch64_instruction_sve_move( io_generated_code, /* load 1 value and broadcast it to all elements */
        l_sve_type == LIBXSMM_AARCH64_SVE_TYPE_B ? LIBXSMM_AARCH64_INSTR_SVE_LD1RB_I_OFF :
        l_sve_type == LIBXSMM_AARCH64_SVE_TYPE_H ? LIBXSMM_AARCH64_INSTR_SVE_LD1RH_I_OFF :
        l_sve_type == LIBXSMM_AARCH64_SVE_TYPE_S ? LIBXSMM_AARCH64_INSTR_SVE_LD1RW_I_OFF :
                                                   LIBXSMM_AARCH64_INSTR_SVE_LD1RD_I_OFF,
                                            i_gp_reg_aux1, LIBXSMM_AARCH64_GP_REG_UNDEF, i_micro_kernel_config->fam_lu_vreg_alpha,
                                            l_pred_reg, l_sve_type );
    } else {
      libxsmm_aarch64_instruction_asimd_struct_r_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LD1R,
                                                       i_gp_reg_aux1, LIBXSMM_AARCH64_GP_REG_UNDEF, i_micro_kernel_config->fam_lu_vreg_alpha,
                                                       LIBXSMM_AARCH64_ASIMD_TUPLETYPE_4S );
    }
  }

  if ((op == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (op == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
    i_micro_kernel_config->mask_helper0_vreg =  i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->mask_helper1_vreg =  i_micro_kernel_config->reserved_zmms + 1;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 2;

    libxsmm_aarch64_instruction_alu_compute_imm12( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_SUB_I,
                                                   LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_XSP, 32, 0 );

    libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp0, 0x200000001 );
    libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp1, 0x800000004 );

    libxsmm_aarch64_instruction_alu_pair_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_STP_I_OFF, LIBXSMM_AARCH64_GP_REG_XSP, 16,
                                               i_gp_reg_tmp0, i_gp_reg_tmp1 );

    libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF,
                                            LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_UNDEF, 16,
                                            i_micro_kernel_config->mask_helper0_vreg, LIBXSMM_AARCH64_ASIMD_WIDTH_Q );

    libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp0, 0x2000000010 );
    libxsmm_aarch64_instruction_alu_set_imm64( io_generated_code, i_gp_reg_tmp1, 0x8000000040 );

    libxsmm_aarch64_instruction_alu_pair_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_STP_I_OFF, LIBXSMM_AARCH64_GP_REG_XSP, 0,
                                               i_gp_reg_tmp0, i_gp_reg_tmp1 );

    libxsmm_aarch64_instruction_asimd_move( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_LDR_I_OFF,
                                            LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_UNDEF, 0,
                                            i_micro_kernel_config->mask_helper1_vreg, LIBXSMM_AARCH64_ASIMD_WIDTH_Q );

    libxsmm_aarch64_instruction_alu_compute_imm12( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_ADD_I,
                                                   LIBXSMM_AARCH64_GP_REG_XSP, LIBXSMM_AARCH64_GP_REG_XSP, 32, 0 );

    if (op == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) {
      i_micro_kernel_config->reserved_zmms += 10;

      i_micro_kernel_config->prng_state0_vreg     = i_micro_kernel_config->reserved_zmms - 1;
      i_micro_kernel_config->prng_state1_vreg     = i_micro_kernel_config->reserved_zmms - 2;
      i_micro_kernel_config->prng_state2_vreg     = i_micro_kernel_config->reserved_zmms - 3;
      i_micro_kernel_config->prng_state3_vreg     = i_micro_kernel_config->reserved_zmms - 4;
      i_micro_kernel_config->dropout_vreg_tmp0    = i_micro_kernel_config->reserved_zmms - 5;
      i_micro_kernel_config->dropout_vreg_tmp1    = i_micro_kernel_config->reserved_zmms - 6;
      i_micro_kernel_config->dropout_vreg_tmp2    = i_micro_kernel_config->reserved_zmms - 7;
      i_micro_kernel_config->dropout_vreg_one     = i_micro_kernel_config->reserved_zmms - 8;
      i_micro_kernel_config->dropout_prob_vreg    = i_micro_kernel_config->reserved_zmms - 9;
      i_micro_kernel_config->dropout_invprob_vreg = i_micro_kernel_config->reserved_zmms - 10;

      libxsmm_generator_load_prng_state_aarch64_asimd( io_generated_code, i_gp_reg_aux0,
                                                       i_micro_kernel_config->prng_state0_vreg, i_micro_kernel_config->prng_state1_vreg,
                                                       i_micro_kernel_config->prng_state2_vreg, i_micro_kernel_config->prng_state3_vreg );
      libxsmm_generator_prepare_dropout_aarch64_asimd( io_generated_code, i_gp_reg_tmp0, i_gp_reg_aux1,
                                                       i_micro_kernel_config->dropout_vreg_one,
                                                       i_micro_kernel_config->dropout_prob_vreg,
                                                       i_micro_kernel_config->dropout_invprob_vreg );
    }

    if (op == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV) {
      i_micro_kernel_config->reserved_zmms += 4;

      i_micro_kernel_config->dropout_vreg_tmp0 = i_micro_kernel_config->reserved_zmms - 1;
      i_micro_kernel_config->dropout_vreg_one  = i_micro_kernel_config->reserved_zmms - 2;
      i_micro_kernel_config->dropout_vreg_zero = i_micro_kernel_config->reserved_zmms - 3;
      i_micro_kernel_config->dropout_prob_vreg = i_micro_kernel_config->reserved_zmms - 4;

      libxsmm_generator_prepare_dropout_inv_aarch64_asimd( io_generated_code, i_gp_reg_tmp0, i_gp_reg_aux1,
                                                           i_micro_kernel_config->dropout_vreg_one,
                                                           i_micro_kernel_config->dropout_vreg_zero,
                                                           i_micro_kernel_config->dropout_prob_vreg );
    }
  }

  if (op == LIBXSMM_MELTW_TYPE_UNARY_XOR) {
    unsigned char l_zero_reg = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->reserved_zmms = l_zero_reg + 1;
    i_micro_kernel_config->zero_vreg = l_zero_reg;
    if( io_generated_code->arch == LIBXSMM_AARCH64_A64FX ) {
      /* the sve data type doesn't matter, maybe we should add a new enum value for that */
      libxsmm_aarch64_instruction_sve_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_SVE_EOR_V, l_zero_reg, l_zero_reg, 0, l_zero_reg, 0, LIBXSMM_AARCH64_SVE_TYPE_S );
    } else {
      libxsmm_aarch64_instruction_asimd_compute( io_generated_code, LIBXSMM_AARCH64_INSTR_ASIMD_EOR_V, l_zero_reg, l_zero_reg, 0, l_zero_reg, LIBXSMM_AARCH64_ASIMD_TUPLETYPE_16B );
    }
  }

  if (op == LIBXSMM_MELTW_TYPE_UNARY_INC) {
    if (!l_is_sve){
      i_micro_kernel_config->vec_ones = i_micro_kernel_config->reserved_zmms;
      i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 1;
      libxsmm_aarch64_instruction_broadcast_scalar_to_vec ( io_generated_code, i_micro_kernel_config->vec_ones, i_gp_reg_tmp0,
                                                            l_tupletype, l_sve_type, l_pred_reg,
                                                            0x3f800000 );
    }
  }

  if (op == LIBXSMM_MELTW_TYPE_UNARY_EXP) {
    i_micro_kernel_config->vec_y = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->vec_z = i_micro_kernel_config->reserved_zmms + 1;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 2;
  }

  if (op == LIBXSMM_MELTW_TYPE_UNARY_GELU || op == LIBXSMM_MELTW_TYPE_UNARY_GELU_INV) {
    unsigned int reserved_zmms = i_micro_kernel_config->reserved_zmms;

    reserved_zmms += 25;

    i_micro_kernel_config->vec_xr         = reserved_zmms - 1;
    i_micro_kernel_config->vec_xa         = reserved_zmms - 2;
    i_micro_kernel_config->vec_index      = reserved_zmms - 3;
    i_micro_kernel_config->vec_C0         = reserved_zmms - 4;
    i_micro_kernel_config->vec_C1         = reserved_zmms - 5;
    i_micro_kernel_config->vec_C2         = reserved_zmms - 6;
    i_micro_kernel_config->vec_thres      = reserved_zmms - 7;
    i_micro_kernel_config->vec_absmask    = reserved_zmms - 8;
    i_micro_kernel_config->vec_scale      = reserved_zmms - 9;
    i_micro_kernel_config->vec_shifter    = reserved_zmms - 10;
    i_micro_kernel_config->vec_halves     = reserved_zmms - 11;
    i_micro_kernel_config->vec_c03        = reserved_zmms - 12;
    i_micro_kernel_config->vec_c02        = reserved_zmms - 13;
    i_micro_kernel_config->vec_c01        = reserved_zmms - 14;
    i_micro_kernel_config->vec_c0         = reserved_zmms - 15;
    i_micro_kernel_config->vec_c13        = reserved_zmms - 16;
    i_micro_kernel_config->vec_c12        = reserved_zmms - 17;
    i_micro_kernel_config->vec_c11        = reserved_zmms - 18;
    i_micro_kernel_config->vec_c1         = reserved_zmms - 19;
    i_micro_kernel_config->vec_c23        = reserved_zmms - 20;
    i_micro_kernel_config->vec_c22        = reserved_zmms - 21;
    i_micro_kernel_config->vec_c21        = reserved_zmms - 22;
    i_micro_kernel_config->vec_c2         = reserved_zmms - 23;
    i_micro_kernel_config->vec_tmp0       = reserved_zmms - 24;
    i_micro_kernel_config->vec_tmp1       = reserved_zmms - 25;

    if (l_is_sve) libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
    if (op == LIBXSMM_MELTW_TYPE_UNARY_GELU ) {
      libxsmm_generator_prepare_coeffs_gelu_ps_minimax3_aarch64( io_generated_code,
          i_micro_kernel_config->vec_thres,
          i_micro_kernel_config->vec_absmask,
          i_micro_kernel_config->vec_scale,
          i_micro_kernel_config->vec_shifter,
          i_micro_kernel_config->vec_halves,
          i_micro_kernel_config->vec_c0,
          i_micro_kernel_config->vec_c01,
          i_micro_kernel_config->vec_c02,
          i_micro_kernel_config->vec_c03,
          i_micro_kernel_config->vec_c1,
          i_micro_kernel_config->vec_c11,
          i_micro_kernel_config->vec_c12,
          i_micro_kernel_config->vec_c13,
          i_micro_kernel_config->vec_c2,
          i_micro_kernel_config->vec_c21,
          i_micro_kernel_config->vec_c22,
          i_micro_kernel_config->vec_c23,
          i_micro_kernel_config->vec_tmp0,
          i_micro_kernel_config->vec_tmp1,
          i_gp_reg_tmp0,
          i_gp_reg_tmp1,
          l_tupletype, l_sve_type, l_pred_reg );
    }

    if (op == LIBXSMM_MELTW_TYPE_UNARY_GELU_INV ) {
      libxsmm_generator_prepare_coeffs_gelu_inv_ps_minimax3_aarch64( io_generated_code,
          i_micro_kernel_config->vec_thres,
          i_micro_kernel_config->vec_absmask,
          i_micro_kernel_config->vec_scale,
          i_micro_kernel_config->vec_shifter,
          i_micro_kernel_config->vec_halves,
          i_micro_kernel_config->vec_c0,
          i_micro_kernel_config->vec_c01,
          i_micro_kernel_config->vec_c02,
          i_micro_kernel_config->vec_c03,
          i_micro_kernel_config->vec_c1,
          i_micro_kernel_config->vec_c11,
          i_micro_kernel_config->vec_c12,
          i_micro_kernel_config->vec_c13,
          i_micro_kernel_config->vec_c2,
          i_micro_kernel_config->vec_c21,
          i_micro_kernel_config->vec_c22,
          i_micro_kernel_config->vec_c23,
          i_micro_kernel_config->vec_tmp0,
          i_micro_kernel_config->vec_tmp1,
          i_gp_reg_tmp0,
          i_gp_reg_tmp1,
          l_tupletype, l_sve_type, l_pred_reg );
    }


    i_micro_kernel_config->reserved_zmms = reserved_zmms;
  }

  if ((op == LIBXSMM_MELTW_TYPE_UNARY_EXP) || (op == LIBXSMM_MELTW_TYPE_UNARY_ELU)) {
    unsigned int reserved_zmms = i_micro_kernel_config->reserved_zmms;

    reserved_zmms += 9;
    i_micro_kernel_config->vec_halves     = reserved_zmms - 1;
    i_micro_kernel_config->vec_c0         = reserved_zmms - 2;
    i_micro_kernel_config->vec_c1         = reserved_zmms - 3;
    i_micro_kernel_config->vec_c2         = reserved_zmms - 4;
    i_micro_kernel_config->vec_c3         = reserved_zmms - 5;
    i_micro_kernel_config->vec_log2e      = reserved_zmms - 6;
    i_micro_kernel_config->vec_expmask    = reserved_zmms - 7;
    i_micro_kernel_config->vec_hi_bound   = reserved_zmms - 8;
    i_micro_kernel_config->vec_lo_bound   = reserved_zmms - 9;

    if (l_is_sve) libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
    libxsmm_generator_prepare_coeffs_exp_ps_3dts_aarch64( io_generated_code,
        i_micro_kernel_config->vec_c0,
        i_micro_kernel_config->vec_c1,
        i_micro_kernel_config->vec_c2,
        i_micro_kernel_config->vec_c3,
        i_micro_kernel_config->vec_halves,
        i_micro_kernel_config->vec_log2e,
        i_micro_kernel_config->vec_expmask,
        i_micro_kernel_config->vec_hi_bound,
        i_micro_kernel_config->vec_lo_bound,
        i_gp_reg_tmp0,
        l_tupletype, l_sve_type, l_pred_reg );
    i_micro_kernel_config->reserved_zmms = reserved_zmms;
  }
  if (op == LIBXSMM_MELTW_TYPE_UNARY_TANH || op == LIBXSMM_MELTW_TYPE_UNARY_TANH_INV) {
    unsigned int reserved_zmms = i_micro_kernel_config->reserved_zmms;
    unsigned int reserved_mask_regs = i_micro_kernel_config->reserved_mask_regs;
    reserved_zmms += 17;

    i_micro_kernel_config->vec_x2        = reserved_zmms - 1;
    if( l_is_sve ){
      i_micro_kernel_config->mask_hi     = reserved_mask_regs++;
      i_micro_kernel_config->mask_lo     = reserved_mask_regs++;
    } else {
      i_micro_kernel_config->mask_hi     = reserved_zmms - 2;
      i_micro_kernel_config->mask_lo     = reserved_zmms - 3;
    }
    i_micro_kernel_config->vec_nom       = reserved_zmms - 4;
    i_micro_kernel_config->vec_denom     = reserved_zmms - 5;
    i_micro_kernel_config->vec_c0        = reserved_zmms - 6;
    i_micro_kernel_config->vec_c1        = reserved_zmms - 7;
    i_micro_kernel_config->vec_c2        = reserved_zmms - 8;
    i_micro_kernel_config->vec_c3        = reserved_zmms - 9;
    i_micro_kernel_config->vec_c1_d      = reserved_zmms - 10;
    i_micro_kernel_config->vec_c2_d      = reserved_zmms - 11;
    i_micro_kernel_config->vec_c3_d      = reserved_zmms - 12;
    i_micro_kernel_config->vec_hi_bound  = reserved_zmms - 13;
    i_micro_kernel_config->vec_lo_bound  = reserved_zmms - 14;
    i_micro_kernel_config->vec_ones      = reserved_zmms - 15;
    i_micro_kernel_config->vec_neg_ones  = reserved_zmms - 16;
    i_micro_kernel_config->vec_tmp0      = reserved_zmms - 17;

    if (l_is_sve) libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
    libxsmm_generator_prepare_coeffs_tanh_ps_rational_78_aarch64( io_generated_code,
        i_micro_kernel_config->vec_c0,
        i_micro_kernel_config->vec_c1,
        i_micro_kernel_config->vec_c2,
        i_micro_kernel_config->vec_c3,
        i_micro_kernel_config->vec_c1_d,
        i_micro_kernel_config->vec_c2_d,
        i_micro_kernel_config->vec_c3_d,
        i_micro_kernel_config->vec_hi_bound,
        i_micro_kernel_config->vec_lo_bound,
        i_micro_kernel_config->vec_ones,
        i_micro_kernel_config->vec_neg_ones,
        i_gp_reg_tmp1,
        l_tupletype, l_sve_type, l_pred_reg );

    i_micro_kernel_config->reserved_mask_regs = reserved_mask_regs;
    i_micro_kernel_config->reserved_zmms = reserved_zmms;
  }

  if (op == LIBXSMM_MELTW_TYPE_UNARY_SIGMOID || op == LIBXSMM_MELTW_TYPE_UNARY_SIGMOID_INV) {
    unsigned int reserved_zmms = i_micro_kernel_config->reserved_zmms;
    unsigned int reserved_mask_regs = i_micro_kernel_config->reserved_mask_regs;
    reserved_zmms += 18;

    i_micro_kernel_config->vec_x2        = reserved_zmms - 1;
    if( l_is_sve ){
      i_micro_kernel_config->mask_hi     = reserved_mask_regs++;
      i_micro_kernel_config->mask_lo     = reserved_mask_regs++;
    } else {
      i_micro_kernel_config->mask_hi     = reserved_zmms - 2;
      i_micro_kernel_config->mask_lo     = reserved_zmms - 3;
    }
    i_micro_kernel_config->vec_nom       = reserved_zmms - 4;
    i_micro_kernel_config->vec_denom     = reserved_zmms - 5;
    i_micro_kernel_config->vec_c0        = reserved_zmms - 6;
    i_micro_kernel_config->vec_c1        = reserved_zmms - 7;
    i_micro_kernel_config->vec_c2        = reserved_zmms - 8;
    i_micro_kernel_config->vec_c3        = reserved_zmms - 9;
    i_micro_kernel_config->vec_c1_d      = reserved_zmms - 10;
    i_micro_kernel_config->vec_c2_d      = reserved_zmms - 11;
    i_micro_kernel_config->vec_c3_d      = reserved_zmms - 12;
    i_micro_kernel_config->vec_hi_bound  = reserved_zmms - 13;
    i_micro_kernel_config->vec_lo_bound  = reserved_zmms - 14;
    i_micro_kernel_config->vec_ones      = reserved_zmms - 15;
    i_micro_kernel_config->vec_neg_ones  = reserved_zmms - 16;
    i_micro_kernel_config->vec_tmp0      = reserved_zmms - 17;
    i_micro_kernel_config->vec_halves    = reserved_zmms - 18;

    if (l_is_sve) libxsmm_generator_set_p_register_aarch64_sve(io_generated_code, l_pred_reg, -1, 0);
    libxsmm_generator_prepare_coeffs_sigmoid_ps_rational_78_aarch64( io_generated_code,
      i_micro_kernel_config->vec_c0,
      i_micro_kernel_config->vec_c1,
      i_micro_kernel_config->vec_c2,
      i_micro_kernel_config->vec_c3,
      i_micro_kernel_config->vec_c1_d,
      i_micro_kernel_config->vec_c2_d,
      i_micro_kernel_config->vec_c3_d,
      i_micro_kernel_config->vec_hi_bound,
      i_micro_kernel_config->vec_lo_bound,
      i_micro_kernel_config->vec_ones,
      i_micro_kernel_config->vec_neg_ones,
      i_micro_kernel_config->vec_halves,
      i_gp_reg_tmp1,
      l_tupletype, l_sve_type, l_pred_reg );

    i_micro_kernel_config->reserved_mask_regs = reserved_mask_regs;
    i_micro_kernel_config->reserved_zmms = reserved_zmms;
  }
}

LIBXSMM_API_INTERN
void libxsmm_finalize_unary_aarch64_kernel_vregs_masks( libxsmm_generated_code*                 io_generated_code,
                                                        libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                        unsigned int                            op,
                                                        unsigned int                            flags,
                                                        unsigned int                            i_gp_reg_tmp,
                                                        const unsigned int                      i_gp_reg_aux0,
                                                        const unsigned int                      i_gp_reg_aux1 ) {

  LIBXSMM_UNUSED(flags);
  LIBXSMM_UNUSED(i_gp_reg_tmp);
  LIBXSMM_UNUSED(i_gp_reg_aux1);

  if ( op == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT ) {
    libxsmm_generator_store_prng_state_aarch64_asimd( io_generated_code, i_gp_reg_aux0,
                                                      i_micro_kernel_config->prng_state0_vreg, i_micro_kernel_config->prng_state1_vreg,
                                                      i_micro_kernel_config->prng_state2_vreg, i_micro_kernel_config->prng_state3_vreg );
  }
}

LIBXSMM_API_INTERN
void libxsmm_configure_aarch64_kernel_vregs_masks( libxsmm_generated_code*                       io_generated_code,
                                                 libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                 const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                 unsigned int                            i_gp_reg_tmp0,
                                                 unsigned int                            i_gp_reg_tmp1,
                                                 const unsigned int                      i_gp_reg_aux0,
                                                 const unsigned int                      i_gp_reg_aux1) {
  /* initialize some values */
  i_micro_kernel_config->reserved_zmms = 0;
  i_micro_kernel_config->reserved_mask_regs = 1;
  i_micro_kernel_config->use_fp32bf16_cvt_replacement = 0;

  /* if we need FP32->BF16 downconverts and we don't have native instruction, then prepare stack */
#if 0
  if ( (LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype2 ) || LIBXSMM_DATATYPE_F32 == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype )) &&
       LIBXSMM_DATATYPE_BF16 == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype ) && (io_generated_code->arch < LIBXSMM_X86_AVX512_CPX)) {
    i_micro_kernel_config->use_fp32bf16_cvt_replacement = 1;
    libxsmm_generator_vcvtneps2bf16_avx512_prep_stack( io_generated_code, i_gp_reg_tmp );
    i_micro_kernel_config->dcvt_mask_aux0 = i_micro_kernel_config->reserved_mask_regs;
    i_micro_kernel_config->dcvt_mask_aux1 = i_micro_kernel_config->reserved_mask_regs + 1;
    i_micro_kernel_config->reserved_mask_regs = i_micro_kernel_config->reserved_mask_regs + 2;
    i_micro_kernel_config->dcvt_zmm_aux0 = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->dcvt_zmm_aux1 = i_micro_kernel_config->reserved_zmms + 1;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 2;
  }
#endif
  if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
    libxsmm_configure_unary_aarch64_kernel_vregs_masks( io_generated_code, i_micro_kernel_config, i_mateltwise_desc->param, i_mateltwise_desc->flags, i_gp_reg_tmp0, i_gp_reg_tmp1, i_gp_reg_aux0, i_gp_reg_aux1);
  }

  if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
    /* This is the temp register used to load the second input */
    i_micro_kernel_config->tmp_vreg = i_micro_kernel_config->reserved_zmms;
    i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 1;

    if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_BINARY_MULADD) {
      i_micro_kernel_config->tmp_vreg2 = i_micro_kernel_config->reserved_zmms;
      i_micro_kernel_config->reserved_zmms = i_micro_kernel_config->reserved_zmms + 1;
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_finalize_kernel_vregs_aarch64_masks( libxsmm_generated_code*                 io_generated_code,
                                                  libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                  const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                  unsigned int                            i_gp_reg_tmp,
                                                  const unsigned int                      i_gp_reg_aux0,
                                                  const unsigned int                      i_gp_reg_aux1) {
  if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
    libxsmm_finalize_unary_aarch64_kernel_vregs_masks( io_generated_code, i_micro_kernel_config, i_mateltwise_desc->param, i_mateltwise_desc->flags, i_gp_reg_tmp, i_gp_reg_aux0, i_gp_reg_aux1);
  }
#if 0
  if (i_micro_kernel_config->use_fp32bf16_cvt_replacement == 1) {
    libxsmm_generator_vcvtneps2bf16_avx512_clean_stack( io_generated_code, i_gp_reg_tmp );
  }
#endif
}

LIBXSMM_API_INTERN
void libxsmm_generator_unary_aarch64_binary_2d_microkernel( libxsmm_generated_code*                 io_generated_code,
                                                            libxsmm_loop_label_tracker*             io_loop_label_tracker,
                                                            libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                            libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                            const libxsmm_meltw_descriptor*         i_mateltwise_desc,
                                                            unsigned int                            i_m,
                                                            unsigned int                            i_n) {

  unsigned int use_m_input_masking, use_m_output_masking, m_trips, m_unroll_factor, m_assm_trips, n_trips, n_unroll_factor, n_assm_trips;
  unsigned int reserved_zmms = i_micro_kernel_config->reserved_zmms;
  unsigned int out_loop_trips, inner_loop_trips, out_loop_reg, inner_loop_reg, out_loop_bound, inner_loop_bound, out_unroll_factor, inner_unroll_factor;
  unsigned int mask_reg_in, mask_reg_out;
  unsigned int i_vlen_in = i_micro_kernel_config->vlen_in;
  unsigned int i_vlen_out = i_micro_kernel_config->vlen_out;
  unsigned int loop_type;

  use_m_input_masking = 0;
  use_m_output_masking = 0;
  mask_reg_in = LIBXSMM_AARCH64_GP_REG_UNDEF;
  mask_reg_out = LIBXSMM_AARCH64_GP_REG_UNDEF;

  /* Configure microkernel masks */
  libxsmm_setup_input_output_aarch64_masks( io_generated_code, i_micro_kernel_config, i_mateltwise_desc,
    i_gp_reg_mapping->gp_reg_scratch_0, i_m, &use_m_input_masking, &mask_reg_in, &use_m_output_masking, &mask_reg_out);
  reserved_zmms = i_micro_kernel_config->reserved_zmms;

  /* Configure microkernel loops */
  libxsmm_configure_microkernel_aarch64_loops( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc, i_m, i_n, use_m_input_masking,
    &m_trips, &n_trips, &m_unroll_factor, &n_unroll_factor, &m_assm_trips, &n_assm_trips,
    &out_loop_trips, &inner_loop_trips, &out_loop_bound, &inner_loop_bound, &out_loop_reg, &inner_loop_reg, &out_unroll_factor, &inner_unroll_factor );

  /* Headers of microkernel loops */
  if (out_loop_trips > 1) {
    libxsmm_generator_loop_header_aarch64(io_generated_code, io_loop_label_tracker, out_loop_reg, out_loop_bound);
  }

  if (inner_loop_trips > 1) {
    libxsmm_generator_loop_header_aarch64(io_generated_code, io_loop_label_tracker, inner_loop_reg, inner_loop_bound);
  }

  /* Load block of registers */
  libxsmm_load_aarch64_2d_reg_block(io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
      i_vlen_in, reserved_zmms, m_unroll_factor, n_unroll_factor, use_m_input_masking, mask_reg_in);

  /* Compute on registers */
  libxsmm_compute_unary_binary_aarch64_2d_reg_block(io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
      i_vlen_in, reserved_zmms, m_unroll_factor, n_unroll_factor, use_m_input_masking, mask_reg_in);

  /* Store block of registers */
  libxsmm_store_aarch64_2d_reg_block(io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
      i_vlen_out, reserved_zmms, m_unroll_factor, n_unroll_factor, use_m_output_masking, mask_reg_out);


  /* Footers of microkernel loops  */
  if (inner_loop_trips > 1) {
    /* Advance input/output pointers */
    loop_type = (inner_loop_reg == i_gp_reg_mapping->gp_reg_m_loop) ? LOOP_TYPE_M : LOOP_TYPE_N;

    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_ADD, inner_unroll_factor, loop_type);

    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_ADD, inner_unroll_factor, loop_type);

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
      adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_ADD, inner_unroll_factor, loop_type);
    }

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, inner_unroll_factor, loop_type);
      } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, inner_unroll_factor, loop_type);
      }
    }

    libxsmm_generator_loop_footer_aarch64(io_generated_code, io_loop_label_tracker, inner_loop_reg, inner_unroll_factor);

    /* Reset input/output pointers  */
    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_SUB, inner_unroll_factor * inner_loop_trips, loop_type);

    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_SUB, inner_unroll_factor * inner_loop_trips, loop_type);

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
      adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_SUB, inner_unroll_factor * inner_loop_trips, loop_type);
    }

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_SUB, inner_unroll_factor * inner_loop_trips, loop_type);
      } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_SUB, inner_unroll_factor * inner_loop_trips, loop_type);
      }
    }
  }

  if (out_loop_trips > 1) {
    /* Advance input/output pointers */
    loop_type = (out_loop_reg == i_gp_reg_mapping->gp_reg_m_loop) ? LOOP_TYPE_M : LOOP_TYPE_N;

    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_ADD, out_unroll_factor, loop_type);

    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_ADD, out_unroll_factor, loop_type);

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
      adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_ADD, out_unroll_factor, loop_type);
    }

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, out_unroll_factor, loop_type);
      } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, out_unroll_factor, loop_type);
      }
    }

    libxsmm_generator_loop_footer_aarch64(io_generated_code, io_loop_label_tracker, out_loop_reg, out_unroll_factor);

    /* Reset input/output pointers  */
    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_SUB, out_unroll_factor * out_loop_trips, loop_type);

    adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
        i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_SUB, out_unroll_factor * out_loop_trips, loop_type);

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
      adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_SUB, out_unroll_factor * out_loop_trips, loop_type);
    }

    if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
      if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
          (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_SUB, out_unroll_factor * out_loop_trips, loop_type);
      } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
        adjust_in_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_SUB, out_unroll_factor * out_loop_trips, loop_type);
      }
    }
  }
}

LIBXSMM_API_INTERN
void libxsmm_generator_unary_binary_aarch64_microkernel( libxsmm_generated_code*          io_generated_code,
                                                 libxsmm_loop_label_tracker*             io_loop_label_tracker,
                                                 libxsmm_mateltwise_gp_reg_mapping*      i_gp_reg_mapping,
                                                 libxsmm_mateltwise_kernel_config*       i_micro_kernel_config,
                                                 const libxsmm_meltw_descriptor*         i_mateltwise_desc ) {
  unsigned int loop_order, m_blocking = 0, out_blocking = 0, out_bound = 0, out_block = 0, n_blocking = 0, inner_blocking = 0, inner_block = 0, inner_bound = 0, n_microkernel = 0, m_microkernel = 0;
  unsigned int out_ind, inner_ind, reset_regs, loop_type;
  unsigned int available_vregs = 32;
  unsigned int l_gp_reg_tmp = LIBXSMM_AARCH64_GP_REG_X16;
  unsigned int l_gp_reg_aux0 = LIBXSMM_AARCH64_GP_REG_UNDEF;
  unsigned int l_gp_reg_aux1 = LIBXSMM_AARCH64_GP_REG_UNDEF;

  /* Some rudimentary checking of M, N and LDs*/
#if 0
  if ( (i_mateltwise_desc->m > i_mateltwise_desc->ldi) ||
       (i_mateltwise_desc->m > i_mateltwise_desc->ldo)    ) {
    LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_LDA );
    return;
  }
#endif

  /* check datatype */
  if ( ( LIBXSMM_GEMM_PRECISION_F32  == LIBXSMM_GETENUM_INP( i_mateltwise_desc->datatype ) )
       &&
       ( LIBXSMM_GEMM_PRECISION_F32  == LIBXSMM_GETENUM_OUT( i_mateltwise_desc->datatype ) ) ) {
    /* fine */
  } else {
    LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_UNSUP_DATATYPE );
    return;
  }

  /* Configure vlens */
  libxsmm_generator_configure_aarch64_vlens(i_mateltwise_desc, i_micro_kernel_config);

  /* set mask lds */
  if ( (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) > 0) ) {
    i_micro_kernel_config->ldo_mask = (i_mateltwise_desc->ldo+15) - ((i_mateltwise_desc->ldo+15)%16);
    i_micro_kernel_config->ldi_mask = (i_mateltwise_desc->ldi+15) - ((i_mateltwise_desc->ldi+15)%16);
  }

  /* let's check that we have bitmask set for dropout and relu backward */
  if ( ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
         (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV) ) && ((i_mateltwise_desc->flags & LIBXSMM_MELTW_FLAG_UNARY_BITMASK_2BYTEMULT) == 0) ) {
    LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_BITMASK_REQUIRED );
    return;
  }

  /* Configure the register mapping for this eltwise kernel */
  i_gp_reg_mapping->gp_reg_in        = LIBXSMM_AARCH64_GP_REG_X8;
  i_gp_reg_mapping->gp_reg_out       = LIBXSMM_AARCH64_GP_REG_X9;
  i_gp_reg_mapping->gp_reg_m_loop    = LIBXSMM_AARCH64_GP_REG_X10;
  i_gp_reg_mapping->gp_reg_n_loop    = LIBXSMM_AARCH64_GP_REG_X11;
  i_gp_reg_mapping->gp_reg_scratch_0 = LIBXSMM_AARCH64_GP_REG_X16;
  i_gp_reg_mapping->gp_reg_scratch_1 = LIBXSMM_AARCH64_GP_REG_X17;
  if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY ) {
    i_gp_reg_mapping->gp_reg_in2  = LIBXSMM_AARCH64_GP_REG_X12;
  } else {
    if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV)       ||
         (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV)    ) {
      i_gp_reg_mapping->gp_reg_relumask = LIBXSMM_AARCH64_GP_REG_X12;
    }
    if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
      i_gp_reg_mapping->gp_reg_relumask = LIBXSMM_AARCH64_GP_REG_X12;
      i_gp_reg_mapping->gp_reg_fam_lualpha = LIBXSMM_AARCH64_GP_REG_X13;
    }
    if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ) {
      i_gp_reg_mapping->gp_reg_fam_lualpha = LIBXSMM_AARCH64_GP_REG_X13;
    }
    if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_UNPACK_TO_BLOCKS) {
      i_gp_reg_mapping->gp_reg_offset = LIBXSMM_AARCH64_GP_REG_X12;
    } else {
      i_gp_reg_mapping->gp_reg_offset = LIBXSMM_AARCH64_GP_REG_UNDEF;
    }
    if ( (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV) ) {
      i_gp_reg_mapping->gp_reg_dropoutmask = LIBXSMM_AARCH64_GP_REG_X12;
      i_gp_reg_mapping->gp_reg_dropoutprob = LIBXSMM_AARCH64_GP_REG_X13;
      if ( i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT ) {
        i_gp_reg_mapping->gp_reg_prngstate = LIBXSMM_AARCH64_GP_REG_X14;
      } else {
        i_gp_reg_mapping->gp_reg_prngstate = LIBXSMM_AARCH64_GP_REG_UNDEF;
      }
    } else {
      i_gp_reg_mapping->gp_reg_dropoutmask = LIBXSMM_AARCH64_GP_REG_UNDEF;
      i_gp_reg_mapping->gp_reg_dropoutprob = LIBXSMM_AARCH64_GP_REG_UNDEF;
      i_gp_reg_mapping->gp_reg_prngstate = LIBXSMM_AARCH64_GP_REG_UNDEF;
    }
  }

  /* load the input pointer and output pointer */
  if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY ) {
    libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                          LIBXSMM_AARCH64_GP_REG_UNDEF, 32, i_gp_reg_mapping->gp_reg_in );
    libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                          LIBXSMM_AARCH64_GP_REG_UNDEF, 56, i_gp_reg_mapping->gp_reg_out );
    if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 64, i_gp_reg_mapping->gp_reg_relumask );
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 40, i_gp_reg_mapping->gp_reg_relumask );
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)  {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_gp_reg_mapping->gp_reg_fam_lualpha );
      l_gp_reg_aux1 = i_gp_reg_mapping->gp_reg_fam_lualpha;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 40, i_gp_reg_mapping->gp_reg_relumask );
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_gp_reg_mapping->gp_reg_fam_lualpha );
      l_gp_reg_aux1 = i_gp_reg_mapping->gp_reg_fam_lualpha;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_gp_reg_mapping->gp_reg_fam_lualpha );
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 64, i_gp_reg_mapping->gp_reg_relumask );
      l_gp_reg_aux1 = i_gp_reg_mapping->gp_reg_fam_lualpha;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 40, i_gp_reg_mapping->gp_reg_relumask );
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_gp_reg_mapping->gp_reg_fam_lualpha );
      l_gp_reg_aux1 = i_gp_reg_mapping->gp_reg_fam_lualpha;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_UNPACK_TO_BLOCKS) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 64, i_gp_reg_mapping->gp_reg_offset );
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 8, i_gp_reg_mapping->gp_reg_prngstate );
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_gp_reg_mapping->gp_reg_dropoutprob );
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 64, i_gp_reg_mapping->gp_reg_dropoutmask );
      l_gp_reg_aux0 = i_gp_reg_mapping->gp_reg_prngstate;
      l_gp_reg_aux1 = i_gp_reg_mapping->gp_reg_dropoutprob;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 40, i_gp_reg_mapping->gp_reg_dropoutmask );
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 0, i_gp_reg_mapping->gp_reg_dropoutprob );
      l_gp_reg_aux1 = i_gp_reg_mapping->gp_reg_dropoutprob;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_QUANT) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 40, i_gp_reg_mapping->gp_reg_quant_sf );
      l_gp_reg_aux0 = i_gp_reg_mapping->gp_reg_quant_sf;
    } else if (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DEQUANT) {
      libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                            LIBXSMM_AARCH64_GP_REG_UNDEF, 40, i_gp_reg_mapping->gp_reg_quant_sf );
      l_gp_reg_aux0 = i_gp_reg_mapping->gp_reg_quant_sf;
    }
  } else if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY ) {
    libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                          LIBXSMM_AARCH64_GP_REG_UNDEF, 32, i_gp_reg_mapping->gp_reg_in );
    libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                          LIBXSMM_AARCH64_GP_REG_UNDEF, 56, i_gp_reg_mapping->gp_reg_in2 );
    libxsmm_aarch64_instruction_alu_move( io_generated_code, LIBXSMM_AARCH64_INSTR_GP_LDR_I_OFF, i_gp_reg_mapping->gp_reg_param_struct,
                                          LIBXSMM_AARCH64_GP_REG_UNDEF, 80, i_gp_reg_mapping->gp_reg_out );
  } else {
    /* This should not happen */
    LIBXSMM_HANDLE_ERROR( io_generated_code, LIBXSMM_ERR_GENERAL );
  }

  /* Based on kernel type reserve zmms and mask registers  */
  libxsmm_configure_aarch64_kernel_vregs_masks( io_generated_code, i_micro_kernel_config, i_mateltwise_desc, i_gp_reg_mapping->gp_reg_scratch_0, i_gp_reg_mapping->gp_reg_scratch_1, l_gp_reg_aux0, l_gp_reg_aux1 );

  available_vregs = available_vregs - i_micro_kernel_config->reserved_zmms;

  /* Configure M and N blocking factors */
  /* todo sve: here we might intercept the m, n blocking size */
  libxsmm_generator_configure_aarch64_M_N_blocking( io_generated_code, i_mateltwise_desc, i_mateltwise_desc->m, i_mateltwise_desc->n, i_micro_kernel_config->vlen_in, &m_blocking, &n_blocking, available_vregs);
  libxsmm_generator_configure_aarch64_loop_order(i_mateltwise_desc, &loop_order, &m_blocking, &n_blocking, &out_blocking, &inner_blocking, &out_bound, &inner_bound);
  i_micro_kernel_config->loop_order = loop_order;

  out_ind = 0;
  while ( out_ind != out_bound ) {
    inner_ind = 0;
    reset_regs = 0;
    while( inner_ind != inner_bound ) {

      out_block = (out_ind < out_blocking) ? out_blocking : out_bound - out_ind;
      inner_block  = (inner_ind < inner_blocking ) ? inner_blocking : inner_bound - inner_ind;
      n_microkernel = (loop_order == NM_LOOP_ORDER) ? out_block : inner_block;
      m_microkernel = (loop_order == MN_LOOP_ORDER) ? out_block : inner_block;


      libxsmm_generator_unary_aarch64_binary_2d_microkernel(io_generated_code, io_loop_label_tracker, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc, m_microkernel, n_microkernel);

      inner_ind += inner_block;

      if (inner_ind != inner_bound) {
        reset_regs = 1;
        /* Advance input/output pointers */
        loop_type = (loop_order == NM_LOOP_ORDER) ? LOOP_TYPE_M : LOOP_TYPE_N;

        adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );

        adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );

        if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
          adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
              i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );
        }

        if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
          if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
              (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
              (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
            adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
                i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );
          } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
            adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
                i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );
          }
        }
      }
    }

    /* If needed, readjust the registers */
    if (reset_regs == 1) {
      loop_type = (loop_order == NM_LOOP_ORDER) ? LOOP_TYPE_M : LOOP_TYPE_N;

      adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_SUB, m_microkernel, n_microkernel, loop_type );

      adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_SUB, m_microkernel, n_microkernel, loop_type );

      if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
        adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_SUB, m_microkernel, n_microkernel, loop_type );
      }

      if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
        if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
            (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
            (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
          adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
              i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_SUB, m_microkernel, n_microkernel, loop_type );
        } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
          adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
              i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_SUB, m_microkernel, n_microkernel, loop_type );
        }
      }
    }

    out_ind += out_block;
    if (out_ind != out_bound) {
      /* Advance input/output pointers */
      loop_type = (loop_order == MN_LOOP_ORDER) ? LOOP_TYPE_M : LOOP_TYPE_N;

      adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_in, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );

      adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
          i_gp_reg_mapping->gp_reg_out, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );

      if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_BINARY) {
        adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
            i_gp_reg_mapping->gp_reg_in2, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );
      }

      if (i_mateltwise_desc->operation == LIBXSMM_MELTW_OPERATION_UNARY) {
        if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU)       || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_RELU_INV) ||
            (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_LEAKY_RELU_INV) ||
            (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU)        || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_ELU_INV) ) {
          adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
              i_gp_reg_mapping->gp_reg_relumask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );
        } else if ((i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT) || (i_mateltwise_desc->param == LIBXSMM_MELTW_TYPE_UNARY_DROPOUT_INV)) {
          adjust_after_microkernel_addr_aarch64_gp_reg( io_generated_code, i_gp_reg_mapping, i_micro_kernel_config, i_mateltwise_desc,
              i_gp_reg_mapping->gp_reg_dropoutmask, LIBXSMM_AARCH64_INSTR_GP_META_ADD, m_microkernel, n_microkernel, loop_type );
        }
      }
    }
  }

  /* save some globale state if needed */
  libxsmm_finalize_kernel_vregs_aarch64_masks( io_generated_code, i_micro_kernel_config, i_mateltwise_desc, l_gp_reg_tmp, l_gp_reg_aux0, l_gp_reg_aux1 );
}
