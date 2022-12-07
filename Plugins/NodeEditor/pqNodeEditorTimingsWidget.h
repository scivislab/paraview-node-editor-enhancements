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

#ifndef pqNodeEditorTimingsWidget_h
#define pqNodeEditorTimingsWidget_h

#include "vtkType.h"
#include <QtWidgets>
#include <QtCharts/QChart>
#include <QtCharts/QBoxSet>

QT_CHARTS_USE_NAMESPACE

class pqNodeEditorTimingsWidget : public QWidget
{
  Q_OBJECT
  
public:
  pqNodeEditorTimingsWidget(QWidget *parent = 0, vtkTypeUInt32 g_id = 0);
  ~pqNodeEditorTimingsWidget();

  void updateTimings();
  void updateTimingsBarChart();
  void updateTimingsBoxPlot();
  void updateTimingsLinePlot();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  
private:
  int mode = 0;
  QBoxSet* createBoxSetFromVector(std::vector<double> timings);
  vtkTypeUInt32 global_id;
  QChart* timingsChart = nullptr;
};

#endif // pqNodeEditorTimingsWidget_h