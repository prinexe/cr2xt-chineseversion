/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2008-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2021 ourairquality <info@ourairquality.org>             *
 *   Copyright (C) 2018,2020-2023 Aleksey Chernov <valexlin@gmail.com>     *
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

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#else
#include <QtGui/QDialog>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#endif

#include "crqtutil.h"

namespace Ui
{
    class SettingsDlg;
}

#define DECL_DEF_CR_FONT_SIZES \
    static int cr_font_sizes[] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, \
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 45, 50, 55, 60, 65, 70, 75, 80, 85 }

struct StyleItem
{
    QString paramName;
    bool updating;
    QStringList values;
    QStringList titles;
    int currentIndex;
    QComboBox* cb;
    PropsRef props;
    void init(QString param, const char* defValue, const char* styleValues[], QString styleTitles[], bool hideFirstItem,
              PropsRef props, QComboBox* cb) {
        updating = true;
        paramName = param;
        this->cb = cb;
        this->props = props;
        currentIndex = -1;
        values.clear();
        titles.clear();
        //CRLog::trace("StyleItem::init %s", paramName.toUtf8().constData());

        QString currentValue = props->getStringDef(paramName.toUtf8().constData(), defValue);

        for (int i = !hideFirstItem ? 0 : 1; styleValues[i]; i++) {
            QString value = styleValues[i];
            if (currentValue == value)
                currentIndex = !hideFirstItem ? i : i - 1;
            values.append(value);
            titles.append(styleTitles[i]);
        }
        if (currentIndex == -1)
            currentIndex = 0;
        cb->clear();
        cb->addItems(titles);
        cb->setCurrentIndex(currentIndex);
        lString8 pname(paramName.toUtf8().constData());
        props->setString(pname.c_str(), values.at(currentIndex));
        //CRLog::trace("StyleItem::init %s done", paramName.toUtf8().constData());
        updating = false;
    }

    void init(QString param, QStringList& styleValues, QStringList& styleTitles, PropsRef props, QComboBox* cb) {
        updating = true;
        paramName = param;
        this->cb = cb;
        this->props = props;
        currentIndex = -1;
        values.clear();
        titles.clear();
        values.append(styleValues);
        titles.append(styleTitles);

        QString currentValue = props->getStringDef(paramName.toUtf8().constData(), "");

        for (int i = 0; i < styleValues.length(); i++) {
            if (currentValue == styleValues.at(i))
                currentIndex = i;
        }
        if (currentIndex == -1)
            currentIndex = 0;
        cb->clear();
        cb->addItems(titles);
        cb->setCurrentIndex(currentIndex);
        lString8 pname(paramName.toUtf8().constData());
        props->setString(pname.c_str(), values.at(currentIndex));
        updating = false;
    }

    void update(int newIndex) {
        //CRLog::trace("StyleItem::update %s %s", paramName.toUtf8().constData(), updating ? "updating" : "");
        if (updating)
            return;
        currentIndex = newIndex;
        lString8 pname(paramName.toUtf8().constData());
        props->setString(pname.c_str(), values.at(currentIndex));
        //CRLog::trace("StyleItem::update '%s' = '%s'", pname, pvalue);
    }
};

class CR3View;
class SampleView;

class SettingsDlg: public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(SettingsDlg)
public:
    explicit SettingsDlg(QWidget* parent, PropsRef props, CR3View* docview = nullptr);
    virtual ~SettingsDlg();
    PropsRef options() const {
        return m_props;
    }
protected:
    virtual void changeEvent(QEvent* e);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void moveEvent(QMoveEvent* event);
    virtual void resizeEvent(QResizeEvent* event);

    void setCheck(const char* optionName, int checkState);
    void optionToUi(const char* optionName, QCheckBox* cb);
    void optionToUiLine(const char* optionName, QLineEdit* le);
    void optionToUiString(const char* optionName, QComboBox* cb);
    void optionToUiIndex(const char* optionName, QComboBox* cb);
    void optionToComboWithFloats(const char* optionName, QComboBox* cb, int defaultIdx);
    void setCheckInversed(const char* optionName, int checkState);
    void optionToUiInversed(const char* optionName, QCheckBox* cb);
    void fontToUi(const char* faceOptionName, const char* sizeOptionName, QComboBox* faceCombo, QComboBox* sizeCombo,
                  const char* defFontFace);
    void updateFontWeights();

    void initStyleControls(const char* styleName);

    QColor getColor(const char* optionName, unsigned def);
    void setColor(const char* optionName, QColor cl);
    void colorDialog(const char* optionName, QString title);

    void setBackground(QWidget* wnd, QColor cl);
    void initSampleWindow();
    void updateStyleSample();
    void updateStyleControlsEnabled();

    /// Expand a CSS template file and write to user config directory
    /// @param templateName Template file name (e.g. "fb2.css")
    /// @return Path to expanded file on success, empty string on failure
    QString expandCssTemplate(const QString& templateName);
private:
    SampleView* m_sample;
    Ui::SettingsDlg* m_ui;
    CR3View* m_docview;
    PropsRef m_props;
    QString m_oldHyph;
    QStringList m_backgroundFiles;
    QStringList m_faceList;
    QStringList m_monoFaceList;
    QStringList m_hyphDictIdList;
    QString m_defFontFace;
    QString m_styleName;
    StyleItem m_styleItemAlignment;
    StyleItem m_styleItemIndent;
    StyleItem m_styleItemMarginBefore;
    StyleItem m_styleItemMarginAfter;
    StyleItem m_styleItemMarginLeft;
    StyleItem m_styleItemMarginRight;
    StyleItem m_styleFontFace;
    StyleItem m_styleFontSize;
    StyleItem m_styleFontWeight;
    StyleItem m_styleFontStyle;
    StyleItem m_styleFontColor;
    StyleItem m_styleLineHeight;
    StyleItem m_styleTextDecoration;
    StyleItem m_verticalAlignDecoration;
    QStringList m_styleNames;

private slots:
    void sampleview_destroyed(QObject* obj);

    void on_cbPageSkin_currentIndexChanged(int index);
    void on_cbTxtPreFormatted_stateChanged(int);
    void on_cbTxtPreFormatted_toggled(bool checked);
    void on_cbStartupAction_currentIndexChanged(int index);
    void on_cbHyphenation_currentIndexChanged(int index);
    void on_sbInterlineSpace_valueChanged(int value);
    void on_sbSpaceWidth_valueChanged(int value);
    void on_sbMinSpaceWidth_valueChanged(int value);
    void on_sbUnusedSpaceThreshold_valueChanged(int value);
    void on_sbMaxLetterSpacing_valueChanged(int value);
    void on_cbTextFontSize_currentTextChanged(QString);
    void on_cbTextFontFace_currentTextChanged(QString);
    void on_cbTitleFontSize_currentTextChanged(QString);
    void on_cbTitleFontFace_currentTextChanged(QString);
    void on_cbLookAndFeel_currentTextChanged(QString);
    void on_btnHeaderTextColor_clicked();
    void on_btnBgColor_clicked();
    void on_btnTextColor_clicked();
    void on_cbMarginTop_currentIndexChanged(int index);
    void on_cbMarginBottom_currentIndexChanged(int index);
    void on_cbMarginLeft_currentIndexChanged(int index);
    void on_cbMarginRight_currentIndexChanged(int index);
    void on_cbFootnotesMode_currentIndexChanged(int index);
    void on_cbShowPageNumber_stateChanged(int s);
    void on_cbShowPageCount_stateChanged(int s);
    void on_cbShowChapterName_stateChanged(int s);
    void on_cbShowChapterPageNumber_stateChanged(int s);
    void on_cbShowChapterPageCount_stateChanged(int s);
    void on_cbShowPositionPercent_stateChanged(int s);
    void on_cbShowBattery_stateChanged(int s);
    void on_cbShowClock_stateChanged(int s);
    void on_cbShowNavbar_stateChanged(int s);
    void on_sbHeaderMarginH_valueChanged(int value);
    void on_sbHeaderMarginV_valueChanged(int value);
    void on_sbNavbarHeight_valueChanged(int value);
    void on_cbShowChapterMarks_stateChanged(int s);
    void on_cbShowBookName_stateChanged(int s);
    void on_cbIgnoreDocumentMargins_stateChanged(int s);
    void on_cbShowPageHeader_currentIndexChanged(int index);
    void on_cbViewMode_currentIndexChanged(int index);
    void on_cbWindowShowScrollbar_stateChanged(int);
    void on_cbWindowShowStatusBar_stateChanged(int);
    void on_cbWindowShowMenu_stateChanged(int);
    void on_cbWindowShowToolbar_stateChanged(int);
    void on_cbWindowFullscreen_stateChanged(int);
    void on_cbWindowFixedTabSize_toggled(bool checked);
    void on_cbAutoClipboard_stateChanged(int);
    void on_cbAutoCmdExec_toggled(bool checked);
    void on_cbSelectionCommand_textChanged(QString);
    void on_cbFontKerning_stateChanged(int);
    void on_cbFloatingPunctuation_stateChanged(int);
    void on_cbFontGamma_currentTextChanged(QString);
    void on_cbStyleName_currentIndexChanged(int index);
    void on_cbDefAlignment_currentIndexChanged(int index);
    void on_cbDefFirstLine_currentIndexChanged(int index);
    void on_cbDefMarginBefore_currentIndexChanged(int index);
    void on_cbDefMarginAfter_currentIndexChanged(int index);
    void on_cbDefMarginLeft_currentIndexChanged(int index);
    void on_cbDefMarginRight_currentIndexChanged(int index);
    void on_cbDefFontSize_currentIndexChanged(int index);
    void on_cbDefFontFace_currentIndexChanged(int index);
    void on_cbDefFontWeight_currentIndexChanged(int index);
    void on_cbDefFontStyle_currentIndexChanged(int index);
    void on_cbDefFontColor_currentIndexChanged(int index);
    void on_cbDefTextDecoration_currentIndexChanged(int index);
    void on_cbDefVerticalAlign_currentIndexChanged(int index);
    void on_cbDefLineHeight_currentIndexChanged(int index);
    void on_cbTTInterpreter_currentIndexChanged(int index);
    void on_cbFontHinting_currentIndexChanged(int index);
    void on_cbEnableEmbeddedFonts_toggled(bool checked);
    void on_cbEnableDocumentStyles_toggled(bool checked);
    void on_btnSelectionColor_clicked();
    void on_btnCommentColor_clicked();
    void on_btnCorrectionColor_clicked();
    void on_cbBookmarkHighlightMode_currentIndexChanged(int index);
    void on_cbImageInlineZoominMode_currentIndexChanged(int index);
    void on_cbImageInlineZoominScale_currentIndexChanged(int index);
    void on_cbImageInlineZoomoutMode_currentIndexChanged(int index);
    void on_cbImageInlineZoomoutScale_currentIndexChanged(int index);
    void on_cbImageBlockZoominMode_currentIndexChanged(int index);
    void on_cbImageBlockZoominScale_currentIndexChanged(int index);
    void on_cbImageBlockZoomoutMode_currentIndexChanged(int index);
    void on_cbImageBlockZoomoutScale_currentIndexChanged(int index);
    void on_cbImageAutoRotate_stateChanged(int s);
    void on_cbFontShaping_currentIndexChanged(int index);
    void on_cbRendFlags_currentIndexChanged(int index);
    void on_cbDOMLevel_currentIndexChanged(int index);
    void on_cbMultiLang_stateChanged(int state);
    void on_cbEnableHyph_stateChanged(int state);
    void on_btnFallbackMan_clicked();
    void on_btnFontFamiliesMan_clicked();
    void on_cbFontWeightChange_currentIndexChanged(int index);
    void on_cbAntialiasingMode_currentIndexChanged(int index);
    void on_tabWidget_currentChanged(int index);
    void on_cbUseGeneratedCSS_toggled(bool checked);
    void on_btnGenerateCSS_clicked();
    void on_btnExpandAllCSS_clicked();
};

#endif // SETTINGSDLG_H
