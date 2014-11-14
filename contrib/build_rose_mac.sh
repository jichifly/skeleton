#!/bin/sh
# build_rose_mac.sh
# 5/25/2011
ME="`basename "$0"`"

usage()
{
  >&2 cat <<EOF
usage: $ME [options] rose_src_path

Build rose on mac.

options:
  --prefix=PATH     prefix of rose installation, default is \$HOME/opt/rose
  --boost=PATH      prefix of boost 1.4.7, default is \$HOME/opt/boost
  --help            print this help

environments:
- TAR: default tar command to use

Note for Mac OS X 10.8
- /usr/share/aclocal must exist
- modify config/determine-os.m4 to add darwin12
- modify config/support-java.m4 remove javaconfig

external dependences:
- macports: libtool
- apple gcc 4.2
- boost 1.4.7
- gfortran
- ccache
- mpich2

environments
- GFORTRAN_PATH     optional, absolute path to the gfortran binary
EOF
}

die()
{
  >&2 echo "$*"
  exit -1
}

for i in "$@"; do
  case "$i" in
    --prefix=*) ROSE_prefix=`echo $i | sed 's/[-a-zA-Z0-9]*=//'` ;;
    --boost=*) BOOST_HOME=`echo $i | sed 's/[-a-zA-Z0-9]*=//'` ;;
    --help) usage; die ;;
    *) ROSE_src=$i
  esac
done

#if [ ! -e "$ROSE_src" ]; then
#  usage
#  die "Rose source directory does not exist"
#fi

test -z "$ROSE_src" && ROSE_src=$HOME/work/rose
test -z "$ROSE_prefix" && ROSE_prefix=$HOME/opt/rose

## Setup Environment

#ROSE_src=$HOME/work/rose
ROSE_build=$ROSE_src/build.mac
#QROSE_prefix=$HOME/opt/qrose

MACPORTS_HOME=/opt/local
test -z "$BOOST_HOME" && BOOST_HOME=$HOME/opt/boost-1.47
#QT_HOME=/nfs/apps/qt/4.5.1
export GFORTRAN_PATH=$MACPORTS_HOME/bin/gfortran-mp-4.8

#MPI_HOME=$MACPORTS_HOME
#MPI_HOME=$HOME/opt/mpich2
MPI_HOME=$MACPORTS_HOME
#export MPICC=$MPI_HOME/bin/mpicc
export MPICXX=$MPI_HOME/bin/mpicxx
export MPIF77=$MPI_HOME/bin/mpif77
export MPIFC=$MPI_HOME/bin/mpif90

# The default /bin/sh does not work any more
#SH=$MACPORTS_HOME/bin/tcsh
#SH=/bin/sh

# See: config/support-dwarf.m4
#DWARF_HOME=$MACPORTS_HOME
#DWARF_INCLUDE=$DWARF_HOME/include/elftoolchain
#DWARF_LIB=$DWARF_HOME/lib/elftoolchain
#
#export LDFLAGS=-L$DWARF_LIB
#export CFLAGS=-I$DWARF_INCLUDE
#export CXXFLAGS=$CFLAGS
#export CPPFLAGS=$CFLAGS

#export JAVA_HOME=/Library/Java/JavaVirtualMachines/1.7.0.jdk/Contents/Home
JDK_FRAMEWORK=/System/Library/Java/JavaVirtualMachines/1.6.0.jdk
export JAVA_HOME=$JDK_FRAMEWORK/Contents/Home

# Use native automake on mac
export PATH=\
$MACPORTS_HOME/bin:\
/usr/bin:\
/bin

#export CCACHE_DIR=$HOME/tmp/rose
#test -e "$CCACHE_DIR" || mkdir -p "$CCACHE_DIR"
ccache -M 10G

export DYLD_LIBRARY_PATH=\
$BOOST_HOME/lib:\
$JDK_FRAMEWORK/Contents/Libraries:$JAVA_HOME/lib
#$JAVA_HOME/lib:$JAVA_HOME/jre/lib:$JAVA_HOME/jre/lib/server
#$QT_HOME/lib:

export LD_LIBRARY_PATH=$DYLD_LIBRARY_PATH

## Check Requirements

require()
{
  for i in "$@"; do
    >/dev/null which "$i" || {
    die "$ME: ERROR: cannot find '$i' in PATH, ABORT"
  }
  done
}
require automake aclocal autoconf perl python gcc ccache libtool glibtool $SH
require gfortran java #$MPICC $MPICXX $MPIF90 $MPIFC

assert_dir()
{
  test -d "$1" -a -x "$1" || die "$ME: ERROR: cannot access directory: $1"
}
assert_dir "$ROSE_src"
test -d "$ROSE_build" || mkdir -p "$ROSE_build"
assert_dir "$ROSE_build"

## Build and Install

CONFIG=
CONFIG="$CONFIG --prefix=$ROSE_prefix"
CONFIG="$CONFIG --with-boost=$BOOST_HOME"
#CONFIG="$CONFIG --with-mpi"
#CONFIG="$CONFIG --with-QRose=$QROSE_prefix"
#CONFIG="$CONFIG --with-qt=$QT_HOME"

#CONFIG="$CONFIG --disable-fortran --disable-java"   # no fortran support
CONFIG="$CONFIG --with-gfortran=$GFORTRAN_PATH"
CONFIG="$CONFIG --with-java=$JAVA_HOME"

#CONFIG="$CONFIG --with-dwarf=$DWARF_HOME"          # enable libdwarf
CONFIG="$CONFIG --disable-tests-directory"          # test disabled, which would fail on mac when DWARF is enabled
CONFIG="$CONFIG --disable-projects-directory"       # no rose projects
CONFIG="$CONFIG --disable-tutorial-directory"       # no rose tutorial
CONFIG="$CONFIG --disable-binary-analysis"
CONFIG="$CONFIG --disable-ltdl-install"
CONFIG="$CONFIG --disable-opencl"
CONFIG="$CONFIG --disable-cuda"
CONFIG="$CONFIG --disable-php"
CONFIG="$CONFIG --without-haskell"

#pushd "$ROSE_src" && \
#  echo === build === && \
#  ./build \
#  && \
#popd && pushd "$ROSE_build" && \
#  echo === configure === && \
#  "$ROSE_src"/configure $CONFIG \
#  --with-alternate_backend_C_compiler=gcc CC="ccache gcc" \
#  --with-alternate_backend_Cxx_compiler=g++ CXX="ccache g++" \
#  && \
#popd &&
pushd "$ROSE_build" && \
  echo === make === && \
  make -j4 && \
  echo === make install === && \
  make install && \
popd && pushd "$ROSE_build/tests/translatorTests" && \
  echo === make install roseCompiler === && \
  make install roseCompiler
#make check

# EOF
#sed "s|/bin/sh|$SH|g" "$ROSE_src"/configure > "$ROSE_src"/configure.tmp && \
#mv "$ROSE_src"/configure "$ROSE_src"/configure.bak && \
#mv "$ROSE_src"/configure.tmp "$ROSE_src"/configure && \
#chmod +x "$ROSE_src"/configure && \
