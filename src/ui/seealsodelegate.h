#ifndef SEEALSODELEGATE_H
#define SEEALSODELEGATE_H

#include <QStyledItemDelegate>

namespace Zeal {

// A delegate that displays a single item in te see also list.
class SeeAlsoDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}

#endif // SEEALSODELEGATE_H
