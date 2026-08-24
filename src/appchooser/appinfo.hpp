#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <qqmlintegration.h>

namespace wormhole::appchooser {

struct AppItem {
    QString desktopId;
    QString name;
    QString comment;
    QString iconName;
    QString exec;
    QStringList mimeTypes;
    QStringList categories;
    bool isRecommended = false;
};

class AppChooserModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType NOTIFY mimeTypeChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        DesktopIdRole = Qt::UserRole + 1,
        NameRole,
        CommentRole,
        IconNameRole,
        ExecRole,
        IsRecommendedRole
    };

    explicit AppChooserModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString mimeType() const { return m_mimeType; }
    void setMimeType(const QString& mime);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString& query);

    int count() const { return m_filteredApps.size(); }

    Q_INVOKABLE void reload();

signals:
    void mimeTypeChanged();
    void searchQueryChanged();
    void countChanged();

private:
    void loadDesktopFiles();
    void applyFilter();

    QString m_mimeType;
    QString m_searchQuery;
    QList<AppItem> m_allApps;
    QList<AppItem> m_filteredApps;
};

} // namespace wormhole::appchooser
