#pragma once

#include <QString>
#include <QStringList>

namespace wormhole::core {

class RecentFiles {
public:
    static QString virtualPath();
    static bool isRecentPath(const QString& path);

    static QStringList paths(int limit = 200);
    static void forget(const QString& filePath);
};

} // namespace wormhole::core
