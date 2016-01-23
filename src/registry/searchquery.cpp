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

#include "searchquery.h"

#include "docset.h"
#include "docsetkeywords.h"

#include <QString>

using namespace Zeal;

namespace {
const char prefixSeparator = ':';
const char keywordSeparator = ',';
}

SearchQuery::SearchQuery()
{
}

SearchQuery::SearchQuery(const QString &query,
                         const QString &keywordPrefix,
                         const QStringList &docsets)
   : m_query(query),
     m_keywordPrefix(keywordPrefix),
     m_enabledDocsets(docsets)
{
}

/**
 * @brief SearchQuery::fromString
 * Creates a search query from its string representation.
 *
 * @param str String representation of a query.
 */
SearchQuery SearchQuery::fromString(const QString &str, const DocsetKeywords docsetKeywords)
{
    const int sepAt = str.indexOf(prefixSeparator);
    const int next = sepAt + 1;

    QString query;
    QString keywordStr;
    QStringList docsets;

    // Try to get keywords from the query.
    if (sepAt > 0) {
        keywordStr = str.left(sepAt).trimmed();
        docsets = tryGetKeywords(keywordStr, docsetKeywords);
    }

    // If keywords were found then query should not include the keywords.
    // Otherwise query should include entire str.
    if (docsets.size() > 0) {
        query = str.mid(next).trimmed();
        keywordStr = keywordStr + ":";
    } else {
        query = str.trimmed();
        keywordStr = QString();
    }

    return SearchQuery(query, keywordStr, docsets);
}

QStringList SearchQuery::tryGetKeywords(QString keywordStr, DocsetKeywords docsetKeywords)
{
    QStringList docsets;
    QStringList candidateKeywords = keywordStr.split(keywordSeparator);

    for (QString candidate: candidateKeywords)
        docsets += docsetKeywords.getKeywordDocsets(candidate);

    return docsets;
}

QString SearchQuery::toString() const
{
    return m_keywordPrefix + m_query;
}

bool SearchQuery::isEmpty() const
{
    return m_query.isEmpty() && m_keywordPrefix.isEmpty();
}

void SearchQuery::setKeywordPrefix(const QString &keywordPrefix)
{
    m_keywordPrefix = keywordPrefix.isEmpty()
            ? keywordPrefix
            : keywordPrefix + prefixSeparator;
}

bool SearchQuery::isEnabled(const Docset *docset) const
{
    return m_enabledDocsets.size() == 0 || m_enabledDocsets.contains(docset->name());
}

int SearchQuery::keywordPrefixSize() const
{
    return m_keywordPrefix.size();
}

QString SearchQuery::query() const
{
    return m_query;
}

void SearchQuery::setQuery(const QString &str)
{
    m_query = str;
}

QString SearchQuery::sanitizedQuery() const
{
    QString q = m_query;
    q.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    q.replace(QLatin1String("_"), QLatin1String("\\_"));
    q.replace(QLatin1String("%"), QLatin1String("\\%"));
    q.replace(QLatin1String("'"), QLatin1String("''"));
    return q;
}

QDataStream &Zeal::operator<<(QDataStream &out, const SearchQuery &query)
{
    out << query.toString();
    return out;
}
