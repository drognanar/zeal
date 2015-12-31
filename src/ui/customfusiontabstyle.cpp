#include "customfusiontabstyle.h"
#include "icons.h"

#include <QStyle>
#include <QWidget>

CustomFusionTabStyle::CustomFusionTabStyle(QStyle *style)
    : QProxyStyle(style)
{
}

QIcon CustomFusionTabStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    switch (standardIcon) {
    case QStyle::SP_DialogCloseButton:
        return Icons::getIcon(Icons::CloseRound);
    default:
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }
}
