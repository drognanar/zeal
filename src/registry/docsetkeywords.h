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

#ifndef DOCSETKEYWORDS_H
#define DOCSETKEYWORDS_H

#include <QMap>
#include <QString>
#include <QStringList>

namespace Zeal {

class Docset;

/**
 * @brief The DocsetKeywords class
 * Class that contains search keywords that can be used to filter searches.
 * Keywords are of two kinds:
 *
 * 1. Docset keyword is used to search for a single docset.
 *    For example keyword `cpp` is most likely mapped to a cpp docset.
 * 2. Group keywords are used to search for multiple docsets.
 *    For example keyword `web` might be mapped to `html`, `js` and `css` docsets.
 */
class DocsetKeywords
{
public:
    /**
     * @brief add
     * Adds a keyword under which a single docset should be searched.
     */
    void add(QString &keywordStr, Docset *docset);

    /**
     * @brief add
     * Adds a keyword under which an entire group should be searched.
     */
    void add(QString &keywordStr, QStringList docsetGroup);

    /**
     * @brief getKeywordDocsets
     * Returns matching list of docsets mapped to a keyword.
     *
     * @param keywordStr Keyword to search for.
     * @return QList() if the keyword is not defined. Matching list of docsets otherwise.
     */
    QStringList getKeywordDocsets(QString &keywordStr) const;

    /**
     * @brief getKeywords
     * @return Returns the list of currently available keywords.
     */
    QStringList getKeywords() const;

private:
    QMap<QString, QStringList> m_keywords;
};

}

#endif // DOCSETKEYWORDS_H
