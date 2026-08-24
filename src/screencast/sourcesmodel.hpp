#pragma once

#include <QAbstractListModel>
#include <QRect>
#include <qqmlintegration.h>
#include "hyprlandipc.hpp"

namespace wormhole::screencast {

class ScreensModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int minX READ minX NOTIFY boundsChanged)
    Q_PROPERTY(int minY READ minY NOTIFY boundsChanged)
    Q_PROPERTY(int totalWidth READ totalWidth NOTIFY boundsChanged)
    Q_PROPERTY(int totalHeight READ totalHeight NOTIFY boundsChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        MakeModelRole,
        ResolutionRole,
        RefreshRateRole,
        ScaleRole,
        FocusedRole,
        ActiveWorkspaceRole,
        WidthRole,
        HeightRole,
        XRole,
        YRole,
        TransformRole,
        EffectiveWidthRole,
        EffectiveHeightRole,
        RotationRole
    };

    explicit ScreensModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int minX() const { return m_minX; }
    int minY() const { return m_minY; }
    int totalWidth() const { return m_totalWidth; }
    int totalHeight() const { return m_totalHeight; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE void focusScreen(const QString& name);

signals:
    void boundsChanged();

private:
    void calculateBounds();

    QList<HyprlandMonitor> m_monitors;
    HyprlandIPC m_ipc;
    int m_minX = 0;
    int m_minY = 0;
    int m_totalWidth = 1920;
    int m_totalHeight = 1080;
};

class WindowsModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        AddressRole = Qt::UserRole + 1,
        TitleRole,
        ClassNameRole,
        InitialClassRole,
        IconNameRole,
        WorkspaceNameRole,
        WorkspaceIdRole,
        PidRole,
        XRole,
        YRole,
        WidthRole,
        HeightRole,
        MonitorIdRole,
        FloatingRole
    };

    explicit WindowsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void focusWindow(const QString& address);

private:
    QList<HyprlandWindow> m_windows;
    HyprlandIPC m_ipc;
};

} // namespace wormhole::screencast
