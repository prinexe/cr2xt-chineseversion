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

#ifndef TABDATA_H
#define TABDATA_H

#include <stddef.h>

#include <QtCore/QString>

class QWidget;
class QLayout;
class QScrollBar;
class CR3View;

class TabData
{
public:
    TabData()
            : m_widget(NULL)
            , m_layout(NULL)
            , m_view(NULL)
            , m_scroll(NULL) { }
    TabData(QWidget* widget, QLayout* layout, CR3View* view, QScrollBar* scrollBar)
            : m_widget(widget)
            , m_layout(layout)
            , m_view(view)
            , m_scroll(scrollBar)
            , m_canGoBack(false)
            , m_canGoForward(false) { }
    TabData(const TabData& obj) {
        m_widget = obj.m_widget;
        m_layout = obj.m_layout;
        m_view = obj.m_view;
        m_scroll = obj.m_scroll;
        m_docPath = obj.m_docPath;
        m_title = obj.m_title;
        m_canGoBack = obj.m_canGoBack;
        m_canGoForward = obj.m_canGoForward;
    }
    TabData& operator=(const TabData& obj) {
        m_widget = obj.m_widget;
        m_layout = obj.m_layout;
        m_view = obj.m_view;
        m_scroll = obj.m_scroll;
        m_docPath = obj.m_docPath;
        m_title = obj.m_title;
        m_canGoBack = obj.m_canGoBack;
        m_canGoForward = obj.m_canGoForward;
        return *this;
    }
    virtual ~TabData() { }
    void cleanup();
    bool isValid() const {
        return NULL != m_widget && NULL != m_layout && NULL != m_view && NULL != m_scroll;
    }
    bool isDocumentOpened() const;
    QWidget* widget() const {
        return m_widget;
    }
    QLayout* layout() const {
        return m_layout;
    }
    CR3View* view() const {
        return m_view;
    }
    QScrollBar* scrollBar() const {
        return m_scroll;
    }
    QString docPath() const {
        return m_docPath;
    }
    void setDocPath(const QString& path) {
        m_docPath = path;
    }
    QString title() const {
        return m_title;
    }
    void setTitle(QString title) {
        m_title = title;
    }
    bool canGoBack() const {
        return m_canGoBack;
    }
    void setCanGoBack(bool flag) {
        m_canGoBack = flag;
    }
    bool canGoForward() const {
        return m_canGoForward;
    }
    void setCanGoForward(bool flag) {
        m_canGoForward = flag;
    }
private:
    QWidget* m_widget;
    QLayout* m_layout;
    CR3View* m_view;
    QScrollBar* m_scroll;
    QString m_docPath;
    QString m_title;
    bool m_canGoBack;
    bool m_canGoForward;
};

#endif // TABDATA_H
