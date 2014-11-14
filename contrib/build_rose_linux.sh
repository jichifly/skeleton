#!/bin/bash
# build_rose_linux.sh
# 5/25/2011
ME="`basename "$0"`"

usage()
{
  >&2 cat <<EOF
  usage: $ME [options] ROSE_SRC_path

Build rose on linux

options:
  --prefix=PATH     prefix of rose installation, default is \$HOME/opt/rose
  --boost=PATH      prefix of boost 1.4.7, default is \$HOME/opt/boost
  --boost=JAVA      prefix of jdk 1.6, default is \$HOME/opt/java
  --help            print this help

environments:
- TAR: default tar command to use

external dependences:
- macports: libtool
- apple gcc 4.4
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

ROSE_SRC=$HOME/src/rose
for i in "$@"; do
  case "$i" in
    --prefix=*) ROSE_PREFIX=`echo $i | sed 's/[-a-zA-Z0-9]*=//'` ;;
    --boost=*) BOOST_HOME=`echo $i | sed 's/[-a-zA-Z0-9]*=//'` ;;
    --java=*) JAVA_HOME=`echo $i | sed 's/[-a-zA-Z0-9]*=//'` ;;
    --help) usage; die ;;
    *) ROSE_SRC=$i
  esac
done

#if [ ! -e "$ROSE_SRC" ]; then
#  usage
#  die "Rose source directory does not exist"
#fi

test -z "$ROSE_SRC" && ROSE_SRC=$HOME/work/rose
test -z "$ROSE_PREFIX" && ROSE_PREFIX=$HOME/opt/rose

## Setup Environment

export CC=gcc-4.4
export CXX=g++-4.4

#ROSE_SRC=$HOME/work/rose
#ROSE_build=$ROSE_SRC/build.linux
ROSE_build=$ROSE_SRC/build.linux
#QROSE_PREFIX=$HOME/opt/qrose

#MACPORTS_HOME=/opt/local
test -z "$BOOST_HOME" && BOOST_HOME=$HOME/opt/boost-1.4.7
#QT_HOME=/nfs/apps/qt/4.5.1
export GFORTRAN_PATH=/usr/bin/gfortran-4.4

#MPI_HOME=$HOME/opt/mpich2
#MPI_HOME=$MACPORTS_HOME
MPI_HOME=/usr
export MPICC=$MPI_HOME/bin/mpicc
export MPICXX=$MPI_HOME/bin/mpicxx
export MPIF77=$MPI_HOME/bin/mpif77
export MPIFC=$MPI_HOME/bin/mpif90

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
#JDK_FRAMEWORK=/System/Library/Java/JavaVirtualMachines/1.6.0.jdk
#export JAVA_HOME=$JDK_FRAMEWORK/Contents/Home

#JVM_LIB=/usr/lib/jvm/java-6-oracle/jre/lib/amd64
#JAVA_HOME=/usr
test -z "$JAVA_HOME" && JAVA_HOME=$HOME/opt/java
JVM_LIB=$HOME/opt/java/jre/lib/amd64

export PATH=\
$JAVA_HOME/bin:\
/usr/bin:\
/bin

#export CCACHE_DIR=$HOME/tmp/rose
#test -e "$CCACHE_DIR" || mkdir -p "$CCACHE_DIR"
ccache -M 10G

export DYLD_LIBRARY_PATH=\
$BOOST_HOME/lib:\
$JVM_LIB:$JVM_LIB/server
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
require automake aclocal autoconf perl python gcc ccache libtool
require $GFORTRAN_PATH java $MPICC $MPICXX $MPIF90 $MPIFC

assert_dir()
{
  test -d "$1" -a -x "$1" || die "$ME: ERROR: cannot access directory: $1"
}
assert_dir "$ROSE_SRC"
test -d "$ROSE_build" || mkdir -p "$ROSE_build"
assert_dir "$ROSE_build"

## Build and Install

CONFIG=
CONFIG="$CONFIG --prefix=$ROSE_PREFIX"
CONFIG="$CONFIG --with-boost=$BOOST_HOME"
#CONFIG="$CONFIG --with-mpi"    # Maybe, it is this causing the issue?
#CONFIG="$CONFIG --with-QRose=$QROSE_PREFIX"
#CONFIG="$CONFIG --with-qt=$QT_HOME"

#CONFIG="$CONFIG --disable-fortran --disable-java"   # no fortran support
CONFIG="$CONFIG --with-gfortran=$GFORTRAN_PATH"
CONFIG="$CONFIG --with-java=$JAVA_HOME"

#CONFIG="$CONFIG --with-dwarf=$DWARF_HOME"           # enable libdwarf
CONFIG="$CONFIG --disable-tests-directory"          # test disabled, which would fail on mac when DWARF is enabled
CONFIG="$CONFIG --disable-tutorial-directory"       # no rose tutorial
CONFIG="$CONFIG --disable-projects-directory"       # no rose projects
CONFIG="$CONFIG --disable-binary-analysis"
CONFIG="$CONFIG --disable-ltdl-install"
CONFIG="$CONFIG --disable-opencl"
CONFIG="$CONFIG --disable-cuda"
CONFIG="$CONFIG --disable-php"
CONFIG="$CONFIG --without-haskell"

pushd "$ROSE_SRC" && \
  echo === build === && \
  ./build \
  && \
popd && pushd "$ROSE_build" && \
  echo === configure === && \
  "$ROSE_SRC"/configure $CONFIG \
  --with-alternate_backend_C_compiler=$CC CC="$CC" \
  --with-alternate_backend_Cxx_compiler=$CXX CXX="$CXX" \
  && \
popd && pushd "$ROSE_build" && \
  echo === make === && \
  make -j 2 && \
  echo === make install === && \
  make install && \
popd && pushd "$ROSE_build/tests/translatorTests" && \
  echo === make install roseCompiler === && \
  make install roseCompiler

#make check
# --with-alternate_backend_C_compiler=$CC CC="ccache $CC"
# --with-alternate_backend_Cxx_compiler=$CXX CXX="ccache $CXX"

# EOF
