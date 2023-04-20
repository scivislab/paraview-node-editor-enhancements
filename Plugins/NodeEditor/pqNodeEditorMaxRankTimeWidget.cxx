/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#include <pqNodeEditorMaxRankTimeWidget.h>

#include <QtCharts/QBarSet>
#include <QPainter>
// #include <QPalette>

QT_CHARTS_USE_NAMESPACE

pqNodeEditorMaxRankTimeWidget::pqNodeEditorMaxRankTimeWidget()
{
    QPainter painter(this);
    QFontMetrics fm = painter.fontMetrics();
    int textHeight = fm.ascent() + fm.descent();

    int minimumHeight = textHeight + 2;
    minimumHeight += pqNodeEditorMaxRankTimeWidget::BARHEIGHT;
    minimumHeight += 2*pqNodeEditorMaxRankTimeWidget::SPACING;

    this->setMinimumHeight(minimumHeight);
}

pqNodeEditorMaxRankTimeWidget::~pqNodeEditorMaxRankTimeWidget(){}

void pqNodeEditorMaxRankTimeWidget::updateTime(double maxTime, double currentTime)
{
  this->maxTime = maxTime;
  this->currentTime = currentTime;
}

void pqNodeEditorMaxRankTimeWidget::paintEvent(QPaintEvent* event)
{     
  QPainter painter(this);
  QFontMetrics fm = painter.fontMetrics();
  int textHeight = fm.ascent() + fm.descent();

  int width = this->width();
  int height = this->height();

  int currentHOffset = 0;
  double wf = this->currentTime / this->maxTime;
  painter.fillRect(0,0, wf * width, pqNodeEditorMaxRankTimeWidget::BARHEIGHT, this->color);

  currentHOffset += pqNodeEditorMaxRankTimeWidget::BARHEIGHT + pqNodeEditorMaxRankTimeWidget::SPACING;
  painter.drawLine(0, currentHOffset, width, currentHOffset);
  
  currentHOffset += pqNodeEditorMaxRankTimeWidget::SPACING + fm.ascent();
  painter.drawText(0,currentHOffset,QString::number(0.0,'f',2) + QString('s'));
  QString maxTime_s = QString::number(this->maxTime,'f',2) + QString('s');
  QRect ub = fm.boundingRect(maxTime_s);
  painter.drawText(width - ub.width() - 3, currentHOffset, maxTime_s);

  QRect xl = fm.boundingRect(this->xLabel);
  painter.drawText(width/2 - xl.width()/2, currentHOffset, this->xLabel);
}