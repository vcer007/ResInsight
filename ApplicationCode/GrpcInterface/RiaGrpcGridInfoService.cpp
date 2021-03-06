/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2019-     Equinor ASA
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
//////////////////////////////////////////////////////////////////////////////////
#include "RiaGrpcGridInfoService.h"
#include "RiaGrpcCallbacks.h"

#include "RigActiveCellInfo.h"
#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"

#include "RimEclipseCase.h"
#include "RimGeoMechCase.h"

#include <string.h> // memcpy

using namespace rips;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaActiveCellInfoStateHandler::RiaActiveCellInfoStateHandler()
    : m_request(nullptr)
    , m_eclipseCase(nullptr)
    , m_activeCellInfo(nullptr)
    , m_currentCellIdx(0u)
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaActiveCellInfoStateHandler::init(const rips::CellInfoRequest* request)
{
    CAF_ASSERT(request);
    m_request = request;

    m_porosityModel  = RiaDefines::PorosityModelType(m_request->porosity_model());
    RimCase* rimCase = RiaGrpcServiceInterface::findCase(m_request->case_id());
    m_eclipseCase    = dynamic_cast<RimEclipseCase*>(rimCase);

    if (!m_eclipseCase)
    {
        return grpc::Status(grpc::NOT_FOUND, "Eclipse Case not found");
    }

    if (!m_eclipseCase->eclipseCaseData() || !m_eclipseCase->eclipseCaseData()->mainGrid())
    {
        return grpc::Status(grpc::NOT_FOUND, "Eclipse Case Data not found");
    }

    m_activeCellInfo = m_eclipseCase->eclipseCaseData()->activeCellInfo(m_porosityModel);

    if (!m_activeCellInfo)
    {
        return grpc::Status(grpc::NOT_FOUND, "Active Cell Info not found");
    }

    size_t globalCoarseningBoxCount = 0;

    for (size_t gridIdx = 0; gridIdx < m_eclipseCase->eclipseCaseData()->gridCount(); gridIdx++)
    {
        m_globalCoarseningBoxIndexStart.push_back(globalCoarseningBoxCount);

        RigGridBase* grid = m_eclipseCase->eclipseCaseData()->grid(gridIdx);

        size_t localCoarseningBoxCount = grid->coarseningBoxCount();
        globalCoarseningBoxCount += localCoarseningBoxCount;
    }

    return grpc::Status::OK;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaActiveCellInfoStateHandler::assignNextActiveCellInfoData(rips::CellInfo* cellInfo)
{
    const std::vector<RigCell>& reservoirCells = m_eclipseCase->eclipseCaseData()->mainGrid()->globalCellArray();

    while (m_currentCellIdx < reservoirCells.size())
    {
        size_t cellIdxToTry = m_currentCellIdx++;
        if (m_activeCellInfo->isActive(cellIdxToTry))
        {
            assignCellInfoData(cellInfo, reservoirCells, cellIdxToTry);
            return grpc::Status::OK;
        }
    }
    return Status(grpc::OUT_OF_RANGE, "We've reached the end. This is not an error but means transmission is finished");
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaActiveCellInfoStateHandler::assignCellInfoData(rips::CellInfo*       cellInfo,
                                                             const std::vector<RigCell>& reservoirCells,
                                                             size_t                      cellIdx)
{
    RigGridBase* grid = reservoirCells[cellIdx].hostGrid();
    CVF_ASSERT(grid != nullptr);
    size_t cellIndex = reservoirCells[cellIdx].gridLocalCellIndex();

    size_t i, j, k;
    grid->ijkFromCellIndex(cellIndex, &i, &j, &k);

    size_t       pi, pj, pk;
    RigGridBase* parentGrid = nullptr;

    if (grid->isMainGrid())
    {
        pi         = i;
        pj         = j;
        pk         = k;
        parentGrid = grid;
    }
    else
    {
        size_t parentCellIdx = reservoirCells[cellIdx].parentCellIndex();
        parentGrid           = (static_cast<RigLocalGrid*>(grid))->parentGrid();
        CVF_ASSERT(parentGrid != nullptr);
        parentGrid->ijkFromCellIndex(parentCellIdx, &pi, &pj, &pk);
    }

    cellInfo->set_grid_index((int)grid->gridIndex());
    cellInfo->set_parent_grid_index((int)parentGrid->gridIndex());

    size_t coarseningIdx = reservoirCells[cellIdx].coarseningBoxIndex();
    if (coarseningIdx != cvf::UNDEFINED_SIZE_T)
    {
        size_t globalCoarseningIdx = m_globalCoarseningBoxIndexStart[grid->gridIndex()] + coarseningIdx;
        cellInfo->set_coarsening_box_index((int)globalCoarseningIdx);
    }
    else
    {
        cellInfo->set_coarsening_box_index(-1);
    }
    {
        rips::Vec3i* local_ijk = new rips::Vec3i;
        local_ijk->set_i((int)i);
        local_ijk->set_j((int)j);
        local_ijk->set_k((int)k);
        cellInfo->set_allocated_local_ijk(local_ijk);
    }
    {
        rips::Vec3i* parent_ijk = new rips::Vec3i;
        parent_ijk->set_i((int)pi);
        parent_ijk->set_j((int)pj);
        parent_ijk->set_k((int)pk);
        cellInfo->set_allocated_parent_ijk(parent_ijk);
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigActiveCellInfo* RiaActiveCellInfoStateHandler::activeCellInfo() const
{
    return m_activeCellInfo;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<RigCell>& RiaActiveCellInfoStateHandler::reservoirCells() const
{
    const std::vector<RigCell>& reservoirCells = m_eclipseCase->eclipseCaseData()->mainGrid()->globalCellArray();
    return reservoirCells;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaActiveCellInfoStateHandler::assignReply(rips::CellInfoArray* reply)
{
    const size_t packageSize = RiaGrpcServiceInterface::numberOfMessagesForByteCount(sizeof(rips::CellInfoArray));
    size_t       packageIndex = 0u;
    reply->mutable_data()->Reserve((int)packageSize);
    for (; packageIndex < packageSize && m_currentCellIdx < m_activeCellInfo->reservoirCellCount(); ++packageIndex)
    {
        rips::CellInfo  singleCellInfo;
        grpc::Status          singleCellInfoStatus = assignNextActiveCellInfoData(&singleCellInfo);
        if (singleCellInfoStatus.ok())
        {
            rips::CellInfo* allocCellInfo = reply->add_data();
            *allocCellInfo = singleCellInfo;
        }
        else
        {
            break;
        }
    }
    if (packageIndex > 0u)
    {
        return Status::OK;
    }
    return Status(grpc::OUT_OF_RANGE, "We've reached the end. This is not an error but means transmission is finished");
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaGrpcGridInfoService::GetGridCount(grpc::ServerContext* context, const rips::Case* request, rips::GridCount* reply)
{
    RimCase* rimCase = findCase(request->id());

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(rimCase);
    size_t          gridCount   = 0u;
    if (eclipseCase)
    {
        gridCount = eclipseCase->mainGrid()->gridCount();
        reply->set_count((int)gridCount);
        return Status::OK;
    }
    return grpc::Status(grpc::NOT_FOUND, "Eclipse Case not found");
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaGrpcGridInfoService::GetGridDimensions(grpc::ServerContext*     context,
                                                          const rips::Case*        request,
                                                          rips::GridDimensions* reply)
{
    RimCase* rimCase = findCase(request->id());

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(rimCase);
    if (eclipseCase)
    {
        size_t gridCount = eclipseCase->mainGrid()->gridCount();
        for (size_t i = 0; i < gridCount; ++i)
        {
            const RigGridBase* grid       = eclipseCase->mainGrid()->gridByIndex(i);
            rips::Vec3i*       dimensions = reply->add_dimensions();
            dimensions->set_i((int)grid->cellCountI());
            dimensions->set_j((int)grid->cellCountJ());
            dimensions->set_k((int)grid->cellCountK());
        }
        return grpc::Status::OK;
    }

    return grpc::Status(grpc::NOT_FOUND, "Eclipse Case not found");
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaGrpcGridInfoService::GetCellCount(grpc::ServerContext* context, const rips::CellInfoRequest* request, rips::CellCount* reply)
{
    RimCase* rimCase = findCase(request->case_id());

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(rimCase);
    if (eclipseCase)
    {
        auto porosityModel = RiaDefines::PorosityModelType(request->porosity_model());
        RigActiveCellInfo* activeCellInfo = eclipseCase->eclipseCaseData()->activeCellInfo(porosityModel);
        reply->set_active_cell_count((int) activeCellInfo->reservoirActiveCellCount());
        reply->set_reservoir_cell_count((int) activeCellInfo->reservoirCellCount());
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::NOT_FOUND, "Eclipse Case not found");
}


//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status
RiaGrpcGridInfoService::GetTimeSteps(grpc::ServerContext* context, const rips::Case* request, rips::TimeStepDates* reply)
{
    RimCase* rimCase = findCase(request->id());

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(rimCase);
    if (eclipseCase)
    {
        std::vector<QDateTime> timeStepDates = eclipseCase->timeStepDates();
        for (QDateTime dateTime : timeStepDates)
        {
            rips::TimeStepDate* date = reply->add_date();
            date->set_year(dateTime.date().year());
            date->set_month(dateTime.date().month());
            date->set_day(dateTime.date().day());
            date->set_hour(dateTime.time().hour());
            date->set_minute(dateTime.time().minute());
            date->set_second(dateTime.time().second());
        }
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::NOT_FOUND, "Eclipse Case not found");
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaGrpcGridInfoService::GetCellInfoForActiveCells(grpc::ServerContext*                      context,
                                                                const rips::CellInfoRequest*        request,
                                                                rips::CellInfoArray*                reply,
                                                                RiaActiveCellInfoStateHandler* stateHandler)
{
    return stateHandler->assignReply(reply);
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiaAbstractGrpcCallback*> RiaGrpcGridInfoService::createCallbacks()
{
    typedef RiaGrpcGridInfoService Self;

    return {new RiaGrpcCallback<Self, Case, GridCount>(this, &Self::GetGridCount, &Self::RequestGetGridCount),
            new RiaGrpcCallback<Self, Case, GridDimensions>(this, &Self::GetGridDimensions, &Self::RequestGetGridDimensions),
            new RiaGrpcCallback<Self, CellInfoRequest, CellCount>(this, &Self::GetCellCount, &Self::RequestGetCellCount),
            new RiaGrpcCallback<Self, Case, TimeStepDates>(this, &Self::GetTimeSteps, &Self::RequestGetTimeSteps),
            new RiaGrpcStreamCallback<Self, CellInfoRequest, CellInfoArray, RiaActiveCellInfoStateHandler>(
                this, &Self::GetCellInfoForActiveCells, &Self::RequestGetCellInfoForActiveCells, new RiaActiveCellInfoStateHandler)};
}

static bool RiaGrpcGridInfoService_init =
    RiaGrpcServiceFactory::instance()->registerCreator<RiaGrpcGridInfoService>(typeid(RiaGrpcGridInfoService).hash_code());
