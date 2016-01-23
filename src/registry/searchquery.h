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

#ifndef SEARCHQUERY_H
#define SEARCHQUERY_H

#include <QStringList>

namespace Zeal {

class Docset;
class DocsetKeywords;

/**
 * @short The search query model.
 */
class SearchQuery
{
public:
    explicit SearchQuery();
    explicit SearchQuery(const QString &query,
            const QString &keywordPrefix = QString(),
            const QStringList &docsets = QStringList());

    /**
     * @brief fromString
     * Creates a search query from a string where a string is parsed as `keyword:query`.
     * However if `keyword` does not correspond to any docset keywords then an entire string
     * it treated as a query.
     *
     * NOTE: Call `DocsetRegistry::getSearchQuery(str)` in order to parse a SearchQuery
     * given currently available keywords.
     *
     * Examples (assuming android, java, c++ docsets are installed):
     *   "android:setTypeFa" #=> docsetFilters = ["android"], coreQuery = "setTypeFa"
     *   "noprefix"          #=> docsetFilters = [], coreQuery = "noprefix"
     *   ":find"             #=> docsetFilters = [], coreQuery = ":find"
     *   "std::string"       #=> docsetFilters = [], coreQuery = "std::string"
     *   "c++:std::string"   #=> docsetFilters = ["c++"], coreQuery = "std::string"
     *
     * Multiple docsets are supported using the ',' character:
     *   "java,android:setTypeFa #=> docsetFilters = ["java", "android"], coreQuery = "setTypeFa"
     *
     * @param str String representation of a
     * @param docsetKeywords List of currently enabled keywords.
     * @return A query with keywords.
     */
    static SearchQuery fromString(const QString &str, const DocsetKeywords docsetKeywords);

    QString toString() const;

    bool isEmpty() const;

    void setKeywordPrefix(const QString &keywordPrefix);

    /// Returns true if the docset falls satisfies a docset filter.
    bool isEnabled(const Docset* docset) const;

    /// Returns the docset filter raw size for the given query
    int keywordPrefixSize() const;

    QString query() const;
    void setQuery(const QString &str);

    /// Returns the core query, sanitized for use in SQL queries
    QString sanitizedQuery() const;

private:
    QString m_query;
    QString m_keywordPrefix;
    QStringList m_enabledDocsets;

    /**
     * @brief tryGetKeywords
     * Tries to extract a list of keywords from a keywordStr
     * given that `query` can be split into `keywordStr:query`.
     *
     * @param keywordStr Comma separated list of keywords.
     * @param docsetKeywords List of available keywords.
     * @return If any of the keywords is not available returns an empty list.
     * Otherwise returns the list of all keywords.
     */
    static QStringList tryGetKeywords(QString keywordStr, DocsetKeywords docsetKeywords);
};

QDataStream &operator<<(QDataStream &out, const SearchQuery &query);

} // namespace Zeal

#endif // SEARCHQUERY_H
