/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2023 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef TABSCOLLECTION_H
#define TABSCOLLECTION_H

#include <QtCore/QString>
#include <QtCore/QVector>

#include <lvstring.h>
#include <crhist.h>

#include "tabdata.h"

class TabsCollection: public QVector<TabData>
{
public:
    class TabProperty
    {
    public:
        TabProperty() { }
        TabProperty(const TabProperty& other) {
            docPath = other.docPath;
            title = other.title;
        }
        TabProperty& operator=(const TabProperty& other) {
            docPath = other.docPath;
            title = other.title;
            return *this;
        }
        QString docPath;
        QString title;
    };
    class TabSession: public QVector<TabProperty>
    {
    public:
        TabSession()
                : QVector<TabProperty>() { }
        QString currentDocument;
    };
public:
    TabsCollection();
    virtual ~TabsCollection();
    TabSession openTabSession(const QString& filename, bool* ok);
    bool saveTabSession(const QString& filename);
    bool saveTabSession() {
        return saveTabSession(QString());
    }
    /// load settings from file
    bool loadSettings(const QString& filename);
    /// save settings from file
    bool saveSettings(const QString& filename);
    /// save settings from file
    bool saveSettings() {
        return saveSettings(QString());
    }
    CRPropRef getSettings() {
        return m_props;
    }
    void setSettings(CRPropRef props);
    void saveWindowPos(QWidget* window, const char* prefix);
    void restoreWindowPos(QWidget* window, const char* prefix, bool allowExtraStates = false);
    /// load history from file
    bool loadHistory(const QString& filename);
    /// save history to file
    bool saveHistory(const QString& filename);
    /// save history to file
    bool saveHistory() {
        return saveHistory(QString());
    }
    CRFileHist* getHistory() {
        return &m_hist;
    }
    QString currentDocument() const {
        return m_currentDocument;
    }
    void setCurrentDocument(const QString& document) {
        m_currentDocument = document;
    }
    int indexByViewId(lUInt64 viewId) const;
    void cleanup();
    void append(const TabData& tab);
protected:
    void upgradeSettings();
private:
    CRPropRef m_props;
    CRFileHist m_hist;
    QString m_sessionFileName;
    lString32 m_settingsFileName;
    lString32 m_historyFileName;
    QString m_currentDocument;
};

#endif // TABSCOLLECTION_H
