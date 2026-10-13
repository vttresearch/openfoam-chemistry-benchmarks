/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2025 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    chemFoam

Description
    Solver for chemistry problems, designed for use on single cell cases to
    provide comparison against other chemistry solvers, that uses a single cell
    mesh, and fields created from the initial conditions.

\*---------------------------------------------------------------------------*/
#ifdef FOAM_PERFETTO
#include <perfetto.h>
#include "chemFoam_tracing_categories.H"
#else
#define TRACE_EVENT_BEGIN(cat, name)
#define TRACE_EVENT_END(cat)
#define TRACE_COUNTER(cat, name, value)
#endif

#include <fstream>

#include "argList.H"
#include "timeSelector.H"
#include "zeroDimensionalFvMesh.H"
#include "chemistryModel.H"
#include "physicoChemicalConstants.H"
#include "OFstream.H"
#include "thermoTypeFunctions.H"

#include "fvcFlux.H"

#include "fvmDdt.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::noParallel();

    #define CREATE_MESH createZeroDimensionalFvMesh.H
    #define NO_CONTROL
    #include "postProcess.H"

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createZeroDimensionalFvMesh.H"
    #include "createFields.H"
    #include "createFieldRefs.H"
    #include "createControls.H"

    // near start of main(), before doing real work
#ifdef FOAM_PERFETTO
    {
        perfetto::TracingInitArgs args;
        args.backends |= perfetto::kInProcessBackend;  // record in-process
        perfetto::Tracing::Initialize(args);
        perfetto::TrackEvent::Register();              // enable track events
    }

    // Start/stop a trace session programmatically.
    std::unique_ptr<perfetto::TracingSession> tracingSession;
    perfetto::TraceConfig cfg;
    // 16 MB ring buffer; adjust as needed
    cfg.add_buffers()->set_size_kb(16 * 1024);
    // Enable the track event data source
    auto* ds = cfg.add_data_sources()->mutable_config();
    ds->set_name("track_event");  // mandatory for track events

    tracingSession = perfetto::Tracing::NewTrace();
    tracingSession->Setup(cfg);
    tracingSession->StartBlocking();  // start recording now
#endif

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.run())
    {
        TRACE_EVENT_BEGIN("chemFoam", "time controls");
        #include "readControls.H"
        #include "setDeltaT.H"

        runTime++;
        TRACE_EVENT_END("chemFoam");
        
        TRACE_COUNTER("chemFoam", "time", runTime.value());

        Info<< "Time = " << runTime.userTimeName() << nl << endl;

        TRACE_EVENT_BEGIN("chemFoam", "solveChemistry.H");
        #include "solveChemistry.H"
        TRACE_EVENT_END("chemFoam");

        TRACE_EVENT_BEGIN("chemFoam", "YEqn.H");
        #include "YEqn.H"
        TRACE_EVENT_END("chemFoam");

        TRACE_EVENT_BEGIN("chemFoam", "hEqn.H and pEqn.H");
        #include "hEqn.H"
        #include "pEqn.H"
        TRACE_EVENT_END("chemFoam");

        TRACE_EVENT_BEGIN("chemFoam", "write output");
        #include "output.H"
        TRACE_EVENT_END("chemFoam");

        TRACE_EVENT_BEGIN("chemFoam", "log timing");
        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
        TRACE_EVENT_END("chemFoam");
    }

    Info << "Number of steps = " << runTime.timeIndex() << endl;
    Info << "End" << nl << endl;

    #ifdef FOAM_PERFETTO
    tracingSession->StopBlocking();
    std::vector<char> traceData = tracingSession->ReadTraceBlocking();
    // Write to a file (name can be per-case or timestamped)
    std::ofstream of("chemFoam.perfetto-trace", std::ios::binary);
    of.write(traceData.data(), static_cast<std::streamsize>(traceData.size()));
    #endif

    return 0;
}


// ************************************************************************* //
