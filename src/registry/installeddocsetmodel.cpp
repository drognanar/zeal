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

#include "installeddocsetmodel.h"

#include "registry/docset.h"
#include "registry/docsetregistry.h"

#include <QVariant>
#include <QSize>
#include <QStandardItem>

using namespace Zeal;

InstalledDocsetModel::InstalledDocsetModel(DocsetRegistry *registry)
    : m_registry(registry)
{
    connect(registry, &DocsetRegistry::docsetAdded, [this]() {
        // TODO: Perhaps we should alert the user to apply changes before installing docsets.
        // TODO: ensure that this also works in keyword groups.
        populateModelData();
    });
    connect(registry, &DocsetRegistry::docsetAboutToBeRemoved, [this]() {
        populateModelData();
    });
    
    setColumnCount(3);
    setHorizontalHeaderLabels(QStringList {"Docset", "Keyword", "Has update"});
    populateModelData();
}

void InstalledDocsetModel::populateModelData()
{
    removeRows(0, rowCount());
    int row = 0;
    for (Docset *docset: m_registry->docsets()) {
        QStandardItem *docsetColumn = new QStandardItem(docset->icon(), docset->name());
        QSize size(docsetColumn->sizeHint().width(), 12);
        docsetColumn->setSizeHint(size);
        docsetColumn->setEditable(false);

        QStandardItem *keywordColumn = new QStandardItem(m_registry->userDefinedKeyword(docset->name()));
        keywordColumn->setEditable(true);

        QStandardItem *updateColumn = new QStandardItem(docset->hasUpdate ? "Has update" : "");
        QFont updateFont = QFont(updateColumn->font());
        updateFont.setItalic(true);
        updateColumn->setFont(updateFont);
        updateColumn->setEditable(false);

        this->insertRow(row, QList<QStandardItem*>{
            docsetColumn,
            keywordColumn,
            updateColumn
        });
        row++;
    }
}
