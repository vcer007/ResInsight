/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018     Statoil ASA
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

#pragma once

#include "cvfBase.h"
#include "cvfCollection.h"
#include "VdeExportPart.h"

class QString;
class RimGridView;

namespace cvf
{
class Part;
} // namespace cvf

//==================================================================================================
///
//==================================================================================================
class RicHoloLensExportImpl
{
public:
    static void partsForExport(const RimGridView* view, cvf::Collection<cvf::Part>* partCollection);

    static std::vector<VdeExportPart> partsForExport(const RimGridView* view);

    static QString nameFromPart(const cvf::Part* part);

    static bool isGrid(const cvf::Part* part);
    static bool isPipe(const cvf::Part* part);
};