/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2009,2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2023 Ren Tatsumoto <tatsu@autistici.org>                *
 *   Copyright (C) 2019-2024 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef CR3WIDGET_H
#define CR3WIDGET_H

#include <qwidget.h>
#include <QScrollBar>
#include "crqtutil.h"
#include "cr3widget_commands.h"

class QFileSystemWatcher;
class LVDocView;
class LVTocItem;
class CRBookmark;

class PropsChangeCallback
{
public:
    virtual void onPropsChange(PropsRef props) = 0;
};

class DocViewStatusCallback
{
public:
    virtual void onDocumentLoaded(lUInt64 viewId, const QString& atitle, const QString& error,
                                  const QString& fullDocPath) = 0;
    virtual void onCanGoBack(lUInt64 viewId, bool canGoBack) = 0;
    virtual void onCanGoForward(lUInt64 viewId, bool canGoForward) = 0;
    virtual void onHoverLink(const QString& href) = 0;
    virtual void onOpenInNewTabRequested(const QString& href) = 0;
};

#define WORD_SELECTOR_ENABLED 1

class CR3View: public QWidget, public LVDocViewCallback
{
    Q_OBJECT

    Q_PROPERTY(QScrollBar* scrollBar READ scrollBar WRITE setScrollBar)

    class DocViewData;

#if WORD_SELECTOR_ENABLED == 1
    LVPageWordSelector* _wordSelector;
protected:
    void startWordSelection();
    QString endWordSelection();
    bool isWordSelection() {
        return _wordSelector != NULL;
    }
#endif
public:
    CR3View(QWidget* parent = 0);
    virtual ~CR3View();

    bool loadDocument(const QString& fileName);
    bool loadLastDocument();
    void setDocumentText(const QString& text);

    QScrollBar* scrollBar() const;

    /// returns the id of this view
    lUInt64 id() const;

    bool isActive() const {
        return _active;
    }
    void setActive(bool value);

    /// get document's table of contents
    LVTocItem* getToc();
    /// return LVDocView associated with widget
    LVDocView* getDocView() {
        return _docview;
    }
    /// go to position specified by xPointer string
    void goToXPointer(const QString& xPointer, bool saveToHistory = false);

    /// returns current page
    int getCurPage();
    /// returns base page number (0 if cover exists, 1 otherwise)
    int getBasePage();

    /// set shared properties (can be used by multiple LVDocView instances)
    void setSharedSettings(CRPropRef props);
    /// set new file history object (can be used by multiple LVDocView instances)
    void setSharedHistory(CRFileHist* hist);

    void setHyphDir(const QString& dirname, bool clear = true);
    const QStringList& getHyphDicts();

    /// load fb2.css file
    bool loadCSS(QString filename);
    /// reload currently opened document (e.g. after CSS change)
    void reloadCurrentDocument();
    /// get CSS directory path
    QString getCssDir() const { return _cssDir; }
    /// resolve CSS path (template vs expanded) based on settings
    static QString resolveCssPath(const QString& cssDir, const QString& templateName, bool useExpanded);
    /// set bookmarks dir
    void setBookmarksDir(const QString& dirname);
    /// apply some set of options
    PropsRef applyOptions(PropsRef props, bool silent);
    /// get current option values
    PropsRef getOptions();
    /// turns on/off Edit mode (forces Scroll view)
    void setEditMode(bool flgEdit);
    /// returns true if edit mode is active
    bool getEditMode() {
        return _editMode;
    }
    /// turns on/off Export mode (suppresses view updates during export)
    void setExportMode(bool enabled);
    /// returns true if export mode is active
    bool isExportMode() const {
        return _exportMode;
    }
    /// enables/disables adding documents to history when loading
    void setHistoryEnabled(bool enabled) {
        _historyEnabled = enabled;
    }
    /// returns true if history tracking is enabled
    bool isHistoryEnabled() const {
        return _historyEnabled;
    }
    QString getDocTitle() const;

    void saveWindowPos(QWidget* window, const char* prefix);
    void restoreWindowPos(QWidget* window, const char* prefix, bool allowExtraStates = false);
    void saveSetting(const char* key, int value);
    int loadSetting(const char* key, int defaultValue);

    void setPropsChangeCallback(PropsChangeCallback* propsCallback) {
        _propsCallback = propsCallback;
    }
    void setDocViewStatusCallback(DocViewStatusCallback* callback) {
        _docViewStatusCallback = callback;
        if (NULL != _docViewStatusCallback) {
            _docViewStatusCallback->onCanGoBack(id(), _canGoBack);
            _docViewStatusCallback->onCanGoForward(id(), _canGoForward);
            _docViewStatusCallback->onHoverLink(QString());
        }
    }
    /// toggle boolean property
    void toggleProperty(const char* name);
    /// returns true if point is inside selected text
    bool isPointInsideSelection(const QPoint& pt);
    /// returns selection text
    QString getSelectionText() {
        return _selText;
    }
    /// returns link at this point if available
    QString getLinkAtPoint(const QPoint& pt);
    bool isOnTextSelectAutoClipboardCopy() const {
        return _onTextSelectAutoClipboardCopy;
    }
    void setOnTextSelectAutoClipboardCopy(bool value) {
        _onTextSelectAutoClipboardCopy = value;
    }
    bool isOnTextSelectAutoCmdExec() const {
        return _onTextSelectAutoCmdExec;
    }
    void setOnTextSelectAutoCmdExec(bool value) {
        _onTextSelectAutoCmdExec = value;
    }
    /// Setting that controls what program is run when text is selected.
    QStringList const& selectionCommand() const {
        return _selectionCommand;
    }
    void setSelectionCommand(QString const& command) {
        _selectionCommand = parseExtCommandLine(command);
    }
    /// create bookmark
    CRBookmark* createBookmark();
    /// go to bookmark and highlight it
    void goToBookmark(CRBookmark* bm);

    /// rotate view, +1 = 90` clockwise, -1 = 90` counterclockwise
    void rotate(int angle);
    /// Override to handle external links
    virtual void OnExternalLink(lString32 url, ldomNode* node);
    /// format detection finished
    virtual void OnLoadFileFormatDetected(doc_format_t fileFormat);
    /// on starting file loading
    virtual void OnLoadFileStart(lString32 filename);
    /// first page is loaded from file an can be formatted for preview
    virtual void OnLoadFileFirstPagesReady();
    /// file load finiished with error
    virtual void OnLoadFileError(lString32 message);
    /// file loading is finished successfully - drawCoveTo() may be called there
    virtual void OnLoadFileEnd();
    /// document formatting started
    virtual void OnFormatStart();
    /// document formatting finished
    virtual void OnFormatEnd();
    /// file progress indicator, called with values 0..100
    virtual void OnLoadFileProgress(int percent);
    /// format progress, called with values 0..100
    virtual void OnFormatProgress(int percent);

public slots:
    void contextMenu(QPoint pos);
    void setScrollBar(QScrollBar* scroll);
    /// on scroll
    void togglePageScrollView();
    void scrollTo(int value);
    void scrollBy(int px);
    void nextPage();
    void prevPage();
    void nextLine();
    void prevLine();
    void nextChapter();
    void prevChapter();
    void firstPage();
    void lastPage();
    void historyBack();
    void historyForward();
    void zoomIn();
    void zoomOut();
    void nextSentence();
    void prevSentence();
    /// Set docview size in physical pixels and resize widget to match
    void setDocViewSize(int width, int height);
signals:
    //void fileNameChanged( const QString & );
protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void updateScroll();
    virtual void doCommand(int cmd, int param = 0);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void changeEvent(QEvent* event);

    virtual void refreshPropFromView(const char* propName);
    virtual QSize minimumSizeHint() const;
    QSize sizeHint() const override;
private slots:
    void resizeTimerTimeout();
    void onCssFileChanged(const QString& path);
private:
    void updateDefProps();
    void clearSelection();
    void startSelection(ldomXPointer p);
    bool endSelection(ldomXPointer p);
    bool updateSelection(ldomXPointer p);
    void checkFontLanguageCompatibility();
    void updateHistoryAvailability();
    void applyTextLangMainLang(lString32 lang);
    void processDelayedCommands();
    void reorderDelayedCommands();
    bool LoadDocumentImpl(const QString& fileName, bool silent);
    void setDocumentTextImpl(const QString& text);

    DocViewData* _data; // to hide non-qt implementation
    LVDocView* _docview;
    QScrollBar* _scroll;
    QTimer* _resizeTimer;
    qreal _dpr; // screen display pixel ratio (for HiDPI screens)
    PropsChangeCallback* _propsCallback;
    DocViewStatusCallback* _docViewStatusCallback;
    QStringList _hyphDicts;
    QCursor _normalCursor;
    QCursor _linkCursor;
    QCursor _selCursor;
    QCursor _waitCursor;
    bool _selecting;
    bool _selected;
    bool _doubleClick;
    ldomXPointer _selStart;
    ldomXPointer _selEnd;
    QString _selText;
    ldomXRange _selRange;
    QString _cssDir;
    QString _bookmarkDir;
    lString32 _filename;
    lString32 _doc_language;
    CR3ViewCommandList _delayed_commands;
    bool _active;
    bool _editMode;
    bool _exportMode;
    bool _historyEnabled;
    QPixmap _exportModeFrame;  // Cached frame to display during export
    int _lastBatteryState;
    int _lastBatteryChargingConn;
    int _lastBatteryChargeLevel;
    bool _canGoBack;
    bool _canGoForward;
    bool _onTextSelectAutoClipboardCopy;
    bool _onTextSelectAutoCmdExec;
    bool _clipboardSupportsMouseSelection;
    // Store a copy of the command that should run when text is selected.
    QStringList _selectionCommand;
    int _wheelIntegralDegrees;
    // For auto-reloading expanded CSS files on change
    QFileSystemWatcher* _cssFileWatcher;
    QString _watchedCssFile;
};

#endif // CR3WIDGET_H
