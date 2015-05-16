/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2007 Christian Nitschkowski <christian.nitschkowski@kdemail.net>

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

#ifndef KWIN_CROSSHAIR_CONFIG_H
#define KWIN_CROSSHAIR_CONFIG_H

#include <kcmodule.h>

#include "ui_crosshair_config.h"

class KActionCollection;

namespace KWin
{

class CrosshairEffectConfigForm : public QWidget, public Ui::CrosshairEffectConfigForm
{
    Q_OBJECT

public:

    explicit CrosshairEffectConfigForm(QWidget* parent);
};

class CrosshairEffectConfig : public KCModule
{
    Q_OBJECT

public:

    explicit CrosshairEffectConfig(QWidget* parent = 0, const QVariantList& args = QVariantList());
    virtual ~CrosshairEffectConfig();

    virtual void save();
    virtual void load();
    virtual void defaults();

private slots:

    void blendChanged(int index);
    void shapeChanged(int index);

private:

    CrosshairEffectConfigForm* m_ui;
    KActionCollection* m_actionCollection;
};

} // namespace

#endif
