#include "../../../../Common_3/Application/Interfaces/IApp.h"
#include "../../../../Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "../../../../Common_3/Utilities/Interfaces/ILog.h"
#include "../../../../Common_3/Utilities/Interfaces/ITime.h"
#include "../../../../Common_3/Utilities/Math/MathTypes.h"
#include "../../../../Common_3/Utilities/Interfaces/IMemory.h"
#include "../../../../Common_3/Graphics/FSL/defaults.h"
#include "./Shaders/FSL/Global.srt.h"

struct Cube
{
    mat4 mModel;
    vec4 mColor;
};

struct Background
{
    vec4 mColor;
};

struct UniformData
{
    CameraMatrix mvp;
    vec4 color;
};

const uint32_t gDataBufferCount = 2;

Renderer* pRenderer = NULL;
Queue*    pGraphicsQueue = NULL;
GpuCmdRing gGraphicsCmdRing = {};
SwapChain*    pSwapChain = NULL;
RenderTarget* pDepthBuffer = NULL;
Semaphore*    pImageAcquiredSemaphore = NULL;

Shader*      pCubeShader = NULL;
Pipeline*    pCubePipeline = NULL;
Buffer*      pCubeVertexBuffer = NULL;
Buffer*      pCubeIndexBuffer = NULL;
DescriptorSet* pDescriptorSet = NULL;
Buffer*      pUniformBuffer[gDataBufferCount] = { NULL };

uint32_t gFrameIndex = 0;
ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;

Cube       gCube;
Background gBackground;
UniformData gUniformData;

struct Vertex
{
    float3 mPosition;
};

static Vertex gCubeVertices[8] = {
    { { -1.0f, -1.0f, -1.0f } },
    { { -1.0f,  1.0f, -1.0f } },
    { {  1.0f,  1.0f, -1.0f } },
    { {  1.0f, -1.0f, -1.0f } },
    { { -1.0f, -1.0f,  1.0f } },
    { { -1.0f,  1.0f,  1.0f } },
    { {  1.0f,  1.0f,  1.0f } },
    { {  1.0f, -1.0f,  1.0f } },
};

static uint16_t gCubeIndices[36] = {
    0,1,2, 0,2,3,
    4,5,6, 4,6,7,
    0,4,7, 0,7,3,
    1,5,6, 1,6,2,
    3,2,6, 3,6,7,
    0,1,5, 0,5,4
};

class RedCubeApp: public IApp
{
public:
    bool Init()
    {
        RendererDesc settings = {};
        initGPUConfiguration(settings.pExtendedSettings);
        initRenderer(GetName(), &settings, &pRenderer);
        if (!pRenderer)
            return false;
        setupGPUConfigurationPlatformParameters(pRenderer, settings.pExtendedSettings);

        QueueDesc queueDesc = {};
        queueDesc.mType = QUEUE_TYPE_GRAPHICS;
        queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
        initQueue(pRenderer, &queueDesc, &pGraphicsQueue);

        GpuCmdRingDesc cmdRingDesc = {};
        cmdRingDesc.pQueue = pGraphicsQueue;
        cmdRingDesc.mPoolCount = gDataBufferCount;
        cmdRingDesc.mCmdPerPoolCount = 1;
        cmdRingDesc.mAddSyncPrimitives = true;
        initGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

        initSemaphore(pRenderer, &pImageAcquiredSemaphore);
        initResourceLoaderInterface(pRenderer);

        RootSignatureDesc rootDesc = {};
        INIT_RS_DESC(rootDesc, "default.rootsig", "compute.rootsig");
        initRootSignature(pRenderer, &rootDesc);

        ShaderLoadDesc shaderDesc = {};
        shaderDesc.mVert.pFileName = "basic.vert";
        shaderDesc.mFrag.pFileName = "basic.frag";
        addShader(pRenderer, &shaderDesc, &pCubeShader);

        BufferLoadDesc vbDesc = {};
        vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        vbDesc.mDesc.mSize = sizeof(gCubeVertices);
        vbDesc.pData = gCubeVertices;
        vbDesc.ppBuffer = &pCubeVertexBuffer;
        addResource(&vbDesc, NULL);

        BufferLoadDesc ibDesc = {};
        ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        ibDesc.mDesc.mSize = sizeof(gCubeIndices);
        ibDesc.pData = gCubeIndices;
        ibDesc.ppBuffer = &pCubeIndexBuffer;
        addResource(&ibDesc, NULL);

        BufferLoadDesc ubDesc = {};
        ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        ubDesc.mDesc.mSize = sizeof(UniformData);
        ubDesc.pData = NULL;
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            ubDesc.ppBuffer = &pUniformBuffer[i];
            addResource(&ubDesc, NULL);
        }

        DescriptorSetDesc desc = SRT_SET_DESC(SrtData, PerFrame, gDataBufferCount, 0);
        addDescriptorSet(pRenderer, &desc, &pDescriptorSet);

        gCube.mColor = { 1.0f, 0.0f, 0.0f, 1.0f };
        gCube.mModel = mat4::identity();
        gBackground.mColor = { 0.0f, 0.0f, 1.0f, 1.0f };

        initScreenshotCapturer(pRenderer, pGraphicsQueue, GetName());
        gGpuProfileToken = initGpuProfiler(pRenderer, pGraphicsQueue, "Graphics");

        return true;
    }

    void Exit()
    {
        exitScreenshotCapturer();
        exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        removeDescriptorSet(pRenderer, pDescriptorSet);
        removeResource(pCubeVertexBuffer);
        removeResource(pCubeIndexBuffer);
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
            removeResource(pUniformBuffer[i]);
        removeShader(pRenderer, pCubeShader);
        removeSemaphore(pRenderer, pImageAcquiredSemaphore);
        exitResourceLoaderInterface(pRenderer);
        removeQueue(pGraphicsQueue);
        exitRenderer(pRenderer);
    }

    bool Load(ReloadDesc* pReloadDesc)
    {
        if (!addSwapChain())
            return false;
        if (!addDepthBuffer())
            return false;
        addPipelines();
        prepareDescriptorSets();
        return true;
    }

    void Unload(ReloadDesc*)
    {
        waitQueueIdle(pGraphicsQueue);
        removePipelines();
        removeSwapChain(pRenderer, pSwapChain);
        removeRenderTarget(pRenderer, pDepthBuffer);
    }

    void Update(float deltaTime)
    {
        static float t = 0.0f;
        t += deltaTime;
        gCube.mModel = mat4::rotationY(t);

        CameraMatrix viewMat = CameraMatrix::lookAt(vec3(0.0f, 0.0f, 5.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
        float aspectInverse = (float)mSettings.mHeight / (float)mSettings.mWidth;
        CameraMatrix projMat = CameraMatrix::perspectiveReverseZ(PI / 2.0f, aspectInverse, 0.1f, 100.0f);
        gUniformData.mvp = projMat * viewMat * gCube.mModel;
        gUniformData.color = gCube.mColor;
    }

    void Draw()
    {
        uint32_t swapchainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];

        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
        FenceStatus fenceStatus;
        getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &elem.pFence);

        BufferUpdateDesc cbv = { pUniformBuffer[gFrameIndex] };
        beginUpdateResource(&cbv);
        memcpy(cbv.pMappedData, &gUniformData, sizeof(gUniformData));
        endUpdateResource(&cbv);

        resetCmdPool(pRenderer, elem.pCmdPool);
        Cmd* cmd = elem.pCmds[0];
        beginCmd(cmd);

        cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);

        RenderTargetBarrier barrier = { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, &barrier);

        BindRenderTargetsDesc bind = {};
        bind.mRenderTargetCount = 1;
        bind.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        bind.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
        bind.mClearColorValues[0] = gBackground.mColor;
        cmdBindRenderTargets(cmd, &bind);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        cmdBindPipeline(cmd, pCubePipeline);
        const uint32_t stride = sizeof(Vertex);
        cmdBindVertexBuffer(cmd, 1, &pCubeVertexBuffer, &stride, NULL);
        cmdBindIndexBuffer(cmd, pCubeIndexBuffer, INDEX_TYPE_UINT16, 0);
        cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSet);
        cmdDrawIndexed(cmd, 36, 0, 0);

        cmdBindRenderTargets(cmd, NULL);

        barrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, &barrier);

        cmdEndGpuFrameProfile(cmd, gGpuProfileToken);
        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = 2;
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = (uint8_t)swapchainImageIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.ppWaitSemaphores = &elem.pSemaphore;
        presentDesc.mSubmitDone = true;
        queuePresent(pGraphicsQueue, &presentDesc);

        flipProfiler();
        gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
    }

    const char* GetName() { return "35_RedCube"; }

private:
    bool addSwapChain()
    {
        SwapChainDesc desc = {};
        desc.mWindowHandle = pWindow->handle;
        desc.mPresentQueueCount = 1;
        desc.ppPresentQueues = &pGraphicsQueue;
        desc.mWidth = mSettings.mWidth;
        desc.mHeight = mSettings.mHeight;
        desc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
        desc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &desc, COLOR_SPACE_SDR_SRGB);
        desc.mColorSpace = COLOR_SPACE_SDR_SRGB;
        desc.mEnableVsync = mSettings.mVSyncEnabled;
        ::addSwapChain(pRenderer, &desc, &pSwapChain);
        return pSwapChain != NULL;
    }

    bool addDepthBuffer()
    {
        RenderTargetDesc rtDesc = {};
        rtDesc.mArraySize = 1;
        rtDesc.mClearValue.depth = 1.0f;
        rtDesc.mClearValue.stencil = 0;
        rtDesc.mDepth = 1;
        rtDesc.mFormat = TinyImageFormat_D32_SFLOAT;
        rtDesc.mStartState = RESOURCE_STATE_DEPTH_WRITE;
        rtDesc.mHeight = mSettings.mHeight;
        rtDesc.mSampleCount = SAMPLE_COUNT_1;
        rtDesc.mSampleQuality = 0;
        rtDesc.mWidth = mSettings.mWidth;
        addRenderTarget(pRenderer, &rtDesc, &pDepthBuffer);
        return pDepthBuffer != NULL;
    }

    void addPipelines()
    {
        VertexLayout layout = {};
        layout.mBindingCount = 1;
        layout.mAttribCount = 1;
        layout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        layout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        layout.mAttribs[0].mBinding = 0;
        layout.mAttribs[0].mLocation = 0;
        layout.mAttribs[0].mOffset = 0;

        RasterizerStateDesc raster = {};
        raster.mCullMode = CULL_MODE_BACK;

        DepthStateDesc depth = {};
        depth.mDepthTest = true;
        depth.mDepthWrite = true;
        depth.mDepthFunc = CMP_LEQUAL;

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        PIPELINE_LAYOUT_DESC(desc, NULL, SRT_LAYOUT_DESC(SrtData, PerFrame), NULL, NULL);
        GraphicsPipelineDesc& pipeline = desc.mGraphicsDesc;
        pipeline.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipeline.mRenderTargetCount = 1;
        pipeline.pDepthState = &depth;
        pipeline.pRasterizerState = &raster;
        pipeline.pShaderProgram = pCubeShader;
        pipeline.pVertexLayout = &layout;
        pipeline.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        pipeline.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        pipeline.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        pipeline.mDepthStencilFormat = pDepthBuffer->mFormat;
        addPipeline(pRenderer, &desc, &pCubePipeline);
    }

    void removePipelines()
    {
        removePipeline(pRenderer, pCubePipeline);
    }

    void prepareDescriptorSets()
    {
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            DescriptorData param = {};
            param.mIndex = SRT_RES_IDX(SrtData, PerFrame, gUniformData);
            param.ppBuffers = &pUniformBuffer[i];
            updateDescriptorSet(pRenderer, i, pDescriptorSet, 1, &param);
        }
    }
};

DEFINE_APPLICATION_MAIN(RedCubeApp)
