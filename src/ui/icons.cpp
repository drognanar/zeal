#include "icons.h"

#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QSize>

char Icons::Back[] = "\uf3cf";
char Icons::Forward[] = "\uf3d1";
char Icons::OpenLink[] = "\uf39c";
char Icons::Plus[] = "\uf218";
char Icons::Minus[] = "\uf2f4";
char Icons::CloseCircled[] = "\uf128";
char Icons::CloseRound[] = "\uf129";
char Icons::Close[] = "\uf12a";
char Icons::Search[] = "\uf2f5";
char Icons::DocumentText[] = "\uf12e";
char Icons::WorldOutline[] = "\uf4d2";

/**
 * @brief Icons::getIcon
 * Converts a text into an icon.
 *
 * @param text Text of the icon.
 */
QIcon Icons::getIcon(char text[])
{
    static int iconSize = 32;
    QFont font("Ionicons", iconSize);
    QPainter painter;

    QSize pmSize = QSize(iconSize, iconSize);
    QRect rect(QPoint(0, 0), pmSize);
    QPixmap pm(pmSize);
    pm.fill(Qt::transparent);

    painter.begin(&pm);
    painter.setFont(font);
    painter.drawText(rect, Qt::AlignCenter | Qt::AlignHCenter, text);
    painter.end();

    return QIcon(pm);
}
