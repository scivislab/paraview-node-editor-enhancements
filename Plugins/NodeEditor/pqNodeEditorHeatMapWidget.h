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
#include <QHelpEvent>
#include <QMouseEvent>
#include <iostream>

#include <pqNodeEditorUtils.h>

class pqNodeEditorHeatMapWidget : public QWidget
{
  Q_OBJECT
  
public:
  pqNodeEditorHeatMapWidget(vtkTypeUInt32 gid);
  ~pqNodeEditorHeatMapWidget();

  void updateHeatMap();
  QImage getCTFImage();

  vtkTypeUInt32 gid;
  
private:
  class QHeatMap : public QLabel
  {
  public:
    bool sortedByTime = true;
    int maxRunNumber = 0;

    QHeatMap()
    {
      setMouseTracking(true);
    }

    // bool event(QEvent *event)
    // {
    //   if (event->type() == QEvent::ToolTip) {
    //     QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
    //     if (true)
    //     {
    //         QToolTip::showText(helpEvent->globalPos(), "test");
    //     } else {
    //         QToolTip::hideText();
    //         event->ignore();
    //     }

    //     return true;
    // }
    // return QWidget::event(event);
    // }

    void setImage(QImage i)
    {
      this->image = i;
      this->hasImage=true;
    }

    void setRange(double min, double max)
    {
      this->localMinTime = min;
      this->localMaxTime = max;
    }

  protected:
    void mousePressEvent(QMouseEvent* ev)
    {
      const QPoint p = ev->pos();
      if (this->xLabelRect.contains(p,false))
      {
        this->sortedByTime = !this->sortedByTime;
        if (sortedByTime)
          xLabel = QString("ranks ordered by time");
        else
          xLabel = QString("ranks ordered by rank id");

        reinterpret_cast<pqNodeEditorHeatMapWidget*>(this->parent())->updateHeatMap();
        this->update();
      }
    }

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
      int spacing = 5;

      int width = this->width();
      int height = this->height();

      // legend
      char format = 'f';
      int prec = 2;
      QRect legendMaxLabelRect = fm.boundingRect(QString::number(localMaxTime,format,prec) + QString('s'));
      QRect legendMinLabelRect = fm.boundingRect(QString::number(localMinTime,format,prec) + QString('s'));
      int legendLabelHeight = legendMinLabelRect.height() + legendMaxLabelRect.height();
      int legendLabelWidth = std::max(legendMinLabelRect.width(),legendMaxLabelRect.width());
      QRect legendRect(width-rightMargin-legendLabelWidth, topMargin + legendLabelHeight/2, legendLabelWidth , height-bottomMargin-topMargin-legendLabelHeight);

      QImage legendImage = reinterpret_cast<pqNodeEditorHeatMapWidget*>(this->parent())->getCTFImage().scaled(
            legendRect.size(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);
      painter.drawImage(legendRect, legendImage);

      painter.drawText(QPoint(legendRect.right() - legendMaxLabelRect.width(), legendRect.top() - fm.descent()), QString::number(localMaxTime,format,prec) + QString('s'));
      painter.drawText(QPoint(legendRect.right() - legendMinLabelRect.width(), legendRect.bottom() + fm.ascent()), QString::number(localMinTime,format,prec) + QString('s'));

      // heat map 
      QRect heatMapRect(leftMargin, topMargin, width-leftMargin-rightMargin-spacing-legendLabelWidth, height-bottomMargin-topMargin);
      painter.save();
      QPen pen = painter.pen();
      pen.setColor(pqNodeEditorUtils::CONSTS::COLOR_GRID);
      pen.setCapStyle(Qt::RoundCap);
      pen.setJoinStyle(Qt::RoundJoin);
      pen.setWidth(1);
      painter.setPen(pen);
      painter.drawRect(heatMapRect);
      painter.restore();

      QImage heatMapDraw = this->image.scaled(
            heatMapRect.size(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation);
      painter.drawImage(heatMapRect, heatMapDraw);
      
      // x label
      QRect xLabelBoundingRect = fm.boundingRect(this->xLabel);
      painter.drawText(QPoint(0.5 * textHeight + heatMapRect.width() / 2 - xLabelBoundingRect.width() / 2, height - fm.descent()), this->xLabel);
      this->xLabelRect = QRect(0.5 * textHeight + heatMapRect.width() / 2 - xLabelBoundingRect.width() / 2, height - xLabelBoundingRect.height(), xLabelBoundingRect.width(), xLabelBoundingRect.height());

      // y label
      QRect yLabelRect = fm.boundingRect(this->yLabel).transposed();
      painter.save();
      painter.translate(QPoint(width/2,height/2));
      painter.rotate(-90);
      painter.translate(QPoint(-height/2,-width/2));
      painter.drawText(QPoint(0.5 * textHeight + height / 2 - yLabelRect.height() / 2, fm.ascent()), this->yLabel);
      painter.restore();

      // y ticks
      if (maxRunNumber > 0)
      {
        QRect yTickLabelRect = fm.boundingRect(QString::number(this->maxRunNumber));
        painter.drawText(heatMapRect.topLeft() - QPoint(yTickLabelRect.width() + 5, -fm.ascent()), QString::number(1));
        yTickLabelRect = fm.boundingRect(QString::number(1));
        painter.drawText(heatMapRect.bottomLeft() - QPoint(yTickLabelRect.width() + 5, fm.descent()), QString::number(this->maxRunNumber));
      }
    }



  private:
    bool hasImage = false;
    QImage image;
    QRect xLabelRect;
    QString xLabel = QString("ranks ordered by time");
    QString yLabel = QString("run #");
    double localMinTime = 0.0;
    double localMaxTime = 0.0;
  };

  QHeatMap* heatmap = nullptr;
  vtkSmartPointer<vtkColorTransferFunction> ctf;
};

#endif // pqNodeEditorHeatMapWidget_h