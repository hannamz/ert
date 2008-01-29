#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <util.h>
#include <hash.h>
#include <multz_config.h>
#include <enkf_ens.h>
#include <enkf_config_node.h>
#include <path_fmt.h>
#include <ecl_static_kw_config.h>
#include <enkf_types.h>
#include <well_config.h>
#include <field_config.h>
#include <equil_config.h>
#include <multz_config.h>
#include <multflt_config.h>
#include <well_obs.h>
#include <pgbox_config.h>
#include <obs_node.h>
#include <obs_data.h>
#include <history.h>
#include <meas_matrix.h>
#include <enkf_state.h>  
#include <analysis.h>
#include <enkf_obs.h>
#include <sched_file.h>
#include <enkf_fs.h>


struct enkf_ens_struct {
  int  		      ens_size;
  meas_matrix_type   *meas_matrix;
  hash_type          *config_hash;
  enkf_obs_type      *obs;
  enkf_state_type  **state_list;
  sched_file_type   *sched_file;
  
  char            **well_list;
  int               Nwells;

  enkf_fs_type     *fs;
  path_fmt_type    *run_path;
  bool              endian_swap;
};


/*****************************************************************/




enkf_impl_type enkf_ens_impl_type(const enkf_ens_type *enkf_ens, const char * ecl_kw_name) {
  enkf_impl_type impl_type;

  if (hash_has_key(enkf_ens->config_hash , ecl_kw_name)) {
    enkf_config_node_type * node = hash_get(enkf_ens->config_hash , ecl_kw_name);
    impl_type = enkf_config_node_get_impl_type(node);
  } else
    impl_type = STATIC;

  return impl_type;
}



/*****************************************************************/
static void enkf_ens_realloc_well_list(enkf_ens_type * enkf_ens) {
  enkf_ens->well_list = realloc(enkf_ens->well_list , enkf_ens->Nwells * sizeof * enkf_ens->well_list);
}

bool enkf_ens_get_endian_swap(const enkf_ens_type * enkf_ens) { return enkf_ens->endian_swap; }


enkf_ens_type * enkf_ens_alloc(const char * run_path , const char * ens_path_static , const char * ens_path_parameter , const char * ens_path_dynamic_forecast , const char * ens_path_dynamic_analyzed , bool endian_swap) {

  enkf_ens_type * enkf_ens = malloc(sizeof *enkf_ens);
  enkf_ens->config_hash    = hash_alloc(10);
  enkf_ens->obs            = enkf_obs_alloc(enkf_ens->sched_file);
  
  enkf_ens->endian_swap   = endian_swap;
  enkf_ens->Nwells        = 0;
  enkf_ens->well_list     = NULL;
  enkf_ens_realloc_well_list(enkf_ens);
  
  enkf_ens->run_path     = path_fmt_alloc_directory_fmt(run_path , true);
  enkf_ens->meas_matrix  = meas_matrix_alloc(enkf_ens->ens_size);
  enkf_ens->state_list   = malloc(enkf_ens->ens_size * sizeof * enkf_ens->state_list);
  {
    int iens;
    for (iens = 0; iens < enkf_ens->ens_size; iens++)
      enkf_ens->state_list[iens] = NULL ; /*enkf_state_alloc(enkf_ens);*/
  }
  return  enkf_ens;
}



bool enkf_ens_has_key(const enkf_ens_type * enkf_ens , const char * key) {
  return hash_has_key(enkf_ens->config_hash , key);
}


const char ** enkf_ens_get_well_list_ref(const enkf_ens_type * ens , int *Nwells) {
  *Nwells = ens->Nwells;
  return (const char **) ens->well_list;
}


void enkf_ens_add_well(enkf_ens_type * enkf_ens , const char *well_name , int size, const char ** var_list) {
  enkf_ens_add_type(enkf_ens , well_name , ecl_summary , WELL,
		       well_config_alloc(well_name , size , var_list));
  
  enkf_ens->Nwells++;
  enkf_ens_realloc_well_list(enkf_ens);
  enkf_ens->well_list[enkf_ens->Nwells - 1] = util_alloc_string_copy(well_name);
}




void enkf_ens_add_type(enkf_ens_type * enkf_ens, 
			  const char * key , 
			  enkf_var_type enkf_type , 
			  enkf_impl_type impl_type , 
			  const void *data) {
  if (enkf_ens_has_key(enkf_ens , key)) {
    fprintf(stderr,"%s: a ensuration object:%s has already been added - aborting \n",__func__ , key);
    abort();
  }

  {
    config_free_ftype              * freef;
    switch(impl_type) {
    case(FIELD):
      freef             = field_config_free__;
      break;
    case(MULTZ):
      freef             = multz_config_free__;
      break;
    case(WELL):
      freef             = well_config_free__;
      break;
    case(MULTFLT):
      freef             = multflt_config_free__;
      break;
    case(EQUIL):
      freef             = equil_config_free__;
      break;
    case(STATIC):
      freef             = ecl_static_kw_config_free__;
      break;
    case(PGBOX):
      freef             = pgbox_config_free__;
      break;
    default:
      fprintf(stderr,"%s : invalid implementation type: %d - aborting \n",__func__ , impl_type);
      abort();
    }
    {
      enkf_config_node_type * node = enkf_config_node_alloc(enkf_type , impl_type , key , NULL , data , freef);
      hash_insert_hash_owned_ref(enkf_ens->config_hash , key , node , enkf_config_node_free__);
    }
  }
}



void enkf_ens_add_type0(enkf_ens_type * enkf_ens , const char *key , int size, enkf_var_type enkf_type , enkf_impl_type impl_type) {
  switch(impl_type) {
  case(STATIC):
    enkf_ens_add_type(enkf_ens , key , enkf_type , impl_type , ecl_static_kw_config_alloc(size , key , key));
    break;
  case(FIELD):
    /*
      enkf_ens_add_type(enkf_ens , key , enkf_type , impl_type , field_ens_alloc(size , key , key)   , field_ens_free__ , field_ens_get_size__);
    */
    fprintf(stderr,"%s: Can not add FIELD ens objects like:%s on the run - these must be from the main program with enkf_ens_add_type - sorry.\n",__func__ , key);
    abort();
    break;
  default:
    fprintf(stderr,"%s only STATIC and FIELD types are implemented - aborting \n",__func__);
    abort();
  }
}



void enkf_ens_free(enkf_ens_type * enkf_ens) {  
  hash_free(enkf_ens->config_hash);
  enkf_obs_free(enkf_ens->obs);
  {
    int i;
    for (i=0; i < enkf_ens->Nwells; i++)
      free(enkf_ens->well_list[i]);
    free(enkf_ens->well_list);
    
    for (i=0; i < enkf_ens->ens_size; i++)
      enkf_state_free(enkf_ens->state_list[i]);
    
  }
  meas_matrix_free(enkf_ens->meas_matrix);
  path_fmt_free(enkf_ens->run_path);
  enkf_fs_free(enkf_ens->fs);
  free(enkf_ens);
}



const enkf_config_node_type * enkf_ens_get_config_ref(const enkf_ens_type * ens, const char * key) {
  if (hash_has_key(ens->config_hash , key)) {
    enkf_config_node_type * node = hash_get(ens->config_hash , key);
    return node;
  } else {
    fprintf(stderr,"%s: ens node:%s does not exist \n",__func__ , key);
    abort();
  }
}


const path_fmt_type * enkf_ens_get_run_path_ref(const enkf_ens_type *ens) { return ens->run_path; }

/*****************************************************************/


static const enkf_config_node_type * enkf_ens_assert_obs(const enkf_ens_type * ens , const char *obs_key , enkf_impl_type impl_type) {
  if (enkf_ens_has_key(ens , obs_key)) {
    const enkf_config_node_type * config_node = enkf_ens_get_config_ref(ens , obs_key);
    if (enkf_config_node_get_impl_type(config_node) == impl_type) 
      return config_node;
    else {
      fprintf(stderr,"%s ensemble object:%s exists - but it is not of correct type - aborting \n",__func__ , obs_key);
      abort();
    }
  } else {
    fprintf(stderr,"%s: ensemble does not have key: %s - aborting \n",__func__ , obs_key);
    abort();
  }
}


void enkf_ens_add_well_obs(enkf_ens_type * ens , const char *obs_key , const char * obs_label , const char * config_file) {
  const enkf_config_node_type * config_node = enkf_ens_assert_obs(ens , obs_key , WELL);
  enkf_obs_add_well_obs(ens->obs , config_node , obs_key , obs_label , config_file);
}


void enkf_ens_add_field_obs(enkf_ens_type * ens, const char * obs_key, const char * obs_label , int size, const int *i , const int *j , const int *k, const double * obs_data , time_t meas_time) { 
  const enkf_config_node_type * config_node = enkf_ens_assert_obs(ens , obs_key , FIELD);
  enkf_obs_add_field_obs(ens->obs , config_node , obs_key , obs_label , size , i , j , k , obs_data , meas_time);
}


void enkf_ens_add_rft_obs(enkf_ens_type * ens , const ecl_rft_node_type * rft_node, const double * p_data) {
  const enkf_config_node_type * config_node = enkf_ens_assert_obs(ens , "PRES" , FIELD);
  enkf_obs_add_rft_obs(ens->obs , config_node , rft_node , p_data);
}





