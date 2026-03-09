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

/*
 * This file contains code snippets from cr3widget.cpp
 * Copyright (C) 2009-2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
 */

#include "tabscollection.h"
#include "cr3widget.h"
#include "crqtutil.h"
#include "app-props.h"

#include <QtCore/QSettings>

#include <lvdocview.h>
#include <lvstreamutils.h>
#include <crlog.h>

TabsCollection::TabsCollection()
        : QVector<TabData>()
        , m_props(LVCreatePropsContainer()) { }

TabsCollection::~TabsCollection() { }

TabsCollection::TabSession TabsCollection::openTabSession(const QString& filename, bool* ok) {
    TabSession session;
    TabProperty data;
    m_sessionFileName = filename;
    QSettings settings(filename, QSettings::IniFormat);
    int size = settings.beginReadArray("tabs");
    session.clear();
    session.reserve(size);
    for (int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        data.docPath = settings.value("doc-filename").toString();
        data.title = settings.value("title").toString();
        session.append(data);
    }
    settings.endArray();
    session.currentDocument = settings.value("current").toString();
    if (ok)
        *ok = (QSettings::NoError == settings.status());
    return session;
}

bool TabsCollection::saveTabSession(const QString& filename) {
    QString fn = !filename.isEmpty() ? filename : m_sessionFileName;
    QSettings settings(fn, QSettings::IniFormat);
    settings.clear();
    settings.beginWriteArray("tabs");
    for (int i = 0; i < QVector<TabData>::size(); i++) {
        const TabData& tab = at(i);
        CR3View* view = tab.view();
        if (NULL != view)
            view->getDocView()->savePosition();
        settings.setArrayIndex(i);
        settings.setValue("doc-filename", tab.docPath());
        settings.setValue("title", tab.title());
    }
    settings.endArray();
    settings.setValue("current", m_currentDocument);
    settings.sync();
    return QSettings::NoError == settings.status();
}

bool TabsCollection::loadSettings(const QString& filename) {
    lString32 fn(qt2cr(filename));
    m_settingsFileName = fn;

    // 1. Load app-shipped defaults (if exists)
    // Try exe directory first (portable mode), then engine data directory (system install)
    lString32 defaultsFile = LVCombinePaths(getExeDir(), cs32("crui-defaults.ini"));
    LVStreamRef defaultsStream = LVOpenFileStream(defaultsFile.c_str(), LVOM_READ);
    if (defaultsStream.isNull()) {
        defaultsFile = LVCombinePaths(getEngineDataDir(), cs32("crui-defaults.ini"));
        defaultsStream = LVOpenFileStream(defaultsFile.c_str(), LVOM_READ);
    }
    CRPropRef defaultProps = LVCreatePropsContainer();
    if (!defaultsStream.isNull() && defaultProps->loadFromStream(defaultsStream.get())) {
        CRLog::info("Loaded app defaults from %s", LCSTR(defaultsFile));
    }

    // 2. Load user settings
    LVStreamRef userStream = LVOpenFileStream(fn.c_str(), LVOM_READ);
    CRPropRef userProps = LVCreatePropsContainer();
    bool hadUserSettings = !userStream.isNull() && userProps->loadFromStream(userStream.get());
    if (hadUserSettings) {
        CRLog::info("Loaded user settings from %s", LCSTR(fn));
    } else {
        CRLog::info("No user settings found at %s", LCSTR(fn));
    }

    // 3. Merge: start with defaults, overlay user settings
    m_props = LVCreatePropsContainer();
    for (int i = 0; i < defaultProps->getCount(); i++)
        m_props->setString(defaultProps->getName(i), defaultProps->getValue(i));
    for (int i = 0; i < userProps->getCount(); i++)
        m_props->setString(userProps->getName(i), userProps->getValue(i));

    upgradeSettings();
    return hadUserSettings || !defaultsStream.isNull();
}

bool TabsCollection::saveSettings(const QString& filename) {
    lString32 fn(qt2cr(filename));
    if (fn.empty())
        fn = m_settingsFileName;
    if (fn.empty())
        return false;
    m_settingsFileName = fn;
    LVStreamRef stream = LVOpenFileStream(fn.c_str(), LVOM_WRITE);
    if (!stream) {
        lString32 upath = LVExtractPath(fn);
        lString8 path = UnicodeToUtf8(upath);
        if (!LVCreateDirectory(upath)) {
            CRLog::error("Cannot create directory %s", path.c_str());
        } else {
            stream = LVOpenFileStream(fn.c_str(), LVOM_WRITE);
        }
    }
    if (stream.isNull()) {
        CRLog::error("Cannot save settings to file %s", LCSTR(fn));
        return false;
    }
    return m_props->saveToStream(stream.get());
}

void TabsCollection::setSettings(CRPropRef props) {
    CRPropRef changed = m_props ^ props;
    // Don't create new props reference, but change existing
    m_props->set(changed | m_props);
}

void TabsCollection::saveWindowPos(QWidget* window, const char* prefix) {
    ::saveWindowPosition(window, m_props, prefix);
}

void TabsCollection::restoreWindowPos(QWidget* window, const char* prefix, bool allowExtraStates) {
    ::restoreWindowPosition(window, m_props, prefix, allowExtraStates);
}

bool TabsCollection::loadHistory(const QString& filename) {
    lString32 fn(qt2cr(filename));
    CRLog::trace("TabsCollection::loadHistory( %s )", UnicodeToUtf8(fn).c_str());
    m_historyFileName = fn;
    LVStreamRef stream = LVOpenFileStream(fn.c_str(), LVOM_READ);
    if (stream.isNull()) {
        return false;
    }
    if (!m_hist.loadFromStream(stream))
        return false;
    return true;
}

bool TabsCollection::saveHistory(const QString& filename) {
    lString32 fn(qt2cr(filename));
    if (fn.empty())
        fn = m_historyFileName;
    if (fn.empty()) {
        CRLog::info("Cannot write history file - no file name specified");
        return false;
    }
    m_historyFileName = fn;
    CRLog::trace("TabsCollection::saveHistory(): filename: %s", LCSTR(fn));
    LVStreamRef stream = LVOpenFileStream(fn.c_str(), LVOM_WRITE);
    if (!stream) {
        lString32 upath = LVExtractPath(fn);
        lString8 path = UnicodeToUtf8(upath);
        if (!LVCreateDirectory(upath)) {
            CRLog::error("Cannot create directory %s", path.c_str());
        } else {
            stream = LVOpenFileStream(fn.c_str(), LVOM_WRITE);
        }
    }
    if (stream.isNull()) {
        CRLog::error("Error while creating history file %s - position will be lost", LCSTR(fn));
        return false;
    }
    for (TabsCollection::iterator it = begin(); it != end(); ++it) {
        CR3View* view = (*it).view();
        if (NULL != view) {
            view->getDocView()->savePosition();
        }
    }
    return m_hist.saveToStream(stream.get());
}

int TabsCollection::indexByViewId(lUInt64 viewId) const {
    for (int i = 0; i < size(); i++) {
        const TabData& tab = at(i);
        const CR3View* view = tab.view();
        if (NULL != view) {
            if (view->id() == viewId) {
                return i;
            }
        }
    }
    return -1;
}

void TabsCollection::cleanup() {
    for (TabsCollection::iterator it = begin(); it != end(); ++it) {
        (*it).cleanup();
    }
    clear();
}

void TabsCollection::append(const TabData& tab) {
    QVector<TabData>::append(tab);
    tab.view()->setSharedSettings(m_props);
    tab.view()->setSharedHistory(&m_hist);
}

void TabsCollection::upgradeSettings() {
    static const char* const obsolete_toolbar_size = "window.toolbar.size";
    static const char* const obsolete_clipboard_autocopy = "clipboard.autocopy";
    static const char* const obsolete_selection_command = "selection.command";
    // add other obsolete properties here
    static const char* const all_obsolete_props[] = { obsolete_toolbar_size, obsolete_clipboard_autocopy,
                                                      obsolete_selection_command, NULL };

    bool changed = false;
    CRPropRef newProps = LVCreatePropsContainer();
    for (int i = 0; i < m_props->getCount(); i++) {
        lString8 propName = lString8(m_props->getName(i));
        lString32 propValue = m_props->getValue(i);
        bool found = false;
        for (int j = 0; all_obsolete_props[j] != NULL; j++) {
            if (propName == all_obsolete_props[j]) {
                CRLog::debug("Found obsolete property: %s=%s", propName.c_str(), LCSTR(propValue));
                if (propName == obsolete_toolbar_size) {
                    int toolbarSize = propValue.atoi();
                    bool showToolbar = 0 != toolbarSize;
                    newProps->setBoolDef(PROP_APP_WINDOW_SHOW_TOOLBAR, showToolbar);
                    CRLog::debug("Save as %s=%d", PROP_APP_WINDOW_SHOW_TOOLBAR, showToolbar ? 1 : 0);
                } else if (propName == obsolete_clipboard_autocopy) {
                    newProps->setString(PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY, propValue);
                    CRLog::debug("Save as %s=%s", PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY, LCSTR(propValue));
                } else if (propName == obsolete_selection_command) {
                    newProps->setString(PROP_APP_SELECTION_COMMAND, propValue);
                    CRLog::debug("Save as %s=%s", PROP_APP_SELECTION_COMMAND, LCSTR(propValue));
                }
                // process other obsolete properties here
                found = true;
            }
        }
        if (!found)
            newProps->setString(propName.c_str(), propValue);
        else
            changed = true;
    }
    if (changed)
        m_props = newProps;
}
