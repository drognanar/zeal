#include "seealsodelegate.h"
#include "registry/searchmodel.h"

#include <QApplication>
#include <QIcon>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>

using namespace Zeal;

void SeeAlsoDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_, const QModelIndex &index) const
{
    QStyleOptionViewItem option(option_);
    option.icon = index.data(SearchModel::Roles::TypeIconRole).value<QIcon>();
    option.text = index.data(Qt::DisplayRole).toString();
    option.features |= QStyleOptionViewItem::HasDisplay;
    option.features |= QStyleOptionViewItem::HasDecoration;

    bool isHeader = index.data(SearchModel::Roles::IsHeaderRole).toBool();

    if (isHeader) {
        // Draw a header item.
        painter->save();

        painter->fillRect(option.rect, option.palette.window());

        QFont headerFont(option.font);
        headerFont.setPointSize(option.font.pointSize() + 1);
        painter->setFont(headerFont);
        painter->drawText(option.rect, Qt::AlignCenter, option.text);

        painter->restore();
        return;
    }

    // Draw a regular item.
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);
}

