/*****************************************************************************
   Major portions of this software are copyrighted by the Medical College
   of Wisconsin, 1994-2000, and are released under the Gnu General Public
   License, Version 2.  See the file README.Copyright for details.
******************************************************************************/

#define  NEED_PARSER_INTERNALS
#include "parser.h"
#include "Amalloc.h"

/***** C routines to interface to the f2c generated parser code *****/

static int printout = 0 ;

void PARSER_set_printout( int p ){ printout = p ; }

/*------------------------------------------------------------------
   Input  = expression string
   Output = structure containing information about how to
            evaluate the expression; should be free-d when unneeded;
            if NULL is returned, an error occurred.
--------------------------------------------------------------------*/

PARSER_code * PARSER_generate_code( char * expression )
{
   logical pr ;
   integer num_code ;
   int nexp ;
   PARSER_code * pc ;
   char *exp,cc ; int ii,jj ;  /* 22 Jul 2003 */

   if( expression == NULL ) return NULL ;
   nexp = strlen( expression ) ;
   if( nexp == 0 ) return NULL ;

   /* 22 Jul 2003: copy into local string, tossing bad stuff */

   exp = AFMALL(char, nexp+4) ;
   for( ii=jj=0 ; ii < nexp ; ii++ ){
     cc = expression[ii] ;
     if( !isspace(cc) && !iscntrl(cc) ) exp[jj++] = cc ;
   }
   exp[jj] = '\0' ;
   nexp = strlen(exp) ; if( nexp == 0 ) return NULL ;

   pc = (PARSER_code *) malloc( sizeof(PARSER_code) ) ;

   pr = (printout) ? TRUE_ : FALSE_ ;

   parser_( exp, &pr, &num_code, pc->c_code, (ftnlen) nexp, (ftnlen) 8 ) ;

   free(exp) ;  /* 22 Jul 2003 */

   if( num_code <= 0 ){ free(pc) ; return NULL ; }

   pc->num_code = (int) num_code ;
   return pc ;
}

/*---------------------------------------------------------------
   pc   = code to evaluate expression
   atoz = double [26] containing values for variables A,B,...,Z
-----------------------------------------------------------------*/

double PARSER_evaluate_one( PARSER_code * pc , double atoz[] )
{
   integer num_code ;
   double  value ;

   if( pc == NULL || pc->num_code <= 0 ) return 0.0 ;

   num_code = (integer) pc->num_code ;

   value = (double) pareval_( &num_code, pc->c_code,
                              (doublereal *) atoz , (ftnlen) 8 ) ;
   return value ;
}

/*----------------------------------------------------------------------
   Return 1 if the given code uses the symbol given by the
   first character of sym, otherwise return 0 - 15 Sep 1999 - RWCox.
------------------------------------------------------------------------*/

extern integer hassym_(char *sym, integer *num_code__, char *c_code__, ftnlen
        sym_len, ftnlen c_code_len) ;

int PARSER_has_symbol( char * sym , PARSER_code * pc )
{
   int hh ;
   char sss[8] ;
   integer num_code ;

   if( !isalpha(sym[0]) ) return 0 ;          /* not alphabetic */

   sss[0] = toupper(sym[0]) ; sss[1] = '\0' ; /* uppercase it */

   num_code = (integer) pc->num_code ;

   hh = (int) hassym_( sss , &num_code , pc->c_code ,
                       (ftnlen) 8 , (ftnlen) 8       ) ;

   return hh ;
}

void PARSER_mark_symbols( PARSER_code * pc , int * sl )
{
   int ii ;
   static char abet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ;

   if( pc == NULL || sl == NULL ) return ;

   for( ii=0 ; ii < 26 ; ii++ )
      sl[ii] = PARSER_has_symbol( abet+ii , pc ) ;

   return ;
}

/*----------------------------------------------------------------------
   pc    = code to evaluate expression
   atoz  = double * [26] containing values for variables A..Z
   nv    = length of vectors
   vout  = double [nv]; will get the output of the expression
             evaluated using the values in atoz

   The only reason for calling this routine instead of
   PARSER_evaluate_one nv different times is efficiency.
------------------------------------------------------------------------*/

void PARSER_evaluate_vector( PARSER_code * pc , double* atoz[] ,
                             int nv , double vout[] )
{
   integer num_code , nvar , ivar , lvec , ldvec ;

   if( pc == NULL || pc->num_code <= 0 ) return ;

   num_code = (integer) pc->num_code ;
   lvec     = (integer) nv ;

   parevec_( &num_code , pc->c_code ,
             (doublereal *) atoz[0]  , (doublereal *) atoz[1] ,
             (doublereal *) atoz[2]  , (doublereal *) atoz[3] ,
             (doublereal *) atoz[4]  , (doublereal *) atoz[5] ,
             (doublereal *) atoz[6]  , (doublereal *) atoz[7] ,
             (doublereal *) atoz[8]  , (doublereal *) atoz[9] ,
             (doublereal *) atoz[10] , (doublereal *) atoz[11] ,
             (doublereal *) atoz[12] , (doublereal *) atoz[13] ,
             (doublereal *) atoz[14] , (doublereal *) atoz[15] ,
             (doublereal *) atoz[16] , (doublereal *) atoz[17] ,
             (doublereal *) atoz[18] , (doublereal *) atoz[19] ,
             (doublereal *) atoz[20] , (doublereal *) atoz[21] ,
             (doublereal *) atoz[22] , (doublereal *) atoz[23] ,
             (doublereal *) atoz[24] , (doublereal *) atoz[25] ,
         &lvec , (doublereal *) vout , (ftnlen) 8 ) ;

   return ;
}

/*----------------------------------------------------------------------
   Evaluate an expression at a set of evenly spaced points.
     expr = expression - first symbol found will be the variable
     nt   = number of points
     tz   = value of first point (the variable)
     dt   = spacing between points
     vec  = pointer to pre-allocated output location of length nt
   Return value is 1 for good results, 0 for errors.

   17 Nov 1999 - RW Cox - adapted from 1deval.c [hence the name]
------------------------------------------------------------------------*/

int PARSER_1deval( char * expr, int nt, float tz, float dt, float * vec )
{
   PARSER_code * pcode = NULL ;
   char sym[4] ;
   double atoz[26] ;
   int ii , kvar ;

   if( expr == NULL || nt <= 0 || vec == NULL ) return 0 ;  /* bad */

   pcode = PARSER_generate_code( expr ) ;        /* compile */
   if( pcode == NULL ) return 0 ;                /* bad news */

   kvar = -1 ;                                   /* find symbol */
   for( ii=0 ; ii < 26 ; ii++ ){
      sym[0] = 'A' + ii ; sym[1] = '\0' ;
      if( PARSER_has_symbol(sym,pcode) ){ kvar = ii ; break ; }
   }

   for( ii=0 ; ii < 26 ; ii++ ) atoz[ii] = 0.0 ; /* initialize */

   if( kvar >= 0 ){                              /* the normal case */
      for( ii=0 ; ii < nt ; ii++ ){
         atoz[kvar] = tz + ii*dt ;
         vec[ii]    = PARSER_evaluate_one( pcode , atoz ) ;
      }
   } else {                                      /* no variable found! */
      vec[0] = PARSER_evaluate_one( pcode , atoz ) ;
      for( ii=1 ; ii < nt ; ii++ ) vec[ii] = vec[0] ;
   }

   free(pcode) ; return 1 ;
}

/*------------------------------------------------------------------------*/
/*! Sort of like strtod(), but with arithmetic -- 03 Sep 2002 - RWCox.
--------------------------------------------------------------------------*/

double PARSER_strtod( char *expr )
{
   PARSER_code * pcode = NULL ;
   double atoz[26] , val ;
   int ii ;

   if( expr == NULL ) return 0 ;                 /* bad */

   pcode = PARSER_generate_code( expr ) ;        /* compile */
   if( pcode == NULL ) return 0 ;                /* bad news */

   for( ii=0 ; ii < 26 ; ii++ ) atoz[ii] = 0.0 ; /* initialize */

   val = PARSER_evaluate_one( pcode , atoz ) ;

   free(pcode) ; return val ;
}

/********************************************************************/
/*** use the C math library to provide Bessel and error functions ***/

doublereal dbesj0_( doublereal * x )
{ return (doublereal) j0( (double) *x ) ; }

doublereal dbesj1_( doublereal * x )
{ return (doublereal) j1( (double) *x ) ; }

doublereal dbesy0_( doublereal * x )
{ return (doublereal) (*x>0) ? y0( (double) *x ) : 0.0 ; }

doublereal dbesy1_( doublereal * x )
{ return (doublereal) (*x>0) ? y1( (double) *x ) : 0.0 ; }

doublereal derf_ ( doublereal * x )
{ return (doublereal) erf( (double) *x ) ; }

doublereal derfc_( doublereal * x )
{ return (doublereal) erfc( (double) *x ) ; }

#include <time.h>

doublereal unif_( doublereal * x )  /* 04 Feb 2000 */
{
   static first=1 ;
   doublereal val ;
   if( first ){ srand48((long)time(NULL)); first=0; }
   val = (doublereal) drand48() ;
   return val ;
}

/********************************************************************/
/*** Legendre Polynomials (0 <= mm <= 20 ; -1 <= xx < 1)          ***/

doublereal legendre_( doublereal *mm , doublereal *xx )
{
   double x = *xx ;
   int    m = (int)(*mm) ;

   if( m < 0 ) return 1.0 ;    /* bad input */

   switch( m ){
    case 0: return 1.0 ;
    case 1: return x ;
    case 2: return (3.0*x*x-1.0)/2.0 ;
    case 3: return (5.0*x*x-3.0)*x/2.0 ;
    case 4: return ((35.0*x*x-30.0)*x*x+3.0)/8.0 ;
    case 5: return ((63.0*x*x-70.0)*x*x+15.0)*x/8.0 ;
    case 6: return (((231.0*x*x-315.0)*x*x+105.0)*x*x-5.0)/16.0 ;
    case 7: return (((429.0*x*x-693.0)*x*x+315.0)*x*x-35.0)*x/16.0 ;
    case 8: return ((((6435.0*x*x-12012.0)*x*x+6930.0)*x*x-1260.0)*x*x+35.0)/128.0;

           /** 07 Feb 2005: this part generated by Maple, then hand massaged **/

    case 9:
      return (0.24609375e1 + (-0.3609375e2 + (0.140765625e3 + (-0.20109375e3
              + 0.949609375e2 * x * x) * x * x) * x * x) * x * x) * x;

    case 10:
      return -0.24609375e0 + (0.1353515625e2 + (-0.1173046875e3 +
              (0.3519140625e3 + (-0.42732421875e3 + 0.18042578125e3 * x * x)
             * x * x) * x * x) * x * x) * x * x;

    case 11:
      return (-0.270703125e1 + (0.5865234375e2 + (-0.3519140625e3 +
             (0.8546484375e3 + (-0.90212890625e3 + 0.34444921875e3 * x * x)
             * x * x) * x * x) * x * x) * x * x) * x;

    case 12:
      return 0.2255859375e0 + (-0.17595703125e2 + (0.2199462890625e3 +
             (-0.99708984375e3 + (0.20297900390625e4 + (-0.1894470703125e4
             + 0.6601943359375e3 * x * x) * x * x) * x * x) * x * x) * x * x)
             * x * x;

    case 13:
      return (0.29326171875e1 + (-0.87978515625e2 + (0.7478173828125e3 +
             (-0.270638671875e4 + (0.47361767578125e4 + (-0.3961166015625e4
             + 0.12696044921875e4 * x * x) * x * x) * x * x) * x * x) * x * x)
            * x * x) * x;

    case 14:
      return -0.20947265625e0 + (0.2199462890625e2 + (-0.37390869140625e3 +
             (0.236808837890625e4 + (-0.710426513671875e4 +
             (0.1089320654296875e5 + (-0.825242919921875e4 +
            0.244852294921875e4 * x * x) * x * x) * x * x) * x * x) * x * x)
           * x * x) * x * x;

    case 15:
      return (-0.314208984375e1 + (0.12463623046875e3 + (-0.142085302734375e4
            + (0.710426513671875e4 + (-0.1815534423828125e5 +
              (0.2475728759765625e5 + (-0.1713966064453125e5 +
               0.473381103515625e4 * x * x) * x * x) * x * x) * x * x)
             * x * x) * x * x) * x * x) * x;

    case 16:
      return 0.196380615234375e0 + (-0.26707763671875e2 + (0.5920220947265625e3
            + (-0.4972985595703125e4 + (0.2042476226806641e5 +
              (-0.4538836059570312e5 + (0.5570389709472656e5 +
               (-0.3550358276367188e5 + 0.9171758880615234e4 * x * x) * x * x)
            * x * x) * x * x) * x * x) * x * x) * x * x) * x * x;

    case 17:
      return (0.3338470458984375e1 + (-0.169149169921875e3 +
             (0.2486492797851562e4 + (-0.1633980981445312e5 +
             (0.5673545074462891e5 + (-0.1114077941894531e6 +
             (0.1242625396728516e6 + (-0.7337407104492188e5 +
              0.1780400253295898e5 * x * x) * x * x) * x * x) * x * x)
           * x * x) * x * x) * x * x) * x * x) * x;

    case 18:
      return -0.1854705810546875e0 + (0.3171546936035156e2 +
            (-0.8880331420898438e3 + (0.9531555725097656e4 +
            (-0.5106190567016602e5 + (0.153185717010498e6 +
            (-0.2692355026245117e6 + (0.275152766418457e6 +
            (-0.1513340215301514e6 + 0.3461889381408691e5 * x * x) * x * x)
           * x * x) * x * x) * x * x) * x * x) * x * x) * x * x) * x * x;

    case 19:
      return (-0.3523941040039062e1 + (0.2220082855224609e3 +
             (-0.4084952453613281e4 + (0.3404127044677734e5 +
             (-0.153185717010498e6 + (0.4038532539367676e6 +
             (-0.6420231216430664e6 + (0.6053360861206055e6 +
             (-0.3115700443267822e6 + 0.6741574058532715e5 * x * x) * x * x)
          * x * x) * x * x) * x * x) * x * x) * x * x) * x * x) * x * x) * x;

    case 20:
      return 0.1761970520019531e0 + (-0.3700138092041016e2 +
            (0.127654764175415e4 + (-0.1702063522338867e5 +
            (0.1148892877578735e6 + (-0.4442385793304443e6 +
            (0.1043287572669983e7 + (-0.1513340215301514e7 +
            (0.1324172688388824e7 + (-0.6404495355606079e6 +
             0.1314606941413879e6 * x * x) * x * x) * x * x) * x * x) * x * x)
            * x * x) * x * x) * x * x) * x * x) * x * x;
   }

	/** if here, m > 20 ==> use recurrence relation **/

   { double k , pk, pkm1, pkm2 ;

     k = 19.0; pkm2 = legendre_( &k , xx ) ;
     k = 20.0; pkm1 = legendre_( &k , xx ) ;

     while( k < m ){
       k += 1.0 ;
       pk = ((2.0*k-1.0)*x*pkm1 - (k-1.0)*pkm2)/k ;
       pkm2 = pkm1 ; pkm1 = pk ;
     }
     return pk ;
   }
}

/**** statistic conversion routines ****/

/*--- macros to create functions ---*/

#undef FUNC3
#undef FUNC2
#undef FUNC1
#undef FUNC0

#define FUNC4(fname,fcode,sfunc)                                      \
   doublereal fname( doublereal * x, doublereal * a, doublereal * b,  \
                                                     doublereal * c ) \
   {  float aux[3] , xx , val ;                                       \
      xx     = (float) (*x) ;                                         \
      aux[0] = (float) (*a) ;                                         \
      aux[1] = (float) (*b) ;                                         \
      aux[2] = (float) (*c) ;                                         \
      val = sfunc( xx , fcode , aux ) ;                               \
      return (doublereal) val ;                                       \
   }

#define FUNC3(fname,fcode,sfunc)                                      \
   doublereal fname( doublereal * x, doublereal * a, doublereal * b ) \
   {  float aux[2] , xx , val ;                                       \
      xx     = (float) (*x) ;                                         \
      aux[0] = (float) (*a) ;                                         \
      aux[1] = (float) (*b) ;                                         \
      val = sfunc( xx , fcode , aux ) ;                               \
      return (doublereal) val ;                                       \
   }

#define FUNC2(fname,fcode,sfunc)                       \
   doublereal fname( doublereal * x , doublereal * a ) \
   {  float aux[1] , xx , val ;                        \
      xx     = (float) (*x) ;                          \
      aux[0] = (float) (*a) ;                          \
      val = sfunc( xx , fcode , aux ) ;                \
      return (doublereal) val ;                        \
   }

#define FUNC1(fname,fcode,sfunc)         \
   doublereal fname( doublereal * x )    \
   {  float  xx , val ;                  \
      xx     = (float) (*x) ;            \
      val = sfunc( xx , fcode , NULL ) ; \
      return (doublereal) val ;          \
   }

#define FUNC_COR_TYPE   2
#define FUNC_TT_TYPE    3
#define FUNC_FT_TYPE    4
#define FUNC_ZT_TYPE    5
#define FUNC_CT_TYPE    6
#define FUNC_BT_TYPE    7
#define FUNC_BN_TYPE    8
#define FUNC_GT_TYPE    9
#define FUNC_PT_TYPE   10

extern float THD_stat_to_pval(float,int,float *) ;
extern float THD_pval_to_stat(float,int,float *) ;
extern float THD_stat_to_zscore(float,int,float *) ;

/*--- now create the functions ---*/

FUNC4(ficotp_,FUNC_COR_TYPE,THD_stat_to_pval)
FUNC4(ficopt_,FUNC_COR_TYPE,THD_pval_to_stat)
FUNC4(ficotz_,FUNC_COR_TYPE,THD_stat_to_zscore)

FUNC2(fitttp_,FUNC_TT_TYPE,THD_stat_to_pval)
FUNC2(fittpt_,FUNC_TT_TYPE,THD_pval_to_stat)
FUNC2(fitttz_,FUNC_TT_TYPE,THD_stat_to_zscore)

FUNC3(fifttp_,FUNC_FT_TYPE,THD_stat_to_pval)
FUNC3(fiftpt_,FUNC_FT_TYPE,THD_pval_to_stat)
FUNC3(fifttz_,FUNC_FT_TYPE,THD_stat_to_zscore)

FUNC1(fizttp_,FUNC_ZT_TYPE,THD_stat_to_pval)
FUNC1(fiztpt_,FUNC_ZT_TYPE,THD_pval_to_stat)
FUNC1(fizttz_,FUNC_ZT_TYPE,THD_stat_to_zscore)

FUNC2(ficttp_,FUNC_CT_TYPE,THD_stat_to_pval)
FUNC2(fictpt_,FUNC_CT_TYPE,THD_pval_to_stat)
FUNC2(ficttz_,FUNC_CT_TYPE,THD_stat_to_zscore)

FUNC3(fibttp_,FUNC_BT_TYPE,THD_stat_to_pval)
FUNC3(fibtpt_,FUNC_BT_TYPE,THD_pval_to_stat)
FUNC3(fibttz_,FUNC_BT_TYPE,THD_stat_to_zscore)

FUNC3(fibntp_,FUNC_BN_TYPE,THD_stat_to_pval)
FUNC3(fibnpt_,FUNC_BN_TYPE,THD_pval_to_stat)
FUNC3(fibntz_,FUNC_BN_TYPE,THD_stat_to_zscore)

FUNC3(figttp_,FUNC_GT_TYPE,THD_stat_to_pval)
FUNC3(figtpt_,FUNC_GT_TYPE,THD_pval_to_stat)
FUNC3(figttz_,FUNC_GT_TYPE,THD_stat_to_zscore)

FUNC2(fipttp_,FUNC_PT_TYPE,THD_stat_to_pval)
FUNC2(fiptpt_,FUNC_PT_TYPE,THD_pval_to_stat)
FUNC2(fipttz_,FUNC_PT_TYPE,THD_stat_to_zscore)
