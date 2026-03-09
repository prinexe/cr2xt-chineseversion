/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2009-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 phi <crispyfrog@163.com>                           *
 *   Copyright (C) 2018 EXL <exlmotodev@gmail.com>                         *
 *   Copyright (C) 2019,2020 Konstantin Potapov <pkbo@users.sourceforge.net>
 *   Copyright (C) 2020,2021 Andy Mender <andymenderunix@gmail.com>        *
 *   Copyright (C) 2023 Ren Tatsumoto <tatsu@autistici.org>                *
 *   Copyright (C) 2026 Harvey Duong <hduongd@gmail.com>                   *
 *   Copyright (C) 2018-2024,2026 Aleksey Chernov <valexlin@gmail.com>     *
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
#include "crqtutil.h"
#include "qpainter.h"
#include "app-props.h"
#include "addbookmarkdlg.h"

#include <lvpagesplitter.h>

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QResizeEvent>
#include <QtGui/QScreen>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#else
#include <QtGui/QResizeEvent>
#include <QtGui/QScrollBar>
#include <QtGui/QMenu>
#include <QtGui/QStyleFactory>
#include <QtGui/QStyle>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#endif
#include <QClipboard>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QDesktopServices>
#include <QProcess>
#include <QTimer>

#include <lvdocview.h>
#include <crtrace.h>
#include <crprops.h>

#include <math.h>

/// to hide non-qt implementation, place all crengine-related fields here
class CR3View::DocViewData
{
    friend class CR3View;
    lString32 _settingsFileName;
    lString32 _historyFileName;
    CRPropRef _props;
};

#if USE_LIMITED_FONT_SIZES_SET
DECL_DEF_CR_FONT_SIZES;
#endif

static void replaceColor(char* str, lUInt32 color) {
    // in line like "0 c #80000000",
    // replace value of color
    for (int i = 0; i < 8; i++) {
        str[i + 5] = toHexDigit((color >> 28) & 0xF);
        color <<= 4;
    }
}

static LVRefVec<LVImageSource> cr_battery_icons;

static LVRefVec<LVImageSource>& getBatteryIcons(lUInt32 color) {
    CRLog::debug("Making list of Battery icon bitmats");
    lUInt32 cl1 = 0x00000000 | (color & 0xFFFFFF);
    lUInt32 cl2 = 0x40000000 | (color & 0xFFFFFF);
    lUInt32 cl3 = 0x80000000 | (color & 0xFFFFFF);
    lUInt32 cl4 = 0xF0000000 | (color & 0xFFFFFF);

    static char color1[] = "0 c #80000000";
    static char color2[] = "X c #80000000";
    static char color3[] = "o c #80AAAAAA";
    static char color4[] = ". c #80FFFFFF";
    // clang-format off

    #define BATTERY_HEADER \
            "28 15 5 1", \
            color1, \
            color2, \
            color3, \
            color4, \
            "  c None",

    static const char * battery8[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0000.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0000.XXXX.XXXX.XXXX.XXXX.0.",
        "....0.XXXX.XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery7[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.oooo.XXXX.XXXX.XXXX.0.",
        ".0000.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0000.oooo.XXXX.XXXX.XXXX.0.",
        "....0.oooo.XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery6[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0......XXXX.XXXX.XXXX.0.",
        ".0000......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0000......XXXX.XXXX.XXXX.0.",
        "....0......XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery5[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0......oooo.XXXX.XXXX.0.",
        ".0000......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0000......oooo.XXXX.XXXX.0.",
        "....0......oooo.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery4[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0...........XXXX.XXXX.0.",
        ".0000...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0000...........XXXX.XXXX.0.",
        "....0...........XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery3[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0...........oooo.XXXX.0.",
        ".0000...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0000...........oooo.XXXX.0.",
        "....0...........oooo.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery2[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0................XXXX.0.",
        ".0000................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0000................XXXX.0.",
        "....0................XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery1[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "   .0................oooo.0.",
        ".0000................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0000................oooo.0.",
        "   .0................oooo.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery0[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "   .0.....................0.",
        ".0000.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0000.....................0.",
        "....0.....................0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };

    static const char * battery_charge[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.....................0.",
        ".0000............XX.......0.",
        ".0..0...........XXXX......0.",
        ".0..0..XX......XXXXXX.....0.",
        ".0..0...XXX...XXXX..XX....0.",
        ".0..0....XXX..XXXX...XX...0.",
        ".0..0.....XXXXXXX.....XX..0.",
        ".0000.......XXXX..........0.",
        "....0........XX...........0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery_frame[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.....................0.",
        ".0000.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0000.....................0.",
        "....0.....................0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };

    const char** icon_bpm[] = {
        battery_charge,
        battery0,
        battery1,
        battery2,
        battery3,
        battery4,
        battery5,
        battery6,
        battery7,
        battery8,
        battery_frame,
        NULL
    };
    // clang-format on

    replaceColor(color1, cl1);
    replaceColor(color2, cl2);
    replaceColor(color3, cl3);
    replaceColor(color4, cl4);

    cr_battery_icons.clear();
    for (int i = 0; icon_bpm[i]; i++)
        cr_battery_icons.add(LVCreateXPMImageSource(icon_bpm[i]));

    return cr_battery_icons;
}

CR3View::CR3View(QWidget* parent)
        : QWidget(parent, Qt::WindowFlags())
        , _scroll(NULL)
        , _propsCallback(NULL)
        , _docViewStatusCallback(NULL)
        , _normalCursor(Qt::ArrowCursor)
        , _linkCursor(Qt::PointingHandCursor)
        , _selCursor(Qt::IBeamCursor)
        , _waitCursor(Qt::WaitCursor)
        , _selecting(false)
        , _selected(false)
        , _doubleClick(false)
        , _active(false)
        , _editMode(false)
        , _exportMode(false)
        , _historyEnabled(true)
        , _lastBatteryState(CR_BATTERY_STATE_NO_BATTERY)
        , _lastBatteryChargingConn(CR_BATTERY_CHARGER_NO)
        , _lastBatteryChargeLevel(0)
        , _canGoBack(false)
        , _canGoForward(false)
        , _onTextSelectAutoClipboardCopy(false)
        , _onTextSelectAutoCmdExec(false)
        , _wheelIntegralDegrees(0)
        , _cssFileWatcher(nullptr) {
    _dpr = 1.0;
#if WORD_SELECTOR_ENABLED == 1
    _wordSelector = NULL;
#endif
    _resizeTimer = new QTimer(this);
    _resizeTimer->setSingleShot(true);
    _data = new DocViewData();
    _data->_props = LVCreatePropsContainer();
    _docview = new LVDocView();
    _docview->setCallback(this);
    _selStart = ldomXPointer();
    _selEnd = ldomXPointer();
    _selText.clear();
    ldomXPointerEx p1;
    ldomXPointerEx p2;
    _selRange.setStart(p1);
    _selRange.setEnd(p2);
#if USE_LIMITED_FONT_SIZES_SET
    LVArray<int> sizes(cr_font_sizes, sizeof(cr_font_sizes) / sizeof(int));
    _docview->setFontSizes(sizes, false);
#endif
    _clipboardSupportsMouseSelection = QApplication::clipboard()->supportsSelection();
    if (_clipboardSupportsMouseSelection)
        CRLog::debug("Clipboard supports global mouse selection");
    else
        CRLog::debug("Clipboard NOT supports global mouse selection");

    _docview->setBatteryIcons(getBatteryIcons(0x000000));
    _docview->setBatteryState(CR_BATTERY_STATE_NO_BATTERY, CR_BATTERY_CHARGER_NO,
                              0); // don't show battery
    //_docview->setBatteryState( 75 ); // 75%
    //    LVStreamRef stream;
    //    stream = LVOpenFileStream("/home/lve/.cr3/textures/old_paper.png", LVOM_READ);
    //stream = LVOpenFileStream("/home/lve/.cr3/textures/tx_wood.jpg", LVOM_READ);
    //stream = LVOpenFileStream("/home/lve/.cr3/backgrounds/Background1.jpg", LVOM_READ);
    //    if ( !stream.isNull() ) {
    //        LVImageSourceRef img = LVCreateStreamCopyImageSource(stream);
    //        if ( !img.isNull() ) {
    //            //img = LVCreateUnpackedImageSource(img, 1256*1256*4, false);
    //            _docview->setBackgroundImage(img, true);
    //        }
    //    }
    lString32 decimalPoint = qt2cr(locale().decimalPoint());
    if (!decimalPoint.empty())
        _docview->setDecimalPointChar(decimalPoint[0]);
    updateDefProps();
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(SCREEN_SIZE_MIN, SCREEN_SIZE_MIN);

    connect(_resizeTimer, SIGNAL(timeout()), this, SLOT(resizeTimerTimeout()));
}

void CR3View::updateDefProps() {
    _data->_props->setStringDef(PROP_APP_START_ACTION, "0");
    _data->_props->setStringDef(PROP_APP_WINDOW_SHOW_MENU, "1");
    _data->_props->setStringDef(PROP_APP_WINDOW_SHOW_SCROLLBAR, "1");
    _data->_props->setStringDef(PROP_APP_WINDOW_SHOW_TOOLBAR, "1");
    _data->_props->setStringDef(PROP_APP_WINDOW_SHOW_STATUSBAR, "0");
    _data->_props->setStringDef(PROP_APP_TABS_FIXED_SIZE, "1");
    _data->_props->setStringDef(PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY, "0");
    _data->_props->setStringDef(PROP_APP_SELECTION_AUTO_CMDEXEC, "0");
    _data->_props->setStringDef(PROP_APP_WINDOW_FULLSCREEN, "0");

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen* screen = this->screen();
#else
    QScreen* screen = QGuiApplication::primaryScreen();
#endif
    lString32 str;
    int d = (int)screen->logicalDotsPerInch();
    // special workaround for MacOS
    if (72 == d)
        d = 96;
    str.appendDecimal(d);
    _data->_props->setString(PROP_RENDER_DPI, str);
    // But we don't apply PROP_RENDER_SCALE_FONT_WITH_DPI property,
    // since for now we are setting the font size in pixels.
#endif

    QStringList styles = QStyleFactory::keys();
    QStyle* s = QApplication::style();
    QString currStyle = s->objectName();
    CRLog::debug("Current system style is %s", currStyle.toUtf8().data());
#ifdef Q_OS_MACOS
    // Default to Fusion style on macOS for consistent cross-platform appearance
    QString defaultStyle = styles.contains("Fusion", Qt::CaseInsensitive) ? "Fusion" : currStyle;
#else
    QString defaultStyle = currStyle;
#endif
    QString style = cr2qt(_data->_props->getStringDef(PROP_APP_WINDOW_STYLE, defaultStyle.toUtf8().data()));
    if (!styles.contains(style, Qt::CaseInsensitive)) {
        style = defaultStyle;
        _data->_props->setString(PROP_APP_WINDOW_STYLE, qt2cr(defaultStyle));
    }
    // Apply the style at startup
    if (style.compare(currStyle, Qt::CaseInsensitive) != 0) {
        CRLog::info("Applying window style: %s", style.toUtf8().data());
        QApplication::setStyle(style);
    }
}

CR3View::~CR3View() {
#if WORD_SELECTOR_ENABLED == 1
    if (_wordSelector)
        delete _wordSelector;
#endif
    delete _cssFileWatcher;
    _docview->savePosition();
    delete _docview;
    delete _data;
}

#if WORD_SELECTOR_ENABLED == 1
void CR3View::startWordSelection() {
    if (isWordSelection())
        endWordSelection();
    _wordSelector = new LVPageWordSelector(_docview);
    update();
}

QString CR3View::endWordSelection() {
    QString text;
    if (isWordSelection()) {
        ldomWordEx* word = _wordSelector->getSelectedWord();
        if (word)
            text = cr2qt(word->getText());
        delete _wordSelector;
        _wordSelector = NULL;
        _docview->clearSelection();
        update();
    }
    return text;
}
#endif

void CR3View::setHyphDir(const QString& dirname, bool clear) {
    HyphMan::initDictionaries(qt2cr(dirname), clear);
    _hyphDicts.clear();
    for (int i = 0; i < HyphMan::getDictList()->length(); i++) {
        HyphDictionary* item = HyphMan::getDictList()->get(i);
        QString fn = cr2qt(item->getId()); //item->getFilename() );
        _hyphDicts.append(fn);
    }
    //_hyphDicts.sort();
}

const QStringList& CR3View::getHyphDicts() {
    return _hyphDicts;
}

LVTocItem* CR3View::getToc() {
    return _docview->getToc();
}

/// go to position specified by xPointer string
void CR3View::goToXPointer(const QString& xPointer, bool saveToHistory) {
    ldomXPointer p = _docview->getDocument()->createXPointer(qt2cr(xPointer));
    if (saveToHistory)
        _docview->savePosToNavigationHistory();
    //if ( _docview->getViewMode() == DVM_SCROLL ) {
    doCommand(DCMD_GO_POS, p.toPoint().y);
    //} else {
    //    doCommand( DCMD_GO_PAGE, item->getPage() );
    //}
}

/// returns current page
int CR3View::getCurPage() {
    return _docview->getCurPage();
}

/// returns base page number (0 if cover exists, 1 otherwise)
int CR3View::getBasePage() {
    return ::getBasePage(_docview->getPageList());
}

void CR3View::setDocumentText(const QString& text) {
    if (_active) {
        setDocumentTextImpl(text);
    } else {
        _delayed_commands.push_back(CR3ViewCommand(CR3ViewCommandType::SetDocumentText, text));
    }
}

bool CR3View::loadLastDocument() {
    CRFileHist* hist = _docview->getHistory();
    if (!hist || hist->getRecords().length() <= 0)
        return false;
    return loadDocument(cr2qt(hist->getRecords()[0]->getFilePathName()));
}

bool CR3View::loadDocument(const QString& fileName) {
    if (_active) {
        return LoadDocumentImpl(fileName, false);
    } else {
        _delayed_commands.push_back(CR3ViewCommand(CR3ViewCommandType::OpenDocument, fileName));
        return true;
    }
}

void CR3View::wheelEvent(QWheelEvent* event) {
    // Get degrees delta from vertical scrolling
    int px = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    int numDegrees = event->angleDelta().y() / 8;
#if MACOS == 1
    px = event->pixelDelta().y();
#endif
#else
    int numDegrees = event->delta() / 8;
#endif
    int numSteps = numDegrees / 15;

    if (_docview->getViewMode() == DVM_SCROLL) {
        if (0 == numSteps) {
            if (0 == px) {
                // 15 degrees -> 1 line
                // x degrees -> x/15 lines
                px = (_docview->getAvgTextLineHeight() * numDegrees) / 15;
            }
            if (0 == px && 0 != numDegrees)
                px = numDegrees > 0 ? 1 : -1;
            if (0 != px)
                scrollBy(-px);
        } else {
            if (numSteps > 0)
                doCommand(DCMD_LINEDOWN, -numSteps);
            else
                doCommand(DCMD_LINEUP, numSteps);
        }
    } else {
        if (0 == numSteps) {
#if MACOS == 1
            if (0 != numDegrees)
                numSteps = numDegrees > 0 ? 1 : -1;
#else
            _wheelIntegralDegrees += numDegrees;
            if (_wheelIntegralDegrees > 0)
                numSteps = (_wheelIntegralDegrees + 3) / 15;
            else
                numSteps = (_wheelIntegralDegrees - 3) / 15;
#endif
        }
        if (0 != numSteps) {
            if (numSteps > 0)
                doCommand(DCMD_PAGEUP, -numSteps);
            else
                doCommand(DCMD_PAGEDOWN, numSteps);
            _wheelIntegralDegrees = 0;
        }
    }
    event->accept();
}

void CR3View::resizeEvent(QResizeEvent* event) {
    if (_exportMode)
        return;  // Ignore resize during export
    if (_active) {
        _resizeTimer->stop();
        _resizeTimer->start(100);
    } else {
        _delayed_commands.push_back(CR3ViewCommand(CR3ViewCommandType::Resize, event->size()));
    }
}

static bool getBatteryState(int& state, int& chargingConn, int& level) {
#ifdef _WIN32
    // update battery state
    SYSTEM_POWER_STATUS bstatus;
    BOOL pow = GetSystemPowerStatus(&bstatus);
    if (pow) {
        state = CR_BATTERY_STATE_DISCHARGING;
        if (bstatus.BatteryFlag & 128)
            state = CR_BATTERY_STATE_NO_BATTERY; // no system battery
        else if (bstatus.BatteryFlag & 8)
            state = CR_BATTERY_STATE_CHARGING; // charging
        chargingConn = CR_BATTERY_CHARGER_NO;
        if (bstatus.ACLineStatus == 1)
            chargingConn = CR_BATTERY_CHARGER_AC; // AC power charging connected
        if (bstatus.BatteryLifePercent >= 0 && bstatus.BatteryLifePercent <= 100)
            level = bstatus.BatteryLifePercent;
        return true;
    }
    return false;
#else
    state = CR_BATTERY_STATE_NO_BATTERY;
    chargingConn = CR_BATTERY_CHARGER_NO;
    level = 0;
    return true;
#endif
}

void CR3View::paintEvent(QPaintEvent* event) {
    if (!_active)
        return;
    if (_exportMode) {
        // Draw cached frame during export
        if (!_exportModeFrame.isNull()) {
            QPainter painter(this);
            painter.drawPixmap(0, 0, _exportModeFrame);
        }
        return;
    }
    // When running in a Wayland session, `QWidget::devicePixelRatio()`
    //  may return an incorrect value before the window contents are drawn
    //  when using non-integer screen scaling.
    //  This problem does not occur when using X11.
    // As a universal solution, update the `dpr` value directly in the paintEvent() function
    //  using the `QPaintDevice::devicePixelRatioF()` function.
    QPainter painter(this);
    qreal dpr = painter.device()->devicePixelRatioF();
    if (fabs(dpr - _dpr) >= 0.01) {
        // DPR changed (e.g., window moved to different monitor, or first paint with actual DPR)
        // Cache new dpr value and trigger proper resize through timer
        CRLog::debug("DPR changed: %.3f -> %.3f, triggering resize", _dpr, dpr);
        _dpr = dpr;
        // Use timer to properly resize docview with correct widget dimensions
        _resizeTimer->stop();
        _resizeTimer->start(0);  // Immediate trigger
        return;  // Skip this paint, will repaint after resize
    }
    int newBatteryState;
    int newChargingConn;
    int newChargeLevel;
    if (getBatteryState(newBatteryState, newChargingConn, newChargeLevel) &&
        (_lastBatteryState != newBatteryState || _lastBatteryChargingConn != newChargingConn ||
         _lastBatteryChargeLevel != newChargeLevel)) {
        _docview->setBatteryState(newBatteryState, newChargingConn, newChargeLevel);
        _lastBatteryState = newBatteryState;
        _lastBatteryChargingConn = newChargingConn;
        _lastBatteryChargeLevel = newChargeLevel;
    }
    applyTextLangMainLang(_doc_language);
    LVDocImageRef ref = _docview->getPageImage(0);
    if (ref.isNull()) {
        //painter.fillRect();
        return;
    }
    LVDrawBuf* buf = ref->getDrawBuf();
    int dx = buf->GetWidth();
    int dy = buf->GetHeight();
    int bpp = buf->GetBitsPerPixel();
    if (bpp == 4 || bpp == 3) {
        QImage img(dx, dy, QImage::Format_RGB16);
        img.setDevicePixelRatio(_dpr);
        for (int i = 0; i < dy; i++) {
            unsigned char* dst = img.scanLine(i);
            unsigned char* src = buf->GetScanLine(i);
            for (int x = 0; x < dx; x++) {
                lUInt16 cl = *src; //(*src << (8 - bpp)) & 0xF8;
                cl = (cl << 8) | (cl << 3) | (cl >> 3);
                src++;
                *dst++ = (cl & 255);
                *dst++ = ((cl >> 8) & 255);
                //                *dst++ = *src++;
                //                *dst++ = 0xFF;
                //                src++;
            }
        }
        painter.drawImage(0, 0, img);
    } else if (bpp == 2) {
        QImage img(dx, dy, QImage::Format_RGB16);
        img.setDevicePixelRatio(_dpr);
        for (int i = 0; i < dy; i++) {
            unsigned char* dst = img.scanLine(i);
            unsigned char* src = buf->GetScanLine(i);
            int shift = 0;
            for (int x = 0; x < dx; x++) {
                lUInt16 cl = *src; //(*src << (8 - bpp)) & 0xF8;
                lUInt16 cl2 = (cl << shift) & 0xC0;
                cl2 = cl2 | (cl2 >> 2);
                cl2 = cl2 | (cl2 >> 4);
                cl2 = ((cl2 << 8) & 0xF800) | ((cl2 << 3) & 0x07E0) | (cl2 >> 3);
                if ((x & 3) == 3) {
                    src++;
                    shift = 0;
                } else {
                    shift += 2;
                }
                *dst++ = (cl2 & 255);
                *dst++ = ((cl2 >> 8) & 255);
                //                *dst++ = *src++;
                //                *dst++ = 0xFF;
                //                src++;
            }
        }
        painter.drawImage(0, 0, img);
    } else if (bpp == 1) {
        QImage img(dx, dy, QImage::Format_RGB16);
        img.setDevicePixelRatio(_dpr);
        for (int i = 0; i < dy; i++) {
            unsigned char* dst = img.scanLine(i);
            unsigned char* src = buf->GetScanLine(i);
            int shift = 0;
            for (int x = 0; x < dx; x++) {
                lUInt16 cl = *src; //(*src << (8 - bpp)) & 0xF8;
                lUInt16 cl2 = (cl << shift) & 0x80;
                cl2 = cl2 ? 0xffff : 0x0000;
                if ((x & 7) == 7) {
                    src++;
                    shift = 0;
                } else {
                    shift++;
                }
                *dst++ = (cl2 & 255);
                *dst++ = ((cl2 >> 8) & 255);
                //                *dst++ = *src++;
                //                *dst++ = 0xFF;
                //                src++;
            }
        }
        painter.drawImage(0, 0, img);
    } else if (bpp == 16) {
        QImage img(dx, dy, QImage::Format_RGB16);
        img.setDevicePixelRatio(_dpr);
        for (int i = 0; i < dy; i++) {
            unsigned char* dst = img.scanLine(i);
            unsigned char* src = buf->GetScanLine(i);
            for (int x = 0; x < dx; x++) {
                *dst++ = *src++;
                *dst++ = *src++;
                //                *dst++ = *src++;
                //                *dst++ = 0xFF;
                //                src++;
            }
        }
        painter.drawImage(0, 0, img);
    } else if (bpp == 32) {
        QImage img(dx, dy, QImage::Format_RGB32);
        img.setDevicePixelRatio(_dpr);
        for (int i = 0; i < dy; i++) {
            unsigned char* dst = img.scanLine(i);
            unsigned char* src = buf->GetScanLine(i);
            for (int x = 0; x < dx; x++) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = 0xFF;
                src++;
            }
        }
        painter.drawImage(0, 0, img);
    }
    if (_editMode) {
        // draw caret
        lvRect cursorRc;
        if (_docview->getCursorRect(cursorRc, false)) {
            if (cursorRc.left < 0)
                cursorRc.left = 0;
            if (cursorRc.top < 0)
                cursorRc.top = 0;
            if (cursorRc.right > dx)
                cursorRc.right = dx;
            if (cursorRc.bottom > dy)
                cursorRc.bottom = dy;
            if (!cursorRc.isEmpty()) {
                painter.setPen(QColor(255, 255, 255));
                painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
                //QPainter::RasterOp_SourceXorDestination;
                //QPainter::CompositionMode_Xor;
                //painter.setBrush(
                painter.drawRect(cursorRc.left, cursorRc.top, cursorRc.width(), cursorRc.height());
            }
        }
    }
    updateScroll();
}

void CR3View::updateScroll() {
    if (_scroll != NULL) {
        // TODO: set scroll range
        const LVScrollInfo* si = _docview->getScrollInfo();
        //bool changed = false;

        // change value before max scroll to avoid a ValueChange event
        if (_scroll->value() > si->maxpos)
            _scroll->setValue(si->pos);
        if (si->maxpos != _scroll->maximum()) {
            _scroll->setMaximum(si->maxpos);
            _scroll->setMinimum(0);
            //changed = true;
        }
        if (si->pagesize != _scroll->pageStep()) {
            _scroll->setPageStep(si->pagesize);
            //changed = true;
        }
        if (si->pos != _scroll->value()) {
            _scroll->setValue(si->pos);
            //changed = true;
        }
    }
}

void CR3View::scrollTo(int value) {
    int currPos = _docview->getScrollInfo()->pos;
    if (currPos != value && value >= 0) {
        doCommand(DCMD_GO_SCROLL_POS, value);
    }
}

void CR3View::scrollBy(int px) {
    _docview->SetPos(_docview->GetPos() + px, true, false);
    update();
}

void CR3View::nextSentence() {
    // for debugging of ReadAloud feature position movement.
    doCommand(DCMD_SELECT_NEXT_SENTENCE, 0);
    update();
}

void CR3View::prevSentence() {
    // for debugging of ReadAloud feature position movement.
    doCommand(DCMD_SELECT_PREV_SENTENCE, 0);
    update();
}

void CR3View::doCommand(int cmd, int param) {
    _docview->doCommand((LVDocCmd)cmd, param);
    update();
    updateHistoryAvailability();
}

void CR3View::togglePageScrollView() {
    if (_editMode)
        return;
    doCommand(DCMD_TOGGLE_PAGE_SCROLL_VIEW, 1);
    refreshPropFromView(PROP_PAGE_VIEW_MODE);
}

void CR3View::setEditMode(bool flgEdit) {
    if (_editMode == flgEdit)
        return;

    if (flgEdit && _data->_props->getIntDef(PROP_PAGE_VIEW_MODE, 0))
        togglePageScrollView();
    _editMode = flgEdit;
    update();
}

void CR3View::setExportMode(bool enabled) {
    if (enabled) {
        _resizeTimer->stop();  // Stop any pending resize
        // Capture current frame BEFORE setting _exportMode
        // (grab() triggers paintEvent internally, which needs _exportMode == false to render)
        _exportModeFrame = grab();
        _exportMode = true;
    } else {
        _exportMode = false;
        // Clear cached frame when leaving export mode
        _exportModeFrame = QPixmap();
    }
}

QString CR3View::getDocTitle() const {
    if (NULL != _docview) {
        lString32 atitle;
        lString32 author = _docview->getAuthors();
        lString32 title = _docview->getTitle();
        if (!title.empty()) {
            if (!author.empty())
                atitle = author + cs32(". ") + title;
            else
                atitle = title;
        } else
            atitle = _filename;
        return cr2qt(atitle);
    }
    return QString();
}

void CR3View::nextPage() {
    doCommand(DCMD_PAGEDOWN, 1);
}
void CR3View::prevPage() {
    doCommand(DCMD_PAGEUP, 1);
}
void CR3View::nextLine() {
    doCommand(DCMD_LINEDOWN, 1);
}
void CR3View::prevLine() {
    doCommand(DCMD_LINEUP, 1);
}
void CR3View::nextChapter() {
    doCommand(DCMD_MOVE_BY_CHAPTER, 1);
}
void CR3View::prevChapter() {
    doCommand(DCMD_MOVE_BY_CHAPTER, -1);
}
void CR3View::firstPage() {
    doCommand(DCMD_BEGIN, 1);
}
void CR3View::lastPage() {
    doCommand(DCMD_END, 1);
}
void CR3View::historyBack() {
    doCommand(DCMD_LINK_BACK, 1);
}
void CR3View::historyForward() {
    doCommand(DCMD_LINK_FORWARD, 1);
}

void CR3View::refreshPropFromView(const char* propName) {
    _data->_props->setString(propName, _docview->propsGetCurrent()->getStringDef(propName, ""));
}

QSize CR3View::minimumSizeHint() const {
    return QSize(SCREEN_SIZE_MIN, SCREEN_SIZE_MIN);
}

QSize CR3View::sizeHint() const {
    if (_docview == nullptr)
        return QWidget::sizeHint();
    int physW = _docview->GetWidth();
    int physH = _docview->GetHeight();
    if (physW <= 0 || physH <= 0)
        return QWidget::sizeHint();

    // Use screen DPR - more reliable than widget DPR before first paint
    qreal dpr = screen() ? screen()->devicePixelRatio() : 1.0;
    int hintW = qCeil(physW / dpr);
    int hintH = qCeil(physH / dpr);
    CRLog::debug("sizeHint: docview=%dx%d, dpr=%.3f, hint=%dx%d", physW, physH, dpr, hintW, hintH);
    return QSize(hintW, hintH);
}

void CR3View::zoomIn() {
    doCommand(DCMD_ZOOM_IN, 1);
    refreshPropFromView(PROP_FONT_SIZE);
}

void CR3View::zoomOut() {
    doCommand(DCMD_ZOOM_OUT, 1);
    refreshPropFromView(PROP_FONT_SIZE);
}

void CR3View::resizeTimerTimeout() {
    if (_exportMode)
        return;  // Ignore resize during export
    QSize sz = size();
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    // Use screen DPR - more reliable before first paint
    qreal dpr = screen() ? screen()->devicePixelRatio() : 1.0;
    if (fabs(dpr - _dpr) >= 0.01) {
        _dpr = dpr;
    }
    sz *= _dpr;
#endif
    _docview->Resize(sz.width(), sz.height());
    update();
}

void CR3View::setDocViewSize(int width, int height) {
    _docview->Resize(width, height);

    // Get DPR from multiple sources for debugging and robustness
    qreal dpr_widget = devicePixelRatioF();
    qreal dpr_screen = screen() ? screen()->devicePixelRatio() : 1.0;
    // Use screen DPR as it's more reliable before first paint
    qreal dpr = dpr_screen;
    CRLog::debug("setDocViewSize: dpr_widget=%.3f, dpr_screen=%.3f, using=%.3f, cached _dpr=%.3f",
                 dpr_widget, dpr_screen, dpr, _dpr);
    if (fabs(dpr - _dpr) >= 0.01) {
        CRLog::debug("setDocViewSize: updating _dpr from %.3f to %.3f", _dpr, dpr);
        _dpr = dpr;
    }
    CRLog::debug("setDocViewSize: %dx%d (physical), dpr=%.3f", width, height, dpr);

    // Calculate the logical size for this widget
    int logicalW = qRound(width / dpr);
    int logicalH = qRound(height / dpr);
    CRLog::debug("setDocViewSize: logical widget size=%dx%d", logicalW, logicalH);

    // Find the top-level MainWindow
    QWidget* topLevel = window();
    if (topLevel == nullptr) {
        CRLog::warn("setDocViewSize: no top-level window found");
        updateGeometry();
        update();
        return;
    }

    // Calculate the extra space used by MainWindow chrome (menu, toolbar, statusbar, tabbar, etc.)
    // We do this by comparing the current MainWindow size to the current CR3View size
    QSize currentViewSize = size();
    QSize currentWindowSize = topLevel->size();
    int extraW = currentWindowSize.width() - currentViewSize.width();
    int extraH = currentWindowSize.height() - currentViewSize.height();
    CRLog::debug("setDocViewSize: extraW=%d, extraH=%d (menu+toolbar+tabbar+statusbar etc.)",
                 extraW, extraH);

    // Calculate new window size
    int newWindowW = logicalW + extraW;
    int newWindowH = logicalH + extraH;
    CRLog::debug("setDocViewSize: new window size=%dx%d", newWindowW, newWindowH);

    // Resize the main window
    topLevel->resize(newWindowW, newWindowH);

    updateGeometry();
    update();
}

QScrollBar* CR3View::scrollBar() const {
    return _scroll;
}

lUInt64 CR3View::id() const {
    return (lUInt64)_docview;
}

void CR3View::setActive(bool value) {
    if (_active != value) {
        _active = value;
        if (_active) {
            processDelayedCommands();
            update();
        }
    }
}

void CR3View::setScrollBar(QScrollBar* scroll) {
    _scroll = scroll;
    if (_scroll != NULL) {
        QObject::connect(_scroll, SIGNAL(valueChanged(int)), this, SLOT(scrollTo(int)));
    }
}

/// load fb2.css file
bool CR3View::loadCSS(QString fn) {
    lString32 filename(qt2cr(fn));
    lString8 css;
    if (LVLoadStylesheetFile(filename, css)) {
        if (!css.empty()) {
            QFileInfo f(fn);
            CRLog::info("Using style sheet from %s", fn.toUtf8().constData());
            _cssDir = f.absolutePath() + "/";
            _docview->setStyleSheet(css);
            return true;
        }
    }
    return false;
}

QString CR3View::resolveCssPath(const QString& cssDir, const QString& templateName, bool useExpanded) {
    if (useExpanded) {
        QString baseName = templateName;
        if (baseName.endsWith(".css"))
            baseName.chop(4);
        // Look for expanded CSS in user config directory (where they are written)
        QString configDir = cr2qt(getConfigDir());
        QString expandedPath = configDir + baseName + CSS_EXPANDED_SUFFIX;
        if (QFileInfo(expandedPath).exists())
            return expandedPath;
        // Fallback to fb2-expanded.css in config dir
        QString fb2ExpandedPath = configDir + "fb2" + CSS_EXPANDED_SUFFIX;
        if (QFileInfo(fb2ExpandedPath).exists())
            return fb2ExpandedPath;
        CRLog::warn("Generated CSS file not found, falling back to template");
    }
    // Return template path (or fallback to fb2.css if template doesn't exist)
    QString templatePath = cssDir + templateName;
    if (QFileInfo(templatePath).exists())
        return templatePath;
    return cssDir + "fb2.css";
}

void CR3View::onCssFileChanged(const QString& path) {
    CRLog::info("Expanded CSS file changed: %s, reloading document...", path.toUtf8().constData());

    // Some editors delete and recreate the file when saving, which removes the watch.
    // Re-add the file to the watcher if it still exists.
    if (_cssFileWatcher && !_cssFileWatcher->files().contains(path)) {
        if (QFileInfo(path).exists()) {
            _cssFileWatcher->addPath(path);
            CRLog::debug("Re-added CSS file to watcher after modification");
        }
    }

    reloadCurrentDocument();
}

void CR3View::reloadCurrentDocument() {
    lString32 docPath = _docview->getFileName();
    if (!docPath.empty()) {
        loadDocument(cr2qt(docPath));
    }
}

void CR3View::setSharedSettings(CRPropRef props) {
    _data->_props = props;
    _docview->propsUpdateDefaults(_data->_props);
    updateDefProps();
    CRPropRef r = _docview->propsApply(_data->_props);
    applyTextLangMainLang(_doc_language);
    PropsRef unknownOptions = cr2qt(r);
    if (_propsCallback != NULL)
        _propsCallback->onPropsChange(unknownOptions);
}

/// toggle boolean property
void CR3View::toggleProperty(const char* name) {
    int state = _data->_props->getIntDef(name, 0) != 0 ? 0 : 1;
    PropsRef props = Props::create();
    props->setString(name, state ? "1" : "0");
    applyOptions(props, true);
}

/// apply some set of options
PropsRef CR3View::applyOptions(PropsRef props, bool silent) {
    //    for ( int i=0; i<_data->_props->getCount(); i++ ) {
    //        CRLog::debug("Old [%d] '%s'=%s ", i, _data->_props->getName(i), UnicodeToUtf8(_data->_props->getValue(i)).c_str() );
    //    }
    //    for ( int i=0; i<props->count(); i++ ) {
    //        CRLog::debug("New [%d] '%s'=%s ", i, props->name(i), props->value(i).toUtf8().data() );
    //    }
    CRPropRef changed = _data->_props ^ qt2cr(props);
    //    for ( int i=0; i<changed->getCount(); i++ ) {
    //        CRLog::debug("Changed [%d] '%s'=%s ", i, changed->getName(i), UnicodeToUtf8(changed->getValue(i)).c_str() );
    //    }
    CRPropRef changed2 = _docview->getDocProps() ^ qt2cr(props);
    CRPropRef propsToApply = changed | changed2;
    // Don't create new props reference, but change existing
    _data->_props->set(changed | _data->_props);
    //    for ( int i=0; i<_data->_props->getCount(); i++ ) {
    //        CRLog::debug("Result [%d] '%s'=%s ", i, _data->_props->getName(i), UnicodeToUtf8(_data->_props->getValue(i)).c_str() );
    //    }
    CRPropRef r = _docview->propsApply(propsToApply);
    applyTextLangMainLang(_doc_language);
    PropsRef unknownOptions = cr2qt(r);
    if (_propsCallback != NULL)
        _propsCallback->onPropsChange(unknownOptions);
    if (!silent)
        checkFontLanguageCompatibility();
    update();
    return unknownOptions;
}

void CR3View::saveWindowPos(QWidget* window, const char* prefix) {
    ::saveWindowPosition(window, _data->_props, prefix);
}

void CR3View::restoreWindowPos(QWidget* window, const char* prefix, bool allowExtraStates) {
    ::restoreWindowPosition(window, _data->_props, prefix, allowExtraStates);
}

void CR3View::saveSetting(const char* key, int value) {
    _data->_props->setInt(key, value);
}

int CR3View::loadSetting(const char* key, int defaultValue) {
    return _data->_props->getIntDef(key, defaultValue);
}

/// get current option values
PropsRef CR3View::getOptions() {
    return Props::clone(cr2qt(_data->_props));
}

void CR3View::setSharedHistory(CRFileHist* hist) {
    _docview->setSharedHistory(hist);
}

void CR3View::contextMenu(QPoint pos) { }

/// returns true if point is inside selected text
bool CR3View::isPointInsideSelection(const QPoint& pos) {
    if (!_selected)
        return false;
    lvPoint pt(pos.x() * _dpr, pos.y() * _dpr);
    ldomXPointerEx p(_docview->getNodeByPoint(pt));
    if (p.isNull())
        return false;
    return _selRange.isInside(p);
}

QString CR3View::getLinkAtPoint(const QPoint& pos) {
    lvPoint pt(pos.x() * _dpr, pos.y() * _dpr);
    ldomXPointer p = _docview->getNodeByPoint(pt);
    if (!p.isNull()) {
        lString32 href = p.getHRef();
        if (!href.empty())
            return cr2qt(href);
    }
    return QString();
}

void CR3View::mouseMoveEvent(QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    lvPoint pt(qRound(event->position().x() * _dpr), qRound(event->position().y() * _dpr));
#else
    lvPoint pt(qRound(event->localPos().x() * _dpr), qRound(event->localPos().y() * _dpr));
#endif
    ldomXPointer p = _docview->getNodeByPoint(pt);
    lString32 href;
    if (!p.isNull()) {
        href = p.getHRef();
        if (_editMode && _selecting)
            _docview->setCursorPos(p);
    } else {
        //CRLog::trace("Node not found for %d, %d", event->x(), event->y());
    }
    if (_selecting) {
        updateSelection(p);
        setCursor(_selCursor);
    } else {
        if (href.empty())
            setCursor(_normalCursor);
        else
            setCursor(_linkCursor);
        if (NULL != _docViewStatusCallback)
            _docViewStatusCallback->onHoverLink(cr2qt(href));
    }
    //CRLog::trace("mouseMoveEvent - doc pos (%d,%d), buttons: %d %d %d %s", pt.x, pt.y, (int)left, (int)right
    //             , (int)mid, href.empty()?"":UnicodeToUtf8(href).c_str()
    //             //, path.empty()?"":UnicodeToUtf8(path).c_str()
    //             );
}

void CR3View::clearSelection() {
    if (_selected) {
        _docview->clearSelection();
        update();
    }
    _selecting = false;
    _selected = false;
    _selStart = ldomXPointer();
    _selEnd = ldomXPointer();
    _selText.clear();
    ldomXPointerEx p1;
    ldomXPointerEx p2;
    _selRange.setStart(p1);
    _selRange.setEnd(p2);
}

void CR3View::startSelection(ldomXPointer p) {
    clearSelection();
    _selecting = true;
    _selStart = p;
    updateSelection(p);
}

bool CR3View::endSelection(ldomXPointer p) {
    if (!_selecting)
        return false;
    updateSelection(p);
    if (_selected) {
    }
    _selecting = false;
    return _selected;
}

bool CR3View::updateSelection(ldomXPointer p) {
    if (!_selecting)
        return false;
    _selEnd = p;
    ldomXRange r(_selStart, _selEnd);
    if (r.getStart().isNull() || r.getEnd().isNull())
        return false;
    r.sort();
    if (_doubleClick && !_editMode) {
        if (!r.getStart().isVisibleWordStart()) {
            //CRLog::trace("calling prevVisibleWordStart : %s", LCSTR(r.getStart().toString()));
            r.getStart().prevVisibleWordStart();
            //CRLog::trace("updated : %s", LCSTR(r.getStart().toString()));
        }
        //lString32 start = r.getStart().toString();
        if (!r.getEnd().isVisibleWordEnd()) {
            //CRLog::trace("calling nextVisibleWordEnd : %s", LCSTR(r.getEnd().toString()));
            r.getEnd().nextVisibleWordEnd();
            //CRLog::trace("updated : %s", LCSTR(r.getEnd().toString()));
        }
    }
    if (r.isNull())
        return false;
    //lString32 end = r.getEnd().toString();
    //CRLog::debug("Range: %s - %s", UnicodeToUtf8(start).c_str(), UnicodeToUtf8(end).c_str());
    r.setFlags(1);
    _docview->selectRange(r);
    _selText = cr2qt(r.getRangeText('\n', 10000));
    _selected = true;
    _selRange = r;
    update();
    return true;
}

void CR3View::checkFontLanguageCompatibility() {
#if USE_LOCALE_DATA == 1
    lString32 fontFace;
    lString32 fileName;
    _data->_props->getString(PROP_FONT_FACE, fontFace);
    _docview->getDocProps()->getString(DOC_PROP_FILE_NAME, fileName);
    lString8 fontFace_u8 = UnicodeToUtf8(fontFace);
    lString32 langCode = _docview->getLanguage();
    lString8 langCode_u8 = UnicodeToUtf8(langCode);
    if (langCode_u8.length() == 0) {
        CRLog::debug("Can't fetch book's language to check font compatibility!");
        return;
    }
    // Skip font compatibility check for undetermined language (ISO 639-2/3 "und")
    // This avoids showing spurious warnings when the book has no language metadata
    lString8 langLower = langCode_u8;
    langLower.lowercase();
    if (langLower == "und" || langLower == "undetermined") {
        CRLog::debug("Skipping font compatibility check for undetermined language: \"%s\"", langCode_u8.c_str());
        return;
    }
    if (fontFace_u8.length() > 0) {
        QString langDescr = getHumanReadableLocaleName(langCode);
        if (!langDescr.isEmpty()) {
            font_lang_compat compat = fontMan->checkFontLangCompat(fontFace_u8, langCode_u8);
            CRLog::debug("Checking font \"%s\" for compatibility with language \"%s\": %d", fontFace_u8.c_str(),
                         langCode_u8.c_str(), (int)compat);
            switch (compat) {
                case font_lang_compat_invalid_tag:
                    CRLog::warn(
                            "Can't find compatible language code in embedded FontConfig catalog: language=\"%s\", filename=\"%s\"",
                            langCode_u8.c_str(), LCSTR(fileName));
                    break;
                case font_lang_compat_none:
                case font_lang_compat_partial:
                    CRLog::warn("Font \"%s\" isn't compatible with language \"%s\". Instead will be used fallback font.",
                                fontFace_u8.c_str(), langDescr.toUtf8().constData());
                    break;
                case font_lang_compat_full:
                    // good, do nothing
                    break;
            }
        } else {
            CRLog::warn("Invalid language tag: \"%s\", filename=\"%s\"", langCode_u8.c_str(), LCSTR(fileName));
        }
    }
#endif
}

void CR3View::updateHistoryAvailability() {
    bool canGoBack = _docview->canGoBack();
    bool canGoForward = _docview->canGoForward();
    if (canGoBack != _canGoBack) {
        _canGoBack = canGoBack;
        if (NULL != _docViewStatusCallback)
            _docViewStatusCallback->onCanGoBack(id(), _canGoBack);
    }
    if (canGoForward != _canGoForward) {
        _canGoForward = canGoForward;
        if (NULL != _docViewStatusCallback)
            _docViewStatusCallback->onCanGoForward(id(), _canGoForward);
    }
}

void CR3View::applyTextLangMainLang(lString32 lang) {
    if (!lang.empty()) {
        CRPropRef props = _docview->propsGetCurrent();
        if (props->getBoolDef(PROP_TEXTLANG_EMBEDDED_LANGS_ENABLED, false)) {
            _docview->propApply(lString8(PROP_TEXTLANG_MAIN_LANG), _doc_language);
            // LVDocView::propApply() calls LVDocView::requestRender() if the property has changed
            // LVDocView::requestRender() doesn't render the document, it only resets the render status
            // If the LVDocView::checkRender() function is then called, the document will be rendered
        }
    }
}

static inline int s_command_weight(CR3ViewCommandType cmd) {
    switch (cmd) {
        case CR3ViewCommandType::OpenDocument:
            return 1;
            break;
        case CR3ViewCommandType::SetDocumentText:
            return 1;
            break;
        case CR3ViewCommandType::Resize:
            return 10;
            break;
    }
    return 0;
}

static struct
{
    bool operator()(CR3ViewCommand a, CR3ViewCommand b) const {
        return s_command_weight(a.command()) > s_command_weight(b.command());
    }
} s_viewCommandWeight;

void CR3View::processDelayedCommands() {
    std::sort(_delayed_commands.begin(), _delayed_commands.end(), s_viewCommandWeight);
    while (!_delayed_commands.isEmpty()) {
        const CR3ViewCommand& cmd = _delayed_commands.first();
        switch (cmd.command()) {
            case Noop:
                break;
            case OpenDocument: {
                LoadDocumentImpl(cmd.data().toString(), true);
                break;
            }
            case SetDocumentText: {
                QString text = cmd.data().toString();
                setDocumentTextImpl(text);
                break;
            }
            case Resize: {
                QSize sz = cmd.data().toSize();
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
                // Use screen DPR - more reliable before first paint
                qreal dpr = screen() ? screen()->devicePixelRatio() : 1.0;
                if (fabs(dpr - _dpr) >= 0.01) {
                    _dpr = dpr;
                }
                sz *= _dpr;
#endif
                _docview->Resize(sz.width(), sz.height());
                update();
                break;
            }
        }
        _delayed_commands.removeFirst();
    }
}

bool CR3View::LoadDocumentImpl(const QString& fileName, bool silent) {
    if (_historyEnabled)
        _docview->savePosition();
    clearSelection();
    lString32 fn = qt2cr(fileName);
    bool res = _docview->LoadDocument(fn.c_str());
    if (res) {
        _doc_language = _docview->getLanguage();
        applyTextLangMainLang(_doc_language);
        //_docview->swapToCache();
        CRLog::debug("Trying to restore position for %s", LCSTR(fn));
        _docview->restorePosition();
        if (!silent)
            checkFontLanguageCompatibility();
    } else {
        _doc_language = lString32::empty_str;
        _docview->createDefaultDocument(lString32::empty_str, qt2cr(tr("Error while opening document ") + fileName));
    }
    update();
    updateHistoryAvailability();
    return res;
}

void CR3View::setDocumentTextImpl(const QString& text) {
    _docview->savePosition();
    clearSelection();
    _docview->createDefaultDocument(lString32::empty_str, qt2cr(text));
    update();
}

void CR3View::mousePressEvent(QMouseEvent* event) {
    bool left = event->button() == Qt::LeftButton;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    bool mid = event->button() == Qt::MiddleButton;
#else
    bool mid = event->button() == Qt::MidButton;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    lvPoint pt(qRound(event->position().x() * _dpr), qRound(event->position().y() * _dpr));
#else
    lvPoint pt(qRound(event->localPos().x() * _dpr), qRound(event->localPos().y() * _dpr));
#endif
    ldomXPointer p = _docview->getNodeByPoint(pt);
    // test imageByPoint
    LVImageSourceRef img = _docview->getImageByPoint(pt);
    if (!img.isNull())
        CRLog::debug("Image %d x %d found", img->GetWidth(), img->GetHeight());
    CRBookmark* bmk = _docview->findBookmarkByPoint(pt);
    if (bmk != NULL) {
        CRLog::trace("Found bookmark of type %d", bmk->getType());
        if (mid)
            AddBookmarkDialog::editBookmark((QWidget*)parent(), this, bmk);
    }
    lString32 path;
    lString32 href;
    if (!p.isNull()) {
        path = p.toString();
        CRLog::debug("mousePressEvent(%s)", LCSTR(path));
        bool ctrlPressed = (event->modifiers() & Qt::ControlModifier) != 0;
        if (ctrlPressed || !_editMode)
            href = p.getHRef();
    }
    if (href.empty()) {
        //CRLog::trace("No href pressed" );
        if (left) {
            if (!p.isNull()) {
                if (_editMode)
                    _docview->setCursorPos(p);
                startSelection(p);
            } else {
                clearSelection();
            }
        }
    } else {
        CRLog::info("Link is selected: %s", UnicodeToUtf8(href).c_str());
        if (left) {
            // link is pressed
            if (_docview->goLink(href)) {
                update();
                updateHistoryAvailability();
            }
        } else if (mid) {
            if (NULL != _docViewStatusCallback)
                _docViewStatusCallback->onOpenInNewTabRequested(cr2qt(href));
        }
    }
    //CRLog::debug("mousePressEvent - doc pos (%d,%d), buttons: %d %d %d", pt.x, pt.y, (int)left, (int)right, (int)mid);
}

void CR3View::mouseReleaseEvent(QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    lvPoint pt(qRound(event->position().x() * _dpr), qRound(event->position().y() * _dpr));
#else
    lvPoint pt(qRound(event->localPos().x() * _dpr), qRound(event->localPos().y() * _dpr));
#endif
    ldomXPointer p = _docview->getNodeByPoint(pt);
    if (!p.isNull()) {
        if (_editMode)
            _docview->setCursorPos(p);
    }
    if (_selecting) {
        endSelection(p);
        setCursor(_normalCursor);
        if (!_selText.isEmpty()) {
            QClipboard* clipboard = QApplication::clipboard();
            if (_clipboardSupportsMouseSelection)
                clipboard->setText(_selText, QClipboard::Selection);
            if (_onTextSelectAutoClipboardCopy)
                clipboard->setText(_selText, QClipboard::Clipboard);
            if (_onTextSelectAutoCmdExec) {
                QStringList args = _selectionCommand;
                if (!args.isEmpty()) {
                    QString programName = args.takeFirst();
                    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
                        if (it->contains("%TEXT%"))
                            it->replace("%TEXT%", _selText);
                    }
                    if (!args.isEmpty()) {
                        QString dbgCmdLine = programName + QString(" ") + args.join(' ');
                        CRLog::debug("Starting program: %s", dbgCmdLine.toUtf8().constData());
                        bool res = QProcess::startDetached(programName, args);
                        if (res)
                            CRLog::debug("the program has been successfully launched");
                        else
                            CRLog::warn("failed to run program");
                    }
                }
            }
        }
    }
    _doubleClick = false;
    //CRLog::debug("mouseReleaseEvent - doc pos (%d,%d), buttons: %d %d %d", pt.x, pt.y, (int)left, (int)right, (int)mid);
}

void CR3View::mouseDoubleClickEvent(QMouseEvent* event) {
    _doubleClick = true;
    QWidget::mouseDoubleClickEvent(event);
    if (_selecting) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        lvPoint pt(qRound(event->position().x() * _dpr), qRound(event->position().y() * _dpr));
#else
        lvPoint pt(qRound(event->localPos().x() * _dpr), qRound(event->localPos().y() * _dpr));
#endif
        ldomXPointer p = _docview->getNodeByPoint(pt);
        updateSelection(p);
        setCursor(_selCursor);
    }
}

void CR3View::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (QEvent::LocaleChange == event->type()) {
        lString32 decimalPoint = qt2cr(locale().decimalPoint());
        if (!decimalPoint.empty()) {
            _docview->setDecimalPointChar(decimalPoint[0]);
            update();
        }
    }
}

/// Override to handle external links
void CR3View::OnExternalLink(lString32 url, ldomNode* node) {
    // TODO: add support of file links
    // only URL supported for now
    QUrl qturl(cr2qt(url));
    QDesktopServices::openUrl(qturl);
}

/// create bookmark
CRBookmark* CR3View::createBookmark() {
    CRBookmark* bm = NULL;
    if (getSelectionText().length() > 0 && !_selRange.isNull()) {
        bm = getDocView()->saveRangeBookmark(_selRange, bmkt_comment, lString32::empty_str);
    } else {
        bm = getDocView()->saveCurrentPageBookmark(lString32::empty_str);
    }

    return bm;
}

void CR3View::goToBookmark(CRBookmark* bm) {
    ldomXPointer start = _docview->getDocument()->createXPointer(bm->getStartPos());
    ldomXPointer end = _docview->getDocument()->createXPointer(bm->getEndPos());
    if (start.isNull())
        return;
    if (end.isNull())
        end = start;
    startSelection(start);
    endSelection(end);
    goToXPointer(cr2qt(bm->getStartPos()), true);
    update();
}

/// rotate view, +1 = 90` clockwise, -1 = 90` counterclockwise
void CR3View::rotate(int angle) {
    getDocView()->doCommand(DCMD_ROTATE_BY, angle);
    update();

    //    int currAngle = _data->_props->getIntDef( PROP_ROTATE_ANGLE, 0 );
    //    int newAngle = currAngle + angle;
    //    newAngle = newAngle % 4;
    //    if ( newAngle < 0 )
    //        newAngle += 4;
    //    if ( newAngle == currAngle )
    //        return;
    //    getDocView()->SetRotateAngle( (cr_rotate_angle_t) newAngle );
    //    _data->_props->setInt( PROP_ROTATE_ANGLE, newAngle );
    //    update();
}

/// format detection finished
void CR3View::OnLoadFileFormatDetected(doc_format_t fileFormat) {
    QString filename = "fb2.css";
    if (_cssDir.length() > 0) {
        switch (fileFormat) {
            case doc_format_txt:
                filename = "txt.css";
                break;
            case doc_format_fb3:
                filename = "fb3.css";
                break;
            case doc_format_rtf:
                filename = "rtf.css";
                break;
            case doc_format_epub:
                filename = "epub.css";
                break;
            case doc_format_html:
                filename = "htm.css";
                break;
            case doc_format_md:
                filename = "markdown.css";
                break;
            case doc_format_doc:
                filename = "doc.css";
                break;
            case doc_format_odt:
            case doc_format_docx:
                filename = "docx.css";
                break;
            case doc_format_chm:
                filename = "chm.css";
                break;
            default:
                    // do nothing
                    ;
        }
        CRLog::debug("CSS file to load: %s", filename.toUtf8().constData());

        // Store the current CSS template filename for use by Settings dialog
        _data->_props->setString(PROP_CSS_CURRENT_TEMPLATE, filename.toUtf8().constData());

        // Check if we should use generated (expanded) CSS file
        bool useGenerated = _data->_props->getBoolDef(PROP_CSS_USE_GENERATED, false);

        // Helper to set up file watcher for expanded CSS files
        auto setupCssWatcher = [this](const QString& cssPath) {
            // Stop watching previous file
            if (_cssFileWatcher) {
                if (!_watchedCssFile.isEmpty()) {
                    _cssFileWatcher->removePath(_watchedCssFile);
                }
            } else {
                _cssFileWatcher = new QFileSystemWatcher(this);
                connect(_cssFileWatcher, &QFileSystemWatcher::fileChanged, this, &CR3View::onCssFileChanged);
            }
            // Start watching new file
            _watchedCssFile = cssPath;
            if (_cssFileWatcher->addPath(cssPath)) {
                CRLog::info("Watching CSS file for changes: %s", cssPath.toUtf8().constData());
            } else {
                CRLog::warn("Failed to watch CSS file: %s", cssPath.toUtf8().constData());
            }
        };

        // Resolve CSS path (handles expanded vs template, with fallbacks)
        QString cssPath = resolveCssPath(_cssDir, filename, useGenerated);
        bool isExpanded = useGenerated && cssPath.endsWith(CSS_EXPANDED_SUFFIX);

        if (isExpanded) {
            CRLog::info("Using generated CSS file: %s", QFileInfo(cssPath).fileName().toUtf8().constData());
            setupCssWatcher(cssPath);
        } else {
            // Not using generated CSS - stop watching if previously watching
            if (_cssFileWatcher && !_watchedCssFile.isEmpty()) {
                _cssFileWatcher->removePath(_watchedCssFile);
                _watchedCssFile.clear();
            }
        }

        if (QFileInfo(cssPath).exists()) {
            loadCSS(cssPath);
        }
    }
}

/// on starting file loading
void CR3View::OnLoadFileStart(lString32 filename) {
    _filename = filename;
    setCursor(_waitCursor);
}

/// file load finiished with error
void CR3View::OnLoadFileError(lString32 message) {
    setCursor(_normalCursor);
    if (NULL != _docViewStatusCallback)
        _docViewStatusCallback->onDocumentLoaded(id(), cr2qt(_filename), cr2qt(message), cr2qt(_filename));
}

/// file loading is finished successfully - drawCoveTo() may be called there
void CR3View::OnLoadFileEnd() {
    setCursor(_normalCursor);
    if (NULL != _docViewStatusCallback && NULL != _docview) {
        QString atitle = getDocTitle();
        QString docPath;
        CRPropRef doc_props = _docview->getDocProps();
        lString32 arcFullPath;
        lString32 path;
        lString32 arcPath = doc_props->getStringDef(DOC_PROP_ARC_PATH);
        lString32 arcName = doc_props->getStringDef(DOC_PROP_ARC_NAME);
        lString32 filepath = doc_props->getStringDef(DOC_PROP_FILE_PATH);
        lString32 filename = doc_props->getStringDef(DOC_PROP_FILE_NAME);
        if (!arcPath.empty() || !arcName.empty()) {
            arcFullPath = LVCombinePaths(arcPath, arcName);
            arcFullPath += ASSET_PATH_PREFIX_S;
            LVAppendPathDelimiter(arcFullPath);
        }
        path = LVCombinePaths(filepath, filename);
        docPath = cr2qt(arcFullPath + path);
        _docViewStatusCallback->onDocumentLoaded(id(), atitle, QString(), docPath);
    }
    updateHistoryAvailability();
}

/// document formatting started
void CR3View::OnFormatStart() {
    setCursor(_waitCursor);
}

/// document formatting finished
void CR3View::OnFormatEnd() {
    setCursor(_normalCursor);
    _docview->updateCache(); // save to cache
}

/// set bookmarks dir
void CR3View::setBookmarksDir(const QString& dirname) {
    _bookmarkDir = dirname;
}

void CR3View::keyPressEvent(QKeyEvent* event) {
#if 0
    // testing sentence navigation/selection
    switch ( event->key() ) {
    case Qt::Key_Z:
        _docview->doCommand(DCMD_SELECT_FIRST_SENTENCE);
        update();
        return;
    case Qt::Key_X:
        _docview->doCommand(DCMD_SELECT_NEXT_SENTENCE);
        update();
        return;
    case Qt::Key_C:
        _docview->doCommand(DCMD_SELECT_PREV_SENTENCE);
        update();
        return;
    }
#endif
#if WORD_SELECTOR_ENABLED == 1
    if (isWordSelection()) {
        MoveDirection dir = DIR_ANY;
        switch (event->key()) {
            case Qt::Key_Left:
            case Qt::Key_A:
                dir = DIR_LEFT;
                break;
            case Qt::Key_Right:
            case Qt::Key_D:
                dir = DIR_RIGHT;
                break;
            case Qt::Key_W:
            case Qt::Key_Up:
                dir = DIR_UP;
                break;
            case Qt::Key_S:
            case Qt::Key_Down:
                dir = DIR_DOWN;
                break;
            case Qt::Key_Q:
            case Qt::Key_Enter:
            case Qt::Key_Escape: {
                QString text = endWordSelection();
                event->setAccepted(true);
                CRLog::debug("Word selected: %s", LCSTR(qt2cr(text)));
            }
                return;
            case Qt::Key_Backspace:
                _wordSelector->reducePattern();
                update();
                break;
            default: {
                int key = event->key();
                if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                    QString text = event->text();
                    if (text.length() == 1) {
                        _wordSelector->appendPattern(qt2cr(text));
                        update();
                    }
                }
            }
                event->setAccepted(true);
                return;
        }
        int dist = (event->modifiers() & Qt::ShiftModifier) ? 5 : 1;
        _wordSelector->moveBy(dir, dist);
        update();
        event->setAccepted(true);
    } else {
        if (event->key() == Qt::Key_F3 && (event->modifiers() & Qt::ShiftModifier)) {
            startWordSelection();
            event->setAccepted(true);
            return;
        }
    }
#endif
    if (!_editMode)
        return;
    switch (event->key()) {
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            break;
    }
}

/// file progress indicator, called with values 0..100
void CR3View::OnLoadFileProgress(int percent) {
    CRLog::info("OnLoadFileProgress(%d%%)", percent);
}

/// format progress, called with values 0..100
void CR3View::OnFormatProgress(int percent) {
    CRLog::info("OnFormatProgress(%d%%)", percent);
}

/// first page is loaded from file an can be formatted for preview
void CR3View::OnLoadFileFirstPagesReady() {
#if 0 // disabled
    if ( !_data->_props->getBoolDef( PROP_PROGRESS_SHOW_FIRST_PAGE, 1 ) ) {
        CRLog::info( "OnLoadFileFirstPagesReady() - don't paint first page because " PROP_PROGRESS_SHOW_FIRST_PAGE " setting is 0" );
        return;
    }
    CRLog::info( "OnLoadFileFirstPagesReady() - painting first page" );
    _docview->setPageHeaderOverride(qt2cr(tr("Loading: please wait...")));
    //update();
    repaint();
    CRLog::info( "OnLoadFileFirstPagesReady() - painting done" );
    _docview->setPageHeaderOverride(lString32::empty_str);
    _docview->requestRender();
    // TODO: remove debug sleep
    //sleep(5);
#endif
}
