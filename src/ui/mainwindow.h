/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "registry/searchquery.h"
#include "registry/cancellationtoken.h"

#include <memory>
#include <QDialog>
#include <QMainWindow>
#include <QModelIndex>

#ifdef USE_WEBENGINE
#define QWebPage QWebEnginePage
#define QWebHistory QWebEngineHistory
#define QWebHistoryItem QWebEngineHistoryItem
#endif

#ifdef USE_APPINDICATOR
struct _AppIndicator;
struct _GtkWidget;
#endif

class QxtGlobalShortcut;

class QShortcut;
class QSystemTrayIcon;
class QTabBar;
class QWebHistory;
class QWebHistoryItem;
class QWebPage;

namespace Ui {
class MainWindow;
}

namespace Zeal {

namespace Core {
class Application;
class Settings;
}

class ListModel;
class NetworkAccessManager;
class SearchItemDelegate;
class SeeAlsoDelegate;
class SearchModel;
class SettingsDialog;
}

/**
 * @brief The SearchState struct
 * Represents state of the UI per each tab.
 */
struct SearchState
{
    // Currently rendered web page.
    QWebPage *page;

    // Model representing sections (see also links).
    Zeal::SearchModel *sectionsList;
    // Model representing searched for items (search query results).
    Zeal::SearchModel *zealSearch;
    // Query being searched for.
    QString searchQuery;

    // List of selected items in the ToC/search list.
    QModelIndexList selections;
    // List of expanded items in the ToC.
    QModelIndexList expansions;

    // Scroll position of the ToC/search.
    int scrollPosition;

    // Scroll position of the see also list.
    int sectionsScroll;

    // Zoom factor of the browser.
    int zoomFactor;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Zeal::Core::Application *app, QWidget *parent = nullptr);
    ~MainWindow() override;

    void bringToFront(const QString &query = QString());
    void createTab();
    void showSettings();

public slots:
    void toggleWindow();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;

private slots:
    void applySettings();
    void back();
    void forward();
    void onSearchComplete();
    void deferOpenDocset(const QModelIndex &index);
    void openDocset(const QModelIndex &index);
    void queryCompleted();
    void scrollSearch();
    void saveTabState();
    void goToTab(int index);
    void closeTab(int index = -1);

private:
    void displayViewActions();
    void displayTitle();
    void displayTreeView();
    void displaySections();
    void setupSearchBoxCompletions();
    void reloadTabState();
    void displayTabs();
    void navigateToc(QKeyEvent *event);
    void navigatePage(QKeyEvent *event);
    SearchState *currentSearchState();
    QString docsetName(const QUrl &url) const;
    QIcon docsetIcon(const QUrl &url) const;
    QAction *addHistoryAction(QWebHistory *history, const QWebHistoryItem &item);
    void createTrayIcon();
    void removeTrayIcon();

    QList<SearchState *> m_tabs;

    // Pointer to a `SearchState` for the currently opened tab.
    SearchState *m_searchState = nullptr;

    std::unique_ptr<Ui::MainWindow> ui;
    Zeal::Core::Application *m_application = nullptr;
    Zeal::Core::Settings *m_settings = nullptr;
    std::unique_ptr<Zeal::NetworkAccessManager> m_zealNetworkManager;
    std::unique_ptr<Zeal::ListModel> m_zealListModel;
    std::unique_ptr<Zeal::SearchItemDelegate> m_searchItemDelegate;
    std::unique_ptr<Zeal::SeeAlsoDelegate> m_seeAlsoItemDelegate;
    std::unique_ptr<Zeal::SettingsDialog> m_settingsDialog;

    std::unique_ptr<QMenu> m_backMenu;
    std::unique_ptr<QMenu> m_forwardMenu;

    Zeal::CancellationToken m_cancelSearch;

    std::unique_ptr<QTimer> m_deferOpenUrl;
    bool m_treeViewClicked = false;

    QxtGlobalShortcut *m_globalShortcut = nullptr;

    std::unique_ptr<QTabBar> m_tabBar;

    QSystemTrayIcon *m_trayIcon = nullptr;

#ifdef USE_APPINDICATOR
    _AppIndicator *m_appIndicator = nullptr;
    _GtkWidget *m_appIndicatorMenu = nullptr;
    _GtkWidget *m_appIndicatorQuitMenuItem = nullptr;
    _GtkWidget *m_appIndicatorShowHideMenuItem = nullptr;
    _GtkWidget *m_appIndicatorMenuSeparator = nullptr;
#endif
};

#endif // MAINWINDOW_H
