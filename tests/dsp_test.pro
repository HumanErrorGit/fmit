# Standalone DSP test harness for FMIT (fork issue #3).
# Builds the Music DSP library against a small CLI driver, no GUI/OpenGL.
#
# Usage (Windows / MSVC):
#   qmake "FFT_LIBDIR=F:/vcpkg/installed/x64-windows" dsp_test.pro
#   nmake -f Makefile.Release
#   (jom does NOT work here -- fails with a false "dependent does not exist"
#    error on Qt 6.8.3 qmake-generated Makefiles; nmake is what was verified)
# Usage (Linux):
#   qmake6 dsp_test.pro && make

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
    message(FFT_LIBDIR=$$FFT_LIBDIR)
    INCLUDEPATH += $$FFT_LIBDIR/include
    LIBS += -L$$FFT_LIBDIR/lib
    msvc: LIBS += $$FFT_LIBDIR/lib/fftw3.lib
    gcc: LIBS += -lfftw3-3
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
