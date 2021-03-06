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

#include "searchablewebview.h"

#include "webview.h"

#include <QLineEdit>
#include <QShortcut>
#include <QStyle>
#include <QResizeEvent>
#include <QUrl>

#ifdef USE_WEBENGINE
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebChannel>
#else
#include <QWebFrame>
#include <QWebHistory>
#include <QWebPage>
#endif

QUrl WebPageHelpers::url(const QWebPage *page)
{
#ifdef USE_WEBENGINE
    return page->url();
#else
    return page->mainFrame()->url();
#endif
}

QString WebPageHelpers::title(const QWebPage *page)
{
#ifdef USE_WEBENGINE
    return page->title();
#else
    return page->history()->currentItem().title();
#endif
}

void WebPageHelpers::load(QWebPage *page, const QUrl &url)
{
#ifdef USE_WEBENGINE
    page->load(url);
#else
    page->mainFrame()->load(url);
#endif
}

SearchableWebView::SearchableWebView(QWidget *parent) :
    QWidget(parent),
    m_searchLineEdit(new QLineEdit(this)),
    m_searchShortcut(new QShortcut(QKeySequence::Find, this)),
    m_webView(new WebView(this))
{
    m_webView->setAttribute(Qt::WA_AcceptTouchEvents, false);

    m_searchLineEdit->hide();
    m_searchLineEdit->installEventFilter(this);
    connect(m_searchLineEdit.get(), &QLineEdit::textChanged, this, &SearchableWebView::find);

    connect(m_searchShortcut.get(), &QShortcut::activated, this, &SearchableWebView::showSearch);

    connect(m_webView.get(), &QWebView::loadFinished, [&](bool ok) {
        Q_UNUSED(ok)
        moveLineEdit();
        injectRegisteredObjects();
    });

    connect(m_webView.get(), &QWebView::urlChanged, this, &SearchableWebView::urlChanged);
    connect(m_webView.get(), &QWebView::titleChanged, this, &SearchableWebView::titleChanged);
    connect(m_webView.get(), &QWebView::loadFinished, this, &SearchableWebView::loadFinished);
#ifdef USE_WEBENGINE
    // not implemented?
    // connect(m_webView->page(), &QWebPage::linkClicked, this, &SearchableWebView::linkClicked);
    m_webChannel = std::unique_ptr<QWebChannel>(new QWebChannel(this));
#else
    connect(m_webView.get(), &QWebView::linkClicked, this, &SearchableWebView::linkClicked);
#endif
}

SearchableWebView::~SearchableWebView()
{
}

void SearchableWebView::setPage(QWebPage *page)
{
    m_webView->setPage(page);

    connect(page, &QWebPage::linkHovered, [&](const QString &link) {
        if (!link.startsWith(QLatin1String("file:")))
            setToolTip(link);
    });
}

int SearchableWebView::zoomFactor() const
{
    return m_webView->zealZoomFactor();
}

void SearchableWebView::setZoomFactor(int value)
{
    m_webView->setZealZoomFactor(value);
}

bool SearchableWebView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_searchLineEdit.get() && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Escape:
            hideSearch();
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            findNext(keyEvent->modifiers() & Qt::ShiftModifier);
            return true;
        default:
            break;
        }
    }

    return QWidget::eventFilter(object, event);
}

void SearchableWebView::load(const QUrl &url)
{
    // Do not set focus on the web view.
    // This way the user can keep on searching without having to refocus the search bar.
    m_webView->setEnabled(false);
    m_webView->load(url);
    m_webView->setEnabled(true);
}

QUrl SearchableWebView::currentUrl()
{
#if USE_WEBENGINE
    return page()->url();
#else
    return page()->currentFrame()->url();
#endif
}

QString SearchableWebView::currentTitle()
{
#if USE_WEBENGINE
    return page()->title();
#else
    return page()->currentFrame()->title();
#endif
}

QWebPage *SearchableWebView::page() const
{
    return m_webView->page();
}

QSize SearchableWebView::sizeHint() const
{
    return m_webView->sizeHint();
}

void SearchableWebView::back()
{
    m_webView->back();
}

void SearchableWebView::forward()
{
    m_webView->forward();
}

bool SearchableWebView::canGoBack() const
{
    return m_webView->history()->canGoBack();
}

bool SearchableWebView::canGoForward() const
{
    return m_webView->history()->canGoForward();
}

void SearchableWebView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Slash:
        showSearch();
        event->accept();
        break;
    default:
        event->ignore();
        break;
    }
}

void SearchableWebView::sendKeyEvent(QKeyEvent *event)
{
    QString code;
    if (event->key() == Qt::Key_Up)
        code = QStringLiteral("document.body.scrollTop -= 40;");
    else if (event->key() == Qt::Key_Down)
        code = QStringLiteral("document.body.scrollTop += 40;");
#ifdef USE_WEBENGINE
    m_webView->page()->runJavaScript(code);
#else
    m_webView->page()->currentFrame()->evaluateJavaScript(code);
#endif
}

void SearchableWebView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_webView->resize(event->size().width(), event->size().height());
    moveLineEdit();
}

void SearchableWebView::showSearch()
{
    m_searchLineEdit->show();
    m_searchLineEdit->setFocus();
    if (!m_searchLineEdit->text().isEmpty()) {
        m_searchLineEdit->selectAll();
        find(m_searchLineEdit->text());
    }
}

void SearchableWebView::hideSearch()
{
    m_searchLineEdit->hide();
#ifdef USE_WEBENGINE
    m_webView->findText(QString());
#else
    m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
#endif
}

void SearchableWebView::registerObject(QString name, QObject *object)
{
#ifdef USE_WEBENGINE
    m_webChannel->registerObject(name, object);
#else
    m_registeredObjects.insert(name, object);
#endif
}

void SearchableWebView::find(const QString &text)
{
#ifdef USE_WEBENGINE
    /// FIXME: There's no way to just show highlight when search term is already selected.
    /// So we need a workaround before switching to Qt WebEngine.
    m_webView->findText(text);
#else
    if (m_webView->selectedText() != text) {
        m_webView->findText(QString(), QWebPage::HighlightAllOccurrences);
        m_webView->findText(QString());
        if (text.isEmpty())
            return;

        m_webView->findText(text, QWebPage::FindWrapsAroundDocument);
    }

    m_webView->findText(text, QWebPage::HighlightAllOccurrences);
#endif
}

void SearchableWebView::findNext(bool backward)
{
    findNext(m_searchLineEdit->text(), backward);
}

void SearchableWebView::findNext(const QString &text, bool backward)
{
#ifdef USE_WEBENGINE
    QWebPage::FindFlags flags = 0;
#else
    QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
#endif
    if (backward)
        flags |= QWebPage::FindBackward;
    m_webView->findText(text, flags);
}

void SearchableWebView::moveLineEdit()
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
#ifdef USE_WEBENGINE
    /// FIXME: scrollbar width
#else
    frameWidth += m_webView->page()->currentFrame()->scrollBarGeometry(Qt::Vertical).width();
#endif
    m_searchLineEdit->move(rect().right() - frameWidth - m_searchLineEdit->sizeHint().width(), rect().top());
    m_searchLineEdit->raise();
}

void SearchableWebView::injectRegisteredObjects()
{
#ifdef USE_WEBENGINE
    m_webView->page()->setWebChannel(m_webChannel.get());
    m_webView->page()->runJavaScript(
                "new QWebChannel(qt.webChannelTransport, function (channel) {"
                "window.zeal = channel.objects.zeal;"
                "});");
#else
    for (const QString objectName: m_registeredObjects.keys()) {
        m_webView->page()->currentFrame()->addToJavaScriptWindowObject(
                    objectName,
                    m_registeredObjects.value(objectName));

    }
#endif
}
