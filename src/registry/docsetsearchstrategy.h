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

#ifndef DOCSETSEARCHSTRATEGY_H
#define DOCSETSEARCHSTRATEGY_H

#include "cancellationtoken.h"

#include <QList>

namespace Zeal {

class SearchQuery;
class SearchResult;

/**
 * @brief The DocsetSearchStrategy class
 * A class that defines a strategy to find results for a search query.
 */
class DocsetSearchStrategy
{
public:
    virtual QList<SearchResult> search(const SearchQuery &searchQuery, CancellationToken token) = 0;

    /// Used to filter out cached results.
    virtual bool validResult(const SearchQuery &searchQuery, SearchResult previousResult,
                             SearchResult &result) = 0;
};

}

#endif // DOCSETSEARCHSTRATEGY_H
