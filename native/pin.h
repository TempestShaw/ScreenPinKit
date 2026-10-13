#pragma once

#include "toolbar.h"
#include <QImage>
#include <QWidget>
class InlineTextEditor;

class PinWindow : public QWidget {
    Q_OBJECT
public:
    explicit PinWindow(QImage image);
    QImage exportedImage() const;
    void setClickThrough(bool enabled);
    bool isClickThrough() const { return clickThrough; }
    void setZoom(qreal value);
    qreal zoomFactor() const { return zoom; }
    void undo();
    void redo();
    void addAnnotation(Annotation annotation);
signals:
    void closed();
    void ocrRequested(QImage image);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    void closeEvent(QCloseEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;
private:
    QPointF imagePoint(QPointF point) const;
    void copy();
    void save();
    void setTool(Tool value);
    void showTools();
    void positionTools();
    QImage image;
    qreal ratio = 1, zoom = 1;
    Tool tool = Tool::Move;
    QColor color = QColor("#ff5a65");
    qreal strokeWidth = 3;
    AnnotationLayer annotations;
    AnnotationToolbar *toolbar;
    InlineTextEditor *textEditor;
    bool moving = false, clickThrough = false;
    QPointF dragStart;
    QPoint windowStart;
};
