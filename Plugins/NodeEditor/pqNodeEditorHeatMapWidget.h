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

#ifndef pqNodeEditorHeatMapWidget_h
#define pqNodeEditorHeatMapWidget_h

#include "vtkType.h"
#include <vtkColorTransferFunction.h>
#include <vtkSmartPointer.h>
#include <QtWidgets>
#include <QLabel>
#include <QPainter>
#include <iostream>

#include <pqNodeEditorUtils.h>

class pqNodeEditorHeatMapWidget : public QWidget
{
  Q_OBJECT
  
public:
  pqNodeEditorHeatMapWidget();
  ~pqNodeEditorHeatMapWidget();

  void update(vtkTypeUInt32 gid);
  
private:
  class QHeatMap : public QLabel
  {
  public:
    void setImage(QImage i)
    {
      this->image = i;
      this->hasImage=true;
    }

  protected:
    void paintEvent(QPaintEvent* event) override
    {     
      if(!this->hasImage)
        return;

      QPainter painter(this);
      QFontMetrics fm = painter.fontMetrics();
      int textHeight = fm.ascent() + fm.descent();
      int leftMargin = textHeight + 1;
      int bottomMargin = textHeight + 1;
      int rightMargin = 1;
      int topMargin = 1;

      int width = this->width();
      int height = this->height();

      // QRect heatMapBoundsRect(leftMargin-1, 0, width-leftMargin-rightMargin+2, height-bottomMargin-topMargin+2);
      QRect heatMapRect(leftMargin, topMargin, width-leftMargin-rightMargin, height-bottomMargin-topMargin);

      painter.save();
      QPen pen = painter.pen();
      pen.setColor(pqNodeEditorUtils::CONSTS::COLOR_GRID);
      pen.setCapStyle(Qt::RoundCap);
      pen.setJoinStyle(Qt::RoundJoin);
      pen.setWidth(1);
      painter.setPen(pen);
      painter.drawRect(heatMapRect);
      painter.restore();

      // QRect heatMapRect(leftMargin+1, 1, width-leftMargin, height-bottomMargin);
      QImage todraw = this->image.scaled(
            heatMapRect.size(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);
      painter.drawImage(heatMapRect, todraw);
      
      QRect xLabelRect = fm.boundingRect(this->xLabel);
      painter.drawText(QPoint(0.5 * textHeight + width / 2 - xLabelRect.width() / 2, height - fm.descent()), this->xLabel);

      QRect yLabelRect = fm.boundingRect(this->yLabel).transposed();
      painter.save();
      painter.translate(QPoint(width/2,height/2));
      painter.rotate(-90);
      painter.translate(QPoint(-height/2,-width/2));
      painter.drawText(QPoint(0.5 * textHeight + height / 2 - yLabelRect.height() / 2, fm.ascent()), this->yLabel);
      painter.restore();
    }

  private:
    bool hasImage = false;
    QImage image;
    QString xLabel = QString("rank ordered by time");
    QString yLabel = QString("iteration");
  };

  QHeatMap* heatmap = nullptr;
  vtkSmartPointer<vtkColorTransferFunction> ctf;
};

#endif // pqNodeEditorHeatMapWidget_h