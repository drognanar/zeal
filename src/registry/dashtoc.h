#ifndef DASHTOC_H
#define DASHTOC_H

#include <QUrl>
#include <QList>

namespace Zeal {

class SearchResult;
class Docset;

/**
 * @brief The DashToc class
 * DashToc parses the .dashtoc file and provides the see also list.
 */
class DashToc
{
public:
    static QList<SearchResult> relatedLinks(const Docset *docset, const QUrl &url);
};

}

#endif // DASHTOC_H
