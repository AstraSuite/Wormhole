#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QQuickPaintedItem>
#include <QString>
#include <qqmlintegration.h>

#include "waylandcapture.hpp"

namespace wormhole::screencast {

class LiveScreenItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(bool isWindow READ isWindow WRITE setIsWindow NOTIFY isWindowChanged)
    Q_PROPERTY(QString appId READ appId WRITE setAppId NOTIFY appIdChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool includeCursor READ includeCursor WRITE setIncludeCursor NOTIFY includeCursorChanged)

public:
    explicit LiveScreenItem(QQuickItem* parent = nullptr);
    ~LiveScreenItem() override;

    QString target() const { return m_target; }
    void setTarget(const QString& target);

    bool live() const { return m_live; }
    void setLive(bool live);

    int fps() const { return m_fps; }
    void setFps(int fps);

    qreal radius() const { return m_radius; }
    void setRadius(qreal radius);

    bool isWindow() const { return m_isWindow; }
    void setIsWindow(bool isWindow);

    QString appId() const { return m_appId; }
    void setAppId(const QString& appId);

    QString title() const { return m_title; }
    void setTitle(const QString& title);

    bool includeCursor() const { return m_includeCursor; }
    void setIncludeCursor(bool includeCursor);

    void paint(QPainter* painter) override;

signals:
    void targetChanged();
    void liveChanged();
    void fpsChanged();
    void radiusChanged();
    void isWindowChanged();
    void appIdChanged();
    void titleChanged();
    void includeCursorChanged();

protected:
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    void restart();
    void stopCapture();
    void onFrame(const QImage& frame);

    QString m_target;
    QString m_appId;
    QString m_title;
    bool m_live = true;
    bool m_includeCursor = false;
    int m_fps = 30;
    qreal m_radius = 0.0;
    bool m_isWindow = false;
    QImage m_currentFrame;
    WaylandCapture* m_capture = nullptr;
};

} // namespace wormhole::screencast
