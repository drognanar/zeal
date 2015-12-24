#ifndef DASHTOC_H
#define DASHTOC_H

#include <QUrl>
#include <QList>

namespace Zeal {

class SearchResult;
class Docset;

// This class defines a method that reads related links from a .dashtoc file.
class DashToc
{
public:
    static QList<SearchResult> relatedLinks(const Docset *docset, const QUrl &url);
};

}

#endif // DASHTOC_H
