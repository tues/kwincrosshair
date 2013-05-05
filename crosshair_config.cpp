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

#include "crosshair_config.h"

#include <kwineffects.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfiggroup.h>
#include <KActionCollection>
#include <kaction.h>
#include <KShortcutsEditor>

#include <QWidget>

namespace KWin
{

KWIN_EFFECT_CONFIG(crosshair, CrosshairEffectConfig)

CrosshairEffectConfigForm::CrosshairEffectConfigForm(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

CrosshairEffectConfig::CrosshairEffectConfig(QWidget* parent, const QVariantList& args) :
    KCModule(EffectFactory::componentData(), parent, args)
{
    m_ui = new CrosshairEffectConfigForm(this);

    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(m_ui);

    connect(m_ui->editor, SIGNAL(keyChange()), this, SLOT(changed()));
    connect(m_ui->spinSize, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->spinWidth, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->comboColors, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));
    connect(m_ui->spinAlpha, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(m_ui->blendComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));
    connect(m_ui->shapeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));
    connect(m_ui->positionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));

    /*connect(m_ui->spinSize, SIGNAL(valueChanged(int)), this, SLOT(myChanged()));
    connect(m_ui->spinWidth, SIGNAL(valueChanged(int)), this, SLOT(myChanged()));
    connect(m_ui->comboColors, SIGNAL(currentIndexChanged(int)), this, SLOT(myChanged()));
    connect(m_ui->spinAlpha, SIGNAL(valueChanged(int)), this, SLOT(myChanged()));
    connect(m_ui->blendComboBox, SIGNAL(currentIndexChanged(bool)), this, SLOT(myChanged()));
    connect(m_ui->shapeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(myChanged()));*/

    // Shortcut config. The shortcut belongs to the component "kwin"!
    m_actionCollection = new KActionCollection(this, KComponentData("kwin"));

    KAction* a = static_cast<KAction*>(m_actionCollection->addAction("ToggleCrosshair"));
    a->setText(i18n("Toggle Crosshair"));
    a->setProperty("isConfigurationAction", true);
    a->setGlobalShortcut(KShortcut(Qt::SHIFT + Qt::META + Qt::Key_F11));

    m_ui->editor->addCollection(m_actionCollection);

    load();
}

CrosshairEffectConfig::~CrosshairEffectConfig()
{
    // Undo (only) unsaved changes to global key shortcuts
    m_ui->editor->undoChanges();
}

void CrosshairEffectConfig::load()
{
    KCModule::load();

    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");

    int size  = conf.readEntry("Size", 2);
    int width = conf.readEntry("LineWidth", 1);
    QColor color = conf.readEntry("Color", QColor(Qt::red));
    int alpha = conf.readEntry("Alpha", 50);
    int shape = conf.readEntry("Shape", 0);
    int blend = conf.readEntry("Blend", 1);
    int position = conf.readEntry("Position", 0);
    m_ui->spinSize->setValue(size);
    m_ui->spinSize->setSuffix(ki18np(" pixel", " pixels"));
    m_ui->spinWidth->setValue(width);
    m_ui->spinWidth->setSuffix(ki18np(" pixel", " pixels"));
    m_ui->spinAlpha->setValue(alpha);
    m_ui->spinAlpha->setSuffix(ki18np("%", "%"));
    m_ui->comboColors->setColor(color);
    m_ui->shapeComboBox->setCurrentIndex(shape);
    m_ui->blendComboBox->setCurrentIndex(blend);
    m_ui->positionComboBox->setCurrentIndex(position);

    emit changed(false);
}

void CrosshairEffectConfig::save()
{
    kDebug(1212) << "Saving config of Crosshair" ;
    //KCModule::save();

    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");

    conf.writeEntry("Size", m_ui->spinSize->value());
    conf.writeEntry("LineWidth", m_ui->spinWidth->value());
    conf.writeEntry("Color", m_ui->comboColors->color());
    conf.writeEntry("Alpha", m_ui->spinAlpha->value());
    conf.writeEntry("Shape", m_ui->shapeComboBox->currentIndex());
    conf.writeEntry("Blend", m_ui->blendComboBox->currentIndex());
    conf.writeEntry("Position", m_ui->positionComboBox->currentIndex());

    m_actionCollection->writeSettings();
    m_ui->editor->save();   // undo() will restore to this state from now on

    conf.sync();

    emit changed(false);
    EffectsHandler::sendReloadMessage("crosshair");
}

void CrosshairEffectConfig::defaults()
{
    m_ui->spinSize->setValue(2);
    m_ui->spinWidth->setValue(1);
    m_ui->comboColors->setColor(Qt::red);
    m_ui->spinAlpha->setValue(50);
    m_ui->shapeComboBox->setCurrentIndex(0);
    m_ui->blendComboBox->setCurrentIndex(1);
    m_ui->positionComboBox->setCurrentIndex(0);
    emit changed(true);
}

void CrosshairEffectConfig::myChanged()
{
    m_ui->spinAlpha->setEnabled(m_ui->blendComboBox->currentIndex() > 0);
    /*
    KConfigGroup conf = EffectsHandler::effectConfig("Crosshair");

    conf.writeEntry("Size", m_ui->spinSize->value());
    conf.writeEntry("LineWidth", m_ui->spinWidth->value());
    conf.writeEntry("Color", m_ui->comboColors->color());
    conf.writeEntry("Alpha", m_ui->spinAlpha->value());
    conf.writeEntry("Shape", m_ui->shapeComboBox->currentIndex());
    conf.writeEntry("Blend", m_ui->blendComboBox->currentIndex());

    conf.sync();
    EffectsHandler::sendReloadMessage("crosshair");*/
}

} // namespace

#include "crosshair_config.moc"
