/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (C) 2009-2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2021-2024 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QMainWindow>
#else
#include <QtGui/QMainWindow>
#endif
#include "cr3widget.h"
#include "tabscollection.h"

namespace Ui
{
    class MainWindowClass;
}

class MainWindow: public QMainWindow, public PropsChangeCallback, DocViewStatusCallback
{
    Q_OBJECT
public:
    MainWindow(const QStringList& filesToOpen, QWidget* parent = 0);
    ~MainWindow();
    CRPropRef getSettings() { return _tabs.getSettings(); }
    void resizeDocViewToSize(int width, int height);
    CR3View* currentCRView() const;
private:
    Ui::MainWindowClass* ui;
    TabsCollection _tabs;
    QStringList _filenamesToOpen;
    int _prevIndex;
    void toggleProperty(const char* name);
    TabData createNewDocTabWidget();
    void addNewDocTab();
    void closeDocTab(int index);
    void syncTabWidget(const QString& currentDocument = QString());
    QString openFileDialogImpl();
    bool isExternalLink(const QString& href);
protected:
    virtual void showEvent(QShowEvent* event);
    virtual void focusInEvent(QFocusEvent* event);
    virtual void closeEvent(QCloseEvent* event);
public slots:
    void contextMenu(QPoint pos);
    void on_actionFindText_triggered();
private:
    virtual void onPropsChange(PropsRef props);
    virtual void onDocumentLoaded(lUInt64 viewId, const QString& atitle, const QString& error,
                                  const QString& fullDocPath);
    virtual void onCanGoBack(lUInt64 viewId, bool canGoBack);
    virtual void onCanGoForward(lUInt64 viewId, bool canGoForward);
    virtual void onHoverLink(const QString& href);
    virtual void onOpenInNewTabRequested(const QString& href);
private slots:
    void on_actionNextPage3_triggered();
    void on_actionToggleEditMode_triggered();
    void on_actionRotate_triggered();
    void on_actionFileProperties_triggered();
    void on_actionShowBookmarksList_triggered();
    void on_actionAddBookmark_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutQT_triggered();
    void on_actionCopy2_triggered();
    void on_actionCopy_triggered();
    void on_actionSettings_triggered();
    void on_actionRecentBooks_triggered();
    void on_actionTOC_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionZoom_In_triggered();
    void on_actionToggle_Full_Screen_triggered();
    void on_actionToggle_Pages_Scroll_triggered();
    void on_actionPrevChapter_triggered();
    void on_actionNextChapter_triggered();
    void on_actionForward_triggered();
    void on_actionBack_triggered();
    void on_actionLastPage_triggered();
    void on_actionFirstPage_triggered();
    void on_actionPrevLine_triggered();
    void on_actionNextLine_triggered();
    void on_actionPrevPage_triggered();
    void on_actionNextPage_triggered();
    void on_actionPrevPage2_triggered();
    void on_actionPrevPage3_triggered();
    void on_actionNextPage2_triggered();
    void on_actionClose_triggered();
    void on_actionResizeToXteink_triggered();
    void on_actionMinimize_triggered();
    void on_actionOpen_triggered();
    void on_actionExport_triggered();
    void on_actionNextSentence_triggered();
    void on_actionPrevSentence_triggered();
    void on_actionNew_tab_triggered();
    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabCloseRequested(int index);
    void on_actionOpen_in_new_tab_triggered();
    void on_actionOpen_link_in_new_tab_triggered();
};

#endif // MAINWINDOW_H
