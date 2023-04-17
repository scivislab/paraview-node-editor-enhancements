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
      
      QRect xLabelBoundingRect = fm.boundingRect(this->xLabel);
      painter.drawText(QPoint(0.5 * textHeight + width / 2 - xLabelBoundingRect.width() / 2, height - fm.descent()), this->xLabel);
      this->xLabelRect = QRect(0.5 * textHeight + width / 2 - xLabelBoundingRect.width() / 2, height - xLabelBoundingRect.height(), xLabelBoundingRect.width(), xLabelBoundingRect.height());

      QRect yLabelRect = fm.boundingRect(this->yLabel).transposed();
      painter.save();
      painter.translate(QPoint(width/2,height/2));
      painter.rotate(-90);
      painter.translate(QPoint(-height/2,-width/2));
      painter.drawText(QPoint(0.5 * textHeight + height / 2 - yLabelRect.height() / 2, fm.ascent()), this->yLabel);
      painter.restore();

      if (maxRunNumber > 0)
      {
        QRect yTickLabelRect = fm.boundingRect(QString::number(this->maxRunNumber));
        painter.drawText(heatMapRect.topLeft() - QPoint(yTickLabelRect.width() + 5, -fm.ascent()), QString::number(this->maxRunNumber));
        yTickLabelRect = fm.boundingRect(QString::number(1));
        painter.drawText(heatMapRect.bottomLeft() - QPoint(yTickLabelRect.width() + 5, fm.descent()), QString::number(1));
      }
    }



  private:
    bool hasImage = false;
    QImage image;
    QRect xLabelRect;
    QString xLabel = QString("ranks ordered by time");
    QString yLabel = QString("run #");
  };

  QHeatMap* heatmap = nullptr;
  vtkSmartPointer<vtkColorTransferFunction> ctf;
};

#endif // pqNodeEditorHeatMapWidget_h