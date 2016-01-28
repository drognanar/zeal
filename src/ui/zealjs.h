#ifndef ZEALJS_H
#define ZEALJS_H

#include <QObject>

namespace Zeal {

/**
 * @brief The ZealJS class
 * A QObject exposing methods that can be accessed by the web view.
 * Index.html uses these methods to create links that can open settings dialog.
 */
class ZealJS : public QObject
{
    Q_OBJECT
public:
    explicit ZealJS(QObject *parent = 0);
    ~ZealJS();

signals:
    void navigateToInstallDocsets();
    void navigateToCreateKeywords();

public slots:
    void installDocsets();
    void createKeywords();
};

}

#endif // ZEALJS_H
