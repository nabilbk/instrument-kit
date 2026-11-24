// SPDX-License-Identifier: MIT
// Qt Test for the C++ types of the IdevInstruments module (no GUI needed).
#include <QSignalSpy>
#include <QTest>
#include <cmath>

#include "annunciatorstate.h"
#include "instrument.h"

class TestInstrument : public QObject
{
    Q_OBJECT

private slots:
    void configFromSpec()
    {
        Instrument in;
        QSignalSpy configSpy(&in, &Instrument::configChanged);
        in.setConfigId(QStringLiteral("car.rpm"));
        QCOMPARE(configSpy.count(), 1);
        QVERIFY(in.configured());
        QCOMPARE(in.label(), QStringLiteral("RPM"));
        QCOMPARE(in.maximum(), 8000.0);
        QCOMPARE(in.zones().size(), 3);
        QCOMPARE(in.scaleLabel(7000), QStringLiteral("7"));
        QCOMPARE(in.scaleCaption(), QStringLiteral("x1000"));
    }

    void unknownConfigId()
    {
        Instrument in;
        QSignalSpy spy(&in, &Instrument::unknownConfigId);
        QTest::ignoreMessage(QtWarningMsg, "Instrument: unknown configId 'nope'");
        in.setConfigId(QStringLiteral("nope"));
        QCOMPARE(spy.count(), 1);
        QVERIFY(!in.configured());
        in.setValue(3.0); // must not crash
        QCOMPARE(in.displayText(), QStringLiteral("---"));
    }

    void valueBeforeConfigIsApplied()
    {
        Instrument in;
        in.setValue(4000);
        in.setConfigId(QStringLiteral("car.rpm"));
        QVERIFY(in.valid());
        QCOMPARE(in.ratio(), 0.5);
        QCOMPARE(in.displayText(), QStringLiteral("4000"));
    }

    void dampingConverges()
    {
        Instrument in;
        in.setConfigId(QStringLiteral("car.rpm")); // tau 80 ms
        in.setValue(1000);
        QCOMPARE(in.displayValue(), 1000.0);
        in.setValue(5000);
        QVERIFY(in.displayValue() < 1001.0);
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(in.displayValue() - 5000.0) < 0.01, 2000);
    }

    void alertsAndInvalid()
    {
        Instrument in;
        in.setConfigId(QStringLiteral("car.coolant"));
        QSignalSpy alertSpy(&in, &Instrument::alertChanged);
        in.setValue(90);
        QCOMPARE(in.alertLevel(), Instrument::Normal);
        in.setValue(120);
        QCOMPARE(in.alertLevel(), Instrument::Warning);
        QCOMPARE(in.alertHigh(), Instrument::Warning);
        QCOMPARE(in.alertLow(), Instrument::Normal);
        in.setValue(std::nan(""));
        QVERIFY(!in.valid());
        QCOMPARE(in.alertLevel(), Instrument::Normal);
        QCOMPARE(in.displayText(), QStringLiteral("---"));
        QCOMPARE(alertSpy.count(), 2);
    }

    void annunciatorBlinkAndAcknowledge()
    {
        AnnunciatorState a;
        QVERIFY(!a.lit());
        a.setActive(true);
        QVERIFY(a.lit());
        a.setActive(false);
        a.setLevel(Instrument::Caution);
        QVERIFY(a.lit());
        a.setLevel(Instrument::Warning);
        QSignalSpy litSpy(&a, &AnnunciatorState::litChanged);
        QTRY_VERIFY_WITH_TIMEOUT(litSpy.count() >= 2, 2000); // flashing at 2 Hz
        a.acknowledge();
        QVERIFY(a.acknowledged());
        QVERIFY(a.lit());
        a.setLevel(Instrument::Caution);
        a.setLevel(Instrument::Warning);
        QVERIFY(!a.acknowledged()); // re-armed by a higher level
    }
};

QTEST_GUILESS_MAIN(TestInstrument)
#include "tst_instrument.moc"
