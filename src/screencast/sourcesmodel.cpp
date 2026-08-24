#include "sourcesmodel.hpp"

namespace wormhole::screencast {

// ScreensModel
ScreensModel::ScreensModel(QObject* parent)
    : QAbstractListModel(parent) {
    reload();
}

void ScreensModel::reload() {
    beginResetModel();
    m_monitors = m_ipc.getMonitors();
    calculateBounds();
    endResetModel();
    emit boundsChanged();
}

void ScreensModel::calculateBounds() {
    if (m_monitors.isEmpty()) {
        m_minX = 0;
        m_minY = 0;
        m_totalWidth = 1920;
        m_totalHeight = 1080;
        return;
    }

    int minX = m_monitors.first().x;
    int minY = m_monitors.first().y;
    int maxX = minX + m_monitors.first().effectiveWidth();
    int maxY = minY + m_monitors.first().effectiveHeight();

    for (const auto& m : m_monitors) {
        if (m.x < minX) minX = m.x;
        if (m.y < minY) minY = m.y;
        int right = m.x + m.effectiveWidth();
        int bottom = m.y + m.effectiveHeight();
        if (right > maxX) maxX = right;
        if (bottom > maxY) maxY = bottom;
    }

    m_minX = minX;
    m_minY = minY;
    m_totalWidth = qMax(1, maxX - minX);
    m_totalHeight = qMax(1, maxY - minY);
}

void ScreensModel::focusScreen(const QString& name) {
    HyprlandIPC::focusMonitor(name);
}

int ScreensModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_monitors.size();
}

QVariant ScreensModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_monitors.size()) {
        return {};
    }

    const auto& m = m_monitors.at(index.row());
    switch (role) {
    case IdRole:
        return m.id;
    case NameRole:
        return m.name;
    case DescriptionRole:
        return m.description;
    case MakeModelRole:
        return m.make.isEmpty() ? m.name : QStringLiteral("%1 %2").arg(m.make, m.model);
    case ResolutionRole:
        return QStringLiteral("%1 × %2").arg(m.effectiveWidth()).arg(m.effectiveHeight());
    case RefreshRateRole:
        return QStringLiteral("%1 Hz").arg(QString::number(m.refreshRate, 'f', 0));
    case RefreshRateHzRole:
        return m.refreshRate;
    case ScaleRole:
        return m.scale;
    case FocusedRole:
        return m.focused;
    case ActiveWorkspaceRole:
        return m.activeWorkspaceName;
    case WidthRole:
        return m.width;
    case HeightRole:
        return m.height;
    case XRole:
        return m.x;
    case YRole:
        return m.y;
    case TransformRole:
        return m.transform;
    case EffectiveWidthRole:
        return m.effectiveWidth();
    case EffectiveHeightRole:
        return m.effectiveHeight();
    case RotationRole:
        return m.rotationDegrees();
    default:
        return {};
    }
}

QHash<int, QByteArray> ScreensModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[MakeModelRole] = "makeModel";
    roles[ResolutionRole] = "resolution";
    roles[RefreshRateRole] = "refreshRate";
    roles[RefreshRateHzRole] = "refreshRateHz";
    roles[ScaleRole] = "scale";
    roles[FocusedRole] = "focused";
    roles[ActiveWorkspaceRole] = "activeWorkspace";
    roles[WidthRole] = "sourceWidth";
    roles[HeightRole] = "sourceHeight";
    roles[XRole] = "posX";
    roles[YRole] = "posY";
    roles[TransformRole] = "transform";
    roles[EffectiveWidthRole] = "effectiveWidth";
    roles[EffectiveHeightRole] = "effectiveHeight";
    roles[RotationRole] = "rotationDeg";
    return roles;
}

// WindowsModel
WindowsModel::WindowsModel(QObject* parent)
    : QAbstractListModel(parent) {
    reload();
}

void WindowsModel::reload() {
    beginResetModel();
    m_windows = m_ipc.getClients();
    endResetModel();
}

void WindowsModel::focusWindow(const QString& address) {
    HyprlandIPC::focusWindow(address);
}

int WindowsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_windows.size();
}

QVariant WindowsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_windows.size()) {
        return {};
    }

    const auto& w = m_windows.at(index.row());
    switch (role) {
    case AddressRole:
        return w.address;
    case TitleRole:
        return w.title;
    case ClassNameRole:
        return w.className;
    case InitialClassRole:
        return w.initialClass;
    case IconNameRole:
        return HyprlandIPC::getAppIconName(w.className.isEmpty() ? w.initialClass : w.className, w.title);
    case WorkspaceNameRole:
        return w.workspaceName;
    case WorkspaceIdRole:
        return w.workspaceId;
    case PidRole:
        return w.pid;
    case XRole:
        return w.x;
    case YRole:
        return w.y;
    case WidthRole:
        return w.width;
    case HeightRole:
        return w.height;
    case MonitorIdRole:
        return w.monitorId;
    case FloatingRole:
        return w.floating;
    default:
        return {};
    }
}

QHash<int, QByteArray> WindowsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[AddressRole] = "address";
    roles[TitleRole] = "title";
    roles[ClassNameRole] = "className";
    roles[InitialClassRole] = "initialClass";
    roles[IconNameRole] = "iconName";
    roles[WorkspaceNameRole] = "workspaceName";
    roles[WorkspaceIdRole] = "workspaceId";
    roles[PidRole] = "pid";
    roles[XRole] = "winX";
    roles[YRole] = "winY";
    roles[WidthRole] = "winWidth";
    roles[HeightRole] = "winHeight";
    roles[MonitorIdRole] = "monitorId";
    roles[FloatingRole] = "floating";
    return roles;
}

} // namespace wormhole::screencast
