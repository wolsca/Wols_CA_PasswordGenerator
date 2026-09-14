#include "gui/IconUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace gui {

QPixmap IconUtils::getPixmap(IconType type, const QColor& color, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    qreal s = size;
    qreal penWidth = qMax(1.5, s / 14.0);
    QPen pen(color, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case IconType::App: {
        // Rounded shield / safe icon
        p.setBrush(QColor(0, 122, 255));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(s * 0.1, s * 0.1, s * 0.8, s * 0.8, s * 0.2, s * 0.2);

        // Keyhole or Lock
        p.setPen(QPen(Qt::white, penWidth * 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawArc(QRectF(s * 0.35, s * 0.26, s * 0.3, s * 0.3), 0, 180 * 16);
        p.setBrush(Qt::white);
        p.drawRoundedRect(QRectF(s * 0.3, s * 0.42, s * 0.4, s * 0.32), s * 0.06, s * 0.06);
        p.setBrush(QColor(0, 122, 255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(s * 0.5, s * 0.53), s * 0.045, s * 0.045);
        p.setPen(QPen(QColor(0, 122, 255), penWidth * 0.9));
        p.drawLine(QPointF(s * 0.5, s * 0.54), QPointF(s * 0.5, s * 0.64));
        break;
    }
    case IconType::Eye: {
        // Eye outline
        QPainterPath path;
        path.moveTo(s * 0.15, s * 0.5);
        path.quadTo(s * 0.5, s * 0.18, s * 0.85, s * 0.5);
        path.quadTo(s * 0.5, s * 0.82, s * 0.15, s * 0.5);
        p.drawPath(path);

        // Iris
        p.setBrush(color);
        p.drawEllipse(QPointF(s * 0.5, s * 0.5), s * 0.13, s * 0.13);
        break;
    }
    case IconType::Refresh: {
        // Circular refresh arrow
        QRectF rect(s * 0.2, s * 0.2, s * 0.6, s * 0.6);
        p.drawArc(rect, 45 * 16, 270 * 16);

        // Arrow head
        QPolygonF arrow;
        arrow << QPointF(s * 0.72, s * 0.18) << QPointF(s * 0.86, s * 0.32) << QPointF(s * 0.68, s * 0.38);
        p.setBrush(color);
        p.drawPolygon(arrow);
        break;
    }
    case IconType::Copy: {
        // Two overlapping document sheets
        p.drawRoundedRect(QRectF(s * 0.32, s * 0.15, s * 0.50, s * 0.58), s * 0.06, s * 0.06);
        p.drawRoundedRect(QRectF(s * 0.18, s * 0.28, s * 0.50, s * 0.58), s * 0.06, s * 0.06);
        break;
    }
    case IconType::Settings: {
        // Gear icon
        p.drawEllipse(QPointF(s * 0.5, s * 0.5), s * 0.16, s * 0.16);
        for (int i = 0; i < 6; ++i) {
            qreal angle = i * (M_PI / 3.0);
            qreal x1 = s * 0.5 + std::cos(angle) * s * 0.28;
            qreal y1 = s * 0.5 + std::sin(angle) * s * 0.28;
            qreal x2 = s * 0.5 + std::cos(angle) * s * 0.40;
            qreal y2 = s * 0.5 + std::sin(angle) * s * 0.40;
            p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
        break;
    }
    case IconType::Vault: {
        // Safe / Box
        p.drawRoundedRect(QRectF(s * 0.16, s * 0.18, s * 0.68, s * 0.64), s * 0.08, s * 0.08);
        p.drawEllipse(QPointF(s * 0.46, s * 0.5), s * 0.12, s * 0.12);
        p.drawLine(QPointF(s * 0.46, s * 0.5), QPointF(s * 0.54, s * 0.5));
        p.drawLine(QPointF(s * 0.72, s * 0.32), QPointF(s * 0.72, s * 0.68));
        break;
    }
    case IconType::Save: {
        // Floppy disk
        p.drawRoundedRect(QRectF(s * 0.2, s * 0.18, s * 0.6, s * 0.64), s * 0.06, s * 0.06);
        p.drawRect(QRectF(s * 0.32, s * 0.18, s * 0.36, s * 0.22));
        p.drawRect(QRectF(s * 0.30, s * 0.52, s * 0.40, s * 0.30));
        break;
    }
    case IconType::Plus: {
        p.drawLine(QPointF(s * 0.5, s * 0.22), QPointF(s * 0.5, s * 0.78));
        p.drawLine(QPointF(s * 0.22, s * 0.5), QPointF(s * 0.78, s * 0.5));
        break;
    }
    case IconType::Edit: {
        // Pencil
        QPainterPath path;
        path.moveTo(s * 0.66, s * 0.20);
        path.lineTo(s * 0.80, s * 0.34);
        path.lineTo(s * 0.36, s * 0.78);
        path.lineTo(s * 0.22, s * 0.80);
        path.lineTo(s * 0.24, s * 0.66);
        path.closeSubpath();
        p.drawPath(path);
        break;
    }
    case IconType::Trash: {
        // Trash can
        p.drawLine(QPointF(s * 0.2, s * 0.28), QPointF(s * 0.8, s * 0.28));
        p.drawArc(QRectF(s * 0.38, s * 0.16, s * 0.24, s * 0.18), 0, 180 * 16);
        p.drawRoundedRect(QRectF(s * 0.28, s * 0.28, s * 0.44, s * 0.54), s * 0.05, s * 0.05);
        p.drawLine(QPointF(s * 0.42, s * 0.38), QPointF(s * 0.42, s * 0.70));
        p.drawLine(QPointF(s * 0.58, s * 0.38), QPointF(s * 0.58, s * 0.70));
        break;
    }
    case IconType::Check: {
        // Checkmark
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.52);
        path.lineTo(s * 0.42, s * 0.72);
        path.lineTo(s * 0.78, s * 0.28);
        p.drawPath(path);
        break;
    }
    case IconType::Lock: {
        p.drawArc(QRectF(s * 0.32, s * 0.20, s * 0.36, s * 0.36), 0, 180 * 16);
        p.drawRoundedRect(QRectF(s * 0.25, s * 0.38, s * 0.50, s * 0.42), s * 0.08, s * 0.08);
        p.setBrush(color);
        p.drawEllipse(QPointF(s * 0.5, s * 0.54), s * 0.05, s * 0.05);
        p.drawLine(QPointF(s * 0.5, s * 0.55), QPointF(s * 0.5, s * 0.66));
        break;
    }
    case IconType::Key: {
        p.drawEllipse(QPointF(s * 0.36, s * 0.42), s * 0.18, s * 0.18);
        p.drawLine(QPointF(s * 0.48, s * 0.54), QPointF(s * 0.78, s * 0.84));
        p.drawLine(QPointF(s * 0.66, s * 0.72), QPointF(s * 0.76, s * 0.62));
        p.drawLine(QPointF(s * 0.74, s * 0.80), QPointF(s * 0.82, s * 0.72));
        break;
    }
    case IconType::User: {
        p.drawEllipse(QPointF(s * 0.5, s * 0.34), s * 0.16, s * 0.16);
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.80);
        path.quadTo(s * 0.5, s * 0.54, s * 0.78, s * 0.80);
        p.drawPath(path);
        break;
    }
    case IconType::Mail: {
        p.drawRoundedRect(QRectF(s * 0.18, s * 0.26, s * 0.64, s * 0.48), s * 0.06, s * 0.06);
        p.drawLine(QPointF(s * 0.18, s * 0.26), QPointF(s * 0.5, s * 0.52));
        p.drawLine(QPointF(s * 0.82, s * 0.26), QPointF(s * 0.5, s * 0.52));
        break;
    }
    case IconType::Import: {
        // Tray / open box at bottom
        p.drawLine(QPointF(s * 0.20, s * 0.55), QPointF(s * 0.20, s * 0.80));
        p.drawLine(QPointF(s * 0.20, s * 0.80), QPointF(s * 0.80, s * 0.80));
        p.drawLine(QPointF(s * 0.80, s * 0.80), QPointF(s * 0.80, s * 0.55));

        // Arrow pointing downwards into the tray
        p.drawLine(QPointF(s * 0.50, s * 0.18), QPointF(s * 0.50, s * 0.60));
        QPolygonF arrow;
        arrow << QPointF(s * 0.35, s * 0.45) << QPointF(s * 0.50, s * 0.62) << QPointF(s * 0.65, s * 0.45);
        p.setBrush(color);
        p.drawPolygon(arrow);
        break;
    }
    case IconType::Cancel: {
        p.drawLine(QPointF(s * 0.26, s * 0.26), QPointF(s * 0.74, s * 0.74));
        p.drawLine(QPointF(s * 0.74, s * 0.26), QPointF(s * 0.26, s * 0.74));
        break;
    }
    }

    return pixmap;
}

QIcon IconUtils::getIcon(IconType type, const QColor& color, int size) {
    QIcon icon;
    icon.addPixmap(getPixmap(type, color, size), QIcon::Normal, QIcon::Off);
    // Add hover / active / disabled pixmaps
    icon.addPixmap(getPixmap(type, color.lighter(130), size), QIcon::Active, QIcon::Off);
    icon.addPixmap(getPixmap(type, QColor(160, 160, 160), size), QIcon::Disabled, QIcon::Off);
    return icon;
}

} // namespace gui
