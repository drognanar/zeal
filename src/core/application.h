/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <QObject>

class QLocalServer;

class MainWindow;

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

namespace Zeal {

class DocsetRegistry;
class SearchQuery;

namespace Core {

class Extractor;
class Settings;

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(const QString &query, QObject *parent = nullptr);
    ~Application() override;

    static QString localServerName();

    QNetworkAccessManager *networkManager() const;
    Settings *settings() const;

    static DocsetRegistry *docsetRegistry();

public slots:
    void extract(const QString &filePath, const QString &destination, const QString &root = QString());
    QNetworkReply *download(const QUrl &url);
    void checkForUpdate(bool quiet = false);

signals:
    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);
    void extractionProgress(const QString &filePath, qint64 extracted, qint64 total);
    void updateCheckDone(const QString &version = QString());
    void updateCheckError(const QString &message);

private slots:
    void applySettings();

private:
    static inline QString userAgent();
    QString userAgentJson() const;

    static Application *m_instance;

    std::unique_ptr<Settings> m_settings;

    std::unique_ptr<QLocalServer> m_localServer;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;

    std::unique_ptr<QThread> m_extractorThread;
    std::unique_ptr<Extractor> m_extractor;

    std::unique_ptr<DocsetRegistry> m_docsetRegistry;

    std::unique_ptr<MainWindow> m_mainWindow;
};

} // namespace Core
} // namespace Zeal

#endif // APPLICATION_H
