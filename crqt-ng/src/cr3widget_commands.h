/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2023,2024 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef CR3WIDGET_COMMANDS_H
#define CR3WIDGET_COMMANDS_H

#include <QtCore/QVariant>
#include <QtCore/QList>

enum CR3ViewCommandType
{
    Noop,
    OpenDocument,
    SetDocumentText,
    Resize
};

class CR3ViewCommand
{
public:
    CR3ViewCommand() {
        m_cmd = CR3ViewCommandType::Noop;
    }
    CR3ViewCommand(CR3ViewCommandType cmd, const QVariant& data) {
        m_cmd = cmd;
        m_data = data;
    }
    CR3ViewCommand(const CR3ViewCommand& other) {
        m_cmd = other.m_cmd;
        m_data = other.m_data;
    }
    CR3ViewCommand& operator=(const CR3ViewCommand& other) {
        m_cmd = other.m_cmd;
        m_data = other.m_data;
        return *this;
    }
    CR3ViewCommandType command() const {
        return m_cmd;
    }
    const QVariant& data() const {
        return m_data;
    }
private:
    CR3ViewCommandType m_cmd;
    QVariant m_data;
};

typedef QList<CR3ViewCommand> CR3ViewCommandList;

#endif // CR3WIDGET_COMMANDS_H
