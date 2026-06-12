// Copyright 2004 "Gilles Degottex"

// This file is part of "fmit"

// "fmit" is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// "fmit" is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#include <stdlib.h>
#include <iostream>
#include <cstdlib>
using namespace std;
#include <signal.h>
#include <qapplication.h>
// #include <qtextcodec.h>
#include <qtranslator.h>
#include <QLibraryInfo>
#include <QPainter>
#include <QPixmap>
#include <QFont>

#include "CppAddons/CAMath.h"

#include "qthelper.h"

#include "CustomInstrumentTunerForm.h"
CustomInstrumentTunerForm* g_main_form = NULL;


QString g_version;
QString FMITVersion(){
    if(!g_version.isEmpty())
        return g_version;

    QString fmitbranchgit(STR(FMITBRANCHGIT));

    QString	fmitversion(STR(FMITVERSION));
    if(!fmitbranchgit.isEmpty() && fmitbranchgit!="master") {
        fmitversion += "-" + fmitbranchgit;
    }
    g_version = fmitversion;

    return g_version;
}

int main(int argc, char** argv)
{
    std::cout << "Free Music Instrument Tuner - Errorbuild (Version " << FMITVersion().toLatin1().constData() << ")" << std::endl;

    QString fmitprefix(STR(PREFIX));

    QApplication a(argc, argv, true);
    QApplication::setQuitOnLastWindowClosed(true);
    QCoreApplication::setOrganizationName("FMIT");
    QCoreApplication::setOrganizationDomain("gillesdegottex.eu");
    QCoreApplication::setApplicationName("FMIT");
    QCoreApplication::setApplicationVersion(FMITVersion());
    // Errorbuild: badge the window/taskbar icon with a coloured "DEV" marker so
    // this build is unmistakable next to a stock FMIT running alongside it.
    QPixmap devIcon = QIcon(":/fmit/ui/images/fmit.svg").pixmap(64, 64);
    {
        QPainter painter(&devIcon);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRect badge(2, 40, 60, 22);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xE8, 0x4A, 0x1A));   // distinctive orange-red
        painter.drawRoundedRect(badge, 5, 5);
        QFont devFont = painter.font();
        devFont.setBold(true);
        devFont.setPixelSize(15);
        painter.setFont(devFont);
        painter.setPen(Qt::white);
        painter.drawText(badge, Qt::AlignCenter, "DEV");
    }
    a.setWindowIcon(QIcon(devIcon));

    // Load translation
    QTranslator qtTranslator;
    std::cout << "INFO: QLocale::system()=" << QLocale::system().name() << std::endl;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QString qtTrPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
    const QString qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
    std::cout << "INFO: Qt translations path=" << qtTrPath.toLatin1().constData() << std::endl;
    QString trFile = "qt_" + QLocale::system().name();
    if (!qtTranslator.load(trFile, qtTrPath)) {
        cout << "ERROR: Failed to load Qt translation file: " << trFile.toLatin1().constData() << " in " << qtTrPath.toLatin1().constData() << endl;
    }
    a.installTranslator(&qtTranslator);
    QTranslator fmitTranslator;
    trFile = QString("fmit_")+QLocale::system().name();

    #ifdef Q_OS_WIN32
        QString trPath = QCoreApplication::applicationDirPath()+"/";
    #else
        QString trPath = fmitprefix + QString("/share/fmit/translations");
    #endif
    cout << "INFO: Loading FMIT translation directory: " << trFile.toLatin1().constData() << " in " << trPath.toLatin1().constData() << endl;
    if (!fmitTranslator.load(trFile, trPath)) {
        cout << "ERROR: Failed to load FMIT translation file: " << trFile.toLatin1().constData() << " in " << trPath.toLatin1().constData() << endl;
    }
    a.installTranslator(&fmitTranslator);

    g_main_form = new CustomInstrumentTunerForm();
	g_main_form->show();
	a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
	a.exec();

	delete g_main_form;

	return 0;
}

