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

#include "docsetregistry.h"

#include "docsetkeywords.h"
#include "searchquery.h"
#include "searchresult.h"

#include <functional>
#include <QtConcurrent/QtConcurrent>
#include <QDir>
#include <QSqlQuery>
#include <QThread>
#include <QUrl>
#include <QVariant>

using namespace Zeal;

DocsetRegistry::DocsetRegistry(QObject *parent) :
    QObject(parent),
    m_thread(new QThread(this))
{
    /// FIXME: Only search should be performed in a separate thread
    moveToThread(m_thread.get());
    m_thread->start();
}

DocsetRegistry::~DocsetRegistry()
{
    m_thread->exit();
    m_thread->wait();
    qDeleteAll(m_docsets);
}

void DocsetRegistry::init(const QString &path)
{
    for (const QString &name : m_docsets.keys())
        remove(name);

    addDocsetsFromFolder(path);
}

int DocsetRegistry::count() const
{
    return m_docsets.count();
}

bool DocsetRegistry::contains(const QString &name) const
{
    return m_docsets.contains(name);
}

QStringList DocsetRegistry::names() const
{
    return m_docsets.keys();
}

QStringList DocsetRegistry::keywords() const
{
    return docsetKeywords().getKeywords();
}

DocsetKeywords DocsetRegistry::docsetKeywords() const
{
    DocsetKeywords keywords;

    // Add keywords for docsets.
    for (const Docset * const docset: docsets())
        for (QString keyword: docset->keywords())
            keywords.add(keyword, const_cast<Docset*>(docset));

    // Add user defined keywords for docsets.
    for (const QString docsetName: m_userDefinedKeywords.keys())
        if (m_docsets.contains(docsetName)) {
            QString keyword = m_userDefinedKeywords.value(docsetName);
            keywords.add(keyword, docset(docsetName));
        }

    // Add keywords for docset groups.
    for (QString keyword: m_docsetGroups.keys())
        keywords.add(keyword, m_docsetGroups.value(keyword));

    return keywords;
}

void DocsetRegistry::setKeywordGroups(const QMap<QString, QStringList> docsetKeywordGroups)
{
    m_docsetGroups = docsetKeywordGroups;
}

void DocsetRegistry::setUserDefinedKeywords(const QMap<QString, QString> docsetKeywords)
{
    m_userDefinedKeywords = docsetKeywords;
}

QString DocsetRegistry::userDefinedKeyword(const QString &docsetName) const
{
    QString userDefinedKeyword = m_userDefinedKeywords.value(docsetName);
    if (userDefinedKeyword != nullptr)
        return userDefinedKeyword;

    Docset *doc = docset(docsetName);
    return doc != nullptr
        ? doc->keywords().first()
        : QString();
}

void DocsetRegistry::remove(const QString &name)
{
    emit docsetAboutToBeRemoved(name);
    delete m_docsets.take(name);
    emit docsetRemoved(name);
}

Docset *DocsetRegistry::docset(const QString &name) const
{
    return m_docsets.value(name);
}

Docset *DocsetRegistry::docset(int index) const
{
    /// TODO: sort docsets
    if (index < 0 || index >= m_docsets.size())
        return nullptr;
    return (m_docsets.cbegin() + index).value();
}

QList<Docset *> DocsetRegistry::docsets() const
{
    return m_docsets.values();
}

void DocsetRegistry::addDocset(const QString &path)
{
    QMetaObject::invokeMethod(this, "_addDocset", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, path));
}

void DocsetRegistry::_addDocset(const QString &path)
{
    Docset *docset = new Docset(path);

    /// TODO: Emit error
    if (!docset->isValid()) {
        delete docset;
        return;
    }

    const QString name = docset->name();

    if (m_docsets.contains(name))
        remove(name);

    m_docsets[name] = docset;
    emit docsetAdded(name);
}

void DocsetRegistry::search(const QString &query, CancellationToken token)
{
    qRegisterMetaType<CancellationToken>("CancellationToken");
    qRegisterMetaType<CancellationToken>("Zeal::CancellationToken");
    QMetaObject::invokeMethod(this, "_runQueryAsync", Qt::QueuedConnection,
                              Q_ARG(QString, query), Q_ARG(CancellationToken, token));
}

void MergeQueryResults(QList<SearchResult> &finalResult, const QList<SearchResult> &partial)
{
    finalResult += partial;
}

SearchQuery DocsetRegistry::getSearchQuery(const QString &queryStr) const
{
    return SearchQuery::fromString(queryStr, docsetKeywords());
}

void DocsetRegistry::_runQueryAsync(const QString &query, const CancellationToken token)
{
    SearchQuery searchQuery = getSearchQuery(query);
    QFuture<QList<SearchResult>> queryResultsFuture = QtConcurrent::mappedReduced(
                docsets(),
                std::bind(&Docset::search, std::placeholders::_1, searchQuery, token),
                &MergeQueryResults);
    QList<SearchResult> queryResults = queryResultsFuture.result();
    std::sort(queryResults.begin(), queryResults.end());

    if (!token.isCancelled()) {
        m_queryResults = queryResults;
        emit queryCompleted();
    }
}

const QList<SearchResult> &DocsetRegistry::queryResults()
{
    return m_queryResults;
}

// Recursively finds and adds all docsets in a given directory.
void DocsetRegistry::addDocsetsFromFolder(const QString &path)
{
    const QDir dir(path);
    for (const QFileInfo &subdir : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
        if (subdir.suffix() == QLatin1String("docset"))
            addDocset(subdir.absoluteFilePath());
        else
            addDocsetsFromFolder(subdir.absoluteFilePath());
    }
}
