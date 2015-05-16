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

static int width_2 = 1;
void CrosshairEffect::reconfigure(ReconfigureFlags)
{
    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");
    size  = conf.readEntry("Size", 20);
    width = conf.readEntry("LineWidth", 1);
    width_2 = width / 2;
    color = conf.readEntry("Color", QColor(255, 48, 48));
    alpha = conf.readEntry("Alpha", 100) / 100.0f;
    color.setAlphaF(alpha);
    shape = conf.readEntry("Shape", 0);
    blend = conf.readEntry("Blend", 6);
    position = conf.readEntry("Position", 0);
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
    case 0:
        currentPosition = getScreenCentre();
        break;
    case 1:
        break;
    case 2:
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
        case 0:
            break;
        case 1:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); /* Opaque */
            break;
        case 2:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); /* Transparent */
            break;
        case 3:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ZERO); /* Black Background */
            break;
        case 4:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); /* Invert */
            break;
        case 5:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR); /* Invert on Black Background */
            break;
        case 6:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); /* Invert with Alpha */
            break;
        case 7:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); /* Darken */
            break;
        case 8:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ONE); /* Lighten */
            break;
        case 9:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO); /* Multiply */
            break;
        }

        glLineWidth(width / 2.0f);

        ShaderManager *shaderManager = ShaderManager::instance();
        if (shape > 0) {
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
        case 0:
            currentPosition = getScreenCentre();
            break;
        case 1:
            currentPosition = getWindowCentre(effects->activeWindow());
            lastWindow = effects->activeWindow();
            break;
        case 2:
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
    case 0:
        // Image
        break;
    case 1:
        v << (x - size) << (y -    0);
        v << (x + size) << (y +    0);
        v << (x -    0) << (y - size);
        v << (x +    0) << (y + size);
        break;
    case 2:
        v << (x - size) << (y -    0);
        v << (x -    1) << (y -    0);
        v << (x + size) << (y +    0);
        v << (x +    1) << (y +    0);
        v << (x -    0) << (y - size);
        v << (x -    0) << (y -    1);
        v << (x +    0) << (y + size);
        v << (x +    0) << (y +    1);
        break;
    case 3:
        v << (x - size) << (y - size);
        v << (x + size) << (y + size);
        v << (x - size) << (y + size);
        v << (x + size) << (y - size);
        break;
    case 4:
        v << (x - size) << (y - size);
        v << (x -    1) << (y -    1);
        v << (x + size) << (y + size);
        v << (x +    1) << (y +    1);
        v << (x - size) << (y + size);
        v << (x -    1) << (y +    1);
        v << (x + size) << (y - size);
        v << (x +    1) << (y -    1);
        break;
    case 5:
        v << (x - size) << (y - size);
        v << (x + size) << (y - size);
        v << (x - size) << (y + size);
        v << (x + size) << (y + size);
        v << (x - size) << (y - size);
        v << (x - size) << (y + size);
        v << (x + size) << (y - size);
        v << (x + size) << (y + size);
        break;
    case 6:
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
        // TODO
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

void CrosshairEffect::slotScreenGeometryChanged(const QSize& size)
{
    if (enabled && position == 0) {
        currentPosition = getScreenCentre();
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowActivated(KWin::EffectWindow* w)
{
    if (enabled && ((position == 1 && w == lastWindow) || position == 2)) {
        currentPosition = getWindowCentre(w);
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old)
{
    if (enabled && ((position == 1 && w == lastWindow) || position == 2)) {
        currentPosition = getWindowCentre(effects->activeWindow());
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowFinishUserMovedResized(KWin::EffectWindow* w)
{
    if (enabled && ((position == 1 && w == lastWindow) || position == 2)) {
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
