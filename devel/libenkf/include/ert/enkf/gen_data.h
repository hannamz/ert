/*
   Copyright (C) 2011  Statoil ASA, Norway.

   The file 'gen_data.h' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#ifndef ERT_GEN_DATA_H
#define ERT_GEN_DATA_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ert/util/util.h>
#include <ert/util/bool_vector.h>
#include <ert/util/double_vector.h>
#include <ert/util/buffer.h>

#include <ert/ecl/ecl_sum.h>
#include <ert/ecl/ecl_file.h>

#include <ert/enkf/enkf_macros.h>
#include <ert/enkf/gen_data_common.h>
#include <ert/enkf/gen_data_config.h>
#include <ert/enkf/forward_load_context.h>

void                      gen_data_assert_size( gen_data_type * gen_data , int size , int report_step);
bool                      gen_data_forward_load(gen_data_type * gen_data , const char * ecl_file , const forward_load_context_type * load_context);
void                      gen_data_free(gen_data_type * );
double                    gen_data_iget_double(const gen_data_type * , int );
gen_data_config_type   *  gen_data_get_config(const gen_data_type * );
const bool_vector_type *  gen_data_get_forward_mask( const gen_data_type * gen_data );
int                       gen_data_get_size(const gen_data_type * );
double                    gen_data_iget_double(const gen_data_type * , int );
void                      gen_data_export(const gen_data_type * gen_data , const char * full_path , gen_data_file_format_type export_type , fortio_type * fortio);
void                      gen_data_export_data(const gen_data_type * gen_data , double_vector_type * export_data);
gen_data_file_format_type gen_data_guess_export_type( const gen_data_type * gen_data );
const char  *             gen_data_get_key( const gen_data_type * gen_data);
void                      gen_data_upgrade_103(const char * filename);
int                       gen_data_get_size( const gen_data_type * gen_data );
void                      gen_data_copy_to_double_vector(const gen_data_type * gen_data , double_vector_type * vector);
bool                      gen_data_fload_with_report_step( gen_data_type * gen_data , const char * filename , const forward_load_context_type * load_context);

UTIL_SAFE_CAST_HEADER(gen_data);
UTIL_SAFE_CAST_HEADER_CONST(gen_data);
VOID_USER_GET_HEADER(gen_data);
VOID_ALLOC_HEADER(gen_data);
VOID_FREE_HEADER(gen_data);
VOID_COPY_HEADER      (gen_data);
VOID_ECL_WRITE_HEADER(gen_data);
VOID_FORWARD_LOAD_HEADER(gen_data);
VOID_INITIALIZE_HEADER(gen_data);
VOID_READ_FROM_BUFFER_HEADER(gen_data);
VOID_WRITE_TO_BUFFER_HEADER(gen_data);
VOID_SERIALIZE_HEADER(gen_data)
VOID_DESERIALIZE_HEADER(gen_data)
VOID_SET_INFLATION_HEADER(gen_data);
VOID_CLEAR_HEADER(gen_data);
VOID_IMUL_HEADER(gen_data);
VOID_IADD_HEADER(gen_data);
VOID_IADDSQR_HEADER(gen_data);
VOID_SCALE_HEADER(gen_data);
VOID_ISQRT_HEADER(gen_data);
#ifdef __cplusplus
}
#endif
#endif
