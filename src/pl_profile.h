/*
   pl_profile, v0.1 (WIP)
   Do this:
        #define PL_PROFILE_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define PL_PROFILE_IMPLEMENTATION
   #include "pl_profile.h"
*/

/*
Index of this file:
// [SECTION] includes
// [SECTION] defines
// [SECTION] forward declarations & basic types
// [SECTION] public api
// [SECTION] enums
// [SECTION] structs
// [SECTION] internal api
// [SECTION] implementation
*/

#ifndef PL_PROFILE_H
#define PL_PROFILE_H

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#ifndef PL_DECLARE_STRUCT
#define PL_DECLARE_STRUCT(name) typedef struct name ##_t name
#endif

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

// forward declarations
PL_DECLARE_STRUCT(plProfileSample);
PL_DECLARE_STRUCT(plProfileFrame);
PL_DECLARE_STRUCT(plProfileContext);

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct plProfileSample_t
{
    double      dStartTime;
    double      dDuration;
    const char* cPName;
    uint32_t    uDepth;
} plProfileSample;

typedef struct plProfileFrame_t
{
    uint32_t*        sbSampleStack;
    plProfileSample* sbSamples;
    uint64_t         ulFrame;
    double           dStartTime;
    double           dDuration;
    double           dInternalDuration;
} plProfileFrame;

typedef struct plProfileContext_t
{
    double          dStartTime;
    plProfileFrame* sbFrames;
    plProfileFrame  tPFrames[2];
    plProfileFrame* tPCurrentFrame;
    plProfileFrame* tPLastFrame;
} plProfileContext;

//-----------------------------------------------------------------------------
// [SECTION] internal api
//-----------------------------------------------------------------------------

// setup/shutdown
void pl__create_profile_context (plProfileContext* tPContext);
void pl__cleanup_profile_context(plProfileContext* tPContext);

// frames
void pl__begin_profile_frame(plProfileContext* tPContext, uint64_t ulFrame);
void pl__end_profile_frame  (plProfileContext* tPContext);

// samples
void pl__begin_profile_sample(plProfileContext* tPContext, const char* cPName);
void pl__end_profile_sample  (plProfileContext* tPContext);

#endif // PL_PROFILE_H

#ifdef PL_PROFILE_IMPLEMENTATION

#ifdef _WIN32
#elif defined(__APPLE__)
#include <time.h>
#else // linux
#endif
#include "pl_ds.h"

#ifndef PL_ASSERT
#include <assert.h>
#define PL_ASSERT(x) assert(x)
#endif


static inline double
pl__get_wall_clock()
{
    #ifdef _WIN32
    #elif defined(__APPLE__)
    return ((double)(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1e9);
    #else // linux
    #endif
}

void
pl__create_profile_context(plProfileContext* tPContext)
{
    tPContext->dStartTime = ((double)(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1e9);
    tPContext->sbFrames = NULL;
    tPContext->tPCurrentFrame = &tPContext->tPFrames[0];
}

void
pl__cleanup_profile_context(plProfileContext* tPContext)
{
    pl_sb_free(tPContext->sbFrames);
}

void
pl__begin_profile_frame(plProfileContext* tPContext, uint64_t ulFrame)
{
    tPContext->tPCurrentFrame = &tPContext->tPFrames[ulFrame % 2];
    tPContext->tPLastFrame = &tPContext->tPFrames[(ulFrame + 1) % 2];
    
    tPContext->tPCurrentFrame->ulFrame = ulFrame;
    tPContext->tPCurrentFrame->dDuration = 0.0;
    tPContext->tPCurrentFrame->dInternalDuration = 0.0;
    tPContext->tPCurrentFrame->dStartTime = pl__get_wall_clock();
    pl_sb_reset(tPContext->tPCurrentFrame->sbSamples);
}

void
pl__end_profile_frame(plProfileContext* tPContext)
{
    tPContext->tPCurrentFrame->dDuration = pl__get_wall_clock() - tPContext->tPCurrentFrame->dStartTime;
}

void
pl__begin_profile_sample(plProfileContext* tPContext, const char* cPName)
{
    const double dCurrentInternalTime = pl__get_wall_clock();
    plProfileFrame* tPCurrentFrame = tPContext->tPCurrentFrame;

    plProfileSample tSample = {
        .cPName = cPName,
        .dDuration = 0.0,
        .dStartTime = pl__get_wall_clock(),
        .uDepth = pl_sb_size(tPCurrentFrame->sbSampleStack),
    };

    uint32_t uSampleIndex = pl_sb_size(tPCurrentFrame->sbSamples);
    pl_sb_push(tPCurrentFrame->sbSampleStack, uSampleIndex);
    pl_sb_push(tPCurrentFrame->sbSamples, tSample);
    tPCurrentFrame->dInternalDuration += pl__get_wall_clock() - dCurrentInternalTime;
}

void
pl__end_profile_sample(plProfileContext* tPContext)
{
    const double dCurrentInternalTime = pl__get_wall_clock();
    plProfileFrame* tPCurrentFrame = tPContext->tPCurrentFrame;
    plProfileSample* tPLastSample = &tPCurrentFrame->sbSamples[pl_sb_pop(tPCurrentFrame->sbSampleStack)];
    PL_ASSERT(tPLastSample && "Begin/end profile sampel mismatch");
    tPLastSample->dDuration = pl__get_wall_clock() - tPLastSample->dStartTime;
    tPLastSample->dStartTime -= tPCurrentFrame->dStartTime;
    tPCurrentFrame->dInternalDuration += pl__get_wall_clock() - dCurrentInternalTime;
}

#endif // PL_PROFILE_IMPLEMENTATION