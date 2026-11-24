// SPDX-License-Identifier: MIT
#include "demorunner.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

int runDemo(int argc, char *argv[], const char *moduleUri, const char *typeName)
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption timeOpt(QStringLiteral("time"),
                                     QStringLiteral("Scenario time in seconds."),
                                     QStringLiteral("seconds"), QStringLiteral("0"));
    const QCommandLineOption grabOpt(QStringLiteral("grab"),
                                     QStringLiteral("Save a screenshot and exit."),
                                     QStringLiteral("file.png"));
    const QCommandLineOption invalidOpt(QStringLiteral("invalid"),
                                        QStringLiteral("Simulate lost sensors (NaN)."));
    parser.addOptions({timeOpt, grabOpt, invalidOpt});
    parser.process(app);

    const double t = parser.value(timeOpt).toDouble();
    const bool grab = parser.isSet(grabOpt);
    constexpr double kLeadIn = 2.0; // seconds of scenario before the screenshot

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("startTime"), grab ? t - kLeadIn : t},
        {QStringLiteral("stopTime"), grab ? t : -1.0},
        {QStringLiteral("sensorFault"), parser.isSet(invalidOpt)},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule(moduleUri, typeName);
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    if (grab) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        const QString path = parser.value(grabOpt);
        // Lead-in plus settle time; 2.6 s also lands in the "lit" half of a 2 Hz flash.
        QTimer::singleShot(2600, &app, [window, path] {
            const QImage image = window ? window->grabWindow() : QImage();
            const bool ok = !image.isNull() && image.save(path);
            if (ok) {
                qInfo("wrote %s (%dx%d)", qPrintable(path), image.width(), image.height());
            } else {
                qCritical("could not grab the window to %s", qPrintable(path));
            }
            QCoreApplication::exit(ok ? 0 : 1);
        });
    }
    return app.exec();
}
