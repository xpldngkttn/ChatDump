#pragma once

BEGIN_SRT(SrtData)
    BEGIN_SRT_SET(PerFrame)
        DECL_CBUFFER(PerFrame, CBUFFER(UniformData), gUniformData)
    END_SRT_SET(PerFrame)
END_SRT(SrtData)
