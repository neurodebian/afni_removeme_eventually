#!/bin/bash
# Repackage AFNI upstream sources. Removes unnecessary 3rd party software and
# non-free code and optionally import into a git directory.
#
# The script also determines a version of the respective AFNI source snapshot.
# This is done by using the latest modification date of any file in the tarball.
# Reason: AFNI's canonical version in AFNI_label.h does not change with
# modifications done to the code. In fact already published release tarballs are
# regularly changed later on -- which makes it impossible to judge which version
# one is using.
#
# Usage:
#   afni_upstream2debian.sh <tarball> [gitdir]
#
#   Where `tarball` can be a downloaded tarball, or '-' to let the script
#   download the latest AFNI snapshot.
#
#   `gitdir` can be any git repository (the GITDIT and _not_ the worktree!!).
#   If `gitdir` is specified the content of the tarball is commited to the
#   current branch, storing the original filename (and MD5), as well as AFNI's
#   version from the version header. If no `gitdir` is given then the tarball
#   gets repackaged and a suitable .orig,tar.gz is placed into the current
#   working directory.
#
# Examples:
#
#   Repacking a tarball (yields corresponding .orig.tar.gz)
#
#     afni_upstream2debian.sh AFNI_2005_10_31_151-.tgz
#
#   Downloading the latest AFNI snapshot and repackage it
#
#     afni_upstream2debian.sh -
#
#   Import the latest sources into a git repository
#
#     afni_upstream2debian.sh - somegit/
#
set -e

tarball=$1
gitdir=$2

if [ -z "$tarball" ]; then
  echo "No tarball specified. Exit."
  exit 1
fi

if [ -n "$gitdir" ]; then
  echo "Got Git repository -- trying to import later on."
  gitdir=$(readlink -f ${gitdir})
fi

# make working directory
curdir=$(pwd)
wdir=$(mktemp -d)

# if '-' as tarball, download the latest snapshot
if [ "$tarball" = "-" ]; then
	echo "Downloading latest AFNI snapshot"
	wget -q http://afni.nimh.nih.gov/pub/dist/tgz/AFNI_latest.tgz
	tarball="AFNI_latest.tgz"
fi

# put upstream source tarball into working dir
tarball_path=$(readlink -f ${tarball})
tarball_md5=$(md5sum ${tarball_path} | cut -d ' ' -f 1,1)
cd $wdir
ln -s $tarball_path

# unpack the source tarball
echo -n "Unpacking sources"
tar xfz $tarball_path

spath=$(find . -mindepth 1 -maxdepth 1 -type d)
echo " extracted into '$(readlink -f ${spath})'"

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
afni_reported_version=$(egrep "#define AFNI_VERSION_LABEL" ${spath}/AFNI_label.h | cut -d '"' -f 2,2 | awk -F '_' '{ print "0." $2 $3 $4 "." $5 }')
# Upstream does not update its own version string ...
# therefore we are going to use the latest modification date of any file in the
# sources
afni_version=$(LC_ALL=C find . -type f -exec ls -og --time-style=long-iso {} \; |sort --key 4 |tail -n1 | cut -d ' ' -f 4,4 | tr -d "-")
echo ${afni_version}


echo -n "Debian repackaging version: "
afni_rpversion="0.$afni_version~dfsg.1"
echo ${afni_rpversion}


# importing sources into git
if [ -n "${gitdir}" ]; then
	git_cmd="git --git-dir=${gitdir} --work-tree=${spath}"
	# and commit it
	${git_cmd} add .
	# we don't care if there is nothing to commit
	echo -e "Importing AFNI ${afni_rpversion}\nImported from tarball: $(basename ${tarball_path})\nOriginal MD5: ${tarball_md5}\nAFNI claimed to be version: ${afni_reported_version}" | \
	${git_cmd} commit -a -F - || true

	upstream_archive="$(dirname ${tarball_path})/afni-${afni_version}.tgz"
	# look into this dir and into a potential 'tarballs' dir
	if [ ! -f ${upstream_archive} -a ! -f ${curdir}/tarballs/$(basename ${upstream_archive}) ]; then
		echo "Storing upstream tarball as ${upstream_archive}"
		cp -a ${tarball_path} ${upstream_archive}
	fi
else
	echo "Append version to source directory names"
	mv ${spath} afni-$afni_rpversion

	echo "Compress repackaged tarball"
	tar czf afni_$afni_rpversion.orig.tar.gz afni-$afni_rpversion

	echo "Copy tarballs to final destination"
	mv afni_$afni_rpversion.orig.tar.gz $curdir
fi


echo "Clean working directory"
rm -rf $wdir


echo "Done"

exit 0

