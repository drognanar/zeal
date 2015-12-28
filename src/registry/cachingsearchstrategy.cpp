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

#include "cachingsearchstrategy.h"
#include "docset.h"

#include "searchresult.h"
#include "searchquery.h"

using namespace Zeal;

CachingSearchStrategy::CachingSearchStrategy(std::unique_ptr<DocsetSearchStrategy> strategy)
    : m_cache(CachingSearchStrategy::CacheSize),
      m_search(std::move(strategy))
{
}

QList<SearchResult> CachingSearchStrategy::search(const SearchQuery &searchQuery, CancellationToken token)
{
    QString candidate = getCacheEntry(searchQuery);
    if (!candidate.isEmpty()) {
        return searchWithCache(searchQuery, candidate);
    } else {
        return searchWithoutCache(searchQuery, token);
    }
}

bool CachingSearchStrategy::validResult(const SearchQuery &searchQuery, SearchResult previousResult,
        SearchResult &result)
{
    return m_search->validResult(searchQuery, previousResult, result);
}

QString CachingSearchStrategy::getCacheEntry(const SearchQuery &searchQuery) const
{
    // TODO: do not use the `fromString` in this method.
    // TODO: remove as many `fromString` uses as possible.
    // TODO: then make the registry process keywords globally.
    for (int i = searchQuery.toString().size(); i > 0; i--) {
        QString candidate = searchQuery.toString().mid(0, i);

        /// Use the cache only if the cache entry contains all results.
        /// Also handle cases where prefix is a docset query `std:`.
        if (m_cache.contains(candidate)
                && m_cache[candidate]->size() < Docset::MaxDocsetResultsCount
                && SearchQuery::fromString(candidate).query().size() == i)
            return candidate;
    }
    return QString();
}

QList<SearchResult> CachingSearchStrategy::searchWithCache(const SearchQuery &searchQuery, const QString &prefix)
{
    QList<SearchResult> *prefixResults = m_cache[prefix];
    QList<SearchResult> results;
    QListIterator<SearchResult> prefixResultsIterator(*prefixResults);
    while (prefixResultsIterator.hasNext()) {
        SearchResult previousResult = prefixResultsIterator.next();
        SearchResult result;
        if (validResult(searchQuery, previousResult, result))
            results.append(result);
    }

    m_cache.insert(searchQuery.toString(), new QList<SearchResult>(results));
    return results;
}

QList<SearchResult> CachingSearchStrategy::searchWithoutCache(const SearchQuery &searchQuery, CancellationToken token)
{
    QList<SearchResult> results = m_search->search(searchQuery, token);

    /// Only cache the results if they are not partial.
    if (!token.isCancelled())
        m_cache.insert(searchQuery.toString(), new QList<SearchResult>(results));
    return results;
}
