/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2009-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 EXL <exlmotodev@gmail.com>                         *
 *   Copyright (C) 2018,2020-2023 Aleksey Chernov <valexlin@gmail.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

// coolreader-ng UI / Qt
// main.cpp - entry point

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif
#if QT_VERSION >= 0x050000
#include <QtCore/QTranslator>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDir>
#else
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#endif

#include <crengine.h>
#include <crtxtenc.h>

#include "config.h"
#include "mainwindow.h"
#include "app-props.h"

#if defined(QT_STATIC)
#if QT_VERSION >= 0x050000
#include <QtPlugin>
#if defined(Q_OS_UNIX)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(Q_OS_WIN)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin);
#elif defined(Q_OS_MACOS)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
#endif
#endif
#endif

// Maximum cache directory size
#define DOC_CACHE_SIZE 128 * 1024 * 1024 // 128Mb

// prototypes
bool InitCREngine(const char* exename, lString32Collection& fontDirs);
void ShutdownCREngine();
#if (USE_FREETYPE == 1)
bool getDirectoryFonts(lString32Collection& pathList, lString32Collection& ext, lString32Collection& fonts,
                       bool absPath);
#endif

static void printHelp() {
    printf("usage: crqt [options] [filename]\n"
           "Options:\n"
           "  -h or --help: this message\n"
           "  -v or --version: print program version\n"
           "  --loglevel=ERROR|WARN|INFO|DEBUG|TRACE: set logging level\n"
           "  --logfile=<filename>|stdout|stderr: set log file\n");
}

static void printVersion() {
    printf("cruiqt " VERSION "; crengine-ng-" CRE_NG_VERSION "\n");
}

int main(int argc, char* argv[]) {
    int res = 0;
    {
#ifdef _DEBUG
        lString8 loglevel("TRACE");
        lString8 logfile("stdout");
#else
        lString8 loglevel("ERROR");
        lString8 logfile("stderr");
#endif
        int optpos = 1;
        for (int i = 1; i < argc; i++) {
            if (!strcmp("-h", argv[i]) || !strcmp("-?", argv[i]) || !strcmp("/?", argv[i]) ||
                !strcmp("--help", argv[i])) {
                printHelp();
                return 0;
            }
            if (!strcmp("-v", argv[i]) || !strcmp("/v", argv[i]) || !strcmp("--version", argv[i])) {
                printVersion();
                return 0;
            }
            if (!strcmp("--stats", argv[i]) && i < argc - 4) {
                if (i != argc - 5) {
                    printf("To calculate character encoding statistics, use cruiqt <infile.txt> <outfile.cpp> <codepagename> <langname>\n");
                    return 1;
                }
                lString8 list;
                FILE* out = fopen(argv[i + 2], "wb");
                if (!out) {
                    printf("Cannot create file %s", argv[i + 2]);
                    return 1;
                }
                MakeStatsForFile(argv[i + 1], argv[i + 3], argv[i + 4], 0, out, list);
                fclose(out);
                return 0;
            }
            lString8 s(argv[i]);
            if (s.startsWith(cs8("--loglevel="))) {
                loglevel = s.substr(11, s.length() - 11);
                optpos++;
            } else if (s.startsWith(cs8("--logfile="))) {
                logfile = s.substr(10, s.length() - 10);
                optpos++;
            }
        }

        // set logger
        if (logfile == "stdout")
            CRLog::setStdoutLogger();
        else if (logfile == "stderr")
            CRLog::setStderrLogger();
        else if (!logfile.empty())
            CRLog::setFileLogger(logfile.c_str());
        if (loglevel == "TRACE")
            CRLog::setLogLevel(CRLog::LL_TRACE);
        else if (loglevel == "DEBUG")
            CRLog::setLogLevel(CRLog::LL_DEBUG);
        else if (loglevel == "INFO")
            CRLog::setLogLevel(CRLog::LL_INFO);
        else if (loglevel == "WARN")
            CRLog::setLogLevel(CRLog::LL_WARN);
        else if (loglevel == "ERROR")
            CRLog::setLogLevel(CRLog::LL_ERROR);
        else
            CRLog::setLogLevel(CRLog::LL_FATAL);

        CRLog::info("main()");
        // QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor); 
        QApplication app(argc, argv);
        // Check if it is a portable installation
        // Check portable mark file ignoring case
        const lString32 portableMarkName = cs32("portable.mark");
        bool portable_found = false;
        LVContainerRef dir = LVOpenDirectory(getExeDir());
        if (!dir.isNull()) {
            for (int i = 0; i < dir->GetObjectCount(); i++) {
                const LVContainerItemInfo* item = dir->GetObjectInfo(i);
                if (NULL != item) {
                    if (!item->IsContainer()) {
                        lString32 name = item->GetName();
                        name = name.lowercase();
                        if (name == portableMarkName) {
                            portable_found = true;
                            break;
                        }
                    }
                }
            }
        }
        if (portable_found) {
            CRLog::debug("Portable mark file is found: '%s'", LCSTR(portableMarkName));
            CRLog::debug("Launch in portable settings mode.");
            setPortableSettingsMode(true);
        }

        lString32 configDir = getConfigDir();
        if (!LVDirectoryExists(configDir))
            LVCreateDirectory(configDir);

        lString32Collection fontDirs;
        lString32 homefonts = configDir + cs32("fonts");
        fontDirs.add(homefonts);
#if MACOS == 1 && USE_FONTCONFIG != 1
        fontDirs.add(cs32("/Library/Fonts"));
        fontDirs.add(cs32("/System/Library/Fonts"));
        fontDirs.add(cs32("/System/Library/Fonts/Supplemental"));
        lString32 home = qt2cr(QDir::toNativeSeparators(QDir::homePath()));
        fontDirs.add(home + cs32("/Library/Fonts"));
#endif
        if (!InitCREngine(argv[0], fontDirs)) {
            printf("Cannot init CREngine - exiting\n");
            return 2;
        }
        QString datadir = cr2qt(getMainDataDir());
        QString translations = datadir + "i18n/";
        QTranslator qtTranslator;
        if (qtTranslator.load("qt_" + QLocale::system().name(),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                              QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
#else
                              QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
#endif
            QApplication::installTranslator(&qtTranslator);

        QTranslator myappTranslator;
        QString trname = "crqt_" + QLocale::system().name();
        CRLog::info("Using translation file %s from dir %s", UnicodeToUtf8(qt2cr(trname)).c_str(),
                    UnicodeToUtf8(qt2cr(translations)).c_str());
        if (myappTranslator.load(trname, translations))
            QApplication::installTranslator(&myappTranslator);
        else
            CRLog::error("Canot load translation file %s from dir %s", UnicodeToUtf8(qt2cr(trname)).c_str(),
                         UnicodeToUtf8(qt2cr(translations)).c_str());
        QStringList filesToOpen;
        for (int i = optpos; i < argc; i++) {
            lString8 opt(argv[i]);
            if (!opt.startsWith("--")) {
                filesToOpen.append(cr2qt(LocalToUnicode(opt)));
            }
        }
        MainWindow w(filesToOpen);
        w.show();
        res = app.exec();
    }
    ShutdownCREngine();
    return res;
}

void ShutdownCREngine() {
    HyphMan::uninit();
    ShutdownFontManager();
    CRLog::setLogger(NULL);
}

#if (USE_FREETYPE == 1)
bool getDirectoryFonts(lString32Collection& pathList, lString32Collection& ext, lString32Collection& fonts,
                       bool absPath) {
    int foundCount = 0;
    lString32 path;
    for (int di = 0; di < pathList.length(); di++) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if (!dir.isNull()) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str());
            for (int i = 0; i < dir->GetObjectCount(); i++) {
                const LVContainerItemInfo* item = dir->GetObjectInfo(i);
                lString32 fileName = item->GetName();
                //lString8 fn = UnicodeToLocal(fileName);
                //printf(" test(%s) ", fn.c_str() );
                if (!item->IsContainer()) {
                    bool found = false;
                    lString32 lc = fileName;
                    lc.lowercase();
                    for (int j = 0; j < ext.length(); j++) {
                        if (lc.endsWith(ext[j])) {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        continue;
                    lString32 fn;
                    if (absPath) {
                        fn = path;
                        if (!fn.empty() && fn[fn.length() - 1] != PATH_SEPARATOR_CHAR)
                            fn << PATH_SEPARATOR_CHAR;
                    }
                    fn << fileName;
                    foundCount++;
                    fonts.add(fn);
                }
            }
        }
    }
    return foundCount > 0;
}
#endif

bool InitCREngine(const char* exename, lString32Collection& fontDirs) {
    CRLog::trace("InitCREngine(%s)", exename);
    InitFontManager(lString8::empty_str);
#if defined(_WIN32) && USE_FONTCONFIG != 1
    wchar_t sysdir_w[MAX_PATH + 1];
    GetWindowsDirectoryW(sysdir_w, MAX_PATH);
    lString32 fontdir = Utf16ToUnicode(sysdir_w);
    fontdir << "\\Fonts\\";
    lString8 fontdir8(UnicodeToUtf8(fontdir));
    // clang-format off
    const char* fontnames[] = {
        "arial.ttf",
        "ariali.ttf",
        "arialb.ttf",
        "arialbi.ttf",
        "arialn.ttf",
        "arialni.ttf",
        "arialnb.ttf",
        "arialnbi.ttf",
        "cour.ttf",
        "couri.ttf",
        "courbd.ttf",
        "courbi.ttf",
        "times.ttf",
        "timesi.ttf",
        "timesb.ttf",
        "timesbi.ttf",
        "comic.ttf",
        "comicbd.ttf",
        "verdana.ttf",
        "verdanai.ttf",
        "verdanab.ttf",
        "verdanaz.ttf",
        "bookos.ttf",
        "bookosi.ttf",
        "bookosb.ttf",
        "bookosbi.ttf",
        "calibri.ttf",
        "calibrii.ttf",
        "calibrib.ttf",
        "calibriz.ttf",
        "cambria.ttf",
        "cambriai.ttf",
        "cambriab.ttf",
        "cambriaz.ttf",
        "georgia.ttf",
        "georgiai.ttf",
        "georgiab.ttf",
        "georgiaz.ttf",
        NULL
    };
    // clang-format on
    for (int fi = 0; fontnames[fi]; fi++) {
        fontMan->RegisterFont(fontdir8 + fontnames[fi]);
    }
#endif // defined(_WIN32) && USE_FONTCONFIG!=1
    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    // use fontconfig

#if USE_FREETYPE == 1
    lString32Collection fontExt;
    fontExt.add(cs32(".ttf"));
    fontExt.add(cs32(".ttc"));
    fontExt.add(cs32(".otf"));
    fontExt.add(cs32(".pfa"));
    fontExt.add(cs32(".pfb"));

    lString32 datadir = getMainDataDir();
    lString32 fontDir = datadir + "fonts";
    fontDirs.add(fontDir);
    LVAppendPathDelimiter(fontDir);

    lString32Collection fonts;
    getDirectoryFonts(fontDirs, fontExt, fonts, true);

    // load fonts from file
    CRLog::debug("%d font files found", fonts.length());
    for (int fi = 0; fi < fonts.length(); fi++) {
        lString8 fn = UnicodeToLocal(fonts[fi]);
        CRLog::trace("loading font: %s", fn.c_str());
        if (!fontMan->RegisterFont(fn)) {
            CRLog::trace("    failed\n");
        }
    }
#endif // USE_FREETYPE==1

    if (!fontMan->GetFontCount()) {
        //error
#if (USE_FREETYPE == 1)
        printf("Fatal Error: Cannot open font file(s) .ttf \nCannot work without font\n");
#else
        printf("Fatal Error: Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF\n");
#endif
        return false;
    }

    printf("%d fonts loaded.\n", fontMan->GetFontCount());

    // init engine cache dir
    ldomDocCache::init(getEngineCacheDir(), (lvsize_t)DOC_CACHE_SIZE);

    return true;
}
