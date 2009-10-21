/*----------------------------------------------------------------------
 * 06 Feb 2008: top-level routines to read GIFTI datasets        [rickr]
 *
 * These main routines should match those in gifti_choice.c.
 *
 * THD_3dim_dataset * THD_open_gifti (char *fname)
 * THD_3dim_dataset * THD_load_gifti (THD_datablock *dblk)
 * Boolean            THD_write_gifti( THD_3dim_dataset *dset, 
                                       int write_data, int forcencode)
 *
 * NI_group         * NI_read_gifti (char *fname, int read_data)
 * int                NI_write_gifti(NI_group *ngr, 
                                     char *fname, int forcencode)
 *----------------------------------------------------------------------
 */

#include "mrilib.h"
#include "gifti_io.h"


typedef struct {        /* put this in thd_gifti.h ? */
    int add_index_list; /* if no NIFTI_INTENT_NODE_LIST, add a default */
    int write_mode;     /* BINARY (0) or ASCII (1) */
    int g_encoding;     /* gifti encoding to use on write */
    int gverb;
    int verb;
} gifti_globs_t;

gifti_globs_t gifti_globs = { 0, NI_BINARY_MODE, 1, 1 };
static gifti_globs_t * GP = &gifti_globs; /* for ease of access */

/*--- read gifti routines ---*/
static NI_group * gifti_to_NSD(gifti_image * gim, int copy_data);

static int add_string_attribute(NI_group * ngr, char * aname, char * value);
static int append_string(char ** dest, int * alen, char * src, char * sep);
static int disp_gifti_globs(char * mesg, gifti_globs_t * g);
static int get_meta_stat_codes( nvpairs *nvp, float *p1, float *p2, float *p3);
static int gifti_globs_from_env(void);
static int gifti_has_NSD_form(gifti_image * gim, int whine);
static int append_vals(char ** str, int * len, char * sep,
                       float f1, float f2, int i1, int i2);
static int nifti_get_min_max_posn(void * vdata, int nitype, int len,
                        double * dmin, int * imin, double * dmax, int * imax);
static int gnsd_add_sparse_data(NI_group *ngr, gifti_image *gim, int add_data);
static int nsd_add_gifti_colms_range(NI_group * ngr, gifti_image * gim);
static int nsd_add_gifti_colms_type(NI_group * ngr, gifti_image * gim);
static int nsd_add_gifti_index_list(NI_group * ngr, gifti_image * gim);
static int nsd_add_gifti_stat_codes(NI_group * ngr, gifti_image * gim);
static int nsd_are_sorted_ints(int * list, int len);

static char * gifti_DA_meta_concat(gifti_image * gim, char * name,
                                   char * def, char * sep);
static char * nifti2suma_typestring(int niftitype);

/*--- write gifti routines ---*/
static int clear_gifti_pointers(gifti_image * gim);
static int nsdg_add_data(NI_element * sdel, gifti_image * gim);
static int nsdg_add_index_list(NI_group *ngr, gifti_image *gim);
static int nsdg_labs_to_meta(NI_group * ngr, gifti_image * gim);
static int nsdg_set_history(NI_group * ngr, gifti_image * gim);
static int nsdg_stat_to_intent(NI_group * ngr, gifti_image * gim);
static int set_gifti_encoding(int encoding);

static gifti_image * NSD_to_gifti(NI_group * ngr, char * fname);

/*
 * - sub-brick selection in NI_read_gifti()?
 *
 */



/* ------------------------------- AFNI ------------------------------- */

THD_3dim_dataset * THD_open_gifti(char * fname)
{
    THD_3dim_dataset * dset;
    NI_group         * ngr;

    ENTRY("THD_open_gifti");

    ngr = NI_read_gifti(fname, 1);

    if( !ngr ) RETURN(NULL);

    dset = THD_ni_surf_dset_to_afni(ngr, 0);

    NI_free_element(ngr);

    if( dset ) {
        char * pp = THD_trailname(fname, 0);
        EDIT_dset_items(dset, ADN_prefix, pp, ADN_none);
        NI_strncpy(dset->dblk->diskptr->brick_name, fname, THD_MAX_NAME);
        THD_set_storage_mode(dset, STORAGE_BY_GIFTI);
    }

    RETURN(dset);
}

/* presumably we've already whined, via 'open' */
int THD_load_gifti(THD_datablock * dblk)
{
    NI_group * ngr;
    char     * fname;
    int        rv;

    ENTRY("THD_load_gifti");

    if( !dblk || !dblk->diskptr || !dblk->diskptr->brick_name ) RETURN(1);

    fname = dblk->diskptr->brick_name;

    ngr = NI_read_gifti(fname, 1);
    if( !ngr ) {
        fprintf(stderr,"** failed to load GIFTI dset '%s'\n", fname);
        RETURN(1);
    } else if( GP->verb > 2 )
        fprintf(stderr,"-- have NI_group NSD, adding sparse data...\n");

    rv = THD_add_sparse_data(dblk->parent, ngr);
    NI_free_element(ngr);
    if( rv <= 0 ){
        fprintf(stderr,"** add sdata returned %d for '%s'\n",rv,fname);
        RETURN(1);
    }
    else if( rv < dblk->nvals ){
        fprintf(stderr,"** loaded only %d vols for '%s'\n",rv,fname);
        RETURN(1);
    } else if (GP->verb > 2)
        fprintf(stderr,"++ THD_load_gifti succeeded, adding %d columns\n", rv);

    RETURN(0);
}

Boolean THD_write_gifti(THD_3dim_dataset * dset, int write_data, int forcencode)
{
    NI_group * ngr;
    char     * prefix;
    int        rv;

    ENTRY("THD_write_gifti");

    gifti_globs_from_env();     /* for thd_gifti */
    set_ni_globs_from_env();    /* for thd_niml */

    prefix = DSET_PREFIX(dset);

    if( !prefix ) {
        if( GP->verb ) fprintf(stderr,"** THD_write_gifti: no dset prefix\n");
        RETURN(False);
    }

    if( GP->verb > 1 )
        fprintf(stderr,"++ THD_write_gifti: converting '%s' to NSD\n", prefix);

    ngr = THD_dset_to_ni_surf_dset(dset, write_data);
    if( !ngr ) {
        fprintf(stderr,"** failed dset to NSD for '%s'\n", prefix);
        RETURN(False);
    }

    rv = NI_write_gifti(ngr, prefix, forcencode);

    NI_free_element(ngr);

    if( rv ) RETURN(False);
    else     RETURN(True);
}

/* ------------------------------- NIML ------------------------------- */

/* read GIFTI and convert to NI_SURF_DSET */
NI_group * NI_read_gifti(char * fname, int read_data)
{
    NI_group    * ngr;
    gifti_image * gim;

    ENTRY("NI_read_gifti");

    gifti_globs_from_env();     /* for thd_gifti */
    set_ni_globs_from_env();    /* for thd_niml */

    if( !fname ) {
        if( GP->verb > 0 ) fprintf(stderr,"** NI_read_gifti: null filename\n");
        RETURN(NULL);
    }

    if( GP->verb > 2 ) fprintf(stderr,"-- NI_read_gifti from '%s'\n", fname );

    gifti_set_verb(GP->gverb);

    gim = gifti_read_image(fname, read_data);
    if( !gim ) {
        if( GP->verb > 1 )
            fprintf(stderr,"-- NI_read_gifti: failed to read '%s'\n", fname);
        RETURN(NULL);
    }

    /* set a filename attribute */
    if( !gifti_get_meta_value(&gim->meta, "filename") )
        gifti_add_to_nvpairs(&gim->meta, "filename", fname);

    ngr = gifti_to_NSD(gim, read_data);

    if( GP->verb > 2 && ngr ) {
        fprintf(stderr,"++ writing NRG.test.niml.dset for test...\n");
        write_niml_file("NRG.test.niml.dset", ngr);
    }

    if( GP->verb > 1 )
        fprintf(stderr,"++ NI_read_gifti from %s (with%s data) %s\n",
                fname, read_data ? "" : "out", ngr ? "succeeded" : "failed");

    if( !ngr ) gifti_free_image(gim);

    RETURN(ngr);
}

/* write the NSD dataset to a GIFTI file
 *
 * if add_index:  add a NIFTI_INTENT_NODE_INDEX DataArray
 */
int NI_write_gifti(NI_group * ngr, char * fname, int forcencode)
{
    gifti_image * gim;
    int           rv;

    ENTRY("NI_write_gifti");

    gifti_globs_from_env();     /* for thd_gifti (maybe done already) */
    gifti_set_verb(GP->gverb);

    if( !ngr || !fname ) {
        fprintf(stderr,"** NI_write_nifti: bad params\n");
        RETURN(1);
    } else if( NI_element_type(ngr) != NI_GROUP_TYPE ) {
        fprintf(stderr,"** NI_write_nifti: ngr is not NI_GROUP_TYPE\n");
        RETURN(1);
    }

    if(GP->verb > 2) fprintf(stderr,"-- NI_write_gifti file %s ...\n", fname);

    set_gifti_encoding(forcencode);
    gim = NSD_to_gifti(ngr, fname);

    if( !gim ) {
        if(GP->verb) fprintf(stderr,"** failed NSD_to_gifti for '%s'\n",fname);
        RETURN(1);
    } else if( GP->verb > 1 )
        fprintf(stderr,"++ have gifti from NSD, writing image to '%s'\n",fname);

    rv = gifti_write_image(gim, fname, 1);
    set_gifti_encoding(GIFTI_ENCODING_UNDEF);
    
    if( GP->verb > 2 )
        fprintf(stderr,"-- gifti_write_image complete, freeing gim...\n");

    clear_gifti_pointers(gim);  /* since they were stolen from ngr */

    gifti_free_image(gim);

    RETURN(rv);
}

/* set all non-NODE_INDEX pointers to NULL,
 * since they were stolen from the NI_group */
static int clear_gifti_pointers(gifti_image * gim)
{
    int c;
    ENTRY("clear_gifti_pointers");
    for( c = 0; c < gim->numDA; c++ )
        if( gim->darray[c]->intent != NIFTI_INTENT_NODE_INDEX )
            gim->darray[c]->data = NULL;
    RETURN(0);
}

/* convert between dataset types: NI_SURF_DSET to GIFTI */
static gifti_image * NSD_to_gifti(NI_group * ngr, char * fname)
{
    gifti_image * gim;
    NI_element  * sdel = NULL;
    void       ** elist = NULL;
    char        * rhs, * id = NULL, * timestr;
    int           numDA, intent, dtype, ndim, dims[GIFTI_DARRAY_DIM_LEN] = {0};
    int           ind, rv;

    ENTRY("NSD_to_gifti");

    /* get dimensions and such to create basic gifti dataset */
    ind = NI_search_group_shallow(ngr, "SPARSE_DATA", &elist);
    if(ind > 0){ sdel = (NI_element *)elist[0]; NI_free(elist); elist = NULL; }
    
    if( !sdel || sdel->vec_num <= 0 || sdel->vec_len <= 0 ) {
        if( GP->verb ) fprintf(stderr,"** NSD_to_gifti: missing SPARSE_DATA\n");
        RETURN(NULL);
    }
    /* ZSS: ni_timestep should be in seconds.
    Before Sep. 18/09 it may have been saved incorrectly in msec */
    timestr = NI_get_attribute(sdel, "ni_timestep");

    /* set basic gifti attributes */
    numDA   = sdel->vec_num;
    intent  = (timestr && *timestr) ? NIFTI_INTENT_TIME_SERIES
                                    : NIFTI_INTENT_NONE;
    dtype   = dtype_niml_to_nifti(sdel->vec_typ[0]);
    ndim    = 1;
    dims[0] = sdel->vec_len;

    /* last check before we start trashing the place */
    if( dtype == 0 ) {
        if( GP->verb > 0 )
            fprintf(stderr,"** NSD2Gii: bad NI_type %d\n", sdel->vec_typ[0]);
        RETURN(NULL);
    }

    if( GP->verb > 2 ) fprintf(stderr,"++ creating gifti_image...\n");

    /*--- create an initial gifti_image ---*/
    gim = gifti_create_image(numDA, intent, dtype, ndim, dims, 0);
    if( !gim ) {
        fprintf(stderr,"** NSD2Gii: failed gifti_create_image\n");
        RETURN(NULL);
    }

    /*--- start populating the image, first with metadata ---*/

    /* add UniqueID MD */
    if( GP->verb > 4 ) fprintf(stderr,"++ adding idcode...\n");
    rhs = NI_get_attribute(ngr, "self_idcode");
    if( !rhs ) NI_get_attribute(ngr, "ni_idcode");
    if( !rhs ) { id = UNIQ_idcode(); rhs = id; }
    gifti_add_to_meta(&gim->meta, "UniqueID", rhs, 1);
    if( id ) free(id);

    /* maybe set the AFNI_Timestep */
    if( timestr && *timestr )
        gifti_add_to_meta(&gim->meta, "AFNI_Timestep", timestr, 1);

    /* range is not used, type was set from SPARSE_DATA */

    rv = nsdg_labs_to_meta(ngr, gim);
    if( !rv ) rv = nsdg_stat_to_intent(ngr, gim);
    if( !rv ) rv = nsdg_set_history(ngr, gim);
    if( !rv ) rv = nsdg_add_data(sdel, gim);
    if( !rv ) rv = nsdg_add_index_list(ngr, gim);

    /* on failure, nuke image */
    if( rv ) { gifti_free_image(gim); RETURN(NULL); }

    RETURN(gim);
}

/* add data to the gifti_image (do not allocate data, just steal pointers) */
static int nsdg_add_data(NI_element * sdel, gifti_image * gim)
{
    giiDataArray * da;
    int            c;

    ENTRY("nsdg_add_data");

    if( GP->verb > 1 ) fprintf(stderr, "++ adding data to gim ...\n");

    /* walk through the list and steal pointers */
    for( c = 0; c < gim->numDA; c++ ) {
        da = gim->darray[c];
        da->data = sdel->vec[c];

        /* set encoding via set_gifti_encoding or A_N_TEXT_DATA or default */
        if( GP->g_encoding )
            da->encoding = GP->g_encoding;
        else if ( GP->write_mode == NI_TEXT_MODE )
            da->encoding = GIFTI_ENCODING_ASCII;
    }

    if( GP->verb > 1 )
      fprintf(stderr,"-- setting encoding to %s\n",
        gifti_list_index2string(gifti_encoding_list, gim->darray[0]->encoding));

    RETURN(0);
}

/* add an INDEX list to the gifti_image
 * (data is allocated, so we must free() later) */
static int nsdg_add_index_list(NI_group *ngr, gifti_image *gim)
{
    giiDataArray  * da;
    NI_element    * nel = NULL;
    void         ** elist = NULL;
    char          * rhs;
    int           * data;  /* new index_list */
    int             ind, len = 0, sorted = 0, def = 0, make_list = 0;

    ENTRY("nsdg_add_index_list");

    if( GP->verb > 4 ) fprintf(stderr,"++ nsdg_add_index_list ...\n");

    ind = NI_search_group_shallow(ngr, "INDEX_LIST", &elist);
    if(ind > 0){ nel = (NI_element *)elist[0]; NI_free(elist); elist = NULL; }
    if( !nel || nel->vec_num <= 0 || nel->vec_len <= 0 ) {
        if( GP->add_index_list ) {
            if(GP->verb>1)
                fprintf(stderr,"-- NSD_to_gifti: no INDEX_LIST, creating...\n");
            make_list = 1;
        } else {
            if(GP->verb>1)
                fprintf(stderr,"-- NSD_to_gifti: no INDEX_LIST, skipping...\n");
            RETURN(0);
        }
    }

    if( make_list) {    /* create a default list */
        len = (int)gim->darray[0]->nvals;
        data = (int *)malloc(len * sizeof(int));
        if( !data ){
            fprintf(stderr,"** failed to alloc IND_LIST, len %d\n",len);
            RETURN(1);
        }
        for( ind = 0; ind < len; ind++ )
            data[ind] = ind;
    } else {
        if( nel->vec_typ[0] != NI_INT ) {
            if(GP->verb>1) fprintf(stderr,"** bad type in INDEX_LIST\n");
            RETURN(1);
        }

        len = nel->vec_len;
        if( len <= 0 ) {
            if( GP->verb > 1 ) fprintf(stderr,"** bad vec_len in INDEX_LIST\n");
            RETURN(1);
        }

        rhs = NI_get_attribute(nel, "sorted_node_def");
        if( rhs )  sorted = *rhs == 'Y' || *rhs == 'y';

        /* see if it is just 1..N */
        if( sorted && len == (int)gim->darray[0]->nvals ) {
            int * ip = nel->vec[0];
            for( ind = 0; ind < len; ind++ )
                if( ip[ind] != ind ) break;
            if( ind == len ) def = 1;
            else             def = 0;
        }

        if( GP->verb > 2 )
            fprintf(stderr,"-- INDEX_LIST len=%d, sorted=%d, default=%d\n",
                    len, sorted, def);

        /* if it is a default and wasn't requested, skip it */
        if( def && ! GP->add_index_list ) {
            if(GP->verb>1) fprintf(stderr,"-- skipping default INDEX_LIST\n");
            RETURN(0);
        }

        /* have 'len', set 'data' */
        data = (int *)malloc(len * sizeof(int));
        if( !data ){
            fprintf(stderr,"** failed to alloc IND_LIST, len %d\n",len);
            RETURN(1);
        }
        memcpy(data, nel->vec[0], len*sizeof(int));
    }

    /* okay, let's actually add the list as NIFTI_INTENT_NODE_INDEX */
    if( GP->verb > 3 ) fprintf(stderr,"++ inserting NODE_INDEX element...\n");

    if( gifti_add_empty_darray(gim, 1) ) RETURN(1);

    da = gim->darray[gim->numDA-1];

    gifti_set_DA_defaults(da);
    da->intent   = NIFTI_INTENT_NODE_INDEX;
    da->datatype = NIFTI_TYPE_INT32;
    da->num_dim  = 1;
    da->dims[0]  = len;
    da->nvals    = len;
    if( GP->g_encoding )
            da->encoding = GP->g_encoding;
    else if ( GP->write_mode == NI_TEXT_MODE )
            da->encoding = GIFTI_ENCODING_ASCII;

    /* set the pointer */
    da->data = data;

    RETURN(0);
}

/* convert any HISTORY_NOTE to gim->meta:History */
static int nsdg_set_history(NI_group * ngr, gifti_image * gim)
{
    NI_element * nel;
    char       * hstr;

    ENTRY("nsdg_set_history");

    if( GP->verb > 4 ) fprintf(stderr,"++ nsdg_set_history...\n");

    nel = NI_find_element_by_aname(ngr,"AFNI_atr","atr_name","HISTORY_NOTE");
    if( !nel ) {
        if( GP->verb > 2 )fprintf(stderr,"-- no history to copy\n");
        RETURN(0);
    }

    hstr = ((char **)nel->vec[0])[0];  /* data is encoded stat string */
    if( GP->verb > 3 )
        fprintf(stderr,"++ G_set_history: hstr = '%s'\n", hstr);

    gifti_add_to_meta(&gim->meta, "AFNI_History", hstr, 1);

    RETURN(0);
}

/* convert any COLMS_LABS to meta */
static int nsdg_labs_to_meta(NI_group * ngr, gifti_image * gim)
{
    ATR_string    atr_str;      /* fill with element values */
    NI_element  * nel;
    char       ** sar, * labstr;
    int           np, ind;

    ENTRY("nsdg_labs_to_meta");

    if( GP->verb > 4 ) fprintf(stderr,"++ nsdg_labs_to_meta...\n");

    nel = NI_find_element_by_aname(ngr,"AFNI_atr","atr_name","COLMS_LABS");
    if( !nel ) {
        if( GP->verb > 2 )fprintf(stderr,"-- no colms_labs for labels\n");
        RETURN(0);
    }

    labstr = ((char **)nel->vec[0])[0];  /* data is encoded stat string */
    if( GP->verb > 3 )
        fprintf(stderr,"++ G_labs_to_meta: labstr = '%s'\n", labstr);

    /* put into ATR_string format, which has no extra allocation */
    atr_str.type = ATR_STRING_TYPE;
    atr_str.name = nel->name;
    atr_str.nch  = strlen(labstr);
    atr_str.ch   = labstr;

    /* rip it into pieces */
    np = nsd_string_atr_to_slist(&sar, gim->numDA, &atr_str);
    if( !sar || np <= 0 ) {
        if( GP->verb > 3 )
            fprintf(stderr,"** have COLMS_LABS, but it is empty\n");
        RETURN(0);
    } else if ( np != gim->numDA && GP->verb > 1 ) /* just warn user */
        fprintf(stderr,"** NSA2S returned only %d of %d ptrs\n",np,gim->numDA);

    /* apply any labels that are not "none" */
    for( ind = 0; ind < gim->numDA; ind++ ) {
        if( sar[ind][0] && strcmp(sar[ind], "none") )
            gifti_add_to_meta(&gim->darray[ind]->meta, "Name", sar[ind], 1);
        free(sar[ind]);
    }
    free(sar);

    RETURN(0);
}

/* convert any COLMS_STATSYM to intent and parms */
static int nsdg_stat_to_intent(NI_group * ngr, gifti_image * gim)
{
    ATR_string    atr_str;      /* fill with element values */
    NI_element  * nel;
    char       ** sar, * statstr;
    char          istr[10] = "intent_p1";         /* update for pX */
    float         parm[3];
    int           scode, np, ind, c;

    ENTRY("nsdg_stat_to_intent");

    if( GP->verb > 4 ) fprintf(stderr,"++ nsdg_stat_to_intent...\n");

    nel = NI_find_element_by_aname(ngr,"AFNI_atr","atr_name","COLMS_STATSYM");
    if( !nel )
        nel=NI_find_element_by_aname(ngr,"AFNI_atr","atr_name","BRICK_STATSYM");
    if( !nel ) {
        if( GP->verb > 2 )fprintf(stderr,"-- no colms_statsym for intents\n");
        RETURN(0);
    }

    statstr = ((char **)nel->vec[0])[0];  /* data is encoded stat string */
    if( GP->verb > 3 )
        fprintf(stderr,"++ G_stat_to_intent: statstr = '%s'\n", statstr);

    /* put into ATR_string format, which has no extra allocation */
    atr_str.type = ATR_STRING_TYPE;
    atr_str.name = nel->name;
    atr_str.nch  = strlen(statstr);
    atr_str.ch   = statstr;

    /* rip it into pieces */
    np = nsd_string_atr_to_slist(&sar, gim->numDA, &atr_str);
    if( !sar || np <= 0 ) {
        if( GP->verb > 3 )
            fprintf(stderr,"** have STATSYM, but it is empty\n");
        RETURN(0);
    } else if ( np != gim->numDA && GP->verb > 0 ) /* just warn user */
        fprintf(stderr,"** NSA2S returned only %d of %d ptrs\n",np,gim->numDA);

    /* apply any valid codes */
    for( ind = 0; ind < gim->numDA; ind++ ) {
        NI_stat_decode(sar[ind], &scode, parm, parm+1, parm+2);
        if( scode >= AFNI_FIRST_STATCODE && scode <= AFNI_LAST_STATCODE ) {
            np = NI_stat_numparam(scode);
            gim->darray[ind]->intent = scode;
            for( c = 0; c < np; c++ ) {
                istr[8] = '1' + c;
                gifti_add_to_meta(&gim->darray[ind]->meta,
                                  istr, MV_format_fval(parm[c]), 1);
            }
        }
        free(sar[ind]);
    }
    free(sar);

    RETURN(0);
}

/* convert between dataset types: GIFTI to NI_SURF_DSET */
/* note: we throw away the gifti data as it is copied   */
static NI_group * gifti_to_NSD(gifti_image * gim, int copy_data)
{
    NI_group * ngr;
    char     * cp, * id = NULL;

    ENTRY("gifti_to_NSD");

    if( !gim ){
        fprintf(stderr,"** gifti_to_NSD: to gifti_image\n");
        RETURN(NULL);
    }

    if( ! gifti_has_NSD_form(gim, GP->verb > 0) ) RETURN(NULL);

    ngr = NI_new_group_element();
    NI_rename_group(ngr, "AFNI_dataset");
    NI_set_attribute(ngr, "dset_type", "Node_Bucket");

    /* get or create an ID code */
    cp = gifti_get_meta_value(&gim->meta, "UniqueID");
    if( !cp ){ id = UNIQ_idcode(); cp = id; }
    NI_set_attribute(ngr, "self_idcode", cp);
    if( id ) free(id);

    cp = gifti_get_meta_value(&gim->meta, "filename");
    if( cp ) NI_set_attribute(ngr, "filename", cp);

    /* add COLMS_LABS from "Name" MetaData values */
    cp = gifti_DA_meta_concat(gim, "Name", "none", ";");
    add_string_attribute(ngr, "COLMS_LABS", cp);
    if( cp ) free(cp);  /* data has been copied */

    nsd_add_gifti_colms_range(ngr, gim);
    nsd_add_gifti_colms_type(ngr, gim);
    nsd_add_gifti_stat_codes(ngr, gim);

    cp = gifti_get_meta_value(&gim->meta, "AFNI_History");
    if( cp ) add_string_attribute(ngr, "HISTORY_NOTE", cp);

    nsd_add_gifti_index_list(ngr, gim);

    gnsd_add_sparse_data(ngr, gim, copy_data);

    RETURN(ngr);
}

/* ------------------------- static functions ------------------------ */

/* add a SPARSE_DATA element from the gifti_image (skip INTENT_NODE_INDEX) */
/* note: delete gifti data as it is copied                                 */
static int gnsd_add_sparse_data(NI_group * ngr, gifti_image * gim, int add_data)
{
    NI_element   * nel;
    giiDataArray * da;
    char         * str;
    int            c, length, nnew, ni_type;

    ENTRY("gnsd_add_sparse_data");

    if( !ngr || !gim || !gim->darray ) RETURN(1);

    if( !gifti_image_has_data(gim) ) {
        if(GP->verb>0) fprintf(stderr,"** gnsd_add_sparse_data: no data\n");
        RETURN(1);
    }

    if( GP->verb > 2 ) fprintf(stderr,"++ NSD: adding SPARSE_DATA element\n");

    length = gim->darray[0]->dims[0];
    nel = NI_new_data_element("SPARSE_DATA", length);

    nnew = 0;  /* count new column (non-NODE_INDEX) */
    for( c = 0; add_data && c < gim->numDA; c++ ) {
        da = gim->darray[c];
        if( da->intent == NIFTI_INTENT_NODE_INDEX ) {
            if( GP->verb > 3 )
                fprintf(stderr,"-- sparse_data: skipping nodes @ ind %d\n",c);
            continue;
        }
        ni_type = dtype_nifti_to_niml(da->datatype);
        if( ni_type < 0 ) {
            fprintf(stderr,"** invalid type for NIML conversion: %d = %s\n",
                    da->datatype, nifti_datatype_string(da->datatype));
            NI_free_element(nel);
            RETURN(1);
        }

        if( GP->verb > 4 )
            fprintf(stderr,"++ adding col, type %d (from %d = %s)\n",
                    ni_type, da->datatype, nifti_datatype_string(da->datatype));

        NI_add_column(nel, ni_type, da->data);  /* finally, the data */
        nnew++;

        /* and nuke it from the gifti dataset */
        free(da->data);  da->data = NULL;
    }

    NI_set_attribute(nel, "data_type", "Node_Bucket_data");
    nel->outmode = GP->write_mode;
    NI_set_attribute(nel, "ni_form", /* set the endian (data already swapped) */
                     gifti_get_this_endian()==GIFTI_ENDIAN_LITTLE ?
                         "lsbfirst" : "msbfirst");

    /* if there is a Timestep, pass it along */
    str = gifti_get_meta_value(&gim->meta, "AFNI_Timestep");
    if( str ) NI_set_attribute(nel, "ni_timestep", str);

    NI_add_to_group(ngr, nel);

    if( GP->verb > 2 )
        fprintf(stderr,"++ NSD: added %d columns in SPARSE_DATA\n", nnew);

    RETURN(0);
}

/* add an INDEX_LIST attribute element from the gifti_image */
static int nsd_add_gifti_index_list(NI_group * ngr, gifti_image * gim)
{
    NI_element   * nel;
    giiDataArray * da;
    int          * node_list, * new_list = NULL;
    int            c, length;

    ENTRY("nsd_add_gifti_index_list");

    if( !ngr || !gim || !gim->darray ) RETURN(1);

    da = gifti_find_DA(gim, NIFTI_INTENT_NODE_INDEX, 0);
    if( GP->verb > 2 )
        fprintf(stderr,"-- add_index_list: %s N_INTENT_NODE_INDEX\n",
                da ? "found" : "did not find");

    if( !da && !GP->add_index_list ) RETURN(0); /* none to add */

    if( !da ) {  /* then generate a default list */
        da = gim->darray[0];  /* base length on first DA (should all match) */
        if( !da ) {
            fprintf(stderr,"** cannot make index list from empty DA\n");
            RETURN(1);
        } else if (da->nvals > MRI_maxint) {
            fprintf(stderr,"** DA nvals exceeds MRI_maxint, bailing\n");
            RETURN(1);
        }
        length = (int)da->dims[0];  /* checked in has_NSD_form */
        new_list = (int *)malloc(length * sizeof(int));
        if( !new_list ) {
            fprintf(stderr,"** failed to alloc len %d index list\n", length);
            RETURN(1);
        }
        for( c = 0; c < length; c++ )  /* fill with default indices */
            new_list[c] = c;
        node_list = new_list;
    } else {
        if( da->datatype != NIFTI_TYPE_INT32 ) {
            fprintf(stderr, "** NODE_INDEX datatype is not INT32\n");
            RETURN(1);
        }

        node_list = da->data;
        length    = da->dims[0];
        if( !node_list ) {
            fprintf(stderr,"** NODE_INDEX missing data\n");
            RETURN(1);
        }
    }

    /* have node_list and length now, create the NI_element and insert it */

    if( GP->verb > 2 ) fprintf(stderr,"++ adding INDEX_LIST, len %d\n", length);

    nel = NI_new_data_element("INDEX_LIST", length);
    nel->outmode = GP->write_mode;  /* ASCII or BINARY */

    /* note whether the list is sorted */
    if( nsd_are_sorted_ints(node_list, length) )
        NI_set_attribute(nel, "sorted_node_def", "Yes");
    else
        NI_set_attribute(nel, "sorted_node_def", "No");

    /* insert the data */
    NI_add_column(nel, NI_INT, node_list);

    if( new_list ) free(new_list);

    NI_add_to_group(ngr, nel);

    RETURN(0);
}

/* add a COLMS_TYPE attribute element from the gifti_image */
static int nsd_add_gifti_stat_codes(NI_group * ngr, gifti_image * gim)
{
    giiDataArray * da;
    float          p1, p2, p3;
    char         * str, * statstr = NULL;
    int            c, statlen = 0, ncodes;

    ENTRY("nsd_add_gifti_stat_codes");

    if( !gim || !gim->darray || !ngr ) RETURN(1);

    /* first, compute the needed space */
    for( c = 0; c < gim->numDA; c++ ) {
        da = gim->darray[c];
        if(da->intent >= NI_STAT_FIRSTCODE && da->intent <= NI_STAT_LASTCODE) {
            p1 = p2 = p3 = 0.0; /* init */
            ncodes = get_meta_stat_codes(&da->meta, &p1, &p2, &p3);
            str = NI_stat_encode(da->intent, p1, p2, p3);
            if( str ) append_string(&statstr, &statlen, str, ";");
            else      append_string(&statstr, &statlen, "none", ";");
            if( GP->verb > 5 )
                fprintf(stderr,"++ add_stat_codes, found %d: %lf, %lf, %lf\n"
                               "   (stat_encode = '%s')\n",
                        ncodes, p1, p2, p3, str ? str : "NULL");
            if( str ) free(str);
        } else append_string(&statstr, &statlen, "none;", NULL);
    }

    add_string_attribute(ngr, "COLMS_STATSYM", statstr);

    if(GP->verb>3)
        fprintf(stderr,"++ adding COLMS_STATSYM element, '%s'\n",statstr);

    free(statstr);

    RETURN(0);
}

/* find and fill up to 3 'intent_p1' values, return num found */
static int get_meta_stat_codes( nvpairs *nvp, float *p1, float *p2, float *p3)
{
    char * value;

    ENTRY("get_meta_stat_codes");

    if( !nvp || nvp->length <= 0 ) RETURN(0);

    /* just try until failure */

    if( !p1 ) RETURN(0);
    value = gifti_get_meta_value(nvp, "intent_p1");
    if( !value ) RETURN(0);
    *p1 = strtod(value, NULL);  /* got one */
    if( GP->verb > 5 )
        fprintf(stderr,"++ setting intent_p1 to %lf from '%s'\n", *p1, value);
        

    if( !p2 ) RETURN(1);
    value = gifti_get_meta_value(nvp, "intent_p2");
    if( !value ) RETURN(1);
    *p2 = strtod(value, NULL);  /* got one */
    if( GP->verb > 5 )
        fprintf(stderr,"++ setting intent_p2 to %lf from '%s'\n", *p2, value);

    if( !p3 ) RETURN(2);
    value = gifti_get_meta_value(nvp, "intent_p3");
    if( !value ) RETURN(2);
    *p3 = strtod(value, NULL);  /* got one */
    if( GP->verb > 5 )
        fprintf(stderr,"++ setting intent_p3 to %lf from '%s'\n", *p3, value);

    RETURN(3);
}

/* add a COLMS_TYPE attribute element from the gifti_image
 *
 * perhaps our choices are Generic_Int, Short, Float, Byte, Double
 * (no unsigned?)
*/
static int nsd_add_gifti_colms_type(NI_group * ngr, gifti_image * gim)
{
    char * str, * ptr, * typestr;
    int    c, len;

    ENTRY("nsd_add_gifti_colms_type");

    if( !gim || !gim->darray || !ngr ) RETURN(1);

    /* first, compute the needed space */
    len = 0;
    for( c = 0; c < gim->numDA; c++ ) {
        str = nifti2suma_typestring(gim->darray[c]->datatype);
        if( str ) len += strlen(str);
    }
    len += gim->numDA + 1;

    typestr = (char *)malloc(len * sizeof(char));
    if(!typestr) {
        fprintf(stderr,"** NAGCT: failed tlen %d alloc\n", len);
        RETURN(1);
    }

    /* now fill the beast */
    ptr = typestr;
    for(c = 0; c < gim->numDA; c++ ) {
        str = nifti2suma_typestring(gim->darray[c]->datatype);
        if( str ) {
            strcpy(ptr, str);   /* insert string */
            ptr += strlen(str);
            *ptr++ = ';';       /* and add separator */
        }
    }
    *ptr = '\0';

    add_string_attribute(ngr, "COLMS_TYPE", typestr);

    if(GP->verb>3)
        fprintf(stderr,"++ adding COLMS_TYPE element, '%s'\n",typestr);

    free(typestr);

    RETURN(0);
}

/* return the SUMA string corresponding to the NIFTI type */
static char * nifti2suma_typestring(int niftitype)
{
    switch(niftitype) {
        case DT_NONE:            return "Col_Type_Undefined";
        case NIFTI_TYPE_INT8:    return "Generic_Byte";
        case NIFTI_TYPE_INT16:   return "Generic_Short";
        case NIFTI_TYPE_INT32:   return "Generic_Int";
        case NIFTI_TYPE_FLOAT32: return "Generic_Float";
        case NIFTI_TYPE_FLOAT64: return "Generic_Double";
    }

    return NULL;  /* bad idea? */
}

/* add a COLMS_RANGE attribute element from the gifti_image */
static int nsd_add_gifti_colms_range(NI_group * ngr, gifti_image * gim)
{
    giiDataArray * da;
    char         * str;
    double         dmin = 0.0, dmax = 0.0;
    int            minp = 0, maxp = 0;
    int            c, len;

    ENTRY("nsd_add_gifti_colms_range");

    if( !ngr || ! gifti_image_has_data(gim) ) {
        if( GP->verb > 2 ) fprintf(stderr,"-- no data for COLMS_RANGE\n");
        RETURN(0);
    }

    /*-- create initial string --*/
    len = 512;
    str = (char *)malloc(len * sizeof(char));
    if( !str ) {
        fprintf(stderr,"** NADGR: failed allof of len %d string\n", len);
        RETURN(1);
    }
    str[0] = '\0';

    /* fill string with range values */
    for( c = 0; c < gim->numDA; c++ ) {
        da = gim->darray[c];
        nifti_get_min_max_posn(da->data, da->datatype, da->dims[0],
                               &dmin, &minp, &dmax, &maxp);
        if( append_vals(&str, &len, ";", dmin, dmax, minp, maxp) )
            RETURN(1);  /* failure */
    }

    /*-- insert the string as an attribute --*/
    add_string_attribute(ngr, "COLMS_RANGE", str);

    if(GP->verb>3) fprintf(stderr,"++ adding COLMS_RANGE element, '%s'\n",str);

    free(str);

    RETURN(0);
}

/* no reason to retype this each time... */
#undef NOTYPE_GET_MIN_MAX_POSN
#define NOTYPE_GET_MIN_MAX_POSN(data,len,min,minp,max,maxp)             \
        do { int eye;                                                   \
             min=max=data[0];  minp=maxp=0;                             \
             for(eye = 1; eye < len; eye++)                             \
                if(data[eye]<min){ min=data[eye]; minp=eye; }           \
                else if(data[eye]>max){ max=data[eye]; maxp=eye; }      \
        } while (0)

/* get min and max values (as float), along with their indices
   (reqiure NIFTI_TYPE datatype)
*/
static int nifti_get_min_max_posn(void * vdata, int nitype, int len,
                        double * dmin, int * imin, double * dmax, int * imax)
{
    int minp, maxp;

    ENTRY("nifti_get_min_max_posn");

    if( !dmin || !dmax || !imin || !imax ) RETURN(1);
    if( !vdata || len <= 0 ) RETURN(0);

    switch(nitype){
        default: {
            fprintf(stderr,"** GMMP: bad nitype %d\n", nitype);
            *dmin = *dmax = 0.0;  *imin = *imax = 0;
            RETURN(1);
        }
        case NIFTI_TYPE_INT8: {
            char * data = (char *)vdata;
            char   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_UINT8: {
            unsigned char * data = (unsigned char *)vdata;
            unsigned char   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_INT16: {
            short * data = (short *)vdata;
            short   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_UINT16: {
            unsigned short * data = (unsigned short *)vdata;
            unsigned short   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_INT32: {
            int * data = (int *)vdata;
            int   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_UINT32: {
            unsigned int * data = (unsigned int *)vdata;
            unsigned int   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_FLOAT32: {
            float * data = (float *)vdata;
            float   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = (double)min;  *dmax = (double)max;
            *imin = minp;         *imax = maxp;
            break;
        }
        case NIFTI_TYPE_FLOAT64: {
            double * data = (double *)vdata;
            double   min, max;
            NOTYPE_GET_MIN_MAX_POSN(data, len, min, minp, max, maxp);
            *dmin = min;  *dmax = max;
            *imin = minp; *imax = maxp;
            break;
        }
    }

    if( GP->verb > 5 )
        fprintf(stderr,"-- min = %lf (@ %d), max = %lf (@ %d)\n",
                *dmin, *imin, *dmax, *imax);

    RETURN(0);
}

/* append 2 floats and ints to str, subject to total length, len,
   and pre-pended with sep */
static int append_vals(char ** str, int * len, char * sep,
                           float f1, float f2, int i1, int i2)
{
    char lbuf[256], fs1[32];  /* for first call to MV_format_fval */
    int  req;

    ENTRY("append_vals");

    if( !str || !*str || !len || !sep ) RETURN(1);
    if( strlen(sep) > 32 )     RETURN(1);

    /* first, just stuff them in a sufficient buffer */
    /* (make a copy of first float, and then format it all) */
    strcpy(fs1, MV_format_fval(f1));
    sprintf(lbuf, "%s %s %d %d%s", fs1, MV_format_fval(f2), i1, i2, sep);

    req = strlen(*str) + strlen(lbuf) + 1;
    if( req > *len )
    {
        *len = req + 512;       /* include some extra space */
        *str = (char *)realloc(*str, *len * sizeof(char));
        if(!*str) {
            fprintf(stderr,"** LAV: failed realloc of len %d string\n",*len);
            RETURN(1);
        }
    }

    strcat(*str, lbuf); /* finally, copy the data in */

    RETURN(0);
}

/* add a string attribute */
static int add_string_attribute(NI_group * ngr, char * aname, char * value)
{
    NI_element * nel;

    if( !ngr || !aname || !value ) return 1;

    nel = NI_new_data_element("AFNI_atr", 1);
    nel->outmode = NI_TEXT_MODE;
    NI_set_attribute(nel, "atr_name", aname);
    NI_add_column(nel, NI_STRING, &value);
    NI_add_to_group(ngr, nel);

    return 1;
}

/* determine whether this gifti dataset has a form appropriate for a
 * NI_SURF_DSET dataset */
static int gifti_has_NSD_form(gifti_image * gim, int whine)
{
    giiDataArray * da;
    int            c, errs = 0;

    if( !gim ) {
        if( whine ) fprintf(stderr,"bad NSD form: GIFTI image is NULL\n");
        return 0;
    }

    if( ! gim->darray ) {
        if(whine) fprintf(stderr,"bad NSD form: GIFTI image has no darray\n");
        errs++;
    }

    if( gim->numDA <= 0 ) {
        if(whine) fprintf(stderr,"bad NSD form: GIFTI image has no DA elems\n");
        errs++;
    }

    if( errs ) return 0;        /* those are terminal erros */

    /* require darray[c] to exist */
    for( c = 0; c < gim->numDA; c++ ) {
        da = gim->darray[c];
        if( !da ) {
            if( whine ) fprintf(stderr,"** missing darray[%d]\n", c);
            return 0; /* early termination */
        } 

        /* be sure dims are valid and consistent */
        if( !gifti_valid_dims(da, whine) ) errs++;

        /* require 1D style format */
        if( gifti_darray_nvals(da) != da->dims[0] ) {
            if( whine )
                fprintf(stderr,"** darray[%d] has nvals = %lld, d[0] = %d\n",
                        c, gifti_darray_nvals(da), da->dims[0]);
            errs++;
        } 

        /* require ROW_MAJOR order */
        if( da->ind_ord != GIFTI_IND_ORD_ROW_MAJOR ) {
            if( whine )
                fprintf(stderr,"** darray[%d] has ind_ord %d (must be %d)\n",
                        c, da->ind_ord, GIFTI_IND_ORD_ROW_MAJOR);
            errs++;
        }

        /* length of single list cannot exceed 2^31-1 */
        if( da->nvals > (long long)MRI_maxint) {
            if( whine )
                fprintf(stderr,"** darray[%d] has length exceeding maxint\n",c);
                errs++;
        }

        if( errs ) {
            if(whine) fprintf(stderr,"   (terminating after bad DataArray)\n");
            return 0;
        }
    }

    if( GP->verb > 2 )
        fprintf(stderr,"-- gifti_image has valid NSD form\n");

    return 1;
}

/* find all DA meta data of the given name, and concat using separater sep
 *
 * if 'name' is not found, use 'defval'
 * if 'sep' is set, separate values with that string
 */
static char * gifti_DA_meta_concat(gifti_image * gim, char * name,
                                   char * def, char * sep)
{
    char * result, * val;
    int    c, length = 0, deflen = 0, seplen = 0;

    if( !gim || !name || !gim->darray || gim->numDA <= 0 )
        return NULL;

    if( sep ) seplen = strlen(sep);
    if( def ) deflen = strlen(def);

    if( GP->verb > 3 )
        fprintf(stderr,"-- making meta string from '%s', def '%s', sep '%s'\n",
                name, def ? def : "NULL", sep ? sep : "NULL");

    /* first compute the total length */

    for( c = 0; c < gim->numDA; c++ ) {
        if( gim->darray[c] ) {
            val = gifti_get_meta_value(&gim->darray[c]->meta, name);
            if( val ) length += strlen(val);
            else      length += deflen;
        }   else      length += deflen;
        length += seplen;
    }

    if( length <= 0 ) return NULL;

    /* allocate memory and fill */

    length += 1; /* for terminating nul char */

    result = (char *)malloc( length * sizeof(char) );
    if( ! result ) {
        fprintf(stderr,"** GDMC: failed to alloc %d chars\n", length);
        return NULL;
    }

    /* now fill the result with data that the length was computed from */

    *result = '\0'; /* now we can use strcat */
    for( c = 0; c < gim->numDA; c++ ) {
        if( gim->darray[c] ) {
            val = gifti_get_meta_value(&gim->darray[c]->meta, name);
            if( val ) strcat(result, val);
            else if( def ) strcat(result, def);
        }   else if( def ) strcat(result, def);

        if( sep ) strcat(result, sep);
    }

    if( GP->verb > 2 )
        fprintf(stderr,"++ made meta string from '%s': '%s'\n", name, result);

    return result;
}

/* be sure there is space for 'src' and 'sep' in 'dest', and insert */
static int append_string(char ** dest, int * alen, char * src, char * sep)
{
    int dlen, slen, seplen, needed, newlen;

    if( !dest || !alen || !src || *alen < 0 ) return 1;

    dlen = (*dest) ? strlen(*dest) : 0;
    slen = strlen(src);
    seplen = (sep) ? strlen(sep) : 0;

    needed = slen + seplen + 1;

    /* do we need more space, captain? */
    if( needed + dlen > *alen ) {
        newlen = *alen + needed + 256;  /* alloc some extra space */
        *dest = (char *)realloc(*dest, newlen * sizeof(char));
        if( !*dest ) {
            fprintf(stderr,"** append_str alloc failed for %d chars\n",*alen);
            return 1;
        }
        if( *alen == 0 ) **dest = '\0';   /* might need initial termination */
        *alen = newlen;
    }

    strcat(*dest, src);
    if(sep) strcat(*dest, sep);

    return 0;
}

static int nsd_are_sorted_ints(int * list, int len)
{
    int c;

ENTRY("are_sorted_ints");

    if( !list || len <= 0 ) RETURN(0);
    for( c = 0; c < len - 1; c++ )
        if( list[c] > list[c+1] )
            RETURN(0);
    RETURN(1);
}

/* allow other functions to control the encoding
 * note: 0 = GIFTI_ENCODING_UNDEF means to clear it */
static int set_gifti_encoding(int encoding)
{
    if( encoding >= GIFTI_ENCODING_UNDEF && encoding <= GIFTI_ENCODING_MAX )
        GP->g_encoding = encoding;
    else
        fprintf(stderr,"** SGE: illegal encoding %d\n", encoding);
    return 0;
}

static int disp_gifti_globs(char * mesg, gifti_globs_t * g)
{
    if( mesg ) fputs(mesg, stderr);

    fprintf(stderr,"gifti_globs_t:\n"
                   "    add_index_list  = %d\n"
                   "    write_mode      = %d\n"
                   "    g_encoding      = %d\n"
                   "    gverb, verb     = %d, %d\n",
                   g->add_index_list, g->write_mode, g->g_encoding,
                   g->gverb, g->verb);
    return 0;
}

static int gifti_globs_from_env(void)
{
    char * ept = NULL;

    ept = my_getenv("AFNI_NIML_DEBUG");
    if( ept ) GP->verb = atoi(ept);       /* adjust if set */
    ept = my_getenv("AFNI_GIFTI_VERB");
    if( ept ) GP->gverb = atoi(ept);     /* adjust if set */

    GP->add_index_list = AFNI_yesenv("AFNI_NSD_ADD_NODES");
    GP->write_mode = AFNI_yesenv("AFNI_NIML_TEXT_DATA") ? NI_TEXT_MODE :
                                                          NI_BINARY_MODE;

    if( GP->verb > 1 ) disp_gifti_globs("gifti_globs_from_env: ", GP);

    return 0;
}
