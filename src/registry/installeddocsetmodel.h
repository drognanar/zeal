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

#ifndef INSTALLEDDOCSETMODEL_H
#define INSTALLEDDOCSETMODEL_H

#include <QStandardItemModel>

namespace Zeal {
    
class DocsetRegistry;
    
/**
 * @brief The InstalledDocsetModel class
 * This model represents items in the installed docsets list.
 */
class InstalledDocsetModel : public QStandardItemModel
{
public:
    enum Columns {
        DocsetColumn,
        KeywordColumn,
        UpdateColumn
    };
    
    explicit InstalledDocsetModel(DocsetRegistry *registry);
    void populateModelData();

private:
    DocsetRegistry *m_registry;
};
    
}

#endif // INSTALLEDDOCSETMODEL_H
