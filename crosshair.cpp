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

#include <math.h>

#include <kdebug.h>

namespace KWin
{

KWIN_EFFECT(crosshair, CrosshairEffect)

CrosshairEffect::CrosshairEffect()
{
    KActionCollection* actionCollection = new KActionCollection(this);
    KAction* a = static_cast<KAction*>(actionCollection->addAction("ToggleCrosshair"));

    a->setText(i18n("Toggle Crosshair"));
    a->setGlobalShortcut(KShortcut(Qt::SHIFT + Qt::META + Qt::Key_F11));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(toggle()));
    a = static_cast<KAction*>(actionCollection->addAction("ToggleCrosshair"));

    connect(effects, SIGNAL(screenGeometryChanged(QSize)), this, SLOT(slotScreenGeometryChanged(QSize)));
    connect(effects, SIGNAL(windowActivated(KWin::EffectWindow*)), this, SLOT(slotWindowActivated(KWin::EffectWindow*)));
    connect(effects, SIGNAL(windowGeometryShapeChanged(KWin::EffectWindow*, QRect)), this, SLOT(slotWindowGeometryShapeChanged(KWin::EffectWindow*, QRect)));
    connect(effects, SIGNAL(windowFinishUserMovedResized(KWin::EffectWindow*)), this, SLOT(slotWindowFinishUserMovedResized(KWin::EffectWindow*)));

    reconfigure(ReconfigureAll);
}

CrosshairEffect::~CrosshairEffect()
{
}

static int width_2 = 1;
void CrosshairEffect::reconfigure(ReconfigureFlags)
{
    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");
    size  = conf.readEntry("Size", 2);
    width = conf.readEntry("LineWidth", 1);
    width_2 = width / 2;
    color = conf.readEntry("Color", QColor(Qt::red));
    alpha = conf.readEntry("Alpha", 50) / 100.0f;
    color.setAlphaF(alpha);
    shape = conf.readEntry("Shape", 0);
    blend = conf.readEntry("Blend", 1);
    position = conf.readEntry("Position", 0);
    enabled = true;

    switch (position) {
    case 0:
        currentPosition = getScreenCentre();
	break;
    case 1:
        currentPosition = getWindowCentre(effects->activeWindow());
        break;
    case 2:
        break;
    }

    createCrosshair(currentPosition, verts);
}

#ifdef KWIN_HAVE_XRENDER_COMPOSITING
void CrosshairEffect::addRect(const QPoint &p1, const QPoint &p2, XRectangle *r, XRenderColor *c)
{
    /*r->x = qMin(p1.x(), p2.x()) - width_2;
    r->y = qMin(p1.y(), p2.y()) - width_2;
    r->width = qAbs(p1.x()-p2.x()) + 1 + width_2;
    r->height = qAbs(p1.y()-p2.y()) + 1 + width_2;
    // fast move -> large rect, <strike>tess...</strike> interpolate a line
    if (r->width > 3*width/2 && r->height > 3*width/2) {
        const int n = sqrt(r->width*r->width + r->height*r->height) / width;
        XRectangle *rects = new XRectangle[n-1];
        const int w = p1.x() < p2.x() ? r->width : -r->width;
        const int h = p1.y() < p2.y() ? r->height : -r->height;
        for (int i = 1; i < n; ++i) {
            rects[i-1].x = p1.x() + i*w/n;
            rects[i-1].y = p1.y() + i*h/n;
            rects[i-1].width = rects[i-1].height = width;
        }
        XRenderFillRectangles(display(), PictOpSrc, effects->xrenderBufferPicture(), c, rects, n - 1);
        delete [] rects;
        r->x = p1.x();
        r->y = p1.y();
        r->width = r->height = width;
    }*/
}
#endif

void CrosshairEffect::paintScreen(int mask, QRegion region, ScreenPaintData& data)
{
    effects->paintScreen(mask, region, data);   // paint normal screen
    if (!enabled)
        return;
    if (effects->compositingType() == OpenGLCompositing) {
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
        glLineWidth(width/2.0f);

        GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
        vbo->reset();
        vbo->setUseColor(true);
        vbo->setColor(color);
        if (ShaderManager::instance()->isValid()) {
            ShaderManager::instance()->pushShader(ShaderManager::ColorShader);
        }

	vbo->setData(verts.size() / 2, 2, verts.data(), NULL);
	vbo->render(GL_LINES);

        if (ShaderManager::instance()->isValid()) {
            ShaderManager::instance()->popShader();
        }

        glLineWidth(1.0);
	glPopAttrib();
    #ifndef KWIN_HAVE_OPENGLES
        glDisable(GL_LINE_SMOOTH);
    #endif
    }
#ifdef KWIN_HAVE_XRENDER_COMPOSITING
    /*if (effects->compositingType() == XRenderCompositing) {
        XRenderColor c = preMultiply(color);
        for (int i = 0; i < marks.count(); ++i) {
            const int n = marks[i].count() - 1;
            if (n > 0) {
                XRectangle *rects = new XRectangle[n];
                for (int j = 0; j < marks[i].count()-1; ++j) {
                    addRect(marks[i][j], marks[i][j+1], &rects[j], &c);
                }
                XRenderFillRectangles(display(), PictOpSrc, effects->xrenderBufferPicture(), &c, rects, n);
                delete [] rects;
            }
        }
        const int n = drawing.count() - 1;
        if (n > 0) {
            XRectangle *rects = new XRectangle[n];
            for (int i = 0; i < n; ++i)
                addRect(drawing[i], drawing[i+1], &rects[i], &c);
            XRenderFillRectangles(display(), PictOpSrc, effects->xrenderBufferPicture(), &c, rects, n);
            delete [] rects;
        }
	}*/
#endif
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
	    break;
	case 2:
	    break;
	}
        createCrosshair(currentPosition, verts);
    }
    effects->addRepaintFull();
}

void CrosshairEffect::createCrosshair(QPointF &pos, QVector<float> &v)
{
    float x = pos.x();
    float y = pos.y();
    v.clear();
    switch (shape) {
    case 0:
        v << (x - size) << (y -    0);
	v << (x + size) << (y +    0);
	v << (x -    0) << (y - size);
	v << (x +    0) << (y + size);
	break;
    case 1:
        v << (x - size) << (y -    0);
        v << (x -    1) << (y -    0);
	v << (x + size) << (y +    0);
	v << (x +    1) << (y +    0);
	v << (x -    0) << (y - size);
	v << (x -    0) << (y -    1);
	v << (x +    0) << (y + size);
	v << (x +    0) << (y +    1);
	break;
    case 2:
        v << (x - size) << (y - size);
	v << (x + size) << (y + size);
	v << (x - size) << (y + size);
	v << (x + size) << (y - size);
	break;
    case 3:
        v << (x - size) << (y - size);
        v << (x -    1) << (y -    1);
	v << (x + size) << (y + size);
	v << (x +    1) << (y +    1);
	v << (x - size) << (y + size);
	v << (x -    1) << (y +    1);
	v << (x + size) << (y - size);
	v << (x +    1) << (y -    1);
	break;
    case 4:
        v << (x - size) << (y - size);
	v << (x + size) << (y - size);
        v << (x - size) << (y + size);
	v << (x + size) << (y + size);
        v << (x - size) << (y - size);
	v << (x - size) << (y + size);
        v << (x + size) << (y - size);
	v << (x + size) << (y + size);
	break;
    case 5:
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
    const QRect& rect = w->geometry();//effects->clientArea(FullArea, effects->activeWindow());
    return QPointF(rect.x() + rect.width () / 2.0f,
		   rect.y() + rect.height() / 2.0f);
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
    /*if (enabled && position > 0) {
        currentPosition = getWindowCentre(w);
        createCrosshair(currentPosition, verts);
    }*/
    EffectsHandler::sendReloadMessage("crosshair");
}

void CrosshairEffect::slotWindowGeometryShapeChanged(KWin::EffectWindow* w, const QRect& old)
{
    if (enabled && position > 0) {
        currentPosition = getWindowCentre(effects->activeWindow());
        createCrosshair(currentPosition, verts);
    }
}

void CrosshairEffect::slotWindowFinishUserMovedResized(KWin::EffectWindow* w)
{
    if (enabled && position > 0) {
        currentPosition = getWindowCentre(effects->activeWindow());
        createCrosshair(currentPosition, verts);
    }
}

} // namespace

#include "crosshair.moc"
