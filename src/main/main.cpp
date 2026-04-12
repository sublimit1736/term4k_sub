#include "ui/HomePageUI.h"
#include "ui/ChartSelectUI.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    while (true) {
        const int action = ui::HomePageUI::run();
        if (action == 1) {
            ui::ChartSelectUI::run();
            continue;
        }
        break;
    }

    return 0;
}
