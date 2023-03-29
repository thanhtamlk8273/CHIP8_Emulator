#include "monitor.h"

Monitor::Monitor(unsigned width, unsigned height, unsigned scale, QWidget *parent)
    : QGraphicsView{parent},
      graphics_width{width},
      graphics_height{height},
      graphics_scale_constant{scale},
      scene()
{
    setScene(&scene);
    QRect viewRect(0, 0, graphics_width * graphics_scale_constant, graphics_height * graphics_scale_constant);
    setSceneRect(viewRect);
}

void Monitor::update(uint8_t* frame_buffer)
{
    QBitmap map = QBitmap::fromData(QSize(graphics_width, graphics_height),
                                    frame_buffer,
                                    QImage::Format_Mono);
    scene.addPixmap(map.scaled(QSize(graphics_width * graphics_scale_constant,
                                     graphics_height * graphics_scale_constant),
                               Qt::IgnoreAspectRatio,
                               Qt::FastTransformation));
}
