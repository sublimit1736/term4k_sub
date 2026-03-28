#pragma once

#include "instances/ChartListInstance.h"

namespace ui {

// Placeholder UI interface for chart list.
class ChartListUI {
public:
    explicit ChartListUI(ChartListInstance *instance = nullptr);

    void bindInstance(ChartListInstance *instance);
    void show();
    void hide();

private:
    ChartListInstance *instance_ = nullptr;
};

} // namespace ui

