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

#ifndef FONTFAMILIESDIALOG_H
#define FONTFAMILIESDIALOG_H

#include <QDialog>

namespace Ui
{
    class FontFamiliesDialog;
}

class FontFamiliesDialog: public QDialog
{
    Q_OBJECT
public:
    explicit FontFamiliesDialog(QWidget* parent);
    ~FontFamiliesDialog();
    void setAvailableFaces(const QStringList& availFaces);
    void setAvailableMonoFaces(const QStringList& availFaces);
    QString serifFontFace() const {
        return m_serifFontFace;
    }
    QString sansSerifFontFace() const {
        return m_sansSerifFontFace;
    }
    QString cursiveFontFace() const {
        return m_cursiveFontFace;
    }
    QString fantasyFontFace() const {
        return m_fantasyFontFace;
    }
    QString monospaceFontFace() const {
        return m_monospaceFontFace;
    }
    void setSerifFontFace(const QString& face);
    void setSansSerifFontFace(const QString& face);
    void setCursiveFontFace(const QString& face);
    void setFantasyFontFace(const QString& face);
    void setMonospaceFontFace(const QString& face);
protected slots:
    void slot_serif_currectTextChanged(const QString&);
    void slot_sansSerif_currectTextChanged(const QString&);
    void slot_cursive_currectTextChanged(const QString&);
    void slot_fantasy_currectTextChanged(const QString&);
    void slot_monospace_currectTextChanged(const QString&);
private:
    QStringList m_availableFaces;
    QStringList m_availableMonoFaces;
    QString m_serifFontFace;
    QString m_sansSerifFontFace;
    QString m_cursiveFontFace;
    QString m_fantasyFontFace;
    QString m_monospaceFontFace;
    Ui::FontFamiliesDialog* ui;
};

#endif // FONTFAMILIESDIALOG_H
