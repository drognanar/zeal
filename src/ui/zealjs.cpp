#include "zealjs.h"

using namespace Zeal;

ZealJS::ZealJS(QObject *parent) : QObject(parent)
{

}

ZealJS::~ZealJS()
{
}

void ZealJS::installDocsets()
{
    emit navigateToInstallDocsets();
}

void ZealJS::createKeywords()
{
    emit navigateToCreateKeywords();
}
