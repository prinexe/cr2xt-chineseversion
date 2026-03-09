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

#include "fontfamiliesdialog.h"
#include "ui_fontfamiliesdialog.h"

FontFamiliesDialog::FontFamiliesDialog(QWidget* parent)
        : QDialog(parent)
        , ui(new Ui::FontFamiliesDialog) {
    ui->setupUi(this);
}

FontFamiliesDialog::~FontFamiliesDialog() {
    delete ui;
}

void FontFamiliesDialog::setAvailableFaces(const QStringList& availFaces) {
    m_availableFaces = availFaces;
    QString oldSerif = ui->cbSerifFace->currentText();
    QString oldSansSerif = ui->cbSansSerifFace->currentText();
    QString oldCursuve = ui->cbCursiveFace->currentText();
    QString oldFantasy = ui->cbFantasyFace->currentText();
    ui->cbSerifFace->clear();
    ui->cbSansSerifFace->clear();
    ui->cbCursiveFace->clear();
    ui->cbFantasyFace->clear();
    ui->cbSerifFace->addItems(m_availableFaces);
    ui->cbSansSerifFace->addItems(m_availableFaces);
    ui->cbCursiveFace->addItems(m_availableFaces);
    ui->cbFantasyFace->addItems(m_availableFaces);
    if (!oldSerif.isEmpty())
        ui->cbSerifFace->setCurrentText(oldSerif);
    if (!oldSansSerif.isEmpty())
        ui->cbSansSerifFace->setCurrentText(oldSansSerif);
    if (!oldCursuve.isEmpty())
        ui->cbCursiveFace->setCurrentText(oldCursuve);
    if (!oldFantasy.isEmpty())
        ui->cbFantasyFace->setCurrentText(oldFantasy);
}

void FontFamiliesDialog::setAvailableMonoFaces(const QStringList& availFaces) {
    m_availableMonoFaces = availFaces;
    QString oldFace = ui->cbMonospaceFace->currentText();
    ui->cbMonospaceFace->clear();
    ui->cbMonospaceFace->addItems(m_availableMonoFaces);
    if (!oldFace.isEmpty())
        ui->cbMonospaceFace->setCurrentText(oldFace);
}

void FontFamiliesDialog::setSerifFontFace(const QString& face) {
    ui->cbSerifFace->setCurrentText(face);
}

void FontFamiliesDialog::setSansSerifFontFace(const QString& face) {
    ui->cbSansSerifFace->setCurrentText(face);
}

void FontFamiliesDialog::setCursiveFontFace(const QString& face) {
    ui->cbCursiveFace->setCurrentText(face);
}

void FontFamiliesDialog::setFantasyFontFace(const QString& face) {
    ui->cbFantasyFace->setCurrentText(face);
}

void FontFamiliesDialog::setMonospaceFontFace(const QString& face) {
    ui->cbMonospaceFace->setCurrentText(face);
}

void FontFamiliesDialog::slot_serif_currectTextChanged(const QString& face) {
    m_serifFontFace = face;
}

void FontFamiliesDialog::slot_sansSerif_currectTextChanged(const QString& face) {
    m_sansSerifFontFace = face;
}

void FontFamiliesDialog::slot_cursive_currectTextChanged(const QString& face) {
    m_cursiveFontFace = face;
}

void FontFamiliesDialog::slot_fantasy_currectTextChanged(const QString& face) {
    m_fantasyFontFace = face;
}

void FontFamiliesDialog::slot_monospace_currectTextChanged(const QString& face) {
    m_monospaceFontFace = face;
}
