// SPDX-License-Identifier: MIT
#pragma once

#include <QColor>
#include <QObject>
#include <QtQml/qqmlregistration.h>

#include "idev/instrument.h"

// Colours from spec/instruments.json ("palette"), shared with the LVGL widgets.
class InstrumentPalette : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QColor normal READ normal CONSTANT)
    Q_PROPERTY(QColor caution READ caution CONSTANT)
    Q_PROPERTY(QColor warning READ warning CONSTANT)
    Q_PROPERTY(QColor background READ background CONSTANT)
    Q_PROPERTY(QColor surface READ surface CONSTANT)
    Q_PROPERTY(QColor surfaceRaised READ surfaceRaised CONSTANT)
    Q_PROPERTY(QColor foreground READ foreground CONSTANT)
    Q_PROPERTY(QColor dim READ dim CONSTANT)
    Q_PROPERTY(QColor needle READ needle CONSTANT)
    Q_PROPERTY(QColor sky READ sky CONSTANT)
    Q_PROPERTY(QColor ground READ ground CONSTANT)
    Q_PROPERTY(QColor symbol READ symbol CONSTANT)

public:
    using QObject::QObject;

    static QColor rgb(uint32_t v) { return QColor::fromRgb(QRgb(0xFF000000u | v)); }

    QColor normal() const { return rgb(idev_palette.normal); }
    QColor caution() const { return rgb(idev_palette.caution); }
    QColor warning() const { return rgb(idev_palette.warning); }
    QColor background() const { return rgb(idev_palette.background); }
    QColor surface() const { return rgb(idev_palette.surface); }
    QColor surfaceRaised() const { return rgb(idev_palette.surface_raised); }
    QColor foreground() const { return rgb(idev_palette.foreground); }
    QColor dim() const { return rgb(idev_palette.dim); }
    QColor needle() const { return rgb(idev_palette.needle); }
    QColor sky() const { return rgb(idev_palette.sky); }
    QColor ground() const { return rgb(idev_palette.ground); }
    QColor symbol() const { return rgb(idev_palette.symbol); }

    Q_INVOKABLE QColor levelColor(int level) const
    {
        return rgb(idev_level_color(static_cast<idev_level_t>(level)));
    }
};
