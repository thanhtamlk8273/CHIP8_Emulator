#ifndef MONITOR_H
#define MONITOR_H

#include <QGraphicsScene>
#include <QGraphicsView>

class Monitor : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Monitor(unsigned width, unsigned height, unsigned scale, QWidget *parent = nullptr);

public slots:
    void update(uint8_t* frame_buffer);

private:
    /* Graphics */
    const unsigned graphics_width;
    const unsigned graphics_height;
    const unsigned graphics_scale_constant;
    QGraphicsScene scene;
};

#endif // MONITOR_H
