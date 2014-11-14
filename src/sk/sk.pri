# sk.pri
# 10/16/2011 jichi
# Skeleton AST library

DEFINES  += WITH_LIB_SK
DEPENDPATH += $$PWD

HEADERS  += \
  $$PWD/skbuilder.h \
  $$PWD/skbuilder_p.h \
  $$PWD/skconf.h \
  $$PWD/skdebug.h \
  $$PWD/skdef.h \
  $$PWD/skglobal.h \
  $$PWD/sknode.h \
  $$PWD/skprofile.h \
  $$PWD/skquery.h \
  $$PWD/skvariant.h

SOURCES  += \
  $$PWD/skbuilder_new.cc \
  $$PWD/skbuilder_ref.cc \
  $$PWD/skbuilder_stat.cc \
  $$PWD/skbuilder_unparse.cc \
  $$PWD/skbuilder_vec.cc \
  $$PWD/skbuilder.cc \
  $$PWD/skconf.cc \
  $$PWD/sknode.cc \
  $$PWD/skquery.cc

OTHER_FILES += \
  $$PWD/skconf.conf

# EOF
