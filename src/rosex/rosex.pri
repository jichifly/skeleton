# rosex.pri
# 10/16/2011 jichi
# ROSE extension library

DEFINES   += WITH_LIB_ROSEX
DEPENDPATH  += $$PWD

HEADERS   += \
  $$PWD/rose_config.h \
  $$PWD/rosex_p.h \
  $$PWD/rosex.h
SOURCES   += \
  $$PWD/rosex.cc \
  $$PWD/rosex_asm.cc \
  $$PWD/rosex_indent.cc

HEADERS   += \
  $$PWD/dataflowtable.h \
  $$PWD/defusegraph.h \
  $$PWD/depgraph.h \
  $$PWD/depgraph_p.h \
  $$PWD/depgraphbuilder_p.h \
  $$PWD/depgraphnode_p.h \
  $$PWD/depgraphopt_p.h \
  $$PWD/loopdepgraph.h \
  $$PWD/loopdepgraph_p.h \
  $$PWD/slice_p.h \
  $$PWD/symbolic.h \
  $$PWD/tac.h
SOURCES   += \
  $$PWD/dataflowtable.cc \
  $$PWD/defusegraph.cc \
  $$PWD/depgraph.cc \
  $$PWD/depgraph_p.cc \
  $$PWD/depgraphopt_p.cc \
  $$PWD/depgraphbuilder_p.cc \
  $$PWD/depgraphnode_p.cc \
  $$PWD/loopdepgraph.cc \
  $$PWD/slice_p.cc \
  $$PWD/symbolic.cc \
  $$PWD/tac.cc

LIBS    += -lrose
LIBS    += -lboost_regex

# On linux, add missing boost libs required by librose
!mac:unix: LIBS += -lboost_thread -lboost_wave -lboost_program_options #-ljvm

# EOF
