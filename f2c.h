// The is a wrapper around the canonical f2c.h. It has been introduced since
// upstream AFNI carries a full and modified copy of F2C that also modifies
// this essential header file. This wrapper header carries the important diff
// between AFNI's version and the most recent F2C upstream.
//
// Please note the AFNI has another copy of f2c.h in eispack/ -- that has been
// overwritten with a copy of this file.
#ifndef F2C_INCLUDE_DEBIANFIX
#define F2C_INCLUDE_DEBIANFIX
#ifndef F2C_INCLUDE
#define NON_UNIX_STDIO     /* RWCox */
#define complex complexxx  /* RWCox */
#include </usr/include/f2c.h>
#include <math.h>
#endif
#endif
