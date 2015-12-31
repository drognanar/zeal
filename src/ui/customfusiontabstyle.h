#ifndef CUSTOMFUSIONTABSTYLE_H
#define CUSTOMFUSIONTABSTYLE_H

#include <QProxyStyle>

class QIcon;
class StandardPixmap;
class QStyleOption;
class QWidget;

class CustomFusionTabStyle : public QProxyStyle
{
public:
    CustomFusionTabStyle(QStyle *style = 0);
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const;
};

#endif // CUSTOMFUSIONTABSTYLE_H
