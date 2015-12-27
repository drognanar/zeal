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

#ifndef CACHINGSEARCHSTRATEGY_H
#define CACHINGSEARCHSTRATEGY_H

#include "cancellationtoken.h"
#include "docsetsearchstrategy.h"

#include <QCache>
#include <QList>
#include <QString>

#include <memory>

namespace Zeal {

class SearchResult;

/**
 * @brief The CachingSearchStrategy class
 * A search strategy that decorates another strategy and provides caching.
 *
 * It keeps a cache of the results of previous searches.
 * If the prefix of a search query exists then the results and found
 * by searching the cached results rather than the entire docset.
 *
 * This is because the results for a query prefix are a superset of
 * results for a query.
 */
class CachingSearchStrategy : public DocsetSearchStrategy
{
public:
    CachingSearchStrategy(std::unique_ptr<DocsetSearchStrategy> strategy);
    QList<SearchResult> search(const QString &query, CancellationToken token) override;
    bool validResult(const QString &query, SearchResult previousResult,
                     SearchResult &result)override;

private:
    QString getCacheEntry(const QString &query) const;
    QList<SearchResult> searchWithCache(const QString &query, const QString &prefix);
    QList<SearchResult> searchWithoutCache(const QString &query, CancellationToken token);

    // Maximum size of the cache.
    const static int CacheSize = 10;
    QCache<QString, QList<SearchResult>> m_cache;

    // A decorated search strategy.
    std::unique_ptr<DocsetSearchStrategy> m_search;

};

}

#endif // CACHINGSEARCHSTRATEGY_H
