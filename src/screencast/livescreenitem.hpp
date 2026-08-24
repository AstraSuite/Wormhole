#pragma once

#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QQuickPaintedItem>
#include <QTimer>
#include <qqmlintegration.h>

namespace wormhole::screencast {

class LiveScreenItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(bool isWindow READ isWindow WRITE setIsWindow NOTIFY isWindowChanged)
    Q_PROPERTY(QString windowGeometry READ windowGeometry WRITE setWindowGeometry NOTIFY windowGeometryChanged)

public:
    explicit LiveScreenItem(QQuickItem* parent = nullptr);
    ~LiveScreenItem() override;

    QString target() const { return m_target; }
    void setTarget(const QString& t);

    bool live() const { return m_live; }
    void setLive(bool l);

    int fps() const { return m_fps; }
    void setFps(int f);

    qreal radius() const { return m_radius; }
    void setRadius(qreal r);

    bool isWindow() const { return m_isWindow; }
    void setIsWindow(bool w);

    QString windowGeometry() const { return m_windowGeometry; }
    void setWindowGeometry(const QString& g);

    void paint(QPainter* painter) override;

signals:
    void targetChanged();
    void liveChanged();
    void fpsChanged();
    void radiusChanged();
    void isWindowChanged();
    void windowGeometryChanged();

private slots:
    void grabFrame();

protected:
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    void updateTimerState();

    QString m_target;
    bool m_live = true;
    int m_fps = 30;
    qreal m_radius = 0.0;
    bool m_isWindow = false;
    QString m_windowGeometry;
    QTimer* m_timer = nullptr;
    QImage m_currentFrame;
    QProcess* m_process = nullptr;
    bool m_busy = false;
};

} // namespace wormhole::screencast
