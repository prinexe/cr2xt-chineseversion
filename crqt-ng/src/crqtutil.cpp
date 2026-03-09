/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2009,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020,2021,2023 Aleksey Chernov <valexlin@gmail.com>     *
 *   Copyright (C) 2023 Ren Tatsumoto <tatsu@autistici.org>                *
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

#include "crqtutil.h"
#include <QStringList>
#include <QWidget>
#include <QPoint>
#include <QDir>
#include <QFileInfo>
#include <QApplication>

#include <crprops.h>
#include <crlocaledata.h>

lString32 qt2cr(const QString& str) {
    return lString32(str.toUtf8().constData());
}

QString cr2qt(const lString32& str) {
    return QString::fromUcs4(str.c_str(), str.length());
}

class CRPropsImpl: public Props
{
    CRPropRef _ref;
public:
    CRPropRef getRef() {
        return _ref;
    }
    CRPropsImpl(CRPropRef ref)
            : _ref(ref) { }
    virtual int count() {
        return _ref->getCount();
    }
    virtual const char* name(int index) {
        return _ref->getName(index);
    }
    virtual QString value(int index) {
        return cr2qt(_ref->getValue(index));
    }
    virtual bool hasProperty(const char* propName) const {
        return _ref->hasProperty(propName);
    }
    virtual bool getString(const char* prop, QString& result) {
        lString32 value;
        if (!_ref->getString(prop, value))
            return false;
        result = cr2qt(value);
        return true;
    }
    virtual QString getStringDef(const char* prop, const char* defValue) {
        return cr2qt(_ref->getStringDef(prop, defValue));
    }
    virtual void setString(const char* prop, const QString& value) {
        _ref->setString(prop, qt2cr(value));
    }
    virtual bool getInt(const char* prop, int& result) {
        return _ref->getInt(prop, result);
    }
    virtual void setInt(const char* prop, int value) {
        _ref->setInt(prop, value);
    }
    virtual int getIntDef(const char* prop, int defValue) {
        return _ref->getIntDef(prop, defValue);
    }
    virtual unsigned getColorDef(const char* prop, unsigned defValue) {
        return _ref->getColorDef(prop, defValue);
    }
    virtual bool getBoolDef(const char* prop, bool defValue) {
        return _ref->getBoolDef(prop, defValue);
    }

    virtual void setHex(const char* propName, int value) {
        _ref->setHex(propName, value);
    }
    virtual CRPropRef& accessor() {
        return _ref;
    }
    virtual ~CRPropsImpl() { }
};

PropsRef cr2qt(CRPropRef& ref) {
    return QSharedPointer<Props>(new CRPropsImpl(ref));
}

const CRPropRef& qt2cr(PropsRef& ref) {
    return ref->accessor();
}

PropsRef Props::create() {
    return QSharedPointer<Props>(new CRPropsImpl(LVCreatePropsContainer()));
}

PropsRef Props::clone(PropsRef v) {
    return QSharedPointer<Props>(new CRPropsImpl(LVClonePropsContainer(((CRPropsImpl*)v.data())->getRef())));
}

/// returns common items from props1 not containing in props2
PropsRef operator-(PropsRef props1, PropsRef props2) {
    return QSharedPointer<Props>(new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() -
                                                 ((CRPropsImpl*)props2.data())->getRef()));
}

/// returns common items containing in props1 or props2
PropsRef operator|(PropsRef props1, PropsRef props2) {
    return QSharedPointer<Props>(new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() |
                                                 ((CRPropsImpl*)props2.data())->getRef()));
}

/// returns common items of props1 and props2
PropsRef operator&(PropsRef props1, PropsRef props2) {
    return QSharedPointer<Props>(new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() &
                                                 ((CRPropsImpl*)props2.data())->getRef()));
}

/// returns added or changed items of props2 compared to props1
PropsRef operator^(PropsRef props1, PropsRef props2) {
    return QSharedPointer<Props>(new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() ^
                                                 ((CRPropsImpl*)props2.data())->getRef()));
}

void cr2qt(QStringList& dst, const lString32Collection& src) {
    dst.clear();
    for (int i = 0; i < src.length(); i++) {
        dst.append(cr2qt(src[i]));
    }
}

void qt2cr(lString32Collection& dst, const QStringList& src) {
    dst.clear();
    for (int i = 0; i < src.length(); i++) {
        dst.add(qt2cr(src[i]));
    }
}

void crGetFontFaceList(QStringList& dst) {
    lString32Collection faceList;
    fontMan->getFaceList(faceList);
    cr2qt(dst, faceList);
}

void crGetFontFaceListFiltered(QStringList& dst, css_font_family_t family, const QString& langTag) {
    lString32Collection faceList;
    lString8 crlangTag = lString8(langTag.toLatin1().data());
    fontMan->getFaceListFiltered(faceList, family, crlangTag);
    cr2qt(dst, faceList);
}

QString getHumanReadableLocaleName(lString32 langTag) {
#if USE_LOCALE_DATA == 1
    QString res;
    CRLocaleData loc(UnicodeToUtf8(langTag));
    if (loc.isValid()) {
        res = loc.langName().c_str();
        if (loc.scriptNumeric() > 0) {
            res.append("-");
            res.append(loc.scriptName().c_str());
        }
        if (loc.regionNumeric() > 0) {
            res.append(" (");
            res.append(loc.regionAlpha3().c_str());
            res.append(")");
        }
    }
#else
    QString res = QT_TRANSLATE_NOOP("crqtutils", "Undetermined");
#endif
    return res;
}

QString crpercent(int p) {
    return QString("%1.%2%").arg(p / 100).arg(p % 100, 2, 10, QLatin1Char('0'));
}

/// save window position to properties
void saveWindowPosition(QWidget* window, CRPropRef props, const char* prefix) {
    // Use geometry() to get the client area position (excluding frame/title bar)
    // This avoids macOS issues where pos()/frameGeometry() causes window position drift
    QRect geom = window->geometry();
    bool minimized = window->isMinimized();
    bool maximized = window->isMaximized();
    bool fs = window->isFullScreen();
    CRPropRef p = props->getSubProps(prefix);
    p->setBool("window.minimized", minimized);
    p->setBool("window.maximized", maximized);
    p->setBool("window.fullscreen", fs);
    if (!minimized && !maximized && !fs) {
        // Save client area geometry (position and size, excluding frame)
        p->setPoint("window.pos", lvPoint(geom.x(), geom.y()));
        p->setPoint("window.size", lvPoint(geom.width(), geom.height()));
    }
}

/// restore window position from properties
void restoreWindowPosition(QWidget* window, CRPropRef props, const char* prefix, bool allowFullscreen) {
    CRPropRef p = props->getSubProps(prefix);
    lvPoint pos;
    bool posRead = p->getPoint("window.pos", pos);
    lvPoint size;
    bool sizeRead = p->getPoint("window.size", size);

    if (posRead && sizeRead) {
        if (size.x > 100 && size.y > 100) {
            // Use setGeometry() for consistent positioning on macOS
            // The saved position is the client area position (from geometry())
            window->setGeometry(pos.x, pos.y, size.x, size.y);
        }
    }
    if (allowFullscreen) {
        bool minimized = p->getBoolDef("window.minimized", false);
        bool maximized = p->getBoolDef("window.maximized", false);
        bool fs = p->getBoolDef("window.fullscreen", false);
        if (fs) {
            window->showFullScreen();
        } else if (maximized) {
            window->showMaximized();
        } else if (minimized) {
            window->showMinimized();
        }
    }
}

static lString32 s_mainDataDir;
static lString32 s_engineDataDir;
static lString32 s_exeDir;
static lString32 s_configDir;
static lString32 s_engineCacheDir;
static bool s_portableMode = false;

lString32 getMainDataDir() {
    if (s_mainDataDir.empty()) {
#if MACOS == 1
        QString exeDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/../Resources/");
        exeDir = QFileInfo(exeDir).absoluteFilePath();
        s_mainDataDir = qt2cr(exeDir);
#elif defined(WIN32)
        QString exeDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/");
        s_mainDataDir = qt2cr(exeDir);
#else
        // Check for AppImage environment (APPDIR is set by AppImage runtime)
        QString appDir = qEnvironmentVariable("APPDIR", QString());
        if (!appDir.isEmpty()) {
            // Use crengine-ng data dir which contains CSS, fonts, backgrounds, textures, i18n
            QString dataDir = QDir::toNativeSeparators(appDir + "/usr/share/crengine-ng/");
            s_mainDataDir = qt2cr(dataDir);
        } else {
            s_mainDataDir = lString32(CRUI_DATA_DIR);
        }
#endif
    }
    return s_mainDataDir;
}

lString32 getEngineDataDir() {
    if (s_engineDataDir.empty()) {
#if MACOS == 1
        QString resDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/../Resources/");
        resDir = QFileInfo(resDir).absoluteFilePath();
        s_engineDataDir = qt2cr(resDir);
#elif defined(WIN32)
        QString exeDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/");
        s_engineDataDir = qt2cr(exeDir);
#else
        // Check for AppImage environment (APPDIR is set by AppImage runtime)
        QString appDir = qEnvironmentVariable("APPDIR", QString());
        if (!appDir.isEmpty()) {
            QString dataDir = QDir::toNativeSeparators(appDir + "/usr/share/crengine-ng/");
            s_engineDataDir = qt2cr(dataDir);
        } else {
            s_engineDataDir = lString32(CRE_NG_DATADIR);
        }
#endif
    }
    return s_engineDataDir;
}

lString32 getExeDir() {
    if (s_exeDir.empty()) {
        QString exeDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/");
        s_exeDir = qt2cr(exeDir);
    }
    return s_exeDir;
}

lString32 getConfigDir() {
    if (s_configDir.empty()) {
#if MACOS == 1
        // Portable mode not supported
        // I have no idea why this might be needed on MacOS
        s_configDir = qt2cr(QDir::homePath() + "/Library/crui/");
#elif defined(WIN32)
        if (s_portableMode) {
            // <exe dir> + ".config/"
            s_configDir = LVCombinePaths(getExeDir(), cs32(".config/"));
        } else {
            // ~/crui/
            s_configDir = qt2cr(QDir::toNativeSeparators(QDir::homePath() + "/crui/"));
        }
#else
        if (s_portableMode) {
            // <exe dir> + ".config"
            s_configDir = LVCombinePaths(getExeDir(), cs32(".config/"));
        } else {
            // Use $XDG_CONFIG_HOME environment variable if set or '~/.config' then concatenate '/crui/'
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            QString xdg_config_home = qEnvironmentVariable("XDG_CONFIG_HOME", QString());
#else
            QByteArray env_value = qgetenv("XDG_CONFIG_HOME");
            QString xdg_config_home = QString::fromLocal8Bit(env_value);
#endif
            if (xdg_config_home.isEmpty())
                xdg_config_home = QDir::homePath() + "/.config";
            QString path = xdg_config_home + "/crui/";
            s_configDir = qt2cr(path);
        }
#endif
    }
    return s_configDir;
}

lString32 getEngineCacheDir() {
    if (s_engineCacheDir.empty()) {
#if MACOS == 1
        s_engineCacheDir = qt2cr(QDir::homePath() + "/Library/crui/cache/");
#elif defined(WIN32)
        if (s_portableMode) {
            // <exe dir> + ".cache/"
            s_engineCacheDir = LVCombinePaths(getExeDir(), cs32(".cache/"));
        } else {
            // ~/crui/cache
            s_engineCacheDir = qt2cr(QDir::toNativeSeparators(QDir::homePath() + "/crui/cache/"));
        }
#else
        if (s_portableMode) {
            // <exe dir> + ".cache"
            s_engineCacheDir = LVCombinePaths(getExeDir(), cs32(".cache/"));
        } else {
            // Use $XDG_CACHE_HOME environment variable if set or '~/.cache' then concatenate '/crui/'
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            QString xdg_cache_home = qEnvironmentVariable("XDG_CACHE_HOME", QString());
#else
            QByteArray env_value = qgetenv("XDG_CACHE_HOME");
            QString xdg_cache_home = QString::fromLocal8Bit(env_value);
#endif
            if (xdg_cache_home.isEmpty())
                xdg_cache_home = QDir::homePath() + "/.cache";
            QString path = xdg_cache_home + "/crui/";
            s_engineCacheDir = qt2cr(path);
        }
#endif
    }
    return s_engineCacheDir;
}

bool getPortableSettingsMode() {
    return s_portableMode;
}

void setPortableSettingsMode(bool mode) {
    s_portableMode = mode;
    // clear cached value to force recreate on
    //  next call to getConfigDir() function
    s_configDir = lString32::empty_str;
    s_engineCacheDir = lString32::empty_str;
}

QStringList parseExtCommandLine(QString const& commandLine) {
    // Parse arguments. Handle quotes correctly.
    QStringList args;
    bool openQuote = false;
    bool possibleDoubleQuote = false;
    bool startNew = true;
    for (QString::const_iterator c = commandLine.begin(), e = commandLine.end(); c != e;) {
        if (*c == '"' && !possibleDoubleQuote) {
            ++c;
            if (!openQuote) {
                openQuote = true;
                if (startNew) {
                    args.push_back(QString());
                    startNew = false;
                }
            } else
                possibleDoubleQuote = true;
        } else if (possibleDoubleQuote && *c != '"') {
            openQuote = false;
            possibleDoubleQuote = false;
        } else if (*c == ' ' && !openQuote) {
            ++c;
            startNew = true;
        } else {
            if (startNew) {
                args.push_back(QString());
                startNew = false;
            }
            args.last().push_back(*c++);
            possibleDoubleQuote = false;
        }
    }

    return args;
}
