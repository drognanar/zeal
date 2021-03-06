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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "zealjs.h"
#include "networkaccessmanager.h"
#include "searchitemdelegate.h"
#include "seealsodelegate.h"
#include "settingsdialog.h"
#include "core/application.h"
#include "core/settings.h"
#include "registry/docsetregistry.h"
#include "registry/listmodel.h"
#include "registry/searchmodel.h"
#include "ui/icons.h"
#include "ui/customfusiontabstyle.h"

#include <QAbstractEventDispatcher>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTabBar>
#include <QTimer>

#ifdef USE_WEBENGINE
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#else
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#endif

#ifdef Q_OS_MACX
#include <QStyleFactory>
#endif
#include <qxtglobalshortcut.h>

/// TODO: [Qt 5.5] Remove in favour of native Qt support (QTBUG-31762)
#ifdef USE_APPINDICATOR
#undef signals
#include <libappindicator/app-indicator.h>
#define signals public
#include <gtk/gtk.h>
#endif

using namespace Zeal;

namespace {
const char startPageUrl[] = "qrc:///browser/start.html";
}

SearchState::SearchState()
    : sectionsList(new SearchModel()),
      zealSearch(new SearchModel())
{
}

SearchState::~SearchState()
{
}

MainWindow::MainWindow(Core::Application *app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_application(app),
    m_settings(app->settings()),
    m_zealNetworkManager(new NetworkAccessManager(this)),
    m_zealListModel(new ListModel(app->docsetRegistry(), this)),
    m_seeAlsoItemDelegate(new SeeAlsoDelegate()),
    m_settingsDialog(new SettingsDialog(app, this)),
    m_zealJs(new ZealJS(this)),
    m_deferOpenUrl(new QTimer()),
    m_globalShortcut(new QxtGlobalShortcut(m_settings->showShortcut, this)),
    m_tabBar(new QTabBar(this))
{
    connect(m_settings, &Core::Settings::updated, this, &MainWindow::applySettings);

    QFontDatabase::addApplicationFont(":/font/ionicons.ttf");

    m_tabBar->installEventFilter(this);

    setWindowIcon(QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.ico"))));

    applySettings();

    // Initialise key grabber.
    connect(m_globalShortcut, &QxtGlobalShortcut::activated, this, &MainWindow::toggleWindow);

    // Initialise ui.
    ui->setupUi(this);

    ui->actionSearchDocsets->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
    connect(ui->actionSearchDocsets, &QAction::triggered,
            ui->lineEdit, static_cast<void (SearchEdit::*)()>(&SearchEdit::setFocus));

    ui->actionSearchInPage->setShortcut(QKeySequence::Find);
    connect(ui->actionSearchInPage, &QAction::triggered, [=]() {
        ui->webView->showSearch();
    });

    ui->actionFindNext->setShortcut(QKeySequence::FindNext);
    connect(ui->actionFindNext, &QAction::triggered, [=]() {
        ui->webView->findNext();
    });

    ui->actionFindPrevious->setShortcut(QKeySequence::FindPrevious);
    connect(ui->actionFindPrevious, &QAction::triggered, [=]() {
        ui->webView->findNext(true);
    });

    ui->backButton->setText(Icons::Back);
    ui->forwardButton->setText(Icons::Forward);
    ui->newTabButton->setText(Icons::Plus);
    ui->openUrlButton->setText(Icons::OpenLink);

    restoreGeometry(m_settings->windowGeometry);
    ui->splitter->restoreState(m_settings->verticalSplitterGeometry);
    connect(ui->splitter, &QSplitter::splitterMoved, [=](int, int) {
        m_settings->verticalSplitterGeometry = ui->splitter->saveState();
    });
    connect(ui->sectionsSplitter, &QSplitter::splitterMoved, [=](int, int) {
        if (ui->sections->isVisible())
            m_settings->sectionsSplitterSizes = ui->sectionsSplitter->sizes();
    });

    // menu
    if (QKeySequence(QKeySequence::Quit) != QKeySequence(QStringLiteral("Ctrl+Q"))) {
        ui->actionQuit->setShortcuts(QList<QKeySequence>{QKeySequence(QStringLiteral("Ctrl+Q")),
                                                         QKeySequence::Quit});
    } else {
        // Quit == Ctrl+Q - don't set the same sequence twice because it causes
        // "QAction::eventFilter: Ambiguous shortcut overload: Ctrl+Q"
        ui->actionQuit->setShortcuts(QList<QKeySequence>{QKeySequence::Quit});
    }
    connect(ui->actionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connect(ui->actionOptions, &QAction::triggered, [=]() {
        showSettings();
    });

    ui->actionBack->setShortcut(QKeySequence::Back);
    addAction(ui->actionBack);
    ui->actionForward->setShortcut(QKeySequence::Forward);
    addAction(ui->actionForward);
    connect(ui->actionBack, &QAction::triggered, this, &MainWindow::back);
    connect(ui->actionForward, &QAction::triggered, this, &MainWindow::forward);

    // Help Menu
    connect(ui->actionReportProblem, &QAction::triggered, [this]() {
        QDesktopServices::openUrl(QStringLiteral("https://github.com/zealdocs/zeal/issues"));
    });
    connect(ui->actionCheckForUpdate, &QAction::triggered,
            m_application, &Core::Application::checkForUpdate);
    connect(ui->actionAboutZeal, &QAction::triggered, [this]() {
        std::unique_ptr<AboutDialog> dialog(new AboutDialog(this));
        dialog->exec();
    });
    connect(ui->actionAboutQt, &QAction::triggered, [this]() {
        QMessageBox::aboutQt(this);
    });

    // Update check
    connect(m_application, &Core::Application::updateCheckError, [this](const QString &message) {
        QMessageBox::warning(this, QStringLiteral("Zeal"), message);
    });

    connect(m_application, &Core::Application::updateCheckDone, [this](const QString &version) {
        if (version.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("Zeal"), tr("You are using the latest Zeal version."));
            return;
        }

        const int ret = QMessageBox::information(this, QStringLiteral("Zeal"),
                                                 QString(tr("A new version <b>%1</b> is available. Open download page?")).arg(version),
                                                 QMessageBox::Yes, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(QStringLiteral("https://zealdocs.org/download.html"));
    });

    m_backMenu = std::unique_ptr<QMenu>(new QMenu(ui->backButton));
    ui->backButton->setMenu(m_backMenu.get());

    m_forwardMenu = std::unique_ptr<QMenu>(new QMenu(ui->forwardButton));
    ui->forwardButton->setMenu(m_forwardMenu.get());

    // treeView and lineEdit
    ui->lineEdit->setFocus();
    ui->lineEdit->setDocsetRegistry(m_application->docsetRegistry());
    m_searchItemDelegate = std::unique_ptr<SearchItemDelegate>(new SearchItemDelegate(ui->treeView));
    connect(ui->lineEdit, &SearchEdit::highlightChanged, [this](const QString &highlight) {
        m_searchItemDelegate->setHighlight(highlight);
    });
    ui->treeView->setItemDelegate(m_searchItemDelegate.get());

    connect(ui->treeView, &QTreeView::clicked, ui->treeView, &QTreeView::activated);

    ui->sections->setItemDelegate(m_seeAlsoItemDelegate.get());
    connect(ui->sections, &QListView::clicked, ui->sections, &QListView::activated);
    connect(ui->treeView, &QTreeView::activated, this, &MainWindow::openDocset);
    connect(ui->sections, &QListView::activated, this, &MainWindow::openDocset);
    connect(ui->forwardButton, &QToolButton::clicked, this, &MainWindow::forward);
    connect(ui->backButton, &QToolButton::clicked, this, &MainWindow::back);

    connect(ui->newTabButton, &QToolButton::clicked, this, &MainWindow::createTab);

    connect(m_zealJs.get(), &ZealJS::navigateToInstallDocsets, [this]() {
        m_settingsDialog->navigateToInstallDocsets();
        showSettings();
    });
    connect(m_zealJs.get(), &ZealJS::navigateToCreateKeywords, [this]() {
        m_settingsDialog->navigateToCreateKeywords();
        showSettings();
    });

    ui->webView->registerObject("zeal", m_zealJs.get());
    connect(ui->webView, &SearchableWebView::urlChanged, [this](const QUrl &url) {
        const QString name = docsetName(url);
        m_tabBar->setTabIcon(m_tabBar->currentIndex(), docsetIcon(url));

        Docset *docset = m_application->docsetRegistry()->docset(name);
        if (docset)
            currentSearchState()->sectionsList->setResults(docset->relatedLinks(url));

        displayViewActions();
    });
    connect(ui->webView, &SearchableWebView::loadFinished, [this](bool ok) {
        if (ok)
            displayTitle();
    });

    connect(ui->webView, &SearchableWebView::titleChanged, [this](const QString &) {
        displayViewActions();
    });

    connect(ui->webView, &SearchableWebView::linkClicked, [this](const QUrl &url) {
        const QString message = tr("Do you want to open an external link?<br>URL: <b>%1</b>");
        int ret = QMessageBox::question(this, QStringLiteral("Zeal"), message.arg(url.toString()));
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(url);
    });

    connect(m_application->docsetRegistry(), &DocsetRegistry::queryCompleted, this, &MainWindow::onSearchComplete);

    connect(m_application->docsetRegistry(), &DocsetRegistry::docsetRemoved,
            this, [this](const QString &name) {
        setupSearchBoxCompletions();
        for (SearchState *searchState : m_tabs) {
            if (docsetName(WebPageHelpers::url(searchState->page)) != name)
                continue;

            WebPageHelpers::load(searchState->page, QUrl(startPageUrl));
            /// TODO: Cleanup history
        }
    });

    connect(m_application->docsetRegistry(), &DocsetRegistry::docsetAdded,
            this, [this](const QString &) {
        setupSearchBoxCompletions();
    });

    connect(m_application->docsetRegistry(), &DocsetRegistry::keywordGroupsChanged,
            this, &MainWindow::setupSearchBoxCompletions);

    connect(ui->lineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        if (!m_searchState || text == m_searchState->searchQuery)
            return;

        m_cancelSearch.cancel();
        m_cancelSearch = CancellationToken();
        m_searchState->searchQuery = text;
        m_application->docsetRegistry()->search(text, m_cancelSearch);
        if (text.isEmpty()) {
            m_searchState->sectionsList->setResults();
            displayTreeView();
        }
    });

    connect(ui->lineEdit, &SearchEdit::arrowKeyPressed, [this](QKeyEvent *event) {
        bool altPressed = event->modifiers() & Qt::AltModifier;
        if (altPressed) {
            navigatePage(event);
        } else {
            navigateToc(event);
        }
    });

    connect(ui->lineEdit, &SearchEdit::returnPressed, [this]() {
        emit ui->treeView->activated(ui->treeView->selectionModel()->currentIndex());
    });

    ui->actionNewTab->setShortcut(QKeySequence::AddTab);
    connect(ui->actionNewTab, &QAction::triggered, this, &MainWindow::createTab);
    addAction(ui->actionNewTab);

    // save the expanded items:
    connect(ui->treeView, &QTreeView::expanded, [this](QModelIndex index) {
        if (m_searchState->expansions.indexOf(index) == -1)
            m_searchState->expansions.append(index);
    });

    connect(ui->treeView, &QTreeView::collapsed, [this](QModelIndex index) {
        m_searchState->expansions.removeOne(index);
    });

#ifdef Q_OS_WIN32
    ui->actionCloseTab->setShortcut(QKeySequence(Qt::Key_W + Qt::CTRL));
#else
    ui->actionCloseTab->setShortcut(QKeySequence::Close);
#endif
    addAction(ui->actionCloseTab);
    connect(ui->actionCloseTab, &QAction::triggered, this, [this]() { closeTab(); });

    m_tabBar->setTabsClosable(true);
    m_tabBar->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    m_tabBar->setExpanding(false);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setDrawBase(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setStyleSheet(QStringLiteral("QTabBar::tab { width: 150px; height: 30px; }"));
#ifdef Q_OS_MACX
    m_tabBar->setStyle(new CustomFusionTabStyle(QStyleFactory::create("Fusion")));
#endif

    connect(m_tabBar.get(), &QTabBar::currentChanged, this, &MainWindow::goToTab);
    connect(m_tabBar.get(), &QTabBar::tabCloseRequested, this, &MainWindow::closeTab);

    {
        QHBoxLayout *layout = reinterpret_cast<QHBoxLayout *>(ui->tabBarFrame->layout());
        layout->insertWidget(2, m_tabBar.get(), 0, Qt::AlignBottom);
    }

    connect(ui->openUrlButton, &QToolButton::clicked, [this]() {
        const QUrl url(WebPageHelpers::url(ui->webView->page()));
        if (url.scheme() != QLatin1String("qrc"))
            QDesktopServices::openUrl(url);
    });

    ui->actionNextTab->setShortcut(QKeySequence::NextChild);
    addAction(ui->actionNextTab);
    connect(ui->actionNextTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() + 1) % m_tabBar->count());
    });

    // For some reason actions with backtab key are not registered on windows.
    // Create an alternative key sequence for windows.
#ifdef Q_OS_WIN32
    ui->actionPreviousTab->setShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Tab));
#else
    ui->actionPreviousTab->setShortcut(QKeySequence::PreviousChild);
#endif
    addAction(ui->actionPreviousTab);
    connect(ui->actionPreviousTab, &QAction::triggered, [this]() {
        m_tabBar->setCurrentIndex((m_tabBar->currentIndex() - 1 + m_tabBar->count()) % m_tabBar->count());
    });

#ifdef Q_OS_OSX
    ui->treeView->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->sections->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    /// TODO: Remove in the future releases
    // Check pre-0.1 docset path
    QString oldDocsetDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    oldDocsetDir.remove(QStringLiteral("Zeal/Zeal"));
    oldDocsetDir += QLatin1String("zeal/docsets");
    if (QFileInfo::exists(oldDocsetDir) && m_settings->docsetPath != oldDocsetDir) {
        QMessageBox::information(this, QStringLiteral("Zeal"),
                                 QString(tr("Old docset storage has been found in <b>%1</b>. "
                                            "You can move docsets to <b>%2</b> or change the docset storage path in the settings. <br><br>"
                                            "Please note, that old docsets cannot be updated automatically, so it is better to download your docsets again. <br><br>"
                                            "Remove or use the old docset storage to avoid this message in the future."))
                                 .arg(QDir::toNativeSeparators(oldDocsetDir), QDir::toNativeSeparators(m_settings->docsetPath)));
    }

    displayViewActions();
    setupSearchBoxCompletions();
    createTab();
    /// FIXME: QTabBar does not emit currentChanged() after the first addTab() call
    reloadTabState();

    if (m_settings->checkForUpdate)
        m_application->checkForUpdate(true);
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_tabs);
}

/**
 * @brief MainWindow::deferOpenDocset
 * Defers to open a docset.
 * @param index
 */
void MainWindow::deferOpenDocset(const QModelIndex &index)
{
    /// PERF: Loading a web page makes the input sluggish.
    /// Load page only once user stops typing.
    m_deferOpenUrl->setSingleShot(true);
    m_deferOpenUrl->disconnect();
    connect(m_deferOpenUrl.get(), &QTimer::timeout, [this, index]() { openDocset(index); });
    m_deferOpenUrl->start(400);
}

/**
 * @brief MainWindow::openDocset
 * Opens a docset.
 * @param index Clicked item to open.
 */
void MainWindow::openDocset(const QModelIndex &index)
{
    const QVariant urlStr = index.sibling(index.row(), 1).data();
    if (urlStr.isNull())
        return;

    /// TODO: Keep anchor separately from file address
    QStringList urlParts = urlStr.toString().split(QLatin1Char('#'));
    QString localUrl = QUrl::fromPercentEncoding(urlParts[0].toUtf8());
    QUrl url = QUrl::fromLocalFile(localUrl);
    if (urlParts.count() > 1)
        /// NOTE: QUrl::DecodedMode is a fix for #121. Let's hope it doesn't break anything.
        url.setFragment(urlParts[1], QUrl::DecodedMode);

    ui->webView->load(url);
}

/**
 * @brief MainWindow::docsetName
 * Returns the name of a docset.
 * @param url
 * @return
 */
QString MainWindow::docsetName(const QUrl &url) const
{
    const QRegExp docsetRegex(QStringLiteral("/([^/]+)[.]docset"));
    return docsetRegex.indexIn(url.path()) != -1 ? docsetRegex.cap(1) : QString();
}

/**
 * @brief MainWindow::docsetIcon
 * Returns the icon for the particular docset.
 * @param docsetName
 * @return
 */
QIcon MainWindow::docsetIcon(const QUrl &url) const
{
    QString name = docsetName(url);
    const Docset * const docset = m_application->docsetRegistry()->docset(name);
    if (docset)
        return docset->icon();
    else if (url.scheme() == "qrc")
        return QIcon(QStringLiteral(":/icons/logo/icon.png"));
    else if (url.scheme() == "file")
        return Icons::getIcon(Icons::DocumentText);
    else
        return Icons::getIcon(Icons::WorldOutline);
}

void MainWindow::queryCompleted()
{
    displayTreeView();

    ui->treeView->setCurrentIndex(m_searchState->zealSearch->index(0, 0, QModelIndex()));
    deferOpenDocset(ui->treeView->currentIndex());
}

/**
 * @brief MainWindow::goToTab
 * Opens a tab with an `index`.
 * @param index
 */
void MainWindow::goToTab(int index)
{
    if (index == -1)
        return;

    saveTabState();
    m_searchState = nullptr;
    reloadTabState();
}

/**
 * @brief MainWindow::closeTab
 * Closes a tab with a specific `index`.
 * @param index Index of the tab to close.
 * If index == -1 then closes the currently opened tab.
 */
void MainWindow::closeTab(int index)
{
    if (index == -1)
        index = m_tabBar->currentIndex();

    if (index == -1)
        return;

    /// TODO: proper deletion here
    SearchState *state = m_tabs.takeAt(index);

    if (m_searchState == state)
        m_searchState = nullptr;

    delete state;

    m_tabBar->removeTab(index);

    if (m_tabs.count() == 0)
        createTab();
}

/**
 * @brief MainWindow::createTab
 * Creates and opens a new tab.
 */
void MainWindow::createTab()
{
    saveTabState();

    SearchState *newTab = new SearchState();

    connect(newTab->zealSearch.get(), &SearchModel::queryCompleted, this, &MainWindow::queryCompleted);
    connect(newTab->sectionsList.get(), &SearchModel::queryCompleted, this, &MainWindow::displaySections);

    newTab->page = new QWebPage(ui->webView);
#ifdef USE_WEBENGINE
    /// FIXME AngularJS workaround (zealnetworkaccessmanager.cpp)
#else
    newTab->page->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    newTab->page->setNetworkAccessManager(m_zealNetworkManager.get());
#endif
    WebPageHelpers::load(newTab->page, QUrl(startPageUrl));

    m_tabs.append(newTab);

    const int index = m_tabBar->addTab(QStringLiteral(""));
    m_tabBar->setCurrentIndex(index);
}

/**
 * @brief MainWindow::showSettings
 * Opens the settings dialog.
 */
void MainWindow::showSettings()
{
    m_globalShortcut->setEnabled(false);
    m_settingsDialog->exec();
    m_globalShortcut->setEnabled(true);
}

/**
 * @brief MainWindow::displayTreeView
 * Displays the tree view with main ToC/search results.
 */
void MainWindow::displayTreeView()
{
    SearchState *searchState = currentSearchState();

    if (!searchState->searchQuery.isEmpty()) {
        ui->treeView->setModel(searchState->zealSearch.get());
        ui->treeView->setRootIsDecorated(false);
        ui->treeView->reset();
    } else {
        ui->treeView->setModel(m_zealListModel.get());
        ui->treeView->setColumnHidden(1, true);
        ui->treeView->setRootIsDecorated(true);
        ui->treeView->reset();
    }
}

/**
 * @brief MainWindow::displaySections
 * Displays the see also panel.
 * If the page has no see also items hides the panel.
 */
void MainWindow::displaySections()
{
    const bool hasResults = currentSearchState()->sectionsList->rowCount();
    ui->sections->setVisible(hasResults);
    ui->seeAlsoLabel->setVisible(hasResults);
    QList<int> sizes = hasResults
            ? m_settings->sectionsSplitterSizes
            : QList<int>({1, 0});
    ui->sectionsSplitter->setSizes(sizes);
}

SearchState *MainWindow::currentSearchState()
{
    return m_tabs.at(m_tabBar->currentIndex());
}

/**
 * @brief MainWindow::displayTabs
 * Displays the tab menu and the tab bar.
 */
void MainWindow::displayTabs()
{
    ui->menuTabs->clear();
    ui->menuTabs->addAction(ui->actionNewTab);
    ui->menuTabs->addAction(ui->actionCloseTab);
    ui->menuTabs->addSeparator();
    ui->menuTabs->addAction(ui->actionNextTab);
    ui->menuTabs->addAction(ui->actionPreviousTab);
    ui->menuTabs->addSeparator();

    ui->actionNextTab->setEnabled(m_tabBar->count() > 1);
    ui->actionPreviousTab->setEnabled(m_tabBar->count() > 1);

    for (int i = 0; i < m_tabs.count(); i++) {
        SearchState *state = m_tabs.at(i);
        QString title = WebPageHelpers::title(state->page);
        QAction *action = ui->menuTabs->addAction(title);
        action->setCheckable(true);
        action->setChecked(i == m_tabBar->currentIndex());

        if (i < 10) {
#ifdef Q_OS_LINUX
            const QKeySequence shortcut = QString("Alt+%1").arg(QString::number((i + 1) % 10));
#else
            const QKeySequence shortcut = QString("Ctrl+%1").arg(QString::number((i + 1) % 10));
#endif

            for (QAction *oldAction : actions()) {
                if (oldAction->shortcut() == shortcut)
                    removeAction(oldAction);
            }

            action->setShortcut(shortcut);
            addAction(action);
        }

        m_tabBar->setTabText(i, title);
        connect(action, &QAction::triggered, [=]() {
            m_tabBar->setCurrentIndex(i);
        });
    }
}

/**
 * @brief MainWindow::reloadTabState
 * Updates the UI to reflect the saved UI state of the currently selected tab.
 */
void MainWindow::reloadTabState()
{
    SearchState *searchState = currentSearchState();

    ui->lineEdit->setText(searchState->searchQuery);
    ui->sections->setModel(searchState->sectionsList.get());

    displaySections();
    displayTreeView();

    // Bring back the selections and expansions
    ui->treeView->blockSignals(true);
    for (const QModelIndex &selection: searchState->selections)
        ui->treeView->selectionModel()->select(selection, QItemSelectionModel::Select);
    for (const QModelIndex &expandedIndex: searchState->expansions)
        ui->treeView->expand(expandedIndex);
    ui->treeView->blockSignals(false);

    ui->webView->setPage(searchState->page);
    ui->webView->setZoomFactor(searchState->zoomFactor);

    m_searchState = searchState;

    // scroll after the object gets loaded
    /// TODO: [Qt 5.4] QTimer::singleShot(100, this, &MainWindow::scrollSearch);
    QTimer::singleShot(100, this, SLOT(scrollSearch()));

    displayViewActions();
}

/**
 * @brief MainWindow::scrollSearch
 * Updates the scrollbar position of the ToC and see also views.
 */
void MainWindow::scrollSearch()
{
    ui->treeView->verticalScrollBar()->setValue(currentSearchState()->scrollPosition);
    ui->sections->verticalScrollBar()->setValue(currentSearchState()->sectionsScroll);
}

/**
 * @brief MainWindow::saveTabState
 * Saves the UI state of the currently opened tab.
 */
void MainWindow::saveTabState()
{
    if (!m_searchState)
        return;

    m_searchState->searchQuery = ui->lineEdit->text();
    m_searchState->selections = ui->treeView->selectionModel()->selectedIndexes();
    m_searchState->scrollPosition = ui->treeView->verticalScrollBar()->value();
    m_searchState->sectionsScroll = ui->sections->verticalScrollBar()->value();
    m_searchState->zoomFactor = ui->webView->zoomFactor();
}

void MainWindow::onSearchComplete()
{
    currentSearchState()->zealSearch->setResults(m_application->docsetRegistry()->queryResults());
}

/**
 * @brief MainWindow::setupSearchBoxCompletions
 * Sets up the search box autocompletions.
 */
void MainWindow::setupSearchBoxCompletions()
{
    QStringList completions;
    for (const QString keyword: m_application->docsetRegistry()->keywords())
        completions << keyword + QLatin1Char(':');

    ui->lineEdit->setCompletions(completions);
}

/**
 * @brief MainWindow::displayViewActions
 * Enables toolbar icons.
 * Displays the view menu (web view history).
 * Displays tabs and tab bar.
 */
void MainWindow::displayViewActions()
{
    ui->actionBack->setEnabled(ui->webView->canGoBack());
    ui->backButton->setEnabled(ui->webView->canGoBack());
    ui->actionForward->setEnabled(ui->webView->canGoForward());
    ui->forwardButton->setEnabled(ui->webView->canGoForward());
    ui->openUrlButton->setEnabled(ui->webView->currentUrl().scheme() != "qrc");

    ui->menuView->clear();
    ui->menuView->addAction(ui->actionBack);
    ui->menuView->addAction(ui->actionForward);
    ui->menuView->addSeparator();

    m_backMenu->clear();
    m_forwardMenu->clear();

    QWebHistory *history = ui->webView->page()->history();
    for (const QWebHistoryItem &item: history->backItems(10))
        m_backMenu->addAction(addHistoryAction(history, item));
    if (history->count() > 0)
        addHistoryAction(history, history->currentItem())->setEnabled(false);
    for (const QWebHistoryItem &item: history->forwardItems(10))
        m_forwardMenu->addAction(addHistoryAction(history, item));

    displayTabs();
}

/**
 * @brief MainWindow::displayTitle
 * Displays the window title.
 */
void MainWindow::displayTitle()
{
    QString name = docsetName(ui->webView->currentUrl());
    if (name.isEmpty())
        setWindowTitle(QString("Zeal - %1").arg(ui->webView->currentTitle()));
    else
        setWindowTitle(QString("Zeal - %1 - %2").arg(name, ui->webView->currentTitle()));
}

/**
 * @brief MainWindow::back
 * Goes to the previous page in a browser.
 */
void MainWindow::back()
{
    ui->webView->back();
    displayViewActions();
}

/**
 * @brief MainWindow::forward
 * Goes to the previous page in a browser.
 */
void MainWindow::forward()
{
    ui->webView->forward();
    displayViewActions();
}

void MainWindow::navigateToc(QKeyEvent *event)
{
    const QModelIndex index = ui->treeView->currentIndex();
    const int nextRow = index.row() + (event->key() == Qt::Key_Down ? 1 : -1);
    if (nextRow >= 0 && nextRow < ui->treeView->model()->rowCount()) {
        const QModelIndex sibling = index.sibling(nextRow, 0);
        ui->treeView->setCurrentIndex(sibling);
    }
}

void MainWindow::navigatePage(QKeyEvent *event)
{
    QKeyEvent *keyEvent = new QKeyEvent(event->type(), event->key(), 0);
    ui->webView->sendKeyEvent(keyEvent);
}

/**
 * @brief MainWindow::addHistoryAction
 * Adds a menu item with a history entry.
 * @param history Web page history. Used to navigate the history.
 * @param item Web page history item.
 * @return An action that navigates to the history item.
 */
QAction *MainWindow::addHistoryAction(QWebHistory *history, const QWebHistoryItem &item)
{
    const QIcon icon = docsetIcon(item.url());
    QAction *backAction = new QAction(icon, item.title(), ui->menuView);
    ui->menuView->addAction(backAction);
    connect(backAction, &QAction::triggered, [=](bool) {
        history->goToItem(item);
    });

    return backAction;
}

#ifdef USE_APPINDICATOR
void appIndicatorToggleWindow(GtkMenu *menu, gpointer data)
{
    Q_UNUSED(menu);
    static_cast<MainWindow *>(data)->toggleWindow();
}
#endif

void MainWindow::createTrayIcon()
{
#ifdef USE_APPINDICATOR
    if (m_trayIcon || m_appIndicator)
        return;
#else
    if (m_trayIcon)
        return;
#endif

#ifdef USE_APPINDICATOR
    const QString desktop = getenv("XDG_CURRENT_DESKTOP");
    const bool isUnity = (desktop.toLower() == QLatin1String("unity"));

    if (isUnity) { // Application Indicators for Unity
        m_appIndicatorMenu = gtk_menu_new();

        m_appIndicatorShowHideMenuItem = gtk_menu_item_new_with_label(qPrintable(tr("Hide")));
        gtk_menu_shell_append(GTK_MENU_SHELL(m_appIndicatorMenu), m_appIndicatorShowHideMenuItem);
        g_signal_connect(m_appIndicatorShowHideMenuItem, "activate",
                         G_CALLBACK(appIndicatorToggleWindow), this);

        m_appIndicatorMenuSeparator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(m_appIndicatorMenu), m_appIndicatorMenuSeparator);

        m_appIndicatorQuitMenuItem = gtk_menu_item_new_with_label(qPrintable(tr("Quit")));
        gtk_menu_shell_append(GTK_MENU_SHELL(m_appIndicatorMenu), m_appIndicatorQuitMenuItem);
        g_signal_connect(m_appIndicatorQuitMenuItem, "activate",
                         G_CALLBACK(QCoreApplication::quit), NULL);

        gtk_widget_show_all(m_appIndicatorMenu);

        /// NOTE: Zeal icon has to be installed, otherwise app indicator won't be shown
        m_appIndicator = app_indicator_new("zeal", "zeal", APP_INDICATOR_CATEGORY_OTHER);

        app_indicator_set_status(m_appIndicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_menu(m_appIndicator, GTK_MENU(m_appIndicatorMenu));
    } else {  // others
#endif
        m_trayIcon = new QSystemTrayIcon(this);
        m_trayIcon->setIcon(windowIcon());
        m_trayIcon->setToolTip(QStringLiteral("Zeal"));

        connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
                return;

            // Disable, when settings window is open
            if (m_settingsDialog->isVisible()) {
                m_settingsDialog->activateWindow();
                return;
            }

            toggleWindow();
        });

        QMenu *trayIconMenu = new QMenu(this);
        QAction *quitAction = trayIconMenu->addAction(tr("&Quit"));
        connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

        m_trayIcon->setContextMenu(trayIconMenu);

        m_trayIcon->show();
#ifdef USE_APPINDICATOR
    }
#endif
}

void MainWindow::removeTrayIcon()
{
#ifdef USE_APPINDICATOR
    if (!m_trayIcon && !m_appIndicator)
        return;
#else
    if (!m_trayIcon)
        return;
#endif

#ifdef USE_APPINDICATOR
    const QString desktop = getenv("XDG_CURRENT_DESKTOP");
    const bool isUnity = (desktop.toLower() == QLatin1String("unity"));

    if (isUnity) {
        g_clear_object(&m_appIndicator);
        g_clear_object(&m_appIndicatorMenu);
        g_clear_object(&m_appIndicatorShowHideMenuItem);
        g_clear_object(&m_appIndicatorMenuSeparator);
        g_clear_object(&m_appIndicatorQuitMenuItem);
    } else {
#endif
        QMenu *trayIconMenu = m_trayIcon->contextMenu();
        delete m_trayIcon;
        m_trayIcon = nullptr;
        delete trayIconMenu;
#ifdef USE_APPINDICATOR
    }
#endif
}

/**
 * @brief MainWindow::bringToFront
 * Brings the window to front.
 * @param query Query which it should execute.
 */
void MainWindow::bringToFront(const QString &query)
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
    ui->lineEdit->setFocus();

    if (!query.isEmpty())
        ui->lineEdit->setText(query);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (m_settings->showSystrayIcon && m_settings->minimizeToSystray
            && event->type() == QEvent::WindowStateChange && isMinimized()) {
        hide();
    }
    QMainWindow::changeEvent(event);
}

/**
 * @brief MainWindow::closeEvent
 * Closes the main window or hides it under a tray icon.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings->windowGeometry = saveGeometry();
    if (m_settings->showSystrayIcon && m_settings->hideOnClose) {
        event->ignore();
        toggleWindow();
    }
}

/**
 * @brief MainWindow::eventFilter
 * Applies an event filter in order to make middle trigger tab closing.
 */
bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_tabBar.get() && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *e = reinterpret_cast<QMouseEvent *>(event);
        if (e->button() == Qt::MiddleButton) {
            const int index = m_tabBar->tabAt(e->pos());
            if (index >= 0) {
                closeTab(index);
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(object, event);
}

/**
 * @brief MainWindow::keyPressEvent
 * Captures global events in order to pass them to the search bar.
 * @param keyEvent
 */
void MainWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch (keyEvent->key()) {
    case Qt::Key_Escape:
        ui->lineEdit->setFocus();
        ui->lineEdit->clearQuery();
        break;
    case Qt::Key_Question:
        ui->lineEdit->setFocus();
        ui->lineEdit->selectQuery();
        break;
    default:
        QMainWindow::keyPressEvent(keyEvent);
        break;
    }
}

/**
 * @brief MainWindow::applySettings
 * Updates application to reflect user settings.
 * Updates the global display shortcut.
 * Updates the tray icon.
 */
void MainWindow::applySettings()
{
    m_globalShortcut->setShortcut(m_settings->showShortcut);
    m_application->docsetRegistry()->setKeywordGroups(m_settings->docsetKeywordGroups);
    m_application->docsetRegistry()->setUserDefinedKeywords(m_settings->docsetKeywords);

    if (m_settings->showSystrayIcon)
        createTrayIcon();
    else
        removeTrayIcon();
}

void MainWindow::toggleWindow()
{
    const bool checkActive = sender() == m_globalShortcut;

    if (!isVisible() || (checkActive && !isActiveWindow())) {
#ifdef USE_APPINDICATOR
        if (m_appIndicator) {
            gtk_menu_item_set_label(GTK_MENU_ITEM(m_appIndicatorShowHideMenuItem),
                                    qPrintable(tr("Hide")));
        }
#endif
        bringToFront();
    } else {
#ifdef USE_APPINDICATOR
        if (m_trayIcon || m_appIndicator) {
            if (m_appIndicator) {
                gtk_menu_item_set_label(GTK_MENU_ITEM(m_appIndicatorShowHideMenuItem),
                                        qPrintable(tr("Show")));
            }
#else
        if (m_trayIcon) {
#endif
            hide();
        } else {
            showMinimized();
        }
    }
}
