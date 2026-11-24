// SPDX-License-Identifier: MIT
#include "demorunner.h"

#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(IdevInstrumentsPlugin)
Q_IMPORT_QML_PLUGIN(IdevDemoSimPlugin)

int main(int argc, char *argv[])
{
    return runDemo(argc, argv, "CockpitPfd", "Main");
}
