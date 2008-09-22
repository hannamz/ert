#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <ctype.h>
#include <menu.h>
#include <enkf_ui_run.h>
#include <enkf_ui_export.h>
#include <enkf_ui_plot.h>
#include <enkf_ui_init.h>
#include <void_arg.h>
#include <enkf_main.h>
#include <enkf_sched.h>


/**
   This file implements the (text based) user interface in the enkf
   system.
*/



/** 
    The main loop.
*/





void enkf_ui_main_menu(enkf_main_type * enkf_main , enkf_sched_type * enkf_sched) {
  void_arg_type * run_arg = void_arg_alloc2(void_pointer , void_pointer);
  void_arg_pack_ptr(run_arg , 0 , enkf_main);
  void_arg_pack_ptr(run_arg , 1 , enkf_sched);
  {
    menu_type * menu = menu_alloc("EnKF main menu" , "qQ");
    menu_add_item(menu , "Initialize EnKF ensemble"     , "iI" , enkf_ui_init_menu   , run_arg);
    menu_add_item(menu , "Run EnKF"                     , "rR" , enkf_ui_run_menu    , run_arg);
    menu_add_item(menu , "Export data to other formats" , "eE" , enkf_ui_export_menu , run_arg);
    menu_add_item(menu , "Plot results"                 , "pP" , enkf_ui_plot_menu   , run_arg);

    /*
      if (enkf_main_get_runlock_mode(enkf_main) == lock_file) {
      menu_add_separator(menu);
      
      menu_add_item(menu , "Clear runlocks" , "cC" , enkf_main_clear_locks__  , enkf_main);
      }
    */
      
    menu_run(menu);
    menu_free(menu);
  }
  void_arg_free(run_arg);
}


