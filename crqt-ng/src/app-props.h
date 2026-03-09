/***************************************************************************
 *   crqt-ng                                                               *
 *   Copyright (c) 2022,2023 Aleksey Chernov <valexlin@gmail.com>          *
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

/*
 * Based on CoolReader code at https://github.com/buggins/coolreader
 * Copyright (C) 2010-2021 by Vadim Lopatin <coolreader.org@gmail.com>
 */

#ifndef APP_PROPS_H
#define APP_PROPS_H

//#define PROP_APP_WINDOW_RECT                "window.rect"
//#define PROP_APP_WINDOW_ROTATE_ANGLE        "window.rotate.angle"
#define PROP_APP_WINDOW_FULLSCREEN             "window.fullscreen"
#define PROP_APP_WINDOW_MINIMIZED              "window.minimized"
#define PROP_APP_WINDOW_MAXIMIZED              "window.maximized"
#define PROP_APP_WINDOW_SHOW_MENU              "window.menu.show"
#define PROP_APP_WINDOW_SHOW_TOOLBAR           "window.toolbar.show"
#define PROP_APP_WINDOW_TOOLBAR_POSITION       "window.toolbar.position"
#define PROP_APP_WINDOW_SHOW_STATUSBAR         "window.statusbar.show"
#define PROP_APP_WINDOW_SHOW_SCROLLBAR         "window.scrollbar.show"
#define PROP_APP_WINDOW_STYLE                  "window.style"
#define PROP_APP_START_ACTION                  "cr3.app.start.action"
#define PROP_APP_SELECTION_AUTO_CLIPBOARD_COPY "crui.app.selection.auto.clipboard.copy"
#define PROP_APP_SELECTION_AUTO_CMDEXEC        "crui.app.selection.auto.cmdexec"
#define PROP_APP_SELECTION_COMMAND             "crui.app.selection.command"
#define PROP_APP_BACKGROUND_IMAGE              "background.image.filename"
#define PROP_APP_LOG_FILENAME                  "crengine.log.filename"
#define PROP_APP_LOG_LEVEL                     "crengine.log.level"
#define PROP_APP_LOG_AUTOFLUSH                 "crengine.log.autoflush"
#define PROP_APP_TABS_FIXED_SIZE               "crui.app.tabs.fixedsize"
#define PROP_APP_SETTINGS_TAB                  "crui.app.settings.tab"
#define PROP_APP_SETTINGS_STYLE_ELEMENT        "crui.app.settings.style.element"
#define PROP_CSS_USE_GENERATED                 "css.use.generated"
#define PROP_CSS_CURRENT_TEMPLATE              "css.current.template"
#define CSS_EXPANDED_SUFFIX                    "-expanded.css"

#endif // APP_PROPS_H
