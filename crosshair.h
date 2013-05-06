/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2007 Lubos Lunak <l.lunak@kde.org>
Copyright (C) 2013 Pawel Bartkiewicz <tuuresairon@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef KWIN_CROSSHAIR_H
#define KWIN_CROSSHAIR_H

#include <kwineffects.h>
#include <kwinglutils.h>

namespace KWin
{

class CrosshairEffect
    : public Effect
{
    Q_OBJECT
public:
    CrosshairEffect();
    ~CrosshairEffect();
    virtual void reconfigure(ReconfigureFlags);
    virtual void paintScreen(int mask, QRegion region, ScreenPaintData& data);
    virtual bool isActive() const;

private slots:
    void toggle();

    void slotScreenGeometryChanged(const QSize& size);
    void slotWindowActivated(KWin::EffectWindow* w);
    void slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old);
    void slotWindowFinishUserMovedResized(KWin::EffectWindow* w);

private:
    void createCrosshair(QPointF &pos, QVector<float> &v);

    QPointF getScreenCentre();
    QPointF getWindowCentre(KWin::EffectWindow* w);

    QVector<float> verts;
    bool enabled;
    int size;
    int width;
    float alpha;
    QColor color;
    int shape;
    int blend;
    int position;
    bool roundPosition;
    QPointF currentPosition;
    KWin::EffectWindow *lastWindow;
};

} // namespace

#endif
