##############################################
# please add new classes in alphabetical order

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(includedir) \
  -I$(OFFLINE_MAIN)/include  \
  -isystem$(ROOTSYS)/include

AM_LDFLAGS = \
  -L$(libdir) \
  -L$(ROOTSYS)/lib \
  -L$(OFFLINE_MAIN)/lib \
  -L$(OFFLINE_MAIN)/lib64

if USE_ONLINE

lib_LTLIBRARIES = \
  libtpc.la

pkginclude_HEADERS = \
  TpcMap.h

libtpc_la_SOURCES = \
  TpcMap.cc

noinst_PROGRAMS = \
  testexternals_tpc

else

lib_LTLIBRARIES = \
  libtpc_io.la \
  libtpc.la

pkginclude_HEADERS = \
  TpcRawDataTree.h \
  TpcClusterCleaner.h \
  TpcClusterizer.h \
  TpcClusterMover.h \
  TpcClusterZCrossingCorrection.h \
  TpcDistortionCorrection.h \
  TpcDistortionCorrectionContainer.h \
  TpcLoadDistortionCorrection.h \
  TpcMap.h \
  TpcRawDataDecoder.h \
  TpcRawWriter.h \
  TpcSimpleClusterizer.h

pcmdir = $(libdir)

# sources for tpc library
libtpc_la_SOURCES = \
  TpcRawDataTree.cc \
  TpcClusterCleaner.cc \
  TpcClusterizer.cc \
  TpcLoadDistortionCorrection.cc \
  TpcMap.cc \
  TpcRawDataDecoder.cc \
  TpcRawWriter.cc \
  TpcSimpleClusterizer.cc

libtpc_la_LIBADD = \
  libtpc_io.la \
  -lmicromegas_io \
  -lfun4all \
  -lSpectrum \
  -lpthread

# sources for io library
libtpc_io_la_SOURCES = \
  $(ROOTDICTS) \
  TpcDistortionCorrection.cc \
  TpcClusterMover.cc \
  TpcClusterZCrossingCorrection.cc

libtpc_io_la_LIBADD = \
  -ltrack_io \
  -lg4detectors_io \
  -ltrackbase_historic_io

noinst_PROGRAMS = \
  testexternals_tpc_io \
  testexternals_tpc

endif

# Rule for generating table CINT dictionaries.
%_Dict.cc: %.h %LinkDef.h
	rootcint -f $@ @CINTDEFS@ $(DEFAULT_INCLUDES) $(AM_CPPFLAGS) $^

#just to get the dependency
%_Dict_rdict.pcm: %_Dict.cc ;

################################################
# linking tests

BUILT_SOURCES = testexternals.cc

testexternals_tpc_io_SOURCES = testexternals.cc
testexternals_tpc_io_LDADD = libtpc_io.la

testexternals_tpc_SOURCES = testexternals.cc
testexternals_tpc_LDADD = libtpc.la

testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

################################################

clean-local:
	rm -f *Dict* *.pcm $(BUILT_SOURCES)
