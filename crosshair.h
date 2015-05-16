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

    static bool supported();

private slots:

    void toggle();

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void resetOffset();
    void saveOffset();

    void slotScreenGeometryChanged(const QSize& size);
    void slotWindowActivated(KWin::EffectWindow* w);
    void slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old);
    void slotWindowFinishUserMovedResized(KWin::EffectWindow* w);

private:

    enum Position
    {
        SCREEN_CENTRE         = 0,
        WINDOW_CENTRE         = 1, /* Single window only */
        CURRENT_WINDOW_CENTRE = 2  /* Always follow window focus */
    };

    enum Shape
    {
        IMAGE        = 0,
        CROSS        = 1,
        HOLLOW_CROSS = 2,
        X            = 3,
        HOLLOW_X     = 4,
        SQUARE       = 5,
        DIAMOND      = 6
    };

    enum BlendMode
    {
        NONE               = 0,
        OPAQUE             = 1,
        TRANSPARENT        = 2,
        BLACK_BG           = 3,
        INVERT             = 4,
        INVERT_ON_BLACK_BG = 5,
        INVERT_WITH_ALPHA  = 6,
        DARKEN             = 7,
        LIGHTEN            = 8,
        MULTIPLY           = 9
    };

    void createCrosshair(QPointF &pos, QVector<float> &v);

    QPointF getScreenCentre();
    QPointF getWindowCentre(KWin::EffectWindow* w);

    bool isEnabledForScreen();
    bool isEnabledForWindow(KWin::EffectWindow* w);

    void updateOffset();

    QVector<float> verts;
    bool enabled;
    int size;
    int width;
    float alpha;
    QColor color;
    Shape shape;
    BlendMode blend;
    Position position;
    bool roundPosition;
    int offsetX;
    int offsetY;
    QString imagePath;
    GLTexture* texture;
    QPointF currentPosition;
    QRect currentPositionRect;
    KWin::EffectWindow *lastWindow;
};

} // namespace

#endif
