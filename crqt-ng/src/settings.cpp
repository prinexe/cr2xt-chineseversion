/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2008-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 EXL <exlmotodev@gmail.com>                         *
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2021 ourairquality <info@ourairquality.org>             *
 *   Copyright (C) 2023 Ren Tatsumoto <tatsu@autistici.org>                *
 *   Copyright (C) 2018-2024 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "settings.h"
#include "ui_settings.h"
#include "app-props.h"
#include "sampleview.h"
#include "cr3widget.h"
#include "crqtutil.h"
#include "fallbackfontsdialog.h"
#include "fontfamiliesdialog.h"
#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QStyleFactory>
#else
#include <QtGui/QColorDialog>
#include <QtGui/QStyleFactory>
#endif
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <lvrend.h> // for BLOCK_RENDERING_FLAGS_XXX presets
#include <lvdocview.h> // for generateExpandedCSS, saveExpandedCSS

static int def_margins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 20, 25, 30, 40, 50, 60 };
#define MAX_MARGIN_INDEX (sizeof(def_margins) / sizeof(int))

DECL_DEF_CR_FONT_SIZES;

#if USE_FREETYPE == 1 && USE_FT_EMBOLDEN
static int synth_weights[] = { 300, 400, 500, 600, 700, 800, 900 };
#else
static int synth_weights[] = { 600 };
#endif
#define SYNTH_WEIGHTS_SZ (sizeof(synth_weights) / sizeof(int))

static int rend_flags[] = { BLOCK_RENDERING_FLAGS_LEGACY, BLOCK_RENDERING_FLAGS_FLAT, BLOCK_RENDERING_FLAGS_BOOK,
                            BLOCK_RENDERING_FLAGS_WEB };
#define MAX_REND_FLAGS_INDEX (sizeof(rend_flags) / sizeof(int))
static int DOM_versions[] = { 0, gDOMVersionCurrent };
#define MAX_DOM_VERSIONS_INDEX (sizeof(DOM_versions) / sizeof(int))

#if USE_FREETYPE == 1
static int aa_variants[] = { font_aa_none,    font_aa_gray,      font_aa_lcd_rgb,
                             font_aa_lcd_bgr, font_aa_lcd_v_rgb, font_aa_lcd_v_bgr };
#else
static int aa_variants[] = { font_aa_none, font_aa_gray };
#endif
#define AA_VARIANTS_SZ (sizeof(aa_variants) / sizeof(int))



static float font_gammas[] = { 0.30f, 0.35f, 0.40f, 0.45f, 0.50f, 0.55f, 0.60f, 0.65f, 0.70f, 0.75f, 0.80f,
                               0.85f, 0.90f, 0.95f, 0.98f, 1.00f, 1.02f, 1.05f, 1.10f, 1.15f, 1.20f, 1.25f,
                               1.30f, 1.35f, 1.40f, 1.45f, 1.50f, 1.60f, 1.70f, 1.80f, 1.90f, 2.00f, 
                               2.10f, 2.20f, 2.30f, 2.40f, 2.50f, 2.60f, 2.70f, 2.80f, 2.90f, 3.00f,
                               3.10f, 3.20f, 3.40f, 3.60f, 3.80f, 4.00f };
#define GAMMA_VARIANTS_SZ (sizeof(font_gammas) / sizeof(float))
static const int default_gamma_index = 15; // gamma 1.0

static bool initDone = false;
static bool suppressOnChange = false;

static void findImagesFromDirectory(lString32 dir, lString32Collection& files) {
    LVAppendPathDelimiter(dir);
    if (!LVDirectoryExists(dir))
        return;
    LVContainerRef cont = LVOpenDirectory(dir.c_str());
    if (!cont.isNull()) {
        for (int i = 0; i < cont->GetObjectCount(); i++) {
            const LVContainerItemInfo* item = cont->GetObjectInfo(i);
            if (!item->IsContainer()) {
                lString32 name = item->GetName();
                name.lowercase();
                if (name.endsWith(".png") || name.endsWith(".jpg") || name.endsWith(".gif") || name.endsWith(".jpeg")) {
                    files.add(dir + item->GetName());
                }
            }
        }
    }
}

static void findBackgrounds(lString32Collection& baseDirs, lString32Collection& files) {
    int i;
    for (i = 0; i < baseDirs.length(); i++) {
        lString32 baseDir = baseDirs[i];
        LVAppendPathDelimiter(baseDir);
        findImagesFromDirectory(baseDir + "backgrounds", files);
    }
    for (i = 0; i < baseDirs.length(); i++) {
        lString32 baseDir = baseDirs[i];
        LVAppendPathDelimiter(baseDir);
        findImagesFromDirectory(baseDir + "textures", files);
    }
}

static QString getWeightName(int weight) {
    QString name;
    switch (weight) {
        case 100:
            name = QString("Thin");
            break;
        case 200:
            name = QString("ExtraLight");
            break;
        case 300:
            name = QString("Light");
            break;
        case 350:
            name = QString("Book");
            break;
        case 400:
            name = QString("Regular");
            break;
        case 500:
            name = QString("Medium");
            break;
        case 600:
            name = QString("SemiBold");
            break;
        case 700:
            name = QString("Bold");
            break;
        case 800:
            name = QString("ExtraBold");
            break;
        case 900:
            name = QString("Heavy/Black");
            break;
        case 950:
            name = QString("ExtraBlack");
            break;
    }
    return name;
}

template <class T>
static inline T my_abs(T value) {
    return (value >= 0) ? value : -value;
}

// clang-format off
static const char* styleNames[] = {
    "def",
    "title",
    "subtitle",
    "pre",
    "link",
    "cite",
    "epigraph",
    "poem",
    "text-author",
    "footnote-link",
    "footnote",
    "footnote-title",
    "annotation",
    NULL
};
// clang-format on

SettingsDlg::SettingsDlg(QWidget* parent, PropsRef props, CR3View* docview)
        : QDialog(parent)
        , m_sample(NULL)
        , m_ui(new Ui::SettingsDlg)
        , m_docview(docview) {
    initDone = false;
    m_props = props;

    m_ui->setupUi(this);

    m_oldHyph = cr2qt(HyphMan::getSelectedDictionary()->getId());

    lString32Collection baseDirs;
    baseDirs.add(getConfigDir());
    baseDirs.add(getMainDataDir());
    lString32Collection bgFiles;
    QStringList bgFileLabels;
    findBackgrounds(baseDirs, bgFiles);
    int bgIndex = 0;
    m_backgroundFiles.append("[NONE]");
    bgFileLabels.append("[NONE]");
    QString bgFile = m_props->getStringDef(PROP_APP_BACKGROUND_IMAGE, "");
    for (int i = 0; i < bgFiles.length(); i++) {
        lString32 fn = bgFiles[i];
        QString f = cr2qt(fn);
        if (f == bgFile)
            bgIndex = i + 1;
        m_backgroundFiles.append(f);
        fn = LVExtractFilenameWithoutExtension(fn);
        bgFileLabels.append(cr2qt(fn));
    }
    m_ui->cbPageSkin->clear();
    m_ui->cbPageSkin->addItems(bgFileLabels);
    m_ui->cbPageSkin->setCurrentIndex(bgIndex);

    int shaping = m_props->getIntDef(PROP_FONT_SHAPING, 1);
#if USE_FREETYPE == 1 && USE_HARFBUZZ == 1
    m_ui->cbFontShaping->addItem(tr("Simple (FreeType only, fastest)"));
    m_ui->cbFontShaping->addItem(tr("Light (HarfBuzz without ligatures)"));
    m_ui->cbFontShaping->addItem(tr("Full (HarfBuzz with ligatures)"));
#else
    m_ui->cbFontShaping->setVisible(false);
    m_ui->label_44->setVisible(false);
#endif
    m_ui->cbFontShaping->setCurrentIndex(shaping);

#if USE_FREETYPE == 1
#else
    m_ui->cbFontKerning->setVisible(false);
    m_ui->label_43->setVisible(false);
    m_ui->cbFontHinting->setVisible(false);
    m_ui->label_35->setVisible(false);
    m_ui->btnFallbackMan->setVisible(false);
    m_ui->label_36->setVisible(false);
#endif
    
#if 1
    m_ui->cbTTInterpreter->setVisible(false);
    m_ui->label_55->setVisible(false);
#endif
    

    int aa = m_props->getIntDef(PROP_FONT_ANTIALIASING, (int)font_aa_gray);
    int aai = 1;
    for (int i = 0; i < AA_VARIANTS_SZ; i++) {
        if (aa == aa_variants[i]) {
            aai = i;
            break;
        }
    }
#if USE_FREETYPE == 1
    m_ui->cbAntialiasingMode->addItem(tr("None"));
    m_ui->cbAntialiasingMode->addItem(tr("Gray"));
    // DISABLED FOR CONVERTER PROJECT
    // m_ui->cbAntialiasingMode->addItem(tr("LCD (RGB)"));
    // m_ui->cbAntialiasingMode->addItem(tr("LCD (BGR)"));
    // m_ui->cbAntialiasingMode->addItem(tr("LCD (RGB) vertical"));
    // m_ui->cbAntialiasingMode->addItem(tr("LCD (BGR) vertical"));
#elif USE_WIN32_FONTS == 1 && USE_WIN32_DRAWFONTS == 0
    m_ui->cbAntialiasingMode->addItem(tr("None"));
    m_ui->cbAntialiasingMode->addItem(tr("Gray"));
#endif
    m_ui->cbAntialiasingMode->setCurrentIndex(aai);

    optionToUi(PROP_APP_WINDOW_FULLSCREEN, m_ui->cbWindowFullscreen);
    optionToUi(PROP_APP_WINDOW_SHOW_MENU, m_ui->cbWindowShowMenu);
    optionToUi(PROP_APP_WINDOW_SHOW_SCROLLBAR, m_ui->cbWindowShowScrollbar);
    optionToUi(PROP_APP_WINDOW_SHOW_TOOLBAR, m_ui->cbWindowShowToolbar);
    optionToUi(PROP_APP_WINDOW_SHOW_STATUSBAR, m_ui->cbWindowShowStatusBar);
    optionToUi(PROP_APP_TABS_FIXED_SIZE, m_ui->cbWindowFixedTabSize);
    optionToUi(PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY, m_ui->cbAutoClipboard);
    optionToUi(PROP_APP_SELECTION_AUTO_CMDEXEC, m_ui->cbAutoCmdExec);
    optionToUiLine(PROP_APP_SELECTION_COMMAND, m_ui->cbSelectionCommand);
    m_ui->cbSelectionCommand->setEnabled(m_props->getBoolDef(PROP_APP_SELECTION_AUTO_CMDEXEC, false));

    // Footnotes mode: 0=hidden, 1=page_bottom (default), 2=inline, 3=inline_block, 4=quote
    int footnotesMode = m_props->getIntDef(PROP_FOOTNOTES_MODE, 1);
    m_ui->cbFootnotesMode->setCurrentIndex(footnotesMode);
    optionToUi(PROP_SHOW_PAGE_NUMBER, m_ui->cbShowPageNumber);
    optionToUi(PROP_SHOW_PAGE_COUNT, m_ui->cbShowPageCount);
    optionToUi(PROP_SHOW_CHAPTER_NAME, m_ui->cbShowChapterName);
    optionToUi(PROP_SHOW_CHAPTER_PAGE_NUMBER, m_ui->cbShowChapterPageNumber);
    optionToUi(PROP_SHOW_CHAPTER_PAGE_COUNT, m_ui->cbShowChapterPageCount);
    optionToUi(PROP_SHOW_POS_PERCENT, m_ui->cbShowPositionPercent);
    optionToUi(PROP_SHOW_BATTERY, m_ui->cbShowBattery);
    optionToUi(PROP_SHOW_TIME, m_ui->cbShowClock);
    optionToUi(PROP_STATUS_NAVBAR, m_ui->cbShowNavbar);
    optionToUi(PROP_STATUS_CHAPTER_MARKS, m_ui->cbShowChapterMarks);
    optionToUi(PROP_SHOW_TITLE, m_ui->cbShowBookName);
    optionToUi(PROP_TXT_OPTION_PREFORMATTED, m_ui->cbTxtPreFormatted);
    optionToUi(PROP_EMBEDDED_STYLES, m_ui->cbEnableDocumentStyles);
    optionToUi(PROP_EMBEDDED_FONTS, m_ui->cbEnableEmbeddedFonts);
    m_ui->cbEnableEmbeddedFonts->setEnabled(m_props->getBoolDef(PROP_EMBEDDED_STYLES, true));
    optionToUi(PROP_FONT_KERNING_ENABLED, m_ui->cbFontKerning);
    optionToUi(PROP_FLOATING_PUNCTUATION, m_ui->cbFloatingPunctuation);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMIN_INLINE_MODE, m_ui->cbImageInlineZoominMode);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE, m_ui->cbImageInlineZoominScale);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE, m_ui->cbImageInlineZoomoutMode);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE, m_ui->cbImageInlineZoomoutScale);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE, m_ui->cbImageBlockZoominMode);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE, m_ui->cbImageBlockZoominScale);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE, m_ui->cbImageBlockZoomoutMode);
    optionToUiIndex(PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE, m_ui->cbImageBlockZoomoutScale);
    optionToUi(PROP_IMG_AUTO_ROTATE, m_ui->cbImageAutoRotate);

    QString gamma = m_props->getStringDef(PROP_FONT_GAMMA, "");
    if (gamma == "")
        m_props->setString(PROP_FONT_GAMMA, "1.0");
    QLocale locale = QLocale::system();
    for (int i = 0; i < GAMMA_VARIANTS_SZ; i++) {
        QString str = locale.toString(font_gammas[i], 'g', 3);
        m_ui->cbFontGamma->addItem(str);
    }
    optionToComboWithFloats(PROP_FONT_GAMMA, m_ui->cbFontGamma, default_gamma_index);

    QString ignoreDocMargins = m_props->getStringDef("styles.body.margin", "");
    m_ui->cbIgnoreDocumentMargins->setCheckState(ignoreDocMargins.length() > 0 ? Qt::Checked : Qt::Unchecked);

    optionToUiIndex(PROP_STATUS_LINE, m_ui->cbShowPageHeader);

    bool b = m_props->getIntDef(PROP_STATUS_LINE, 0) > 0;
    m_ui->cbShowPageNumber->setEnabled(b);
    m_ui->cbShowPageCount->setEnabled(b);
    m_ui->cbShowChapterName->setEnabled(b);
    m_ui->cbShowChapterPageNumber->setEnabled(b);
    m_ui->cbShowChapterPageCount->setEnabled(b);
    m_ui->cbShowPositionPercent->setEnabled(b);
    m_ui->cbShowBattery->setEnabled(b);
    m_ui->cbShowClock->setEnabled(b);
    m_ui->cbShowNavbar->setEnabled(b);
    m_ui->cbShowChapterMarks->setEnabled(b && m_props->getBoolDef(PROP_STATUS_NAVBAR, true));
    m_ui->cbShowBookName->setEnabled(b);

    // Header margin spinboxes
    m_ui->sbHeaderMarginH->setValue(m_props->getIntDef(PROP_PAGE_HEADER_MARGIN_H, 5));
    m_ui->sbHeaderMarginV->setValue(m_props->getIntDef(PROP_PAGE_HEADER_MARGIN_V, 3));
    m_ui->sbNavbarHeight->setValue(m_props->getIntDef(PROP_PAGE_HEADER_NAVBAR_H, 5));
    m_ui->sbHeaderMarginH->setEnabled(b);
    m_ui->sbHeaderMarginV->setEnabled(b);
    bool chapterMarksEnabled = b && m_props->getBoolDef(PROP_STATUS_NAVBAR, true) && m_props->getBoolDef(PROP_STATUS_CHAPTER_MARKS, true);
    m_ui->sbNavbarHeight->setEnabled(chapterMarksEnabled);
    m_ui->lblHeaderMarginH->setEnabled(b);
    m_ui->lblHeaderMarginV->setEnabled(b);
    m_ui->lblNavbarHeight->setEnabled(chapterMarksEnabled);

    m_ui->cbStartupAction->setCurrentIndex(m_props->getIntDef(PROP_APP_START_ACTION, 0));

    int lp = m_props->getIntDef(PROP_LANDSCAPE_PAGES, 2);
    int vm = m_props->getIntDef(PROP_PAGE_VIEW_MODE, 1);
    if (vm == 0)
        m_ui->cbViewMode->setCurrentIndex(2);
    else
        m_ui->cbViewMode->setCurrentIndex(lp == 1 ? 0 : 1);
    int hinting = m_props->getIntDef(PROP_FONT_HINTING, 2);
    m_ui->cbFontHinting->setCurrentIndex(hinting);
    int interpteter = m_props->getIntDef(PROP_FONT_INTERPRETER, 40);
    m_ui->cbTTInterpreter->model()->sort(0, Qt::DescendingOrder);
    m_ui->cbTTInterpreter->setCurrentIndex(interpteter == 40 ? 0 : interpteter == 38 ? 1 : 2);
    int highlight = m_props->getIntDef(PROP_HIGHLIGHT_COMMENT_BOOKMARKS, 1);
    m_ui->cbBookmarkHighlightMode->setCurrentIndex(highlight);

    int rendFlagsIndex = 0;
    int rendFlags = m_props->getIntDef(PROP_RENDER_BLOCK_RENDERING_FLAGS, BLOCK_RENDERING_FLAGS_WEB);
    for (int i = 0; i < MAX_REND_FLAGS_INDEX; i++) {
        if (rendFlags == rend_flags[i]) {
            rendFlagsIndex = i;
            break;
        }
    }
    m_ui->cbRendFlags->setCurrentIndex(rendFlagsIndex);

    int DOMVersionIndex = 0;
    int DOMVersion = m_props->getIntDef(PROP_REQUESTED_DOM_VERSION, gDOMVersionCurrent);
    for (int i = 0; i < MAX_DOM_VERSIONS_INDEX; i++) {
        if (DOMVersion == DOM_versions[i]) {
            DOMVersionIndex = i;
            break;
        }
    }
    m_ui->cbDOMLevel->setCurrentIndex(DOMVersionIndex);

    // Initialize separate margin controls
    auto setMarginComboIndex = [](QComboBox* cb, int marginValue) {
        int mi = 0;
        for (int i = 0; i < (int)MAX_MARGIN_INDEX; i++) {
            if (marginValue <= def_margins[i]) {
                mi = i;
                break;
            }
        }
        cb->setCurrentIndex(mi);
    };
    setMarginComboIndex(m_ui->cbMarginTop, m_props->getIntDef(PROP_PAGE_MARGIN_TOP, 8));
    setMarginComboIndex(m_ui->cbMarginBottom, m_props->getIntDef(PROP_PAGE_MARGIN_BOTTOM, 8));
    setMarginComboIndex(m_ui->cbMarginLeft, m_props->getIntDef(PROP_PAGE_MARGIN_LEFT, 8));
    setMarginComboIndex(m_ui->cbMarginRight, m_props->getIntDef(PROP_PAGE_MARGIN_RIGHT, 8));

    QStringList styles = QStyleFactory::keys();
    QString style = m_props->getStringDef(PROP_APP_WINDOW_STYLE, "");
    m_ui->cbLookAndFeel->addItems(styles);
    QStyle* s = QApplication::style();
    QString currStyle = s->objectName();
    CRLog::debug("Current system style is %s", currStyle.toUtf8().data());
    if (!styles.contains(style, Qt::CaseInsensitive)) {
#ifdef Q_OS_MACOS
        // Default to Fusion style on macOS for consistent cross-platform appearance
        style = styles.contains("Fusion", Qt::CaseInsensitive) ? "Fusion" : currStyle;
#else
        style = currStyle;
#endif
    }
    // Case-insensitive search (QStringList::indexOf doesn't support Qt::CaseInsensitive)
    int index = -1;
    for (int i = 0; i < styles.size(); i++) {
        if (styles[i].compare(style, Qt::CaseInsensitive) == 0) {
            index = i;
            break;
        }
    }
    if (index >= 0)
        m_ui->cbLookAndFeel->setCurrentIndex(index);

    crGetFontFaceList(m_faceList);
    crGetFontFaceListFiltered(m_monoFaceList, css_ff_monospace, "");
    m_ui->cbTextFontFace->addItems(m_faceList);
    m_ui->cbTitleFontFace->addItems(m_faceList);
    QStringList sizeList;
    LVArray<int> sizes(cr_font_sizes, sizeof(cr_font_sizes) / sizeof(int));
    for (int i = 0; i < sizes.length(); i++)
        sizeList.append(QString("%1").arg(sizes[i]));
    m_ui->cbTextFontSize->addItems(sizeList);
    m_ui->cbTitleFontSize->addItems(sizeList);

    m_defFontFace = "DejaVu Sans";
    static const char* goodFonts[] = { "DejaVu Sans", "Noto Sans", "FreeSans", "Liberation Sans", "Arial", NULL };
    for (int i = 0; goodFonts[i]; i++) {
        if (m_faceList.indexOf(QString(goodFonts[i])) >= 0) {
            m_defFontFace = goodFonts[i];
            break;
        }
    }

    fontToUi(PROP_FONT_FACE, PROP_FONT_SIZE, m_ui->cbTextFontFace, m_ui->cbTextFontSize,
             m_defFontFace.toLatin1().data());
    fontToUi(PROP_STATUS_FONT_FACE, PROP_STATUS_FONT_SIZE, m_ui->cbTitleFontFace, m_ui->cbTitleFontSize,
             m_defFontFace.toLatin1().data());

    updateFontWeights();

    //		{_("90%"), "90"},
    //		{_("100%"), "100"},
    //		{_("110%"), "110"},
    //		{_("120%"), "120"},
    //		{_("140%"), "140"},
    //PROP_INTERLINE_SPACE
    //PROP_HYPHENATION_DICT
    int interlineValue = m_props->getIntDef(PROP_INTERLINE_SPACE, 100);
    // Clamp to valid range
    if (interlineValue < 60)
        interlineValue = 60;
    if (interlineValue > 150)
        interlineValue = 150;
    m_ui->sbInterlineSpace->setValue(interlineValue);

    // PROP_FORMAT_SPACE_WIDTH_SCALE_PERCENT
    int spaceWidthValue = m_props->getIntDef(PROP_FORMAT_SPACE_WIDTH_SCALE_PERCENT, 100);
    // Clamp to valid range (10-500)
    if (spaceWidthValue < 10)
        spaceWidthValue = 10;
    if (spaceWidthValue > 500)
        spaceWidthValue = 500;
    m_ui->sbSpaceWidth->setValue(spaceWidthValue);

    // PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT
    int minSpaceWidthValue = m_props->getIntDef(PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT, 70);
    // Clamp to valid range (25-100)
    if (minSpaceWidthValue < 25)
        minSpaceWidthValue = 25;
    if (minSpaceWidthValue > 100)
        minSpaceWidthValue = 100;
    m_ui->sbMinSpaceWidth->setValue(minSpaceWidthValue);

    // PROP_FORMAT_MAX_ADDED_LETTER_SPACING_PERCENT
    int maxLetterSpacingValue = m_props->getIntDef(PROP_FORMAT_MAX_ADDED_LETTER_SPACING_PERCENT, 0);
    // Clamp to valid range (0-20)
    if (maxLetterSpacingValue < 0)
        maxLetterSpacingValue = 0;
    if (maxLetterSpacingValue > 20)
        maxLetterSpacingValue = 20;
    m_ui->sbMaxLetterSpacing->setValue(maxLetterSpacingValue);

    // PROP_FORMAT_UNUSED_SPACE_THRESHOLD_PERCENT
    int unusedSpaceThresholdValue = m_props->getIntDef(PROP_FORMAT_UNUSED_SPACE_THRESHOLD_PERCENT, 5);
    // Clamp to valid range (0-20)
    if (unusedSpaceThresholdValue < 0)
        unusedSpaceThresholdValue = 0;
    if (unusedSpaceThresholdValue > 20)
        unusedSpaceThresholdValue = 20;
    m_ui->sbUnusedSpaceThreshold->setValue(unusedSpaceThresholdValue);
    // Disable unused space threshold when max letter spacing is 0
    m_ui->sbUnusedSpaceThreshold->setEnabled(maxLetterSpacingValue > 0);

    optionToUi(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, m_ui->cbMultiLang);
    optionToUi(PROP_TEXTLANG_HYPHENATION_ENABLED, m_ui->cbEnableHyph);
    int hi = -1;
    QString v = m_props->getStringDef(PROP_HYPHENATION_DICT, "@algorithm"); //HYPH_DICT_ID_ALGORITHM;
    for (int i = 0; i < HyphMan::getDictList()->length(); i++) {
        HyphDictionary* item = HyphMan::getDictList()->get(i);
        if (v == cr2qt(item->getFilename()) || v == cr2qt(item->getId()))
            hi = i;
        QString s = cr2qt(item->getTitle());
        if (item->getType() == HDT_NONE)
            s = tr("[No hyphenation]");
        else if (item->getType() == HDT_ALGORITHM)
            s = tr("[Algorythmic hyphenation]");
        m_hyphDictIdList.append(cr2qt(item->getId()));
        m_ui->cbHyphenation->addItem(s);
    }
    m_ui->cbHyphenation->setCurrentIndex(hi >= 0 ? hi : 1);
    bool legacy_render = (0 == rendFlags) || (0 == DOMVersionIndex);
    m_ui->label_47->setVisible(!legacy_render);
    m_ui->cbMultiLang->setVisible(!legacy_render);
    m_ui->label_48->setVisible(!legacy_render);
    m_ui->cbEnableHyph->setVisible(!legacy_render);
    if (legacy_render) {
        m_ui->label_9->setVisible(true);
        m_ui->cbHyphenation->setVisible(true);
    } else {
        bool embedded_lang = m_props->getBoolDef(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, true);
        m_ui->label_9->setVisible(!embedded_lang);
        m_ui->cbHyphenation->setVisible(!embedded_lang);
        m_ui->label_48->setVisible(embedded_lang);
        m_ui->cbEnableHyph->setVisible(embedded_lang);
    }

    // Restore last selected tab (before sample window init)
    int lastTab = m_docview->loadSetting(PROP_APP_SETTINGS_TAB, 0);
    if (lastTab >= 0 && lastTab < m_ui->tabWidget->count())
        m_ui->tabWidget->setCurrentIndex(lastTab);

    // Only initialize sample window if on Styles tab (index 2)
    if (m_ui->tabWidget->currentIndex() == 2) {
        initSampleWindow();
        updateStyleSample();
    }

    m_styleNames.clear();
    m_styleNames.append(tr("Default paragraph style"));
    m_styleNames.append(tr("Title"));
    m_styleNames.append(tr("Subtitle"));
    m_styleNames.append(tr("Preformatted text"));
    m_styleNames.append(tr("Link"));
    m_styleNames.append(tr("Cite / quotation"));
    m_styleNames.append(tr("Epigraph"));
    m_styleNames.append(tr("Poem"));
    m_styleNames.append(tr("Text author"));
    m_styleNames.append(tr("Footnote link"));
    m_styleNames.append(tr("Footnote"));
    m_styleNames.append(tr("Footnote title"));
    m_styleNames.append(tr("Annotation"));
    m_ui->cbStyleName->clear();
    m_ui->cbStyleName->addItems(m_styleNames);
    int lastStyleElement = m_docview->loadSetting(PROP_APP_SETTINGS_STYLE_ELEMENT, 0);
    if (lastStyleElement < 0 || lastStyleElement >= m_ui->cbStyleName->count())
        lastStyleElement = 0;
    m_ui->cbStyleName->setCurrentIndex(lastStyleElement);
    initStyleControls(styleNames[lastStyleElement]);

    // Initialize "Use expanded CSS" checkbox (default to unchecked/false)
    bool useGenCSS = m_props->getBoolDef(PROP_CSS_USE_GENERATED, false);
    m_ui->cbUseGeneratedCSS->setCheckState(useGenCSS ? Qt::Checked : Qt::Unchecked);
    updateStyleControlsEnabled();

    // Restore dialog size if previously saved
    m_docview->restoreWindowPos(this, "settings.");

    initDone = true;

    //m_ui->cbPageSkin->addItem(QString("[None]"), QVariant());
}

SettingsDlg::~SettingsDlg() {
    // Save UI state (persists regardless of OK/Cancel)
    m_docview->saveSetting(PROP_APP_SETTINGS_TAB, m_ui->tabWidget->currentIndex());
    m_docview->saveSetting(PROP_APP_SETTINGS_STYLE_ELEMENT, m_ui->cbStyleName->currentIndex());

    // Save dialog size for next time
    m_docview->saveWindowPos(this, "settings.");
    delete m_ui;
}

void SettingsDlg::changeEvent(QEvent* e) {
    switch (e->type()) {
        case QEvent::LanguageChange:
            m_ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void SettingsDlg::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (m_sample) {
        // Only show sample on Styles tab (index 2)
        bool onStylesTab = (m_ui->tabWidget->currentIndex() == 2);
        m_sample->setVisible(onStylesTab);
        if (onStylesTab)
            m_sample->updatePositionForParent();
    }
}

void SettingsDlg::hideEvent(QHideEvent* event) {
    QDialog::hideEvent(event);
    if (m_sample)
        m_sample->setVisible(false);
}

void SettingsDlg::moveEvent(QMoveEvent* event) {
    QDialog::moveEvent(event);
    if (m_sample)
        m_sample->updatePositionForParent();
}

void SettingsDlg::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    if (m_sample)
        m_sample->updatePositionForParent();
}

void SettingsDlg::optionToUi(const char* optionName, QCheckBox* cb) {
    bool state = m_props->getBoolDef(optionName, true);
    CRLog::debug("optionToUI(%s,%d)", optionName, (int)state);
    cb->setCheckState(state ? Qt::Checked : Qt::Unchecked);
}

void SettingsDlg::optionToUiLine(const char* optionName, QLineEdit* le) {
    QString value = m_props->getStringDef(optionName, "");
    CRLog::debug("optionToUILine(%s,%s)", optionName, value.toUtf8().constData());
    le->setText(value);
}

void SettingsDlg::initStyleControls(const char* styleName) {
    m_styleName = styleName;
    QString prefix = QString("styles.") + styleName + ".";
    bool hideInherited = strcmp(styleName, "def") == 0;
    static const char* alignmentStyles[] = {
        "", // inherited
        "text-align: justify",
        "text-align: left",
        "text-align: center",
        "text-align: right",
        NULL,
    };

    QString alignmentStyleNames[] = {
        tr("-"), tr("Justify"), tr("Left"), tr("Center"), tr("Right"),
    };
    m_styleItemAlignment.init(prefix + "align", NULL, alignmentStyles, alignmentStyleNames, hideInherited, m_props,
                              m_ui->cbDefAlignment);

    static const char* indentStyles[] = {
        "", // inherited
        "text-indent: 0em",
        "text-indent: 0.5em",
        "text-indent: 1em",
        "text-indent: 1.2em",
        "text-indent: 2em",
        "text-indent: -1.2em",
        "text-indent: -2em",
        NULL,
    };

    QString indentStyleNames[] = {
        tr("-"), tr("No indent"), tr("Tiny Indent"), tr("Small Indent"), tr("Normal Indent"), tr("Big Indent"), tr("Small Outdent"), tr("Big Outdent"),
    };

    m_styleItemIndent.init(prefix + "text-indent", NULL, indentStyles, indentStyleNames, hideInherited, m_props,
                           m_ui->cbDefFirstLine);

    static const char* marginTopStyles[] = {
        "", // inherited
        "margin-top: 0em",
        "margin-top: 0.2em",
        "margin-top: 0.3em",
        "margin-top: 0.5em",
        "margin-top: 1em",
        "margin-top: 2em",
        NULL,
    };

    static const char* marginBottomStyles[] = {
        "", // inherited
        "margin-bottom: 0em",
        "margin-bottom: 0.2em",
        "margin-bottom: 0.3em",
        "margin-bottom: 0.5em",
        "margin-bottom: 1em",
        "margin-bottom: 2em",
        NULL,
    };

    QString marginTopBottomStyleNames[] = {
        tr("-"),
        tr("0"),
        tr("20% of line height"),
        tr("30% of line height"),
        tr("50% of line height"),
        tr("100% of line height"),
        tr("150% of line height"),
    };

    static const char* marginLeftStyles[] = {
        "", // inherited
        "margin-left: 0em",
        "margin-left: 0.5em",
        "margin-left: 1em",
        "margin-left: 1.5em",
        "margin-left: 2em",
        "margin-left: 4em",
        "margin-left: 10%",
        "margin-left: 15%",
        "margin-left: 20%",
        "margin-left: 30%",
        NULL,
    };

    static const char* marginRightStyles[] = {
        "", // inherited
        "margin-right: 0em",
        "margin-right: 0.5em",
        "margin-right: 1em",
        "margin-right: 1.5em",
        "margin-right: 2em",
        "margin-right: 4em",
        "margin-right: 5%",
        "margin-right: 10%",
        "margin-right: 15%",
        "margin-right: 20%",
        "margin-right: 30%",
        NULL,
    };

    QString marginLeftRightStyleNames[] = {
        tr("-"),
        tr("0"),
        tr("50% of line height"),
        tr("100% of line height"),
        tr("150% of line height"),
        tr("200% of line height"),
        tr("400% of line height"),
        tr("5% of line width"),
        tr("10% of line width"),
        tr("15% of line width"),
        tr("20% of line width"),
        tr("30% of line width"),
    };

    m_styleItemMarginBefore.init(prefix + "margin-top", NULL, marginTopStyles, marginTopBottomStyleNames, hideInherited,
                                 m_props, m_ui->cbDefMarginBefore);
    m_styleItemMarginAfter.init(prefix + "margin-bottom", NULL, marginBottomStyles, marginTopBottomStyleNames,
                                hideInherited, m_props, m_ui->cbDefMarginAfter);
    m_styleItemMarginLeft.init(prefix + "margin-left", NULL, marginLeftStyles, marginLeftRightStyleNames, false,
                               m_props, m_ui->cbDefMarginLeft);
    m_styleItemMarginRight.init(prefix + "margin-right", NULL, marginRightStyles, marginLeftRightStyleNames, false,
                                m_props, m_ui->cbDefMarginRight);

    // clang-format off
    static const char* fontWeightStyles[] = {
        "", // inherited
        "font-weight: normal",
        "font-weight: bold",
        "font-weight: bolder",
        "font-weight: lighter",
        NULL,
    };
    QString fontWeightStyleNames[] = {
        tr("-"),
        tr("Normal"),
        tr("Bold"),
        tr("Bolder"),
        tr("Lighter"),
    };
    // clang-format on
    m_styleFontWeight.init(prefix + "font-weight", NULL, fontWeightStyles, fontWeightStyleNames, false, m_props,
                           m_ui->cbDefFontWeight);

    static const char* fontSizeStyles[] = {
        "", // inherited
        "font-size: 110%",
        "font-size: 120%",
        "font-size: 150%",
        "font-size: 100%",
        "font-size: 90%",
        "font-size: 80%",
        "font-size: 70%",
        "font-size: 60%",
        NULL,
    };
    QString fontSizeStyleNames[] = {
        tr("-"),
        tr("Increase: 110%"),
        tr("Increase: 120%"),
        tr("Increase: 150%"),
        tr("Override: 100%"),
        tr("Decrease: 90%"),
        tr("Decrease: 80%"),
        tr("Decrease: 70%"),
        tr("Decrease: 60%"),
    };
    m_styleFontSize.init(prefix + "font-size", NULL, fontSizeStyles, fontSizeStyleNames, false, m_props,
                         m_ui->cbDefFontSize);

    static const char* fontStyleStyles[] = {
        "", // inherited
        "font-style: normal",
        "font-style: italic",
        NULL,
    };
    QString fontStyleStyleNames[] = {
        tr("-"),
        tr("Normal"),
        tr("Italic"),
    };
    m_styleFontStyle.init(prefix + "font-style", NULL, fontStyleStyles, fontStyleStyleNames, false, m_props,
                          m_ui->cbDefFontStyle);

    QStringList faces;
    QStringList faceValues;
    faces.append("-");
    faceValues.append("");
    faces.append(tr("[Default Sans Serif]"));
    faceValues.append("font-family: sans-serif");
    faces.append(tr("[Default Serif]"));
    faceValues.append("font-family: serif");
    faces.append(tr("[Default Monospace]"));
    faceValues.append("font-family: monospace");
    for (int i = 0; i < m_faceList.length(); i++) {
        QString face = m_faceList.at(i);
        faces.append(face);
        faceValues.append(QString("font-family: \"%1\";").arg(face));
    }
    m_styleFontFace.init(prefix + "font-face", faceValues, faces, m_props, m_ui->cbDefFontFace);

    // clang-format off
    static const char* fontColorStyles[] = {
        "", // inherited
        "color: black",
        "color: green",
        "color: silver",
        "color: lime",
        "color: gray",
        "color: olive",
        "color: white",
        "color: yellow",
        "color: maroon",
        "color: navy",
        "color: red",
        "color: blue",
        "color: purple",
        "color: teal",
        "color: fuchsia",
        "color: aqua",
        NULL,
    };
    QString fontColorStyleNames[] = {
        tr("-"),
        tr("Black"),
        tr("Green"),
        tr("Silver"),
        tr("Lime"),
        tr("Gray"),
        tr("Olive"),
        tr("White"),
        tr("Yellow"),
        tr("Maroon"),
        tr("Navy"),
        tr("Red"),
        tr("Blue"),
        tr("Purple"),
        tr("Teal"),
        tr("Fuchsia"),
        tr("Aqua"),
    };
    // clang-format on
    m_styleFontColor.init(prefix + "color", NULL, fontColorStyles, fontColorStyleNames, false, m_props,
                          m_ui->cbDefFontColor);

    // clang-format off
    static const char* lineHeightStyles[] = {
        "", // inherited
        "line-height: 75%",
        "line-height: 80%",
        "line-height: 85%",
        "line-height: 90%",
        "line-height: 95%",
        "line-height: 100%",
        "line-height: 110%",
        "line-height: 120%",
        "line-height: 130%",
        "line-height: 140%",
        "line-height: 150%",
        NULL,
    };
    QString lineHeightStyleNames[] = {
        "-",
        "75%",
        "80%",
        "85%",
        "90%",
        "95%",
        "100%",
        "110%",
        "120%",
        "130%",
        "140%",
        "150%",
    };
    // clang-format on
    m_styleLineHeight.init(prefix + "line-height", NULL, lineHeightStyles, lineHeightStyleNames, false, m_props,
                           m_ui->cbDefLineHeight);
    // clang-format off
    static const char* textDecorationStyles[] = {
        "", // inherited
        "text-decoration: none",
        "text-decoration: underline",
        "text-decoration: line-through",
        "text-decoration: overline",
        NULL,
    };
    QString textDecorationStyleNames[] = {
        tr("-"),
        tr("None"),
        tr("Underline"),
        tr("Line through"),
        tr("Overline"),
    };
    // clang-format on
    m_styleTextDecoration.init(prefix + "text-decoration", NULL, textDecorationStyles, textDecorationStyleNames, false,
                               m_props, m_ui->cbDefTextDecoration);

    static const char* verticalAlignStyles[] = {
        "", // inherited
        "vertical-align: baseline",
        "vertical-align: sub",
        "vertical-align: super",
        NULL,
    };
    QString verticalAlignStyleNames[] = {
        tr("-"),
        tr("Baseline"),
        tr("Subscript"),
        tr("Superscript"),
    };
    m_verticalAlignDecoration.init(prefix + "vertical-align", NULL, verticalAlignStyles, verticalAlignStyleNames, false,
                                   m_props, m_ui->cbDefVerticalAlign);
}

void SettingsDlg::optionToUiString(const char* optionName, QComboBox* cb) {
    QString value = m_props->getStringDef(optionName, "");
    int index = -1;
    for (int i = 0; i < cb->count(); i++) {
        if (cb->itemText(i) == value) {
            index = i;
            break;
        }
    }
    if (index < 0)
        index = 0;
    cb->setCurrentIndex(index);
}

void SettingsDlg::optionToUiIndex(const char* optionName, QComboBox* cb) {
    int value = m_props->getIntDef(optionName, 0);
    if (value < 0)
        value = 0;
    if (value >= cb->count())
        value = cb->count() - 1;
    cb->setCurrentIndex(value);
}

void SettingsDlg::optionToComboWithFloats(const char* optionName, QComboBox* cb, int defaultIdx) {
    QLocale locale = QLocale::system();
    QString str_value = m_props->getStringDef(optionName, "");
    bool parseRes;
    float value = locale.toFloat(str_value, &parseRes);
    if (!parseRes) {
        // parse using 'C' locale as fallback
        value = str_value.toFloat(&parseRes);
    }
    if (!parseRes)
        value = font_gammas[defaultIdx];
    int index = -1;
    for (int i = 0; i < cb->count(); i++) {
        float item_value = locale.toFloat(cb->itemText(i), &parseRes);
        if (parseRes) {
            if (my_abs(value - item_value) < 0.01f) {
                index = i;
                break;
            }
        }
    }
    if (index < 0)
        index = defaultIdx;
    cb->setCurrentIndex(index);
}

void SettingsDlg::optionToUiInversed(const char* optionName, QCheckBox* cb) {
    int state = (m_props->getIntDef(optionName, 1) != 0) ? 1 : 0;
    CRLog::debug("optionToUIInversed(%s,%d)", optionName, state);
    cb->setCheckState(!state ? Qt::Checked : Qt::Unchecked);
}

void SettingsDlg::setCheck(const char* optionName, int checkState) {
    int value = (checkState == Qt::Checked) ? 1 : 0;
    CRLog::debug("setCheck(%s,%d)", optionName, value);
    m_props->setInt(optionName, value);
}

void SettingsDlg::setCheckInversed(const char* optionName, int checkState) {
    int value = (checkState == Qt::Checked) ? 0 : 1;
    CRLog::debug("setCheckInversed(%s,%d)", optionName, value);
    m_props->setInt(optionName, value);
}

void SettingsDlg::on_cbFontKerning_stateChanged(int s) {
    setCheck(PROP_FONT_KERNING_ENABLED, s);
    updateStyleSample();
}

void SettingsDlg::on_cbWindowFullscreen_stateChanged(int s) {
    setCheck(PROP_APP_WINDOW_FULLSCREEN, s);
}

void SettingsDlg::on_cbWindowFixedTabSize_toggled(bool checked) {
    setCheck(PROP_APP_TABS_FIXED_SIZE, checked ? Qt::Checked : Qt::Unchecked);
}

void SettingsDlg::on_cbWindowShowToolbar_stateChanged(int s) {
    setCheck(PROP_APP_WINDOW_SHOW_TOOLBAR, s);
}

void SettingsDlg::on_cbWindowShowMenu_stateChanged(int s) {
    setCheck(PROP_APP_WINDOW_SHOW_MENU, s);
}

void SettingsDlg::on_cbWindowShowStatusBar_stateChanged(int s) {
    setCheck(PROP_APP_WINDOW_SHOW_STATUSBAR, s);
}

void SettingsDlg::on_cbWindowShowScrollbar_stateChanged(int s) {
    setCheck(PROP_APP_WINDOW_SHOW_SCROLLBAR, s);
}

void SettingsDlg::on_cbAutoClipboard_stateChanged(int s) {
    setCheck(PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY, s);
}

void SettingsDlg::on_cbAutoCmdExec_toggled(bool checked) {
    setCheck(PROP_APP_SELECTION_AUTO_CMDEXEC, checked ? Qt::Checked : Qt::Unchecked);
    m_ui->cbSelectionCommand->setEnabled(checked);
}

void SettingsDlg::on_cbSelectionCommand_textChanged(QString s) {
    m_props->setString(PROP_APP_SELECTION_COMMAND, s);
}

void SettingsDlg::on_cbViewMode_currentIndexChanged(int index) {
    switch (index) {
        case 0:
            m_props->setInt(PROP_LANDSCAPE_PAGES, 1);
            m_props->setInt(PROP_PAGE_VIEW_MODE, 1);
            break;
        case 1:
            m_props->setInt(PROP_LANDSCAPE_PAGES, 2);
            m_props->setInt(PROP_PAGE_VIEW_MODE, 1);
            break;
        default:
            m_props->setInt(PROP_PAGE_VIEW_MODE, 0);
            break;
    }
}

void SettingsDlg::on_cbShowPageHeader_currentIndexChanged(int index) {
    m_props->setInt(PROP_STATUS_LINE, index);
    bool b = index > 0;
    m_ui->cbShowPageNumber->setEnabled(b);
    m_ui->cbShowPageCount->setEnabled(b);
    m_ui->cbShowChapterName->setEnabled(b);
    m_ui->cbShowChapterPageNumber->setEnabled(b);
    m_ui->cbShowChapterPageCount->setEnabled(b);
    m_ui->cbShowPositionPercent->setEnabled(b);
    m_ui->cbShowBattery->setEnabled(b);
    m_ui->cbShowClock->setEnabled(b);
    m_ui->cbShowNavbar->setEnabled(b);
    m_ui->cbShowChapterMarks->setEnabled(b && m_ui->cbShowNavbar->isChecked());
    m_ui->cbShowBookName->setEnabled(b);
    // Header margin spinboxes
    m_ui->sbHeaderMarginH->setEnabled(b);
    m_ui->sbHeaderMarginV->setEnabled(b);
    bool chapterMarksEnabled = b && m_ui->cbShowNavbar->isChecked() && m_ui->cbShowChapterMarks->isChecked();
    m_ui->sbNavbarHeight->setEnabled(chapterMarksEnabled);
    m_ui->lblHeaderMarginH->setEnabled(b);
    m_ui->lblHeaderMarginV->setEnabled(b);
    m_ui->lblNavbarHeight->setEnabled(chapterMarksEnabled);
}

void SettingsDlg::on_cbShowNavbar_stateChanged(int s) {
    setCheck(PROP_STATUS_NAVBAR, s);
    m_ui->cbShowChapterMarks->setEnabled(s == Qt::Checked);
    bool chapterMarksEnabled = (s == Qt::Checked) && m_ui->cbShowChapterMarks->isChecked();
    m_ui->sbNavbarHeight->setEnabled(chapterMarksEnabled);
    m_ui->lblNavbarHeight->setEnabled(chapterMarksEnabled);
}

void SettingsDlg::on_cbShowChapterMarks_stateChanged(int s) {
    setCheck(PROP_STATUS_CHAPTER_MARKS, s);
    m_ui->sbNavbarHeight->setEnabled(s == Qt::Checked);
    m_ui->lblNavbarHeight->setEnabled(s == Qt::Checked);
}

void SettingsDlg::on_cbShowBookName_stateChanged(int s) {
    setCheck(PROP_SHOW_TITLE, s);
}

void SettingsDlg::on_cbShowPageNumber_stateChanged(int s) {
    setCheck(PROP_SHOW_PAGE_NUMBER, s);
}

void SettingsDlg::on_cbShowPageCount_stateChanged(int s) {
    setCheck(PROP_SHOW_PAGE_COUNT, s);
}

void SettingsDlg::on_cbShowChapterName_stateChanged(int s) {
    setCheck(PROP_SHOW_CHAPTER_NAME, s);
}

void SettingsDlg::on_cbShowChapterPageNumber_stateChanged(int s) {
    setCheck(PROP_SHOW_CHAPTER_PAGE_NUMBER, s);
}

void SettingsDlg::on_cbShowChapterPageCount_stateChanged(int s) {
    setCheck(PROP_SHOW_CHAPTER_PAGE_COUNT, s);
}

void SettingsDlg::on_cbShowPositionPercent_stateChanged(int s) {
    setCheck(PROP_SHOW_POS_PERCENT, s);
}

void SettingsDlg::on_cbShowClock_stateChanged(int s) {
    setCheck(PROP_SHOW_TIME, s);
}

void SettingsDlg::on_cbShowBattery_stateChanged(int s) {
    setCheck(PROP_SHOW_BATTERY, s);
}

void SettingsDlg::on_sbHeaderMarginH_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_PAGE_HEADER_MARGIN_H, value);
}

void SettingsDlg::on_sbHeaderMarginV_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_PAGE_HEADER_MARGIN_V, value);
}

void SettingsDlg::on_sbNavbarHeight_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_PAGE_HEADER_NAVBAR_H, value);
}

void SettingsDlg::on_cbFootnotesMode_currentIndexChanged(int index) {
    if (!initDone)
        return;
    m_props->setInt(PROP_FOOTNOTES_MODE, index);
}

void SettingsDlg::on_cbMarginTop_currentIndexChanged(int index) {
    if (!initDone)
        return;
    int m = def_margins[index];
    m_props->setInt(PROP_PAGE_MARGIN_TOP, m);
}

void SettingsDlg::on_cbMarginBottom_currentIndexChanged(int index) {
    if (!initDone)
        return;
    int m = def_margins[index];
    m_props->setInt(PROP_PAGE_MARGIN_BOTTOM, m);
}

void SettingsDlg::on_cbMarginLeft_currentIndexChanged(int index) {
    if (!initDone)
        return;
    int m = def_margins[index];
    m_props->setInt(PROP_PAGE_MARGIN_LEFT, m);
}

void SettingsDlg::on_cbMarginRight_currentIndexChanged(int index) {
    if (!initDone)
        return;
    int m = def_margins[index];
    m_props->setInt(PROP_PAGE_MARGIN_RIGHT, m);
}

void SettingsDlg::setBackground(QWidget* wnd, QColor cl) {
    QPalette pal(wnd->palette());
    pal.setColor(QPalette::Window, cl);
    pal.setColor(QPalette::Base, cl);
    wnd->setPalette(pal);
}

void SettingsDlg::initSampleWindow() {
    if (NULL == m_sample) {
        m_sample = new SampleView(this);
        connect(m_sample, SIGNAL(destroyed(QObject*)), this, SLOT(sampleview_destroyed(QObject*)));
    }
    CR3View* creview = m_sample->creview();
    creview->applyOptions(m_props, true);
    LVDocView* sampledocview = creview->getDocView();
    sampledocview->setShowCover(false);
    sampledocview->setViewMode(DVM_SCROLL, 1);

    // Try to get text from current document page, fall back to placeholder
    QString sampleText;
    if (m_docview && !m_docview->getDocView()->getFileName().empty()) {
        lString32 pageText = m_docview->getDocView()->getPageText(true);
        if (!pageText.empty()) {
            sampleText = cr2qt(pageText);
        }
    }
    if (sampleText.isEmpty()) {
        QString testPhrase = tr("The quick brown fox jumps over the lazy dog. ");
        sampleText = testPhrase + testPhrase + testPhrase + testPhrase + testPhrase;
    }
    sampledocview->createDefaultDocument(lString32::empty_str, qt2cr(sampleText));

    // Set main language for hyphenation to match the main document view
    lString32 mainLang;
    if (m_docview && !m_docview->getDocView()->getFileName().empty()) {
        mainLang = m_docview->getDocView()->getLanguage();
    }
    if (mainLang.empty()) {
        mainLang = U"en";
    }
    sampledocview->propApply(lString8(PROP_TEXTLANG_MAIN_LANG), mainLang);

    // Load CSS to match the main document view styling
    QString cssDir;
    if (m_docview && !m_docview->getCssDir().isEmpty()) {
        cssDir = m_docview->getCssDir();
    } else {
        cssDir = cr2qt(getMainDataDir()) + "/";
    }
    QString templateName = m_props->getStringDef(PROP_CSS_CURRENT_TEMPLATE, "fb2.css");
    bool useExpanded = m_props->getBoolDef(PROP_CSS_USE_GENERATED, false);
    QString cssPath = CR3View::resolveCssPath(cssDir, templateName, useExpanded);
    creview->loadCSS(cssPath);
}

void SettingsDlg::updateStyleSample() {
    QColor txtColor = getColor(PROP_FONT_COLOR, 0x000000);
    QColor bgColor = getColor(PROP_BACKGROUND_COLOR, 0xFFFFFF);
    QColor headerColor = getColor(PROP_STATUS_FONT_COLOR, 0xFFFFFF);
    QColor selectionColor = getColor(PROP_HIGHLIGHT_SELECTION_COLOR, 0xC0C0C0);
    QColor commentColor = getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xC0C000);
    QColor correctionColor = getColor(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xC00000);
    setBackground(m_ui->frmTextColor, txtColor);
    setBackground(m_ui->frmBgColor, bgColor);
    setBackground(m_ui->frmHeaderTextColor, headerColor);
    setBackground(m_ui->frmSelectionColor, selectionColor);
    setBackground(m_ui->frmCommentColor, commentColor);
    setBackground(m_ui->frmCorrectionColor, correctionColor);

    // Only create/update sample window when on Styles tab (index 2)
    if (m_ui->tabWidget->currentIndex() != 2)
        return;

    if (NULL == m_sample) {
        initSampleWindow();
        m_sample->show();
    }
    CR3View* sample = m_sample->creview();
    sample->applyOptions(m_props, true);
    sample->getDocView()->setShowCover(false);
    sample->getDocView()->setViewMode(DVM_SCROLL, 1);

    sample->getDocView()->getPageImage(0);

    if (!m_props->getBoolDef(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, true))
        HyphMan::getDictList()->activate(qt2cr(m_oldHyph));
}

void SettingsDlg::sampleview_destroyed(QObject* obj) {
    disconnect(m_sample, SIGNAL(destroyed(QObject*)));
    m_sample = NULL;
}

QColor SettingsDlg::getColor(const char* optionName, unsigned def) {
    lvColor cr(m_props->getColorDef(optionName, def));
    return QColor(cr.r(), cr.g(), cr.b());
}

void SettingsDlg::setColor(const char* optionName, QColor cl) {
    m_props->setHex(optionName, lvColor(cl.red(), cl.green(), cl.blue()).get());
}

void SettingsDlg::colorDialog(const char* optionName, QString title) {
    QColorDialog dlg;
    dlg.setWindowTitle(title);
    dlg.setCurrentColor(getColor(optionName, 0x000000));
    if (dlg.exec() == QDialog::Accepted) {
        setColor(optionName, dlg.currentColor());
        updateStyleSample();
    }
}

void SettingsDlg::on_btnTextColor_clicked() {
    colorDialog(PROP_FONT_COLOR, tr("Text color"));
}

void SettingsDlg::on_btnBgColor_clicked() {
    colorDialog(PROP_BACKGROUND_COLOR, tr("Background color"));
}

void SettingsDlg::on_btnHeaderTextColor_clicked() {
    colorDialog(PROP_STATUS_FONT_COLOR, tr("Page header text color"));
}

void SettingsDlg::on_cbLookAndFeel_currentTextChanged(QString styleName) {
    if (!initDone)
        return;
    CRLog::debug("on_cbLookAndFeel_currentIndexChanged(%s)", styleName.toUtf8().data());
    m_props->setString(PROP_APP_WINDOW_STYLE, styleName);
}

void SettingsDlg::on_cbTitleFontFace_currentTextChanged(QString s) {
    if (!initDone)
        return;
    m_props->setString(PROP_STATUS_FONT_FACE, s);
}

void SettingsDlg::on_cbTitleFontSize_currentTextChanged(QString s) {
    if (!initDone)
        return;
    m_props->setString(PROP_STATUS_FONT_SIZE, s);
}

void SettingsDlg::on_cbTextFontFace_currentTextChanged(QString s) {
    if (!initDone)
        return;
    m_props->setString(PROP_FONT_FACE, s);
    updateFontWeights();
    updateStyleSample();
}

void SettingsDlg::on_cbTextFontSize_currentTextChanged(QString s) {
    if (!initDone)
        return;
    m_props->setString(PROP_FONT_SIZE, s);
    updateStyleSample();
}

void SettingsDlg::fontToUi(const char* faceOptionName, const char* sizeOptionName, QComboBox* faceCombo,
                           QComboBox* sizeCombo, const char* defFontFace) {
    QString faceName = m_props->getStringDef(faceOptionName, defFontFace);
    int faceIndex = faceCombo->findText(faceName);
    if (faceIndex >= 0)
        faceCombo->setCurrentIndex(faceIndex);
    if (sizeCombo) {
        QString sizeName = m_props->getStringDef(sizeOptionName, sizeCombo->itemText(4).toUtf8().data());
        int sizeIndex = sizeCombo->findText(sizeName);
        if (sizeIndex >= 0)
            sizeCombo->setCurrentIndex(sizeIndex);
    }
}

void SettingsDlg::updateFontWeights() {
    QString selectedFontFace = m_ui->cbTextFontFace->currentText();
    lString8 face8 = UnicodeToUtf8(qt2cr(selectedFontFace));
    int i;
    int weight = 0;
    LVArray<int> weights;
    LVArray<int> nativeWeights;
    fontMan->GetAvailableFontWeights(nativeWeights, face8);
#if USE_FREETYPE == 1
    if (nativeWeights.length() > 0) {
        // combine with synthetic weights
        int synth_idx = 0;
        int j;
        int prev_weight = 0;
        for (i = 0; i < nativeWeights.length(); i++) {
            weight = nativeWeights[i];
            for (j = synth_idx; j < SYNTH_WEIGHTS_SZ; j++) {
                int synth_weight = synth_weights[j];
                if (synth_weight < weight) {
                    if (synth_weight > prev_weight)
                        weights.add(synth_weight);
                } else
                    break;
            }
            synth_idx = j;
            weights.add(weight);
            prev_weight = weight;
        }
        for (j = synth_idx; j < SYNTH_WEIGHTS_SZ; j++) {
            if (synth_weights[j] > weight)
                weights.add(synth_weights[j]);
        }
    }
#else
    weights.add(nativeWeights);
#endif
    // fill items
    suppressOnChange = true;
    int normalIndex = 0;
    m_ui->cbFontWeightChange->clear();
    if (weights.empty()) {
        // Invalid font
        return;
    }
    for (i = 0; i < weights.length(); i++) {
        weight = weights[i];
        if (400 == weight)
            normalIndex = i;
        QString label = QString("%1").arg(weight);
        QString descr = getWeightName(weight);
        if (nativeWeights.indexOf(weight) < 0) {
            if (!descr.isEmpty())
                descr += QString(", %1").arg(tr("synthetic*"));
            else
                descr = tr("synthetic");
        }
        if (!descr.isEmpty())
            label = QString("%1 (%2)").arg(label).arg(descr);
        m_ui->cbFontWeightChange->addItem(label, QVariant(weight));
    }
    // select active
    int fontWeightIndex = normalIndex;
    int fontWeight = m_props->getIntDef(PROP_FONT_BASE_WEIGHT, 400);
    for (i = 0; i < weights.length(); i++) {
        if (fontWeight == weights[i]) {
            fontWeightIndex = i;
            break;
        }
    }
    suppressOnChange = false;
    m_ui->cbFontWeightChange->setCurrentIndex(fontWeightIndex);
}

void SettingsDlg::on_sbInterlineSpace_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_INTERLINE_SPACE, value);
    updateStyleSample();
}

void SettingsDlg::on_sbSpaceWidth_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_FORMAT_SPACE_WIDTH_SCALE_PERCENT, value);
    updateStyleSample();
}

void SettingsDlg::on_sbMinSpaceWidth_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT, value);
    updateStyleSample();
}

void SettingsDlg::on_sbUnusedSpaceThreshold_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_FORMAT_UNUSED_SPACE_THRESHOLD_PERCENT, value);
    updateStyleSample();
}

void SettingsDlg::on_sbMaxLetterSpacing_valueChanged(int value) {
    if (!initDone)
        return;
    m_props->setInt(PROP_FORMAT_MAX_ADDED_LETTER_SPACING_PERCENT, value);
    // Disable unused space threshold when max letter spacing is 0
    m_ui->sbUnusedSpaceThreshold->setEnabled(value > 0);
    updateStyleSample();
}

void SettingsDlg::on_cbHyphenation_currentIndexChanged(int index) {
    QString s = m_hyphDictIdList[index < m_hyphDictIdList.count() ? index : 1];
    m_props->setString(PROP_HYPHENATION_DICT, s);
    updateStyleSample();
}

void SettingsDlg::on_cbStartupAction_currentIndexChanged(int index) {
    m_props->setInt(PROP_APP_START_ACTION, index);
}

void SettingsDlg::on_cbTxtPreFormatted_toggled(bool checked) { }

void SettingsDlg::on_cbTxtPreFormatted_stateChanged(int s) {
    setCheck(PROP_TXT_OPTION_PREFORMATTED, s);
}

void SettingsDlg::on_cbPageSkin_currentIndexChanged(int index) {
    if (!initDone)
        return;
    if (index >= 0 && index < m_backgroundFiles.length())
        m_props->setString(PROP_APP_BACKGROUND_IMAGE, m_backgroundFiles[index]);
}

void SettingsDlg::on_cbFloatingPunctuation_stateChanged(int s) {
    setCheck(PROP_FLOATING_PUNCTUATION, s);
    updateStyleSample();
}

void SettingsDlg::on_cbFontGamma_currentTextChanged(QString s) {
    if (!initDone)
        return;
    bool parseRes;
    // Parse string as float using system locale
    QLocale locale = QLocale::system();
    float value = locale.toFloat(s, &parseRes);
    if (!parseRes) {
        // Parse using 'C' locale as fallback
        value = s.toFloat(&parseRes);
    }
    if (parseRes) {
        // Convert float to string using 'C' locale
        m_props->setString(PROP_FONT_GAMMA, QString::number(value, 'g', 3));
    }
    updateStyleSample();
}

void SettingsDlg::on_cbStyleName_currentIndexChanged(int index) {
    if (index >= 0 && initDone)
        initStyleControls(styleNames[index]);
}

void SettingsDlg::on_cbDefAlignment_currentIndexChanged(int index) {
    m_styleItemAlignment.update(index);
}

void SettingsDlg::on_cbDefFirstLine_currentIndexChanged(int index) {
    m_styleItemIndent.update(index);
}

void SettingsDlg::on_cbDefMarginBefore_currentIndexChanged(int index) {
    m_styleItemMarginBefore.update(index);
}

void SettingsDlg::on_cbDefMarginAfter_currentIndexChanged(int index) {
    m_styleItemMarginAfter.update(index);
}

void SettingsDlg::on_cbDefMarginLeft_currentIndexChanged(int index) {
    m_styleItemMarginLeft.update(index);
}

void SettingsDlg::on_cbDefMarginRight_currentIndexChanged(int index) {
    m_styleItemMarginRight.update(index);
}

void SettingsDlg::on_cbDefFontSize_currentIndexChanged(int index) {
    m_styleFontSize.update(index);
}

void SettingsDlg::on_cbDefFontFace_currentIndexChanged(int index) {
    m_styleFontFace.update(index);
}

void SettingsDlg::on_cbDefFontWeight_currentIndexChanged(int index) {
    m_styleFontWeight.update(index);
}

void SettingsDlg::on_cbDefFontStyle_currentIndexChanged(int index) {
    m_styleFontStyle.update(index);
}

void SettingsDlg::on_cbDefFontColor_currentIndexChanged(int index) {
    m_styleFontColor.update(index);
}

void SettingsDlg::on_cbDefTextDecoration_currentIndexChanged(int index) {
    m_styleTextDecoration.update(index);
}

void SettingsDlg::on_cbDefVerticalAlign_currentIndexChanged(int index) {
    m_verticalAlignDecoration.update(index);
}

void SettingsDlg::on_cbDefLineHeight_currentIndexChanged(int index) {
    m_styleLineHeight.update(index);
}

void SettingsDlg::on_cbTTInterpreter_currentIndexChanged(int index) {
    m_props->setInt(PROP_FONT_INTERPRETER, index == 0 ? 40 : index == 1 ? 38 : 35);
    updateStyleSample();
}
void SettingsDlg::on_cbFontHinting_currentIndexChanged(int index) {
    m_props->setInt(PROP_FONT_HINTING, index);
    updateStyleSample();
}

void SettingsDlg::on_cbEnableEmbeddedFonts_toggled(bool checked) {
    setCheck(PROP_EMBEDDED_FONTS, checked ? Qt::Checked : Qt::Unchecked);
}

void SettingsDlg::on_cbIgnoreDocumentMargins_stateChanged(int s) {
    QString value = (s == Qt::Checked) ? "margin: 0em !important" : "";
    m_props->setString("styles.body.margin", value);
}

void SettingsDlg::on_cbEnableDocumentStyles_toggled(bool checked) {
    setCheck(PROP_EMBEDDED_STYLES, checked ? Qt::Checked : Qt::Unchecked);
    m_ui->cbEnableEmbeddedFonts->setEnabled(checked);
    m_ui->cbIgnoreDocumentMargins->setEnabled(checked);
}

void SettingsDlg::on_btnSelectionColor_clicked() {
    colorDialog(PROP_HIGHLIGHT_SELECTION_COLOR, tr("Selection color"));
}

void SettingsDlg::on_btnCommentColor_clicked() {
    colorDialog(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, tr("Comment bookmark color"));
}

void SettingsDlg::on_btnCorrectionColor_clicked() {
    colorDialog(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, tr("Correction bookmark color"));
}

void SettingsDlg::on_cbBookmarkHighlightMode_currentIndexChanged(int index) {
    if (!initDone)
        return;
    m_props->setInt(PROP_HIGHLIGHT_COMMENT_BOOKMARKS, index);
}

void SettingsDlg::on_cbImageInlineZoominMode_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMIN_INLINE_MODE, index);
}

void SettingsDlg::on_cbImageInlineZoominScale_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE, index);
}

void SettingsDlg::on_cbImageInlineZoomoutMode_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE, index);
}

void SettingsDlg::on_cbImageInlineZoomoutScale_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE, index);
}

void SettingsDlg::on_cbImageBlockZoominMode_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE, index);
}

void SettingsDlg::on_cbImageBlockZoominScale_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE, index);
}

void SettingsDlg::on_cbImageBlockZoomoutMode_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE, index);
}

void SettingsDlg::on_cbImageBlockZoomoutScale_currentIndexChanged(int index) {
    m_props->setInt(PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE, index);
}

void SettingsDlg::on_cbImageAutoRotate_stateChanged(int s) {
    setCheck(PROP_IMG_AUTO_ROTATE, s);
}

void SettingsDlg::on_cbFontShaping_currentIndexChanged(int index) {
    m_props->setInt(PROP_FONT_SHAPING, index);
    updateStyleSample();
}

void SettingsDlg::on_cbRendFlags_currentIndexChanged(int index) {
    if (index < 0 || index >= MAX_REND_FLAGS_INDEX)
        index = 0;
    m_props->setInt(PROP_RENDER_BLOCK_RENDERING_FLAGS, rend_flags[index]);
    int DOMVersion = m_props->getIntDef(PROP_REQUESTED_DOM_VERSION, gDOMVersionCurrent);
    bool legacy_render = (0 == index) || (DOMVersion < 20180524);
    m_ui->label_47->setVisible(!legacy_render);
    m_ui->cbMultiLang->setVisible(!legacy_render);
    m_ui->label_48->setVisible(!legacy_render);
    m_ui->cbEnableHyph->setVisible(!legacy_render);
    if (legacy_render) {
        m_ui->label_9->setVisible(true);
        m_ui->cbHyphenation->setVisible(true);
    } else {
        bool embedded_lang = m_props->getBoolDef(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, true);
        m_ui->label_9->setVisible(!embedded_lang);
        m_ui->cbHyphenation->setVisible(!embedded_lang);
        m_ui->label_48->setVisible(embedded_lang);
        m_ui->cbEnableHyph->setVisible(embedded_lang);
    }
    updateStyleSample();
}

void SettingsDlg::on_cbDOMLevel_currentIndexChanged(int index) {
    if (index < 0 || index >= MAX_DOM_VERSIONS_INDEX)
        index = 0;
    int rendFlags = m_props->getIntDef(PROP_RENDER_BLOCK_RENDERING_FLAGS, 0);
    bool legacy_render = (0 == rendFlags) || (0 == index);
    m_ui->label_47->setVisible(!legacy_render);
    m_ui->cbMultiLang->setVisible(!legacy_render);
    m_ui->label_48->setVisible(!legacy_render);
    m_ui->cbEnableHyph->setVisible(!legacy_render);
    if (legacy_render) {
        m_ui->label_9->setVisible(true);
        m_ui->cbHyphenation->setVisible(true);
    } else {
        bool embedded_lang = m_props->getBoolDef(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, true);
        m_ui->label_9->setVisible(!embedded_lang);
        m_ui->cbHyphenation->setVisible(!embedded_lang);
        m_ui->label_48->setVisible(embedded_lang);
        m_ui->cbEnableHyph->setVisible(embedded_lang);
    }
    m_props->setInt(PROP_REQUESTED_DOM_VERSION, DOM_versions[index]);
    updateStyleSample();
}

void SettingsDlg::on_cbMultiLang_stateChanged(int state) {
    setCheck(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, state);
    m_ui->label_9->setVisible(state != Qt::Checked);
    m_ui->cbHyphenation->setVisible(state != Qt::Checked);
    m_ui->label_48->setVisible(state == Qt::Checked);
    m_ui->cbEnableHyph->setVisible(state == Qt::Checked);
    // don't update preview to not change static field TextLangMan::_embedded_langs_enabled too early!
}

void SettingsDlg::on_cbEnableHyph_stateChanged(int state) {
    setCheck(PROP_TEXTLANG_HYPHENATION_ENABLED, state);
    updateStyleSample();
}

void SettingsDlg::on_btnFallbackMan_clicked() {
    FallbackFontsDialog dlg(this, m_faceList);
    QString fallbackFaces = m_props->getStringDef(PROP_FALLBACK_FONT_FACES, m_defFontFace.toLatin1().data());
    dlg.setFallbackFaces(fallbackFaces);
    if (dlg.exec() == QDialog::Accepted) {
        fallbackFaces = dlg.fallbackFaces();
        m_props->setString(PROP_FALLBACK_FONT_FACES, fallbackFaces);
        updateStyleSample();
    }
}

void SettingsDlg::on_btnFontFamiliesMan_clicked() {
    QString serifFace = m_props->getStringDef(PROP_GENERIC_SERIF_FONT_FACE, m_defFontFace.toLatin1().data());
    QString sansSerifFace = m_props->getStringDef(PROP_GENERIC_SANS_SERIF_FONT_FACE, m_defFontFace.toLatin1().data());
    QString cursiveFace = m_props->getStringDef(PROP_GENERIC_CURSIVE_FONT_FACE, m_defFontFace.toLatin1().data());
    QString fantasyFace = m_props->getStringDef(PROP_GENERIC_FANTASY_FONT_FACE, m_defFontFace.toLatin1().data());
    QString monospaceFace = m_props->getStringDef(PROP_GENERIC_MONOSPACE_FONT_FACE, m_defFontFace.toLatin1().data());
    FontFamiliesDialog dlg(this);
    dlg.setAvailableFaces(m_faceList);
    dlg.setAvailableMonoFaces(m_monoFaceList);
    dlg.setSerifFontFace(serifFace);
    dlg.setSansSerifFontFace(sansSerifFace);
    dlg.setCursiveFontFace(cursiveFace);
    dlg.setFantasyFontFace(fantasyFace);
    dlg.setMonospaceFontFace(monospaceFace);
    if (dlg.exec() == QDialog::Accepted) {
        serifFace = dlg.serifFontFace();
        sansSerifFace = dlg.sansSerifFontFace();
        cursiveFace = dlg.cursiveFontFace();
        fantasyFace = dlg.fantasyFontFace();
        monospaceFace = dlg.monospaceFontFace();
        m_props->setString(PROP_GENERIC_SERIF_FONT_FACE, serifFace);
        m_props->setString(PROP_GENERIC_SANS_SERIF_FONT_FACE, sansSerifFace);
        m_props->setString(PROP_GENERIC_CURSIVE_FONT_FACE, cursiveFace);
        m_props->setString(PROP_GENERIC_FANTASY_FONT_FACE, fantasyFace);
        m_props->setString(PROP_GENERIC_MONOSPACE_FONT_FACE, monospaceFace);
    }
}

void SettingsDlg::on_cbFontWeightChange_currentIndexChanged(int index) {
    if (suppressOnChange)
        return;
    QVariant data = m_ui->cbFontWeightChange->currentData(Qt::UserRole);
    int weight = data.toInt();
    m_props->setInt(PROP_FONT_BASE_WEIGHT, weight);
    QString face = m_props->getStringDef(PROP_FONT_FACE, "");
    LVArray<int> nativeWeights;
    fontMan->GetAvailableFontWeights(nativeWeights, UnicodeToUtf8(qt2cr(face)));
    //m_ui->cbFontHinting->setEnabled(nativeWeights.indexOf(weight) >= 0);
    updateStyleSample();
}

void SettingsDlg::on_cbAntialiasingMode_currentIndexChanged(int index) {
    if (index < 0 || index >= AA_VARIANTS_SZ)
        index = 0;
    int aa = aa_variants[index];
    m_props->setInt(PROP_FONT_ANTIALIASING, aa);
    updateStyleSample();
}

void SettingsDlg::on_tabWidget_currentChanged(int index) {
    if (!initDone)
        return;
    // Show/hide sample window based on tab (Styles tab is index 2)
    bool onStylesTab = (index == 2);
    if (onStylesTab) {
        if (!m_sample) {
            initSampleWindow();
            updateStyleSample();
        }
        m_sample->setVisible(true);
        m_sample->updatePositionForParent();
    } else if (m_sample) {
        m_sample->setVisible(false);
    }
}

void SettingsDlg::updateStyleControlsEnabled() {
    bool useGenerated = m_props->getBoolDef(PROP_CSS_USE_GENERATED, false);
    // Disable style controls when using expanded CSS (the Expand button remains enabled)
    // Disabling scrollArea automatically disables all child widgets (the cbDef* combo boxes)
    m_ui->cbStyleName->setEnabled(!useGenerated);
    m_ui->scrollArea->setEnabled(!useGenerated);
}

QString SettingsDlg::expandCssTemplate(const QString& templateName) {
    // Read templates from app data directory
    QString dataDir = cr2qt(getMainDataDir());
    // Write expanded files to user config directory (same location as crui.ini)
    QString configDir = cr2qt(getConfigDir());

    QString baseName = templateName;
    if (baseName.endsWith(".css"))
        baseName.chop(4);

    QString templatePath = dataDir + "/" + templateName;
    QString outputFile = baseName + CSS_EXPANDED_SUFFIX;
    QString outputPath = configDir + outputFile;

    lString32 inPath = qt2cr(templatePath);
    lString32 outPath = qt2cr(outputPath);
    CRPropRef styleProps = qt2cr(m_props);

    if (saveExpandedCSS(inPath.c_str(), outPath.c_str(), styleProps)) {
        return outputPath;
    }
    return QString();
}

void SettingsDlg::on_cbUseGeneratedCSS_toggled(bool checked) {
    if (!initDone)
        return;

    // If enabling expanded CSS, ensure the expanded file exists for the current template
    if (checked) {
        QString dataDir = cr2qt(getMainDataDir()) + "/";
        QString currentTemplate = m_props->getStringDef(PROP_CSS_CURRENT_TEMPLATE, "fb2.css");

        // Check if expanded CSS already exists using resolveCssPath
        QString resolvedPath = CR3View::resolveCssPath(dataDir, currentTemplate, true);
        bool expandedExists = resolvedPath.endsWith(CSS_EXPANDED_SUFFIX);

        // If expanded CSS file doesn't exist, generate it silently
        if (!expandedExists) {
            expandCssTemplate(currentTemplate);
        }
    }

    setCheck(PROP_CSS_USE_GENERATED, checked ? Qt::Checked : Qt::Unchecked);
    updateStyleControlsEnabled();
    // Reload document to apply CSS change immediately
    if (m_docview) {
        m_docview->applyOptions(m_props, true);
        m_docview->reloadCurrentDocument();
    }
}

void SettingsDlg::on_btnGenerateCSS_clicked() {
    // Get CSS templates from application data path
    QString dataDir = cr2qt(getMainDataDir());
    QString configDir = cr2qt(getConfigDir());

    // Get list of available CSS template files
    QStringList cssFilters;
    cssFilters << "*.css";
    QDir cssDir(dataDir);
    QStringList cssFiles = cssDir.entryList(cssFilters, QDir::Files);

    // Filter out already-expanded files
    QStringList templateFiles;
    for (const QString& file : cssFiles) {
        if (!file.endsWith(CSS_EXPANDED_SUFFIX)) {
            templateFiles.append(file);
        }
    }

    if (templateFiles.isEmpty()) {
        QMessageBox::warning(this, tr("Expand CSS"),
            tr("No CSS template files found in data directory."));
        return;
    }

    // Find default selection: current document's CSS style, fallback to fb2.css
    QString currentTemplate = m_props->getStringDef(PROP_CSS_CURRENT_TEMPLATE, "fb2.css");
    int defaultIndex = templateFiles.indexOf(currentTemplate);
    if (defaultIndex < 0) {
        defaultIndex = templateFiles.indexOf("fb2.css");
        if (defaultIndex < 0)
            defaultIndex = 0;
    }

    // Let user select which template to expand
    bool ok;
    QString selectedFile = QInputDialog::getItem(this, tr("Expand CSS"),
        tr("Select CSS template to expand:"), templateFiles, defaultIndex, false, &ok);

    if (!ok || selectedFile.isEmpty())
        return;

    // Generate output filename (will be written to config dir)
    QString baseName = selectedFile;
    if (baseName.endsWith(".css"))
        baseName.chop(4);
    QString outputFile = baseName + CSS_EXPANDED_SUFFIX;
    QString outputPath = configDir + outputFile;

    // Check if file exists in config directory
    if (QFile::exists(outputPath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Expand CSS"),
            tr("File '%1' already exists. Overwrite?").arg(outputPath),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes)
            return;
    }

    // Generate expanded CSS using helper
    QString result = expandCssTemplate(selectedFile);

    if (!result.isEmpty()) {
        QMessageBox::information(this, tr("Expand CSS"),
            tr("Successfully created '%1'").arg(result));
    } else {
        QMessageBox::critical(this, tr("Expand CSS"),
            tr("Failed to expand CSS file. Check that the template exists and output path is writable."));
    }
}

void SettingsDlg::on_btnExpandAllCSS_clicked() {
    // Get CSS templates from application data path
    QString dataDir = cr2qt(getMainDataDir());
    QString configDir = cr2qt(getConfigDir());

    // Get list of available CSS template files
    QStringList cssFilters;
    cssFilters << "*.css";
    QDir cssDir(dataDir);
    QStringList cssFiles = cssDir.entryList(cssFilters, QDir::Files);

    // Filter out already-expanded files
    QStringList templateFiles;
    for (const QString& file : cssFiles) {
        if (!file.endsWith(CSS_EXPANDED_SUFFIX)) {
            templateFiles.append(file);
        }
    }

    if (templateFiles.isEmpty()) {
        QMessageBox::warning(this, tr("Expand All CSS"),
            tr("No CSS template files found in data directory."));
        return;
    }

    // Ask confirmation once for all files
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Expand All CSS"),
        tr("This will create/overwrite expanded CSS files for %1 templates in:\n%2\n\nContinue?")
            .arg(templateFiles.size())
            .arg(configDir),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    // Expand all templates using helper
    for (const QString& templateFile : templateFiles) {
        QString result = expandCssTemplate(templateFile);
        if (!result.isEmpty()) {
            successCount++;
        } else {
            failCount++;
            failedFiles.append(templateFile);
        }
    }

    // Show result
    if (failCount == 0) {
        QMessageBox::information(this, tr("Expand All CSS"),
            tr("Successfully created %1 expanded CSS files.").arg(successCount));
    } else {
        QMessageBox::warning(this, tr("Expand All CSS"),
            tr("Created %1 files, failed %2 files:\n%3")
                .arg(successCount)
                .arg(failCount)
                .arg(failedFiles.join(", ")));
    }
}
