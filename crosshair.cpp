/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2006 Lubos Lunak <l.lunak@kde.org>
Copyright (C) 2007 Christian Nitschkowski <christian.nitschkowski@kdemail.net>
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

#include "crosshair.h"

#include <kwinconfig.h>
#include <kwinglutils.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>

#include <math.h>

#include <kdebug.h>

#include <QVector4D>

namespace KWin
{

KWIN_EFFECT(crosshair, CrosshairEffect)
KWIN_EFFECT_SUPPORTED(crosshair, CrosshairEffect::supported())

CrosshairEffect::CrosshairEffect()
    : texture(NULL)
{
    KActionCollection* actionCollection = new KActionCollection(this);
    KAction* a;

    a = static_cast<KAction*>(actionCollection->addAction("ToggleCrosshair"));
    a->setText(i18n("Toggle Crosshair"));
    a->setGlobalShortcut(KShortcut(Qt::SHIFT + Qt::META + Qt::Key_F11));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(toggle()));

    a = static_cast<KAction*>(actionCollection->addAction("MoveCrosshairUp"));
    a->setText(i18n("Move Crosshair Up"));
    a->setGlobalShortcut(KShortcut());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(moveUp()));

    a = static_cast<KAction*>(actionCollection->addAction("MoveCrosshairDown"));
    a->setText(i18n("Move Crosshair Down"));
    a->setGlobalShortcut(KShortcut());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(moveDown()));

    a = static_cast<KAction*>(actionCollection->addAction("MoveCrosshairLeft"));
    a->setText(i18n("Move Crosshair Left"));
    a->setGlobalShortcut(KShortcut());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(moveLeft()));

    a = static_cast<KAction*>(actionCollection->addAction("MoveCrosshairRight"));
    a->setText(i18n("Move Crosshair Right"));
    a->setGlobalShortcut(KShortcut());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(moveRight()));

    a = static_cast<KAction*>(actionCollection->addAction("ResetCrosshairOffset"));
    a->setText(i18n("Reset Crosshair Offset"));
    a->setGlobalShortcut(KShortcut());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(resetOffset()));

    a = static_cast<KAction*>(actionCollection->addAction("SaveCrosshairOffset"));
    a->setText(i18n("Save Crosshair Offset"));
    a->setGlobalShortcut(KShortcut());
    connect(a, SIGNAL(triggered(bool)), this, SLOT(saveOffset()));

    connect(effects, SIGNAL(screenGeometryChanged(QSize)), this, SLOT(slotScreenGeometryChanged(QSize)));
    connect(effects, SIGNAL(windowActivated(KWin::EffectWindow*)), this, SLOT(slotWindowActivated(KWin::EffectWindow*)));
    connect(effects, SIGNAL(windowGeometryShapeChanged(KWin::EffectWindow*, QRect)), this, SLOT(slotWindowGeometryShapeChanged(KWin::EffectWindow*, QRect)));
    connect(effects, SIGNAL(windowFinishUserMovedResized(KWin::EffectWindow*)), this, SLOT(slotWindowFinishUserMovedResized(KWin::EffectWindow*)));

    reconfigure(ReconfigureAll);
}

CrosshairEffect::~CrosshairEffect()
{
    if (texture != NULL) {
        delete texture;
    }
}

void CrosshairEffect::reconfigure(ReconfigureFlags)
{
    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");

    size  = conf.readEntry("Size", 20);
    width = conf.readEntry("LineWidth", 1) / 2.0f;

    color = conf.readEntry("Color", QColor(255, 48, 48));
    alpha = conf.readEntry("Alpha", 100) / 100.0f;
    color.setAlphaF(alpha);

    shape    = static_cast<Shape>    (conf.readEntry("Shape",    static_cast<int>(IMAGE)));
    blend    = static_cast<BlendMode>(conf.readEntry("Blend",    static_cast<int>(INVERT_WITH_ALPHA)));
    position = static_cast<Position> (conf.readEntry("Position", static_cast<int>(SCREEN_CENTRE)));

    roundPosition = conf.readEntry("RoundPosition", true);

    offsetX = conf.readEntry("OffsetX", 0);
    offsetY = conf.readEntry("OffsetY", 0);

    imagePath = conf.readEntry("Image", KGlobal::dirs()->findResource("data", "kwin/crosshair_glow.png"));

    enabled = false;

    if (texture != NULL) {
        delete texture;
        texture = NULL;
    }

    if (shape == 0 && !imagePath.isEmpty()) {
        QImage image(imagePath);
        texture = new GLTexture(image);
    }

    switch (position) {
        case SCREEN_CENTRE:
            currentPosition = getScreenCentre();
            break;

        case WINDOW_CENTRE:
            break;

        case CURRENT_WINDOW_CENTRE:
            currentPosition = getWindowCentre(effects->activeWindow());
            break;
    }

    createCrosshair(currentPosition, verts);

    if ((effects->compositingType() & OpenGLCompositing) == 0) {
        kDebug() << "Unsupported compositing type (not OpenGL)!";
    }
}

void CrosshairEffect::paintScreen(int mask, QRegion region, ScreenPaintData& data)
{
    effects->paintScreen(mask, region, data);   // paint normal screen

    if (!enabled)
        return;

    if (effects->compositingType() & OpenGLCompositing) {
#ifndef KWIN_HAVE_OPENGLES
        glEnable(GL_LINE_SMOOTH);
#endif
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);

        switch (blend) {
            case NONE:
                break;

            case OPAQUE:
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;

            case TRANSPARENT:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;

            case BLACK_BG:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
                break;

            case INVERT:
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
                break;

            case INVERT_ON_BLACK_BG:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR);
                break;

            case INVERT_WITH_ALPHA:
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                break;

            case DARKEN:
                glEnable(GL_BLEND);
                glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                break;

            case LIGHTEN:
                glEnable(GL_BLEND);
                glBlendFunc(GL_DST_COLOR, GL_ONE);
                break;

            case MULTIPLY:
                glEnable(GL_BLEND);
                glBlendFunc(GL_DST_COLOR, GL_ZERO); /* Multiply */
                break;

            default:
                kDebug() << "Invalid blending mode!";
                break;
        }

        glLineWidth(width);

        ShaderManager *shaderManager = ShaderManager::instance();
        if (shape != IMAGE) {
            shaderManager->pushShader(ShaderManager::ColorShader);

            GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
            vbo->reset();
            vbo->setUseColor(true);
            vbo->setColor(color);
            vbo->setData(verts.size() / 2, 2, verts.data(), NULL);
            vbo->render(GL_LINES);

            shaderManager->popShader();
        } else if (texture != NULL) {
            shaderManager->pushShader(ShaderManager::SimpleShader);

            GLShader *shader = shaderManager->getBoundShader();
            shader->setUniform(GLShader::Saturation, 1.0);
            shader->setUniform(GLShader::ModulationConstant, QVector4D(
                                   color.redF(),
                                   color.greenF(),
                                   color.blueF(),
                                   alpha));

            texture->bind();
            texture->render(region, currentPositionRect);
            texture->unbind();

            shaderManager->popShader();
        }

        glLineWidth(1.0f);
        glPopAttrib();
#ifndef KWIN_HAVE_OPENGLES
        glDisable(GL_LINE_SMOOTH);
#endif
    }
}

void CrosshairEffect::toggle()
{
    enabled = !enabled;
    if (enabled) {
        switch (position) {
            case SCREEN_CENTRE:
                currentPosition = getScreenCentre();
                break;

            case WINDOW_CENTRE:
                currentPosition = getWindowCentre(effects->activeWindow());
                lastWindow = effects->activeWindow();
                break;

            case CURRENT_WINDOW_CENTRE:
                currentPosition = getWindowCentre(effects->activeWindow());
                lastWindow = effects->activeWindow();
                break;
        }
        createCrosshair(currentPosition, verts);
    }
    effects->addRepaintFull();
}

void CrosshairEffect::createCrosshair(QPointF &pos, QVector<float> &v)
{
    float x = pos.x() + offsetX;
    float y = pos.y() + offsetY;

    if (roundPosition) {
        x = round(x);
        y = round(y);
    }

    currentPositionRect = QRect(x - size, y - size, 2 * size, 2 * size);

    v.clear();
    switch (shape) {
        case IMAGE:
            break;

        case CROSS:
            v << (x - size) << (y -    0);
            v << (x + size) << (y +    0);
            v << (x -    0) << (y - size);
            v << (x +    0) << (y + size);
            break;

        case HOLLOW_CROSS:
            v << (x - size) << (y -    0);
            v << (x -    1) << (y -    0);
            v << (x + size) << (y +    0);
            v << (x +    1) << (y +    0);
            v << (x -    0) << (y - size);
            v << (x -    0) << (y -    1);
            v << (x +    0) << (y + size);
            v << (x +    0) << (y +    1);
            break;

        case X:
            v << (x - size) << (y - size);
            v << (x + size) << (y + size);
            v << (x - size) << (y + size);
            v << (x + size) << (y - size);
            break;

        case HOLLOW_X:
            v << (x - size) << (y - size);
            v << (x -    1) << (y -    1);
            v << (x + size) << (y + size);
            v << (x +    1) << (y +    1);
            v << (x - size) << (y + size);
            v << (x -    1) << (y +    1);
            v << (x + size) << (y - size);
            v << (x +    1) << (y -    1);
            break;

        case SQUARE:
            v << (x - size) << (y - size);
            v << (x + size) << (y - size);
            v << (x - size) << (y + size);
            v << (x + size) << (y + size);
            v << (x - size) << (y - size);
            v << (x - size) << (y + size);
            v << (x + size) << (y - size);
            v << (x + size) << (y + size);
            break;

        case DIAMOND:
            v << (x       ) << (y - size);
            v << (x + size) << (y       );
            v << (x + size) << (y       );
            v << (x       ) << (y + size);
            v << (x       ) << (y + size);
            v << (x - size) << (y       );
            v << (x - size) << (y       );
            v << (x       ) << (y - size);
            break;

        default:
            kDebug() << "Invalid shape!";
            break;
    }
}

bool CrosshairEffect::isActive() const
{
    return enabled;
}

QPointF CrosshairEffect::getScreenCentre()
{
    const QRect& rect = effects->clientArea(ScreenArea, effects->activeScreen(), 0);
    return QPointF(rect.x() + rect.width () / 2.0f,
                   rect.y() + rect.height() / 2.0f);
}

QPointF CrosshairEffect::getWindowCentre(KWin::EffectWindow* w)
{
    if (w != NULL) {
        const QRect& rect = w->geometry();
        return QPointF(rect.x() + rect.width () / 2.0f,
                       rect.y() + rect.height() / 2.0f);
    } else {
        // We can't do anything
        return QPointF(0, 0);
    }
}

bool CrosshairEffect::isEnabledForScreen()
{
    return enabled && (position == SCREEN_CENTRE);
}

bool CrosshairEffect::isEnabledForWindow(KWin::EffectWindow* w)
{
    return enabled
        // Check if always enabled for current window
        && (position == CURRENT_WINDOW_CENTRE
            // Otherwise check if it's the window set by user
            || (position == WINDOW_CENTRE && w == lastWindow));
}

void CrosshairEffect::slotScreenGeometryChanged(const QSize& size)
{
    Q_UNUSED(size);

    if (isEnabledForScreen()) {
        currentPosition = getScreenCentre();
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowActivated(KWin::EffectWindow* w)
{
    if (isEnabledForWindow(w)) {
        currentPosition = getWindowCentre(w);
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old)
{
    Q_UNUSED(old);

    if (isEnabledForWindow(w)) {
        currentPosition = getWindowCentre(effects->activeWindow());
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowFinishUserMovedResized(KWin::EffectWindow* w)
{
    if (isEnabledForWindow(w)) {
        currentPosition = getWindowCentre(effects->activeWindow());
        createCrosshair(currentPosition, verts);
    }
}

bool CrosshairEffect::supported()
{
    return effects->compositingType() & OpenGLCompositing;
}

void CrosshairEffect::updateOffset()
{
    createCrosshair(currentPosition, verts);
    effects->addRepaintFull();
}

void CrosshairEffect::moveUp()
{
    offsetY -= 1;
    updateOffset();
}

void CrosshairEffect::moveDown()
{
    offsetY += 1;
    updateOffset();
}

void CrosshairEffect::moveLeft()
{
    offsetX -= 1;
    updateOffset();
}

void CrosshairEffect::moveRight()
{
    offsetX += 1;
    updateOffset();
}

void CrosshairEffect::resetOffset()
{
    offsetX = 0;
    offsetY = 0;
    updateOffset();
}

void CrosshairEffect::saveOffset()
{
    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");
    conf.writeEntry("OffsetX", offsetX);
    conf.writeEntry("OffsetY", offsetY);
    conf.sync();
}

} // namespace

#include "crosshair.moc"
