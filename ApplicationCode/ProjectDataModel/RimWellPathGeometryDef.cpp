/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018 -    equinor ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////
#include "RimWellPathGeometryDef.h"

#include "cafCmdFeatureMenuBuilder.h"
#include "cafPdmUiTableViewEditor.h"
#include "cafPdmUiTreeOrdering.h"

#include "RigWellPath.h"
#include "RiaPolyArcLineSampler.h"
#include "RimWellPathTarget.h"
#include "RimModeledWellPath.h"

namespace caf
{
template<>
void caf::AppEnum< RimWellPathGeometryDef::WellStartType >::setUp()
{
    addItem(RimWellPathGeometryDef::START_AT_FIRST_TARGET, "START_AT_FIRST_TARGET",   "Start at First Target");
    addItem(RimWellPathGeometryDef::START_AT_SURFACE,      "START_AT_SURFACE",         "Start at Surface");
    addItem(RimWellPathGeometryDef::START_FROM_OTHER_WELL, "START_FROM_OTHER_WELL",   "Branch");
    addItem(RimWellPathGeometryDef::START_AT_AUTO_SURFACE, "START_AT_AUTO_SURFACE",   "Auto Surface");

    setDefault(RimWellPathGeometryDef::START_AT_FIRST_TARGET);
}
}

CAF_PDM_SOURCE_INIT(RimWellPathGeometryDef, "WellPathGeometryDef");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPathGeometryDef::RimWellPathGeometryDef()
{
    CAF_PDM_InitObject("Trajectory", ":/Well.png", "", "");

    CAF_PDM_InitFieldNoDefault(&m_wellStartType, "WellStartType", "Start Type", "", "", "");
    CAF_PDM_InitField(&m_referencePoint, "ReferencePos", cvf::Vec3d(0,0,0), "UTM Reference Point", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_parentWell, "ParentWell", "Parent Well", "", "", "");
    CAF_PDM_InitField(&m_kickoffDepthOrMD, "KickoffDepthOrMD", 100.0, "Kickoff Depth", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_wellTargets, "WellPathTargets", "Well Targets", "", "", "");
    m_wellTargets.uiCapability()->setUiEditorTypeName(caf::PdmUiTableViewEditor::uiEditorTypeName());
    //m_wellTargets.uiCapability()->setUiTreeHidden(true);
    m_wellTargets.uiCapability()->setUiTreeChildrenHidden(true);
    m_wellTargets.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::TOP);
    m_wellTargets.uiCapability()->setCustomContextMenuEnabled(true);


    m_wellTargets.push_back(new RimWellPathTarget());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPathGeometryDef::~RimWellPathGeometryDef()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::ref<RigWellPath> RimWellPathGeometryDef::createWellPathGeometry()
{
    cvf::ref<RigWellPath> wellPathGeometry = new RigWellPath; 
    
    if (m_wellTargets.size() < 2) return wellPathGeometry;

    RiaPolyArcLineSampler arcLineSampler(lineArcEndpoints());

    arcLineSampler.sampledPointsAndMDs(30,
                                         false,
                                         &(wellPathGeometry->m_wellPathPoints),
                                         &(wellPathGeometry->m_measuredDepths));

    return wellPathGeometry;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::updateWellPathVisualization()
{
    RimModeledWellPath* modWellPath;
    this->firstAncestorOrThisOfTypeAsserted(modWellPath);
    modWellPath->updateWellPathVisualization();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::insertTarget(RimWellPathTarget* targetToInsertBefore, RimWellPathTarget* targetToInsert)
{
   size_t index = m_wellTargets.index(targetToInsertBefore);
   if (index < m_wellTargets.size()) m_wellTargets.insert(index, targetToInsert);
   else m_wellTargets.push_back(targetToInsert);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::deleteTarget(RimWellPathTarget* targetTodelete)
{
    m_wellTargets.removeChildObject(targetTodelete);
    delete targetTodelete;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::fieldChangedByUi(const caf::PdmFieldHandle* changedField, 
                                              const QVariant& oldValue, 
                                              const QVariant& newValue)
{
    if (&m_referencePoint == changedField)
    {
        std::cout << "fieldChanged" << std::endl;
    }

    updateWellPathVisualization();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    uiOrdering.add(&m_wellStartType);
    if (m_wellStartType == START_FROM_OTHER_WELL)
    {
        uiOrdering.add(&m_parentWell);
        m_kickoffDepthOrMD.uiCapability()->setUiName("Measured Depth");
        uiOrdering.add(&m_kickoffDepthOrMD);
    }

    if (m_wellStartType == START_AT_SURFACE)
    {
        m_kickoffDepthOrMD.uiCapability()->setUiName("Kick-Off Depth");
        uiOrdering.add(&m_kickoffDepthOrMD);
    }

    uiOrdering.add(&m_referencePoint);
    uiOrdering.add(&m_wellTargets);
    uiOrdering.skipRemainingFields(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName)
{
    uiTreeOrdering.skipRemainingChildren(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3d> RimWellPathGeometryDef::lineArcEndpoints()
{
    std::vector<cvf::Vec3d> endPoints;
    for (RimWellPathTarget* target: m_wellTargets)
    {
        endPoints.push_back( target->targetPointXYZ() + m_referencePoint() );
    }

    return endPoints;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellPathGeometryDef::defineCustomContextMenu(const caf::PdmFieldHandle* fieldNeedingMenu, 
                                                     QMenu* menu, 
                                                     QWidget* fieldEditorWidget)
{
    caf::CmdFeatureMenuBuilder menuBuilder;
    
    menuBuilder << "RicNewWellPathListTargetFeature";
    menuBuilder << "Separator";
    menuBuilder << "RicDeleteWellPathTargetFeature";

    menuBuilder.appendToMenu(menu);
}