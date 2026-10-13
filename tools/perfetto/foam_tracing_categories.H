// foam_tracing_categories.H
// Shared Perfetto category definitions for this repository.
//
// NOTE: PERFETTO_TRACK_EVENT_STATIC_STORAGE() must be placed in exactly one
// translation unit (src/ODE/perfetto_sdk.C). Do not add it here.

#ifndef FOAM_TRACING_CATEGORIES_H
#define FOAM_TRACING_CATEGORIES_H

#include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("chemFoam").SetDescription("chemFoam internals"),
    perfetto::Category("ode").SetDescription("ODE solver internals")
);

#endif // FOAM_TRACING_CATEGORIES_H
