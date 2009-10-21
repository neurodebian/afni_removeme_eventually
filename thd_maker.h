/*****************************************************************************
   Major portions of this software are copyrighted by the Medical College
   of Wisconsin, 1994-2000, and are released under the Gnu General Public
   License, Version 2.  See the file README.Copyright for details.
******************************************************************************/
   
#ifndef _MCW_3DMAKER_HEADER_
#define _MCW_3DMAKER_HEADER_

#include "mrilib.h"

#define FREEUP(x) do{if((x) != NULL){free((x)); (x)=NULL;}}while(0)

extern THD_3dim_dataset *
    MAKER_4D_to_typed_fim( THD_3dim_dataset * old_dset ,
                           char * new_prefix , int new_datum ,
                           int ignore , int detrend ,
                           generic_func * user_func ,
                           void * user_data ) ;

extern THD_3dim_dataset *
    MAKER_4D_to_typed_fith( THD_3dim_dataset * old_dset ,
                            char * new_prefix , int new_datum ,
                            int ignore , int detrend ,
                            generic_func * user_func ,
                            void * user_data ) ;

extern THD_3dim_dataset *
    MAKER_4D_to_typed_fbuc( THD_3dim_dataset * old_dset ,
                            char * new_prefix , int new_datum ,
                            int ignore , int detrend ,
                            int nbrik , generic_func * user_func ,
                            void * user_data ) ;

#endif
