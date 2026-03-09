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

#ifndef CRQTUTIL_H
#define CRQTUTIL_H

#include <QString>
#include <QStringList>
#include <QSharedPointer>

#include <crengine.h>

lString32 qt2cr(const QString& str);
QString cr2qt(const lString32& str);

class Props;
typedef QSharedPointer<Props> PropsRef;

/// proxy class for CoolReader properties
class Props
{
public:
    static PropsRef create();
    static PropsRef clone(PropsRef v);
    virtual int count() = 0;
    virtual const char* name(int index) = 0;
    virtual QString value(int index) = 0;
    virtual bool getString(const char* prop, QString& result) = 0;
    virtual QString getStringDef(const char* prop, const char* defValue) = 0;
    virtual void setString(const char* prop, const QString& value) = 0;
    virtual void setInt(const char* prop, int value) = 0;
    virtual bool getInt(const char* prop, int& result) = 0;
    virtual int getIntDef(const char* prop, int defValue) = 0;
    virtual unsigned getColorDef(const char* prop, unsigned defValue) = 0;
    virtual bool getBoolDef(const char* prop, bool defValue) = 0;
    virtual void setHex(const char* propName, int value) = 0;
    virtual bool hasProperty(const char* propName) const = 0;
    virtual const CRPropRef& accessor() = 0;
    virtual ~Props() { }
};

/// returns common items from props1 not containing in props2
PropsRef operator-(PropsRef props1, PropsRef props2);
/// returns common items containing in props1 or props2
PropsRef operator|(PropsRef props1, PropsRef props2);
/// returns common items of props1 and props2
PropsRef operator&(PropsRef props1, PropsRef props2);
/// returns added or changed items of props2 compared to props1
PropsRef operator^(PropsRef props1, PropsRef props2);

/// adapter from coolreader property collection to qt
PropsRef cr2qt(CRPropRef& ref);
/// adapter from qt property collection to coolreader
const CRPropRef& qt2cr(PropsRef& ref);

void cr2qt(QStringList& dst, const lString32Collection& src);
void qt2cr(lString32Collection& dst, const QStringList& src);

/// format p as percent*100 - e.g. 1234->"12.34%"
QString crpercent(int p);

void crGetFontFaceList(QStringList& dst);
void crGetFontFaceListFiltered(QStringList& dst, css_font_family_t family, const QString& langTag);

QString getHumanReadableLocaleName(lString32 langTag);

class QWidget;
/// save window position to properties
void saveWindowPosition(QWidget* window, CRPropRef props, const char* prefix);
/// restore window position from properties
void restoreWindowPosition(QWidget* window, CRPropRef props, const char* prefix, bool allowFullscreen = false);

lString32 getMainDataDir();
lString32 getEngineDataDir();
lString32 getExeDir();
lString32 getConfigDir();
lString32 getEngineCacheDir();

bool getPortableSettingsMode();
void setPortableSettingsMode(bool mode);

/// Given a command line (name of the executable with optional arguments),
/// separates-out the name and all the arguments into a list. Supports quotes
/// and double-quotes.
QStringList parseExtCommandLine(QString const&);

#endif // CRQTUTIL_H
