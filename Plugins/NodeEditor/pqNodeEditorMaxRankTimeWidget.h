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

#ifndef pqNodeEditorMaxRankTimeWidget_h
#define pqNodeEditorMaxRankTimeWidget_h

#include <QLabel>
// #inlcude <QColor>

class pqNodeEditorMaxRankTimeWidget : public QLabel
{
  Q_OBJECT
  
public:
  pqNodeEditorMaxRankTimeWidget();
  ~pqNodeEditorMaxRankTimeWidget();

  void updateTime(double maxTime, double currentTime);

protected:
  void paintEvent(QPaintEvent* event);

private:
  QString xLabel = QString("max rank time");
  QColor color = QColor(32,159,223);
  double maxTime = 0.0;
  double currentTime = 0.0;

  static const int BARHEIGHT = 10;
  static const int SPACING = 2;
};

#endif // pqNodeEditorMaxRankTimeWidget_h