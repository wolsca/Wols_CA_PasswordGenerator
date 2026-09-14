#pragma once

#include <QIcon>
#include <QPixmap>
#include <QColor>
#include <QSize>

namespace gui {

enum class IconType {
    App,
    Eye,
    Refresh,
    Copy,
    Settings,
    Vault,
    Save,
    Plus,
    Edit,
    Trash,
    Check,
    Lock,
    Key,
    User,
    Mail,
    Import
};

class IconUtils {
public:
    static QIcon getIcon(IconType type, const QColor& color = QColor(60, 60, 60), int size = 32);
    static QPixmap getPixmap(IconType type, const QColor& color = QColor(60, 60, 60), int size = 32);
};

} // namespace gui
