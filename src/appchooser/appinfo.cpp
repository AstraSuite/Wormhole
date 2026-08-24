#include "appinfo.hpp"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

namespace wormhole::appchooser {

AppChooserModel::AppChooserModel(QObject* parent)
    : QAbstractListModel(parent) {
    loadDesktopFiles();
}

int AppChooserModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_filteredApps.size();
}

QVariant AppChooserModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filteredApps.size())
        return {};

    const auto& app = m_filteredApps.at(index.row());
    switch (role) {
    case DesktopIdRole:
        return app.desktopId;
    case NameRole:
        return app.name;
    case CommentRole:
        return app.comment;
    case IconNameRole:
        return app.iconName;
    case ExecRole:
        return app.exec;
    case IsRecommendedRole:
        return app.isRecommended;
    default:
        return {};
    }
}

QHash<int, QByteArray> AppChooserModel::roleNames() const {
    return {
        { DesktopIdRole, "desktopId" },
        { NameRole, "name" },
        { CommentRole, "comment" },
        { IconNameRole, "iconName" },
        { ExecRole, "exec" },
        { IsRecommendedRole, "isRecommended" }
    };
}

void AppChooserModel::setMimeType(const QString& mime) {
    if (m_mimeType != mime) {
        m_mimeType = mime;
        emit mimeTypeChanged();
        applyFilter();
    }
}

void AppChooserModel::setSearchQuery(const QString& query) {
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilter();
    }
}

void AppChooserModel::reload() {
    loadDesktopFiles();
}

void AppChooserModel::loadDesktopFiles() {
    m_allApps.clear();

    QStringList appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QSet<QString> seenIds;

    for (const QString& dirPath : appDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        QDirIterator it(dirPath, QStringList{ QStringLiteral("*.desktop") }, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString desktopId = QFileInfo(filePath).fileName();
            if (seenIds.contains(desktopId)) continue;

            QSettings desktopFile(filePath, QSettings::IniFormat);
            desktopFile.beginGroup(QStringLiteral("Desktop Entry"));

            if (desktopFile.value(QStringLiteral("Type")).toString() != QLatin1String("Application")) {
                desktopFile.endGroup();
                continue;
            }
            if (desktopFile.value(QStringLiteral("NoDisplay"), false).toBool()) {
                desktopFile.endGroup();
                continue;
            }

            QString name = desktopFile.value(QStringLiteral("Name")).toString();
            if (name.isEmpty()) {
                desktopFile.endGroup();
                continue;
            }

            AppItem item;
            item.desktopId = desktopId;
            item.name = name;
            item.comment = desktopFile.value(QStringLiteral("Comment")).toString();
            item.iconName = desktopFile.value(QStringLiteral("Icon")).toString();
            item.exec = desktopFile.value(QStringLiteral("Exec")).toString();

            QString mimes = desktopFile.value(QStringLiteral("MimeType")).toString();
            if (!mimes.isEmpty()) {
                item.mimeTypes = mimes.split(QChar(';'), Qt::SkipEmptyParts);
            }

            QString cats = desktopFile.value(QStringLiteral("Categories")).toString();
            if (!cats.isEmpty()) {
                item.categories = cats.split(QChar(';'), Qt::SkipEmptyParts);
            }

            desktopFile.endGroup();

            seenIds.insert(desktopId);
            m_allApps.append(item);
        }
    }

    // Sort apps alphabetically by name
    std::sort(m_allApps.begin(), m_allApps.end(), [](const AppItem& a, const AppItem& b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });

    applyFilter();
}

void AppChooserModel::applyFilter() {
    beginResetModel();
    m_filteredApps.clear();

    QList<AppItem> recommended;
    QList<AppItem> others;

    QString q = m_searchQuery.trimmed().toLower();
    QString mime = m_mimeType.trimmed().toLower();

    for (AppItem app : m_allApps) {
        if (!q.isEmpty()) {
            bool matchesQuery = app.name.toLower().contains(q) ||
                               app.comment.toLower().contains(q) ||
                               app.desktopId.toLower().contains(q);
            if (!matchesQuery) continue;
        }

        bool isRec = false;
        if (!mime.isEmpty()) {
            for (const QString& m : app.mimeTypes) {
                if (m.trimmed().toLower() == mime) {
                    isRec = true;
                    break;
                }
            }
        }
        app.isRecommended = isRec;

        if (isRec) {
            recommended.append(app);
        } else {
            others.append(app);
        }
    }

    m_filteredApps = recommended + others;
    endResetModel();
    emit countChanged();
}

} // namespace wormhole::appchooser
