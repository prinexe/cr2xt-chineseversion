/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2022,2023 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "sampleview.h"

#include <QVBoxLayout>
#include <QCloseEvent>

#include "cr3widget.h"

SampleView::SampleView(QWidget* parent)
        : QWidget(parent, Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                          Qt::WindowDoesNotAcceptFocus) {
    setWindowTitle(tr("Style Preview"));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_view = new CR3View(this);
    layout->addWidget(m_view, 10);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setMinimumSize(200, 300);
    updatePositionForParent();
    m_view->setActive(true);
}

void SampleView::updatePositionForParent() {
    QPoint parentPos = parentWidget()->pos();
    QSize parentFrameSize = parentWidget()->frameSize();
    move(parentPos.x() + parentFrameSize.width(), parentPos.y() + 40);
}
