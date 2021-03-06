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

#ifndef SEARCHABLEWEBVIEW_H
#define SEARCHABLEWEBVIEW_H

#include <memory>
#include <QWidget>

#ifdef USE_WEBENGINE
    #define QWebPage QWebEnginePage
#endif

class QLineEdit;
class QUrl;
class QWebPage;
class QWebChannel;
class QShortcut;

class WebView;

/**
 * @brief The WebPageHelpers class.
 * Provides static helpers to unify interaction with QWebPage and QWebEnginePage.
 */
class WebPageHelpers
{
public:
    /**
     * @brief The url method.
     * Returns the url of the page.
     */
    static QUrl url(const QWebPage *page);

    /**
     * @brief The title method.
     * Returns the title of the page.
     */
    static QString title(const QWebPage *page);

    /**
     * @brief The load method.
     * Makes a web page load the specified url.
     */
    static void load(QWebPage *page, const QUrl &url);
};

/**
 * @brief The SearchableWebView class
 * A web view that has an inbuilt search field.
 */
class SearchableWebView : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableWebView(QWidget *parent = nullptr);
    ~SearchableWebView();

    void load(const QUrl &url);
    QSize sizeHint() const override;
    QWebPage *page() const;
    bool canGoBack() const;
    bool canGoForward() const;
    void setPage(QWebPage *page);
    void sendKeyEvent(QKeyEvent *event);

    int zoomFactor() const;
    void setZoomFactor(int value);

    void showSearch();
    void findNext(bool backward = false);

    bool eventFilter(QObject *object, QEvent *event) override;

    QUrl currentUrl();
    QString currentTitle();
    void registerObject(QString name, QObject* object);

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void linkClicked(const QUrl &url);
    void loadFinished(bool ok);

public slots:
    void back();
    void forward();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void hideSearch();
    void find(const QString &text);
    void findNext(const QString &text, bool backward = false);
    void moveLineEdit();

    /**
     * @brief injectRegisteredObjects
     */
    void injectRegisteredObjects();

    std::unique_ptr<QLineEdit> m_searchLineEdit;
    std::unique_ptr<QShortcut> m_searchShortcut;
    std::unique_ptr<WebView> m_webView;

#ifdef USE_WEBENGINE
    std::unique_ptr<QWebChannel> m_webChannel;
#else
    QHash<QString, QObject*> m_registeredObjects;
#endif
};

#endif // SEARCHABLEWEBVIEW_H
