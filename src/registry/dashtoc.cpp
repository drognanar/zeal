#include "dashtoc.h"
#include "searchresult.h"

#include <memory>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

using namespace Zeal;

QList<SearchResult> DashToc::relatedLinks(const Docset *docset, const QUrl &url)
{
    QString fileName = url.toLocalFile();
    QString dashTocFileName = fileName + ".dashtoc";

    QFile file(dashTocFileName);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QList<SearchResult>();
    }

    QJsonParseError jsonError;
    QJsonObject jsonObject = QJsonDocument::fromJson(file.readAll(), &jsonError).object();

    if (jsonError.error != QJsonParseError::NoError)
        return QList<SearchResult>();

    QList<SearchResult> results;
    QJsonArray entries = jsonObject[QStringLiteral("entries")].toArray();
    for (QJsonValue entry: entries) {
        QJsonObject entryObject = entry.toObject();
        QString name = entryObject[QStringLiteral("name")].toString();
        bool isHeader = entryObject[QStringLiteral("isHeader")].toBool();
        QString entryType = entryObject[QStringLiteral("entryType")].toString();
        QString path = entryObject[QStringLiteral("path")].toString();

        QString fullPath = fileName + "#" + QUrl::fromPercentEncoding(path.toUtf8());
        results.append(SearchResult{name, "", entryType, const_cast<Docset*>(docset), fullPath, "", 0, isHeader});
    }

    return results;
}
