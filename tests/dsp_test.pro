# Standalone DSP test harness for FMIT (fork issue #3).
# Builds the Music DSP library against a small CLI driver, no GUI/OpenGL.
#
# Usage (Windows / MinGW):
#   qmake "FFT_LIBDIR=F:\FMIT-Error\lib\fftw-3.3.5-dll64" dsp_test.pro
#   mingw32-make -f Makefile.Release
# Usage (Linux):
#   qmake dsp_test.pro && make

QT += core
QT -= gui

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = dsp_test

INCLUDEPATH += ../libs

# FFTW (mirrors fmit.pro)
win32 {
    isEmpty(FFT_LIBDIR){
        contains(QT_ARCH, x86_64): FFT_LIBDIR = "$$_PRO_FILE_PWD_/../../lib/fftw-3.3.5-dll64"
        else:                      FFT_LIBDIR = "$$_PRO_FILE_PWD_/../../lib/fftw-3.3.5-dll32"
    }
    INCLUDEPATH += $$FFT_LIBDIR
    LIBS += -L$$FFT_LIBDIR
    msvc: LIBS += $$FFT_LIBDIR/libfftw3-3.lib
    gcc:  LIBS += -lfftw3-3
}
unix {
    !isEmpty(FFT_LIBDIR){
        INCLUDEPATH += $$FFT_LIBDIR/include
        LIBS += -L$$FFT_LIBDIR/lib
    }
    LIBS += -lfftw3
}

SOURCES += dsp_test.cpp \
    ../libs/Music/Algorithm.cpp \
    ../libs/Music/Autocorrelation.cpp \
    ../libs/Music/CFFTW3.cpp \
    ../libs/Music/CombedFT.cpp \
    ../libs/Music/Convolution.cpp \
    ../libs/Music/CumulativeDiff.cpp \
    ../libs/Music/CumulativeDiffAlgo.cpp \
    ../libs/Music/Filter.cpp \
    ../libs/Music/FreqAnalysis.cpp \
    ../libs/Music/LPC.cpp \
    ../libs/Music/MultiCumulativeDiffAlgo.cpp \
    ../libs/Music/Music.cpp \
    ../libs/Music/Note.cpp \
    ../libs/Music/SPWindow.cpp \
    ../libs/Music/TimeAnalysis.cpp \
    ../libs/CppAddons/CAMath.cpp
