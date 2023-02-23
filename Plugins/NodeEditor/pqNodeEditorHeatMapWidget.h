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
#include <QtWidgets>
#include <QLabel>
#include <QPainter>
#include <iostream>

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
      int leftMargin = textHeight;
      int bottomMargin = textHeight;

      int width = this->width();
      int height = this->height();

      QRect heatMapRect(leftMargin, 0, width-leftMargin, height-bottomMargin);
      painter.drawImage(heatMapRect, this->image);
      
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
    QString yLabel = QString("iter.");
  };

  QHeatMap* heatmap = nullptr;
  void sortMPIRanksByTime(std::vector<double> timings);
};

#endif // pqNodeEditorHeatMapWidget_h