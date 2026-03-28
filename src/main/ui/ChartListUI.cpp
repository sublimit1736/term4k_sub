#include "ChartListUI.h"

namespace ui {

ChartListUI::ChartListUI(ChartListInstance *instance) : instance_(instance) {}

void ChartListUI::bindInstance(ChartListInstance *instance) {
    instance_ = instance;
}

void ChartListUI::show() {}

void ChartListUI::hide() {}

} // namespace ui

