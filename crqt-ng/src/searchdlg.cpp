/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2010,2014 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Daniel Bedrenko <d.bedrenko@gmail.com>             *
 *   Copyright (C) 2020,2024 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "cr3widget.h"
#include <QEvent>
#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#include <QtWidgets/QMessageBox>
#else
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>
#endif
#include "searchdlg.h"
#include "ui_searchdlg.h"

SearchDialog* SearchDialog::_instance = NULL;

bool SearchDialog::showDlg(QWidget* parent, CR3View* docView) {
    if (NULL == _instance) {
        _instance = new SearchDialog(parent, docView);
    }
    _instance->show();
    _instance->raise();
    _instance->activateWindow();
    return true;
}

SearchDialog::SearchDialog(QWidget* parent, CR3View* docView)
        : QDialog(parent)
        , ui(new Ui::SearchDialog)
        , _docview(docView) {
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);
    ui->cbCaseSensitive->setCheckState(Qt::Unchecked);
    ui->rbForward->toggle();
    connect(ui->btnFindNext, SIGNAL(clicked()), this, SLOT(slot_btnFindNext()));
    connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(this, SIGNAL(finished(int)), this, SLOT(slot_finished(int)));
}

SearchDialog::~SearchDialog() {
    delete ui;
}

void SearchDialog::changeEvent(QEvent* e) {
    QDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

bool SearchDialog::findText(lString32 pattern, int origin, bool reverse, bool caseInsensitive) {
    if (pattern.empty())
        return false;
    if (pattern != _lastPattern && origin == 1)
        origin = 0;
    _lastPattern = pattern;
    LVArray<ldomWord> words;
    lvRect rc;
    _docview->getDocView()->GetPos(rc);
    int pageHeight = rc.height();
    int start = -1;
    int end = -1;
    if (reverse) {
        // reverse
        if (origin == 0) {
            // from end current page to first page
            end = rc.bottom;
        } else if (origin == -1) {
            // from last page to end of current page
            start = rc.bottom;
        } else { // origin == 1
            // from prev page to first page
            end = rc.top;
        }
    } else {
        // forward
        if (origin == 0) {
            // from current page to last page
            start = rc.top;
        } else if (origin == -1) {
            // from first page to current page
            end = rc.top;
        } else { // origin == 1
            // from next page to last
            start = rc.bottom;
        }
    }
    CRLog::debug("CRViewDialog::findText: Current page: %d .. %d", rc.top, rc.bottom);
    CRLog::debug("CRViewDialog::findText: searching for text '%s' from %d to %d origin %d", LCSTR(pattern), start, end,
                 origin);
    if (_docview->getDocView()->getDocument()->findText(pattern, caseInsensitive, reverse, start, end, words, 200,
                                                        pageHeight)) {
        CRLog::debug("CRViewDialog::findText: pattern found");
        _docview->getDocView()->clearSelection();
        _docview->getDocView()->selectWords(words);
        ldomMarkedRangeList* ranges = _docview->getDocView()->getMarkedRanges();
        if (ranges) {
            if (ranges->length() > 0) {
                int pos = ranges->get(0)->start.y;
                _docview->getDocView()->SetPos(pos);
            }
        }
        return true;
    }
    CRLog::debug("CRViewDialog::findText: pattern not found");
    return false;
}

void SearchDialog::slot_btnFindNext() {
    bool found = false;
    QString pattern = ui->edPattern->text();
    lString32 p32 = qt2cr(pattern);
    bool reverse = ui->rbBackward->isChecked();
    bool caseInsensitive = ui->cbCaseSensitive->checkState() != Qt::Checked;
    found = findText(p32, 1, reverse, caseInsensitive);
    if (!found)
        found = findText(p32, -1, reverse, caseInsensitive);
    if (!found) {
        QMessageBox::information(this, tr("Not found"), tr("Search pattern is not found in document"),
                                 QMessageBox::Close);
    } else {
        _docview->update();
    }
}

void SearchDialog::slot_finished(int result) {
    // This dialog instance will be deleted by Qt because we have
    //  set the WA_DeleteOnClose attribute for this window to true.
    SearchDialog::_instance = NULL;
}
