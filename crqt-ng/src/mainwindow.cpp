/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2009-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 Mihail Slobodyanuk <slobodyanukma@gmail.com>       *
 *   Copyright (C) 2019,2020 Konstantin Potapov <pkbo@users.sourceforge.net>
 *   Copyright (C) 2023 Ren Tatsumoto <tatsu@autistici.org>                *
 *   Copyright (C) 2018,2020-2024 Aleksey Chernov <valexlin@gmail.com>     *
 *   Copyright (C) 2025 Sergey Alirzaev <l29ah@riseup.net>                 *
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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qglobal.h>

#include "xtexportdlg.h"
#include "xtexportprofile.h"
#if QT_VERSION >= 0x050000
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QStyleFactory>
#else
#include <QtGui/QFileDialog>
#include <QtGui/QStyleFactory>
#endif

#include <QClipboard>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <lvbasedrawbuf.h>

#include "app-props.h"
#include "settings.h"
#include "tocdlg.h"
#include "recentdlg.h"
#include "aboutdlg.h"
#include "filepropsdlg.h"
#include "bookmarklistdlg.h"
#include "addbookmarkdlg.h"
#include "crqtutil.h"
#include "wolexportdlg.h"
#include "searchdlg.h"

#ifndef ENABLE_BOOKMARKS_DIR
#define ENABLE_BOOKMARKS_DIR 1
#endif

// In the engine, the maximum number of open documents is 16
// 1 reserved for preview in settings dialog
#define MAX_TABS_COUNT 15

MainWindow::MainWindow(const QStringList& filesToOpen, QWidget* parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindowClass)
        , _filenamesToOpen(filesToOpen)
        , _prevIndex(-1) {
    ui->setupUi(this);

    // "New Tab" tool button on tab widget
    QPushButton* newTabButton = new QPushButton(this);
    newTabButton->setIcon(ui->actionNew_tab->icon());
    newTabButton->setToolTip(ui->actionNew_tab->toolTip());
    newTabButton->setFlat(true);
    ui->tabWidget->setCornerWidget(newTabButton, Qt::TopLeftCorner);
    connect(newTabButton, SIGNAL(clicked()), this, SLOT(on_actionNew_tab_triggered()));

    QIcon icon = QIcon(":/icons/icons/crqt.png");
    CRLog::warn("\n\n\n*** ######### application icon %s\n\n\n", icon.isNull() ? "null" : "found");
    qApp->setWindowIcon(icon);

    addAction(ui->actionOpen);
    addAction(ui->actionRecentBooks);
    addAction(ui->actionTOC);
    // addAction(ui->actionToggle_Full_Screen);
    addAction(ui->actionSettings);
    addAction(ui->actionClose);
    // addAction(ui->actionToggle_Pages_Scroll);
    addAction(ui->actionMinimize);
    addAction(ui->actionNextPage);
    addAction(ui->actionPrevPage);
    addAction(ui->actionNextPage2);
    addAction(ui->actionPrevPage2);
    addAction(ui->actionNextPage3);
    addAction(ui->actionPrevPage3);
    addAction(ui->actionNextLine);
    addAction(ui->actionPrevLine);
    addAction(ui->actionFirstPage);
    addAction(ui->actionLastPage);
    addAction(ui->actionBack);
    addAction(ui->actionForward);
    addAction(ui->actionNextChapter);
    addAction(ui->actionPrevChapter);
    addAction(ui->actionZoom_In);
    addAction(ui->actionZoom_Out);
    addAction(ui->actionCopy);
    addAction(ui->actionCopy2); // alternative shortcut
    addAction(ui->actionAddBookmark);
    addAction(ui->actionShowBookmarksList);
    addAction(ui->actionToggleEditMode);
    addAction(ui->actionNextSentence);
    addAction(ui->actionPrevSentence);
    addAction(ui->actionNew_tab);

    // Init tabs container
    QString configDir = cr2qt(getConfigDir());
    QString iniFile = configDir + "crui.ini";
    QString histFile = configDir + "cruihist.bmk";
    if (!_tabs.loadSettings(iniFile)) {
        // If config file not found in config dir
        //  save its to config dir
        _tabs.saveSettings(iniFile);
    }
    if (!_tabs.loadHistory(histFile)) {
        _tabs.saveHistory(histFile);
    }
    // Add one tab
    addNewDocTab();
    _tabs.restoreWindowPos(this, "main.", true);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    _tabs.saveWindowPos(this, "main.");
    _tabs.saveHistory();
    _tabs.saveSettings();
    int tabIndex = ui->tabWidget->currentIndex();
    if (tabIndex >= 0 && tabIndex < _tabs.size())
        _tabs.setCurrentDocument(_tabs[tabIndex].docPath());
    else
        _tabs.setCurrentDocument("");
    _tabs.saveTabSession(cr2qt(getConfigDir()) + "tabs.ini");
}

MainWindow::~MainWindow() {
    ui->tabWidget->clear();
    _tabs.cleanup();
    delete ui;
}

TabData MainWindow::createNewDocTabWidget() {
    QWidget* widget = new QWidget(ui->tabWidget, Qt::Widget);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    CR3View* view = new CR3View(widget);
    QScrollBar* scrollBar = new QScrollBar(Qt::Vertical, widget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(view, 10);
    layout->addWidget(scrollBar, 0);
    view->setScrollBar(scrollBar);
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

    QString configDir = cr2qt(getConfigDir());
    QString engineDataDir = cr2qt(getEngineDataDir());
    QString cssFile = configDir + "fb2.css";
    QString cssFileInEngineDir = engineDataDir + "fb2.css";
    QString mainHyphDir = engineDataDir + "hyph" + QDir::separator();
    QString userHyphDir = configDir + "hyph" + QDir::separator();
    QString bookmarksDir = configDir + "bookmarks" + QDir::separator();

    view->setHyphDir(mainHyphDir);
    view->setHyphDir(userHyphDir + "hyph" + QDir::separator(), false);
    view->setPropsChangeCallback(this);
    view->setDocViewStatusCallback(this);
    if (!view->loadCSS(cssFile)) {
        view->loadCSS(cssFileInEngineDir);
    }
#if ENABLE_BOOKMARKS_DIR == 1
    view->setBookmarksDir(bookmarksDir);
#endif
    TabData tab(widget, layout, view, scrollBar);
    tab.setTitle(view->getDocTitle());
    return tab;
}

void MainWindow::addNewDocTab() {
    if (_tabs.size() >= MAX_TABS_COUNT) {
        QMessageBox::warning(this, tr("Warning"), tr("The maximum number of tabs has been exceeded!"), QMessageBox::Ok);
        return;
    }
    TabData tab = createNewDocTabWidget();
    if (tab.isValid()) {
        _tabs.append(tab);
        int tabIndex = ui->tabWidget->addTab(tab.widget(), tab.title());
        ui->tabWidget->setCurrentIndex(tabIndex);
        ui->tabWidget->setTabToolTip(tabIndex, tab.title());
    } else {
        CRLog::error("MainWindow::addNewDocTab(): failed to create new tab!");
    }
}

void MainWindow::closeDocTab(int index) {
    if (index >= 0 && index < _tabs.size()) {
        CRLog::debug("MainWindow::closeDocTab(): closing tab with index=%d", index);
        _tabs.saveHistory();
        TabData tab = _tabs[index];
        // First, we remove the element from the '_tabs' container
        //  so that we use actual data in the tab's currentChanged() handler.
        //  Which is called after the tab is removed.
        _tabs.remove(index);
        // Remove tab
        ui->tabWidget->removeTab(index);
        //  and delete tab widgets
        tab.cleanup();
    } else {
        CRLog::error("MainWindow::closeDocTab(): invalid index specified (%d), _tabs.size()=%d", index, _tabs.size());
    }
}

CR3View* MainWindow::currentCRView() const {
    CR3View* view = NULL;
    int tabIndex = ui->tabWidget->currentIndex();
    if (tabIndex >= 0 && tabIndex < _tabs.size()) {
        return _tabs[tabIndex].view();
    } else {
        CRLog::error(
                "MainWindow::currentCRView(): invalid current tab index (%d), tabWidget->count()=%d, _tabs.size()=%d",
                tabIndex, ui->tabWidget->count(), _tabs.size());
    }
    return view;
}

void MainWindow::syncTabWidget(const QString& currentDocument) {
    QString currentDocFilePath = currentDocument;
    if (currentDocFilePath.isEmpty()) {
        QWidget* widget = ui->tabWidget->currentWidget();
        for (TabsCollection::const_iterator it = _tabs.begin(); it != _tabs.end(); ++it) {
            const TabData& tab = *it;
            if (widget == tab.widget()) {
                currentDocFilePath = tab.docPath();
                break;
            }
        }
    }
    ui->tabWidget->blockSignals(true);
    ui->tabWidget->clear();
    int tabIndex = -1;
    QString currentTitle;
    for (TabsCollection::const_iterator it = _tabs.begin(); it != _tabs.end(); ++it) {
        const TabData& tab = *it;
        int index = ui->tabWidget->addTab(tab.widget(), tab.title());
        ui->tabWidget->setTabToolTip(index, tab.title());
        if (tab.docPath() == currentDocFilePath)
            tabIndex = index;
    }
    ui->tabWidget->blockSignals(false);
    if (-1 == tabIndex)
        tabIndex = ui->tabWidget->count() - 1;
    if (tabIndex >= 0) {
        ui->tabWidget->setCurrentIndex(tabIndex);
        currentTitle = _tabs[tabIndex].title();
    }
    if (!currentTitle.isEmpty())
        setWindowTitle("crqt-ng - " + currentTitle);
    else
        setWindowTitle("crqt-ng");
}

QString MainWindow::openFileDialogImpl() {
    QString lastPath;
    LVPtrVector<CRFileHistRecord>& files = _tabs.getHistory()->getRecords();
    if (files.length() > 0) {
        lString32 pathname = files[0]->getFilePathName();
        lString32 arcPathName, arcItemPathName;
        if (LVSplitArcName(pathname, arcPathName, arcItemPathName))
            lastPath = cr2qt(LVExtractPath(arcPathName));
        else
            lastPath = cr2qt(LVExtractPath(pathname));
    }
    QString all_fmt_flt =
#if (USE_CHM == 1) && ((USE_CMARK_GFM == 1) || (USE_MD4C == 1))
            QString(" (*.fb2 *.fb3 *.txt *.tcr *.rtf *.odt *.doc *.docx *.epub *.html *.shtml *.htm *.md *.chm *.zip *.pdb *.pml *.prc *.pml *.mobi);;");
#elif (USE_CHM == 1)
            QString(" (*.fb2 *.fb3 *.txt *.tcr *.rtf *.odt *.doc *.docx *.epub *.html *.shtml *.htm *.chm *.zip *.pdb *.pml *.prc *.pml *.mobi);;");
#else
            QString(" (*.fb2 *.fb3 *.txt *.tcr *.rtf *.odt *.doc *.docx *.epub *.html *.shtml *.htm *.zip *.pdb *.pml *.prc *.pml *.mobi);;");
#endif

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open book file"), lastPath,
                                                    // clang-format off
            tr("All supported formats") + all_fmt_flt +
                    tr("FB2 books") + QString(" (*.fb2 *.fb2.zip);;") +
                    tr("FB3 books") + QString(" (*.fb3);;") +
                    tr("Text files") + QString(" (*.txt);;") +
                    tr("Rich text") +  QString(" (*.rtf);;") +
                    tr("MS Word document") + QString(" (*.doc *.docx);;") +
                    tr("Open Document files") + QString(" (*.odt);;") +
                    tr("HTML files") + QString(" (*.shtml *.htm *.html);;") +
#if (USE_CMARK_GFM == 1) || (USE_MD4C == 1)
                    tr("Markdown files") + QString(" (*.md);;") +
#endif
                    tr("EPUB files") + QString(" (*.epub);;") +
#if USE_CHM == 1
                    tr("CHM files") + QString(" (*.chm);;") +
#endif
                    tr("MOBI files") + QString(" (*.mobi *.prc *.azw);;") +
                    tr("PalmDOC files") + QString(" (*.pdb *.pml);;") +
                    tr("ZIP archives") + QString(" (*.zip)"));
    // clang-format on
    return fileName;
}

bool MainWindow::isExternalLink(const QString& href) {
    bool res = false;
    if (!href.startsWith('#')) {
        int pos = href.indexOf(':');
        if (pos >= 0) {
            QString protocol = href.left(pos);
            res = protocol != "file";
        }
    }
    return res;
}

void MainWindow::on_actionExport_triggered() {
    CR3View* view = currentCRView();
    if (view == nullptr) {
        CRLog::debug("NULL view in current tab!");
        return;
    }

    // Dialog has WA_DeleteOnClose attribute, so it auto-deletes when closed
    XtExportDlg* dlg = new XtExportDlg(this, view->getDocView());
    dlg->setWindowTitle(tr("导出为 XT* 格式"));
    dlg->exec();
}

void MainWindow::on_actionOpen_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    QString fileName = openFileDialogImpl();
    if (fileName.length() == 0)
        return;
    if (!view->loadDocument(fileName)) {
        // error
    } else {
        update();
    }
}

void MainWindow::on_actionMinimize_triggered() {
    showMinimized();
}

void MainWindow::on_actionResizeToXteink_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view)
        return;

    // Default resolution
    int width = 480;
    int height = 800;

    // Try to get resolution from last used export profile
    CRPropRef props = getSettings();
    QString lastProfile = cr2qt(props->getStringDef("xtexport.last.profile", ""));

    if (!lastProfile.isEmpty()) {
        XtExportProfileManager profileManager;
        profileManager.initialize();
        int idx = profileManager.indexOfProfile(lastProfile);
        if (idx >= 0) {
            XtExportProfile* profile = profileManager.profileByIndex(idx);
            if (profile && profile->isValid()) {
                width = profile->width;
                height = profile->height;
            }
        }
    }

    view->setDocViewSize(width, height);
}

void MainWindow::resizeDocViewToSize(int width, int height) {
    CR3View* view = currentCRView();
    if (NULL == view)
        return;
    view->setDocViewSize(width, height);
}

void MainWindow::on_actionClose_triggered() {
    close();
}

void MainWindow::on_actionNextPage_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->nextPage();
}

void MainWindow::on_actionPrevPage_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->prevPage();
}

void MainWindow::on_actionNextPage2_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->nextPage();
}

void MainWindow::on_actionPrevPage2_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->prevPage();
}

void MainWindow::on_actionPrevPage3_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->prevPage();
}

void MainWindow::on_actionNextLine_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->nextLine();
}

void MainWindow::on_actionPrevLine_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->prevLine();
}

void MainWindow::on_actionFirstPage_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->firstPage();
}

void MainWindow::on_actionLastPage_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->lastPage();
}

void MainWindow::on_actionBack_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->historyBack();
}

void MainWindow::on_actionForward_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->historyForward();
}

void MainWindow::on_actionNextChapter_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->nextChapter();
}

void MainWindow::on_actionPrevChapter_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->prevChapter();
}

void MainWindow::on_actionToggle_Pages_Scroll_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->togglePageScrollView();
}

void MainWindow::on_actionToggle_Full_Screen_triggered() {
    toggleProperty(PROP_APP_WINDOW_FULLSCREEN);
}

void MainWindow::on_actionZoom_In_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->zoomIn();
}

void MainWindow::on_actionZoom_Out_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->zoomOut();
}

void MainWindow::on_actionTOC_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    TocDlg::showDlg(this, view);
}

void MainWindow::on_actionRecentBooks_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    RecentBooksDlg::showDlg(this, view);
}

void MainWindow::on_actionSettings_triggered() {
    CR3View* currView = currentCRView();
    if (NULL == currView) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    SettingsDlg dlg(this, currView->getOptions(), currView);
    if (dlg.exec() == QDialog::Accepted) {
        for (TabsCollection::iterator it = _tabs.begin(); it != _tabs.end(); ++it) {
            CR3View* view = (*it).view();
            if (NULL != view) {
                PropsRef newProps = dlg.options();
                _tabs.setSettings(qt2cr(newProps));
                view->applyOptions(newProps, view != currView);
                view->getDocView()->requestRender();
                // docview is not rendered here, only planned
                _tabs.saveSettings();
            }
        }
    }
}

void MainWindow::toggleProperty(const char* name) {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->toggleProperty(name);
}

void MainWindow::onPropsChange(PropsRef props) {
    for (int i = 0; i < props->count(); i++) {
        QString const name = props->name(i);
        QString const value = props->value(i);
        int iv = value.toInt();
        bool bv = iv != 0;
        CRLog::debug("MainWindow::onPropsChange [%d] '%s'=%s ", i, props->name(i), props->value(i).toUtf8().data());
        if (name == PROP_APP_WINDOW_FULLSCREEN) {
            bool state = windowState().testFlag(Qt::WindowFullScreen);
            if (state != bv)
                setWindowState(windowState() ^ Qt::WindowFullScreen);
        } else if (name == PROP_APP_WINDOW_SHOW_MENU) {
            ui->menuBar->setVisible(bv);
        } else if (name == PROP_APP_WINDOW_SHOW_SCROLLBAR) {
            for (int i = 0; i < _tabs.count(); i++) {
                const TabData& tab = _tabs[i];
                if (NULL != tab.scrollBar())
                    tab.scrollBar()->setVisible(bv);
            }
        } else if (name == PROP_APP_BACKGROUND_IMAGE) {
            lString32 fn = qt2cr(value);
            LVImageSourceRef img;
            if (!fn.empty() && fn[0] != '[') {
                CRLog::debug("Background image file: %s", LCSTR(fn));
                LVStreamRef stream = LVOpenFileStream(fn.c_str(), LVOM_READ);
                if (!stream.isNull()) {
                    img = LVCreateStreamImageSource(stream);
                }
            }
            fn.lowercase();
            bool tiled = (fn.pos(cs32("\\textures\\")) >= 0 || fn.pos(cs32("/textures/")) >= 0);
            for (int i = 0; i < _tabs.count(); i++) {
                const TabData& tab = _tabs[i];
                if (NULL != tab.view())
                    tab.view()->getDocView()->setBackgroundImage(img, tiled);
            }
        } else if (name == PROP_APP_WINDOW_SHOW_TOOLBAR) {
            ui->mainToolBar->setVisible(bv);
        } else if (name == PROP_APP_WINDOW_SHOW_STATUSBAR) {
            ui->statusBar->setVisible(bv);
        } else if (name == PROP_APP_WINDOW_STYLE) {
            QApplication::setStyle(value);
        } else if (name == PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY) {
            for (int i = 0; i < _tabs.count(); i++) {
                const TabData& tab = _tabs[i];
                if (NULL != tab.view())
                    tab.view()->setOnTextSelectAutoClipboardCopy(bv);
            }
        } else if (name == PROP_APP_SELECTION_AUTO_CMDEXEC) {
            for (int i = 0; i < _tabs.count(); i++) {
                const TabData& tab = _tabs[i];
                if (NULL != tab.view())
                    tab.view()->setOnTextSelectAutoCmdExec(bv);
            }
        } else if (name == PROP_APP_SELECTION_COMMAND) {
            for (int i = 0; i < _tabs.count(); i++) {
                const TabData& tab = _tabs[i];
                if (NULL != tab.view())
                    tab.view()->setSelectionCommand(value);
            }
        } else if (name == PROP_APP_TABS_FIXED_SIZE) {
            if (bv) {
                ui->tabWidget->setStyleSheet("QTabBar::tab { width: 7em; }");
            } else {
                ui->tabWidget->setStyleSheet("");
            }
        }
    }
}

void MainWindow::onDocumentLoaded(lUInt64 viewId, const QString& atitle, const QString& error,
                                  const QString& fullDocPath) {
    CRLog::debug("MainWindow::onDocumentLoaded '%s', error=%s ", atitle.toLocal8Bit().constData(),
                 error.toLocal8Bit().constData());
    int cbIndex = _tabs.indexByViewId(viewId);
    int currentIndex = ui->tabWidget->currentIndex();
    if (cbIndex >= 0) {
        if (error.isEmpty()) {
            TabData& tab = _tabs[cbIndex];
            tab.setTitle(atitle);
            tab.setDocPath(fullDocPath);
            ui->tabWidget->setTabText(cbIndex, atitle);
            ui->tabWidget->setTabToolTip(cbIndex, atitle);
        }
        if (cbIndex == currentIndex) {
            if (error.isEmpty()) {
                setWindowTitle("crqt-ng - " + atitle);
            } else {
                setWindowTitle("crqt-ng");
            }
        }
    }
}

void MainWindow::onCanGoBack(lUInt64 viewId, bool canGoBack) {
    int cbIndex = _tabs.indexByViewId(viewId);
    int currentIndex = ui->tabWidget->currentIndex();
    if (cbIndex >= 0 && cbIndex < _tabs.size()) {
        _tabs[cbIndex].setCanGoBack(canGoBack);
        if (cbIndex == currentIndex)
            ui->actionBack->setEnabled(canGoBack);
    }
}

void MainWindow::onCanGoForward(lUInt64 viewId, bool canGoForward) {
    int cbIndex = _tabs.indexByViewId(viewId);
    int currentIndex = ui->tabWidget->currentIndex();
    if (cbIndex >= 0 && cbIndex < _tabs.size()) {
        _tabs[cbIndex].setCanGoForward(canGoForward);
        if (cbIndex == currentIndex)
            ui->actionForward->setEnabled(canGoForward);
    }
}

void MainWindow::onHoverLink(const QString& href) {
    if (href.isEmpty())
        ui->statusBar->clearMessage();
    else
        ui->statusBar->showMessage(href);
}

void MainWindow::onOpenInNewTabRequested(const QString& href) {
    ui->actionOpen_link_in_new_tab->setData(href);
    emit ui->actionOpen_link_in_new_tab->triggered();
}

void MainWindow::contextMenu(QPoint pos) {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    QMenu* menu = new QMenu;
    if (view->isPointInsideSelection(pos)) {
        menu->addAction(ui->actionCopy);
        menu->addAction(ui->actionAddBookmark);
    } else {
        QString linkHRef = view->getLinkAtPoint(pos);
        if (!linkHRef.isEmpty() && !isExternalLink(linkHRef)) {
            ui->actionOpen_link_in_new_tab->setData(linkHRef);
            menu->addAction(ui->actionOpen_link_in_new_tab);
        }
        menu->addAction(ui->actionOpen);
        menu->addAction(ui->actionRecentBooks);
        menu->addAction(ui->actionExport);
        menu->addAction(ui->actionTOC);
        // menu->addAction(ui->actionToggle_Full_Screen);
        menu->addAction(ui->actionSettings);
        menu->addAction(ui->actionAddBookmark);
        menu->addAction(ui->actionClose);
    }
    menu->exec(view->mapToGlobal(pos));
}

void MainWindow::on_actionCopy_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    QString txt = view->getSelectionText();
    if (txt.length() > 0) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(txt, QClipboard::Clipboard);
    }
}

void MainWindow::on_actionCopy2_triggered() {
    on_actionCopy_triggered();
}

static bool firstShow = true;

void MainWindow::showEvent(QShowEvent* event) {
    if (!firstShow)
        return;
    CRLog::debug("first showEvent()");
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    firstShow = false;
    int n = view->getOptions()->getIntDef(PROP_APP_START_ACTION, 0);
    if (!_filenamesToOpen.isEmpty()) {
        // file names specified at command line
        CRLog::info("Startup Action: filename passed in command line");
        int processed = 0;
        int index = 0;
        TabData tab;
        for (QStringList::const_iterator it = _filenamesToOpen.begin(); it != _filenamesToOpen.end(); ++it, ++index) {
            const QString& filePath = *it;
            if (index < _tabs.size()) {
                tab = _tabs[index];
            } else {
                tab = createNewDocTabWidget();
                _tabs.append(tab);
            }
            // When opening files from the command line,
            //  open them immediately without delay.
            //  To have information on all tabs at once.
            tab.view()->setActive(true);
            if (!tab.view()->loadDocument(filePath)) {
                CRLog::error("cannot load document \"%s\"", filePath.toLocal8Bit().constData());
            }
            processed++;
            if (processed >= MAX_TABS_COUNT)
                break;
        }
        if (_tabs.size() > processed) {
            for (int i = processed; i < _tabs.size(); i++) {
                _tabs[i].cleanup();
            }
            _tabs.resize(processed);
        }
        if (0 == _tabs.size()) {
            tab = createNewDocTabWidget();
            if (tab.isValid()) {
                _tabs.append(tab);
            }
        }
        syncTabWidget();
    } else if (n == 0) {
        // restore session
        CRLog::info("Startup Action: Restore session (restore tabs)");
        bool ok;
        TabsCollection::TabSession session = _tabs.openTabSession(cr2qt(getConfigDir()) + "tabs.ini", &ok);
        if (ok && session.size() > 0) {
            int processed = 0;
            int index = 0;
            TabData tab;
            for (TabsCollection::TabSession::const_iterator it = session.begin(); it != session.end(); ++it, ++index) {
                const TabsCollection::TabProperty& data = *it;
                if (index < _tabs.size()) {
                    tab = _tabs[index];
                } else {
                    tab = createNewDocTabWidget();
                    _tabs.append(tab);
                }
                tab.setTitle(data.title);
                tab.setDocPath(data.docPath);
                _tabs[index] = tab;
                if (!data.docPath.isEmpty()) {
                    if (!tab.view()->loadDocument(data.docPath)) {
                        CRLog::error("cannot load document \"%s\"", data.docPath.toLocal8Bit().constData());
                    }
                }
                processed++;
                if (processed >= MAX_TABS_COUNT)
                    break;
            }
            if (_tabs.size() > processed) {
                for (int i = processed; i < _tabs.size(); i++) {
                    _tabs[i].cleanup();
                }
                _tabs.resize(processed);
            }
            if (0 == _tabs.size()) {
                tab = createNewDocTabWidget();
                if (tab.isValid()) {
                    _tabs.append(tab);
                }
            }
            syncTabWidget(session.currentDocument);
        } else {
            view->loadLastDocument();
        }
    } else if (n == 1) {
        // show recent books dialog
        CRLog::info("Startup Action: Show recent books dialog");
        //hide();
        RecentBooksDlg::showDlg(this, view);
        //show();
    } else if (n == 2) {
        // show file open dialog
        CRLog::info("Startup Action: Show file open dialog");
        //hide();
        on_actionOpen_triggered();
        //RecentBooksDlg::showDlg( ui->view );
        //show();
    }
    // Automatically resize to Xteink device dimensions on first show
    on_actionResizeToXteink_triggered();
}

static bool firstFocus = true;

void MainWindow::focusInEvent(QFocusEvent* event) {
    if (!firstFocus)
        return;
    CRLog::debug("first focusInEvent()");
    //    int n = ui->view->getOptions()->getIntDef( PROP_APP_START_ACTION, 0 );
    //    if ( n==1 ) {
    //        // show recent books dialog
    //        CRLog::info("Startup Action: Show recent books dialog");
    //        RecentBooksDlg::showDlg( ui->view );
    //    }

    firstFocus = false;
}

void MainWindow::on_actionAboutQT_triggered() {
    QApplication::aboutQt();
}

void MainWindow::on_actionAbout_triggered() {
    AboutDialog::showDlg(this);
}

void MainWindow::on_actionAddBookmark_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    AddBookmarkDialog::showDlg(this, view);
}

void MainWindow::on_actionShowBookmarksList_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    BookmarkListDialog::showDlg(this, view);
}

void MainWindow::on_actionFileProperties_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    FilePropsDialog::showDlg(this, view);
}

void MainWindow::on_actionFindText_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    SearchDialog::showDlg(this, view);
    //    QMessageBox * mb = new QMessageBox( QMessageBox::Information, tr("Not implemented"), tr("Search is not implemented yet"), QMessageBox::Close, this );
    //    mb->exec();
}

void MainWindow::on_actionRotate_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->rotate(1);
}

void MainWindow::on_actionToggleEditMode_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->setEditMode(!view->getEditMode());
}

void MainWindow::on_actionNextPage3_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->nextPage();
}

void MainWindow::on_actionNextSentence_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->nextSentence();
}

void MainWindow::on_actionPrevSentence_triggered() {
    CR3View* view = currentCRView();
    if (NULL == view) {
        CRLog::debug("NULL view in current tab!");
        return;
    }
    view->prevSentence();
}

void MainWindow::on_actionNew_tab_triggered() {
    addNewDocTab();
}

void MainWindow::on_tabWidget_currentChanged(int index) {
    QString title;
    if (_prevIndex >= 0 && _prevIndex < _tabs.size()) {
        const TabData& tab = _tabs[_prevIndex];
        CR3View* view = tab.view();
        if (NULL != view) {
            view->getDocView()->swapToCache();
            view->getDocView()->savePosition();
            view->setActive(false);
        }
    }
    if (index >= 0 && index < _tabs.size()) {
        const TabData& tab = _tabs[index];
        CR3View* view = tab.view();
        if (NULL != view) {
            view->setActive(true);
            title = tab.title();
            view->getDocView()->swapToCache();
            view->getDocView()->savePosition();
            ui->actionBack->setEnabled(tab.canGoBack());
            ui->actionForward->setEnabled(tab.canGoForward());
        }
    }
    if (!title.isEmpty())
        setWindowTitle("crqt-ng - " + title);
    else
        setWindowTitle("crqt-ng");
    _prevIndex = index;
}

void MainWindow::on_tabWidget_tabCloseRequested(int index) {
    closeDocTab(index);
}

void MainWindow::on_actionOpen_in_new_tab_triggered() {
    if (_tabs.size() >= MAX_TABS_COUNT) {
        QMessageBox::warning(this, tr("Warning"), tr("The maximum number of tabs has been exceeded!"), QMessageBox::Ok);
        return;
    }
    QString fileName = openFileDialogImpl();
    if (fileName.length() == 0)
        return;
    TabData tab = createNewDocTabWidget();
    if (tab.isValid()) {
        _tabs.append(tab);
        CR3View* view = tab.view();
        int tabIndex = ui->tabWidget->addTab(tab.widget(), tab.title());
        ui->tabWidget->setCurrentIndex(tabIndex);
        ui->tabWidget->setTabToolTip(tabIndex, tab.title());
        if (!view->loadDocument(fileName)) {
            // error
        } else {
            update();
        }
    } else {
        CRLog::error("MainWindow::on_actionOpen_in_new_tab_triggered(): failed to create new tab!");
    }
}

void MainWindow::on_actionOpen_link_in_new_tab_triggered() {
    if (_tabs.size() >= MAX_TABS_COUNT) {
        QMessageBox::warning(this, tr("Warning"), tr("The maximum number of tabs has been exceeded!"), QMessageBox::Ok);
        return;
    }
    QString linkHRef = ui->actionOpen_link_in_new_tab->data().toString();
    if (!linkHRef.isEmpty() && !isExternalLink(linkHRef)) {
        QString currentDocFilePath;
        lString32 currentDocDir32;
        int pos = linkHRef.indexOf('#');
        QString filename = linkHRef.left(pos);
        QString intHRef = linkHRef.mid(pos);
        if (filename.startsWith("file://"))
            filename = filename.mid(7);
        lString32 filename32 = qt2cr(filename);
        int currentIndex = ui->tabWidget->currentIndex();
        if (currentIndex >= 0) {
            TabData& tab = _tabs[currentIndex];
            currentDocFilePath = tab.docPath();
            // We cannot use QFileInfo::absolutePath() here,
            //  as this path could be the path to an archive asset &
            //  we don't know how QFileInfo handles
            //  (will handle in future versions of Qt) such paths.
            currentDocDir32 = LVExtractPath(qt2cr(currentDocFilePath), true);
        }
        if (filename.isEmpty())
            filename = currentDocFilePath;
        else if (!LVIsAbsolutePath(filename32)) {
            filename = cr2qt(LVCombinePaths(currentDocDir32, filename32));
        }
        TabData tab = createNewDocTabWidget();
        if (tab.isValid()) {
            _tabs.append(tab);
            CR3View* view = tab.view();
            int tabIndex = ui->tabWidget->addTab(tab.widget(), tab.title());
            ui->tabWidget->setCurrentIndex(tabIndex);
            ui->tabWidget->setTabToolTip(tabIndex, tab.title());
            if (view->loadDocument(filename)) {
                if (!intHRef.isEmpty())
                    view->getDocView()->goLink(qt2cr(intHRef), false);
                update();
            } else {
                // error
            }
        } else {
            CRLog::error("MainWindow::on_actionOpen_in_new_tab_triggered(): failed to create new tab!");
        }
    }
}
