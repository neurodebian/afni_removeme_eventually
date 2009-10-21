#!/bin/bash
# Repackage AFNI upstream sources. Removes unnecessary 3rd party software and
# non-free code.
#
# Usage:
#   repack-afni.sh <tarball>
#

set -e

# make working directory
curdir=$(pwd)
wdir=$(mktemp -d)
origsrc=$1

# put upstream source tarball into working dir
origsrc_path=$(readlink -f ${origsrc})
cd $wdir
ln -s $origsrc_path

# unpack the source tarball
echo -n "Unpacking sources"
tar xfz $origsrc_path

spath=$(find . -mindepth 1 -maxdepth 1 -type d)
echo "extracted into '$spath'"

###############
# repackaging #
###############
echo "Repackaging"

echo "Remove unnecessary 3rd-party code"
# gts, GLUT, GLw
rm -rf ${spath}/SUMA/{gts,GLUT,GLw_local}

# libjpeg
rm -rf ${spath}/jpeg-6b

#libnifti
rm -rf ${spath}/nifti

#libvolpack
rm -rf ${spath}/{volpack,volpack.h}

# libqhull
rm -rf ${spath}/qhulldir

# netcdf
rm -rf ${spath}/{netcdf-3.5.0,netcdf.h}

# Apple dlcompat
rm -rf ${spath}/dlcompat

# bzip2
rm -rf ${spath}/bzip2*

# fftw
rm -f ${spath}/fftw.h

# f2c
rm -fr ${spath}/f2cdir
rm -f ${spath}/f2c.h

# gifsicle
rm -rf ${spath}/gifsicle*

echo "Remove problematic code"
# non-commercial license (README also says non-distribution)
rm -rf ${spath}/3DEdge

# non-commercial, non-distribution license
rm -rf ${spath}/svm

# non-free/non-commerical (used to be in Debian, but got removed and is
# upstream dead)
rm -rf ${spath}/mpeg_encodedir


echo -n "Determine AFNI version: "
# Upstream does not update its own version string ...
# afni_version=$(egrep "#define AFNI_VERSION_LABEL" ${spath}/AFNI_label.h | cut -d '"' -f 2,2 | awk -F '_' '{ print "0." $2 $3 $4 "." $5 }')
# therefore we are going to use the latest modification date of any file in the
# sources
afni_version=$(LC_ALL=C find afni_src/ -type f -exec ls -og --time-style=long-iso {} \; |sort --key 4 |tail -n1 | cut -d ' ' -f 4,4 | tr -d "-")
echo ${afni_version}

echo -n "Debian repackaging version: "
afni_rpversion="0.$afni_version~dfsg.1"
echo ${afni_rpversion}

echo "Append version to source directory names"
mv ${spath} afni-$afni_rpversion


echo "Compress repackaged tarball"
tar czf afni_$afni_rpversion.orig.tar.gz afni-$afni_rpversion


echo "Copy tarballs to final destination"
mv afni_$afni_rpversion.orig.tar.gz $curdir


echo "Clean working directory"
rm -rf $wdir


echo "Done"

exit 0

