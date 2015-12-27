#ifndef SEEALSODELEGATE_H
#define SEEALSODELEGATE_H

#include <QStyledItemDelegate>

namespace Zeal {

/**
 * @brief The SeeAlsoDelegate class
 * A delegate that defines how to render items in the see also list.
 * It provides support for sections items.
 * It renders the icon of the result (method/function/class) instead of docset icon.
 */
class SeeAlsoDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}

#endif // SEEALSODELEGATE_H
