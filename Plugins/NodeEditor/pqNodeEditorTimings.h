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

#ifndef pqNodeEditorTimings_h
#define pqNodeEditorTimings_h

#include "vtkType.h"
#include "vtkSmartPointer.h"
#include "vtkPVTimerInformation.h"
#include "vtkSMProxy.h"
#include <vector>
#include <map>
#include <set>

class pqNodeEditorTimings
{
public:
  static void refreshTimingLogs();

  static double getLatestLocalTimings(vtkTypeUInt32 global_Id);
  static std::vector<double> getLatestServerTimings(vtkTypeUInt32 global_Id);
  static std::vector<double> getLatestDataServerTimings(vtkTypeUInt32 global_Id);
  static std::vector<double> getLocalTimings(vtkTypeUInt32 global_Id);
  static std::vector<std::vector<double>> getServerTimings(vtkTypeUInt32 global_Id);
  static std::vector<std::vector<double>> getDataServerTimings(vtkTypeUInt32 global_Id);
  static double getMaxTime();

  static void addGlobalId(vtkTypeUInt32 global_Id);
  static void removeGlobalId(vtkTypeUInt32 global_Id);

private:
  static void addClientTimerInformation(vtkSmartPointer<vtkPVTimerInformation> timerInfo);
  static void addServerTimerInformation(vtkSmartPointer<vtkPVTimerInformation> timerInfo, bool isDataServer);
  static void updateMax();

  static std::map<vtkTypeUInt32, std::vector<double>> localTimings;
  static std::map<vtkTypeUInt32, std::vector<std::vector<double>>> serverTimings;
  static std::map<vtkTypeUInt32, std::vector<std::vector<double>>> dataServerTimings;
  static std::set<vtkTypeUInt32> globalIds;
  static double max;

  static std::vector<vtkSmartPointer<vtkSMProxy>> LogRecorderProxies;
};

#endif // pqNodeEditorTimings_h