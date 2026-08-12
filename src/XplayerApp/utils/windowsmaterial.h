#ifndef WINDOWSMATERIAL_H
#define WINDOWSMATERIAL_H

#include <QWidget>

namespace WindowsMaterial
{

enum class Kind {
    MainWindow,
    Acrylic,
};

bool applyBackdrop(WId windowId, Kind kind);
bool setDarkMode(WId windowId, bool enabled);

} // namespace WindowsMaterial

#endif
