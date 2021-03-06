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

#ifndef DOCSETREGISTRY_H
#define DOCSETREGISTRY_H

#include "cancellationtoken.h"
#include "docset.h"
#include "searchresult.h"

#include <memory>
#include <QMap>

class QThread;

namespace Zeal {

class DocsetKeywords;
class SearchQuery;

/**
 * @brief The DocsetRegistry class
 * A docset registry manages all docsets. It is used to perform queries.
 */
class DocsetRegistry : public QObject
{
    Q_OBJECT
public:
    explicit DocsetRegistry(QObject *parent = nullptr);
    ~DocsetRegistry() override;

    void init(const QString &path);

    int count() const;
    bool contains(const QString &name) const;
    QStringList names() const;
    void remove(const QString &name);

    Docset *docset(const QString &name) const;
    Docset *docset(int index) const;

    void search(const QString &query, CancellationToken token);
    const QList<SearchResult> &queryResults();
    QList<Docset *> docsets() const;
    SearchQuery getSearchQuery(const QString &queryStr) const;

    QStringList keywords() const;
    void setKeywordGroups(const QMap<QString, QStringList> docsetKeywordGroups);
    void setUserDefinedKeywords(const QMap<QString, QString> docsetKeywords);
    QString userDefinedKeyword(const QString &docsetName) const;

    /// The number of results that should be retuned by the search.
    static const int MaxResults = 100;

public slots:
    void addDocset(const QString &path);

signals:
    void docsetAdded(const QString &name);
    void docsetAboutToBeRemoved(const QString &name);
    void docsetRemoved(const QString &name);
    void keywordGroupsChanged();
    void queryCompleted();

private slots:
    void _addDocset(const QString &path);
    void _runQueryAsync(const QString &query, const CancellationToken token);

private:
    void addDocsetsFromFolder(const QString &path);
    DocsetKeywords docsetKeywords() const;

    std::unique_ptr<QThread> m_thread;
    QMap<QString, Docset *> m_docsets;
    QMap<QString, QStringList> m_docsetGroups;
    QMap<QString, QString> m_userDefinedKeywords;
    QList<SearchResult> m_queryResults;
};

} // namespace Zeal

#endif // DOCSETREGISTRY_H
