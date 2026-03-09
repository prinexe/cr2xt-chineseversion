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

#include "tabdata.h"

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QLayout>
#else
#include <QtGui/QScrollBar>
#include <QtGui/QLayout>
#endif

#include "cr3widget.h"
#include <lvdocview.h>

void TabData::cleanup() {
    if (NULL != m_scroll) {
        delete m_scroll;
        m_scroll = NULL;
    }
    if (NULL != m_view) {
        m_view->getDocView()->swapToCache();
        m_view->disconnect();
        delete m_view;
        m_view = NULL;
    }
    if (NULL != m_layout) {
        delete m_layout;
        m_layout = NULL;
    }
    if (NULL != m_widget) {
        delete m_widget;
        m_widget = NULL;
    }
}

bool TabData::isDocumentOpened() const {
    if (NULL != m_view)
        return m_view->getDocView()->isDocumentOpened();
    return false;
}
