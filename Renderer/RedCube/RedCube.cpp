#include "../../../../Common_3/Application/Interfaces/IApp.h"
#include "../../../../Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../Common_3/Utilities/Interfaces/ILog.h"
#include "../../../../Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "../../../../Common_3/Utilities/Math/MathTypes.h"
#include "../../../../Common_3/Utilities/Interfaces/IMemory.h"
#include "../../../../Common_3/Utilities/RingBuffer.h"
#include "../../../../Common_3/Graphics/FSL/defaults.h"
#include "./Shaders/FSL/Global.srt.h"

// Simple vertex structure
struct Vertex
{
    vec3 position;
    vec3 normal;
};

static Vertex gCubeVertices[] = {
    // front
    { { -1.f, -1.f, 1.f }, { 0.f, 0.f, 1.f } },
    { { 1.f, -1.f, 1.f }, { 0.f, 0.f, 1.f } },
    { { 1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f } },
    { { -1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f } },
    // back
    { { 1.f, -1.f, -1.f }, { 0.f, 0.f, -1.f } },
    { { -1.f, -1.f, -1.f }, { 0.f, 0.f, -1.f } },
    { { -1.f, 1.f, -1.f }, { 0.f, 0.f, -1.f } },
    { { 1.f, 1.f, -1.f }, { 0.f, 0.f, -1.f } },
    // left
    { { -1.f, -1.f, -1.f }, { -1.f, 0.f, 0.f } },
    { { -1.f, -1.f, 1.f }, { -1.f, 0.f, 0.f } },
    { { -1.f, 1.f, 1.f }, { -1.f, 0.f, 0.f } },
    { { -1.f, 1.f, -1.f }, { -1.f, 0.f, 0.f } },
    // right
    { { 1.f, -1.f, 1.f }, { 1.f, 0.f, 0.f } },
    { { 1.f, -1.f, -1.f }, { 1.f, 0.f, 0.f } },
    { { 1.f, 1.f, -1.f }, { 1.f, 0.f, 0.f } },
    { { 1.f, 1.f, 1.f }, { 1.f, 0.f, 0.f } },
    // top
    { { -1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f } },
    { { 1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f } },
    { { 1.f, 1.f, -1.f }, { 0.f, 1.f, 0.f } },
    { { -1.f, 1.f, -1.f }, { 0.f, 1.f, 0.f } },
    // bottom
    { { -1.f, -1.f, -1.f }, { 0.f, -1.f, 0.f } },
    { { 1.f, -1.f, -1.f }, { 0.f, -1.f, 0.f } },
    { { 1.f, -1.f, 1.f }, { 0.f, -1.f, 0.f } },
    { { -1.f, -1.f, 1.f }, { 0.f, -1.f, 0.f } },
};

static uint16_t gCubeIndices[] = {
    0,1,2, 0,2,3,
    4,5,6, 4,6,7,
    8,9,10, 8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23
};

struct UniformData
{
    mat4 mvp;
};

const uint32_t gDataBufferCount = 2;

Renderer* pRenderer = nullptr;
Queue* pGraphicsQueue = nullptr;
GpuCmdRing gGraphicsCmdRing = {};
SwapChain* pSwapChain = nullptr;
RenderTarget* pDepthBuffer = nullptr;
Semaphore* pImageAcquiredSemaphore = nullptr;

Shader* pShader = nullptr;
Pipeline* pPipeline = nullptr;
RootSignature* pRootSignature = nullptr;

Buffer* pVertexBuffer = nullptr;
Buffer* pIndexBuffer = nullptr;
Buffer* pUniformBuffer[gDataBufferCount] = { nullptr };

DescriptorSet* pDescriptorSetUniforms = nullptr;

uint32_t gFrameIndex = 0;
UniformData gUniformData;

class RedCube : public IApp
{
public:
    bool Init()
    {
        RendererDesc settings{};
        initGPUConfiguration(settings.pExtendedSettings);
        initRenderer(GetName(), &settings, &pRenderer);
        if (!pRenderer)
        {
            ShowUnsupportedMessage(getUnsupportedGPUMsg());
            return false;
        }
        setupGPUConfigurationPlatformParameters(pRenderer, settings.pExtendedSettings);

        QueueDesc queueDesc = {};
        queueDesc.mType = QUEUE_TYPE_GRAPHICS;
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
        initRootSignature(pRenderer, &rootDesc, &pRootSignature);

        BufferLoadDesc vbDesc{};
        vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        vbDesc.mDesc.mSize = sizeof(gCubeVertices);
        vbDesc.pData = gCubeVertices;
        vbDesc.ppBuffer = &pVertexBuffer;
        addResource(&vbDesc, nullptr);

        BufferLoadDesc ibDesc{};
        ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
        ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        ibDesc.mDesc.mSize = sizeof(gCubeIndices);
        ibDesc.pData = gCubeIndices;
        ibDesc.ppBuffer = &pIndexBuffer;
        addResource(&ibDesc, nullptr);

        BufferLoadDesc ubDesc = {};
        ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        ubDesc.mDesc.mSize = sizeof(UniformData);
        ubDesc.pData = nullptr;
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            ubDesc.ppBuffer = &pUniformBuffer[i];
            addResource(&ubDesc, nullptr);
        }

        addShaders();
        addDescriptorSets();
        if (!addSwapChain() || !addDepthBuffer())
            return false;
        addPipelines();
        prepareDescriptorSets();

        return true;
    }

    void Exit()
    {
        waitQueueIdle(pGraphicsQueue);

        removePipelines();
        removeDescriptorSets();
        removeShaders();

        for (uint32_t i = 0; i < gDataBufferCount; ++i)
            removeResource(pUniformBuffer[i]);
        removeResource(pVertexBuffer);
        removeResource(pIndexBuffer);

        removeRenderTarget(pRenderer, pDepthBuffer);
        removeSwapChain(pRenderer, pSwapChain);

        exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        exitSemaphore(pRenderer, pImageAcquiredSemaphore);
        exitRootSignature(pRenderer, pRootSignature);
        exitResourceLoaderInterface(pRenderer);
        exitQueue(pRenderer, pGraphicsQueue);
        exitRenderer(pRenderer);
        exitGPUConfiguration();
        pRenderer = nullptr;
    }

    bool Load(ReloadDesc* reload)
    {
        if (!addSwapChain() || !addDepthBuffer())
            return false;
        addPipelines();
        prepareDescriptorSets();
        return true;
    }

    void Unload(ReloadDesc* reload)
    {
        waitQueueIdle(pGraphicsQueue);
        removePipelines();
        removeRenderTarget(pRenderer, pDepthBuffer);
        removeSwapChain(pRenderer, pSwapChain);
    }

    void Update(float deltaTime)
    {
        mat4 view = mat4::translation(vec3(0.0f, 0.0f, -5.0f));
        mat4 proj = mat4::perspectiveReverseZ(PI / 4.0f, (float)mSettings.mWidth / (float)mSettings.mHeight, 0.1f, 100.0f);
        gUniformData.mvp = proj * view;
    }

    void Draw()
    {
        uint32_t swapchainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, nullptr, &swapchainImageIndex);

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

        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
        RenderTargetBarrier barrier = { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &barrier);

        BindRenderTargetsDesc bind = {};
        bind.mRenderTargetCount = 1;
        bind.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        bind.mDepthStencil = { pDepthBuffer, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bind);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        cmdBindPipeline(cmd, pPipeline);
        cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetUniforms);
        const uint32_t stride = sizeof(Vertex);
        cmdBindVertexBuffer(cmd, 1, &pVertexBuffer, &stride, nullptr);
        cmdBindIndexBuffer(cmd, pIndexBuffer, INDEX_TYPE_UINT16, 0);
        cmdDrawIndexed(cmd, 36, 0, 0);

        cmdBindRenderTargets(cmd, nullptr);
        barrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &barrier);
        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

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

        gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
    }

    const char* GetName() { return "RedCube"; }

    bool addSwapChain()
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mWidth = mSettings.mWidth;
        swapChainDesc.mHeight = mSettings.mHeight;
        swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
        swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
        swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
        swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
        ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
        return pSwapChain != nullptr;
    }

    bool addDepthBuffer()
    {
        RenderTargetDesc depthRT = {};
        depthRT.mArraySize = 1;
        depthRT.mClearValue.depth = 0.f;
        depthRT.mClearValue.stencil = 0;
        depthRT.mDepth = 1;
        depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
        depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
        depthRT.mHeight = mSettings.mHeight;
        depthRT.mSampleCount = SAMPLE_COUNT_1;
        depthRT.mSampleQuality = 0;
        depthRT.mWidth = mSettings.mWidth;
        addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);
        return pDepthBuffer != nullptr;
    }

    void addDescriptorSets()
    {
        DescriptorSetDesc desc = SRT_SET_DESC(SrtData, PerFrame, gDataBufferCount, 0);
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
    }

    void removeDescriptorSets()
    {
        removeDescriptorSet(pRenderer, pDescriptorSetUniforms);
    }

    void addShaders()
    {
        ShaderLoadDesc shaderDesc = {};
        shaderDesc.mVert.pFileName = "cube.vert";
        shaderDesc.mFrag.pFileName = "cube.frag";
        addShader(pRenderer, &shaderDesc, &pShader);
    }

    void removeShaders()
    {
        removeShader(pRenderer, pShader);
    }

    void addPipelines()
    {
        VertexLayout vertexLayout = {};
        vertexLayout.mBindingCount = 1;
        vertexLayout.mAttribCount = 2;
        vertexLayout.mBindings[0].mStride = sizeof(Vertex);
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[0].mOffset = 0;
        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 0;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[1].mOffset = sizeof(vec3);

        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_BACK;

        DepthStateDesc depthStateDesc = {};
        depthStateDesc.mDepthTest = true;
        depthStateDesc.mDepthWrite = true;
        depthStateDesc.mDepthFunc = CMP_GEQUAL;

        PipelineDesc desc = {};
        PIPELINE_LAYOUT_DESC(desc, SRT_LAYOUT_DESC(SrtData, PerFrame), NULL, NULL, NULL);
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = &depthStateDesc;
        pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
        pipelineSettings.pShaderProgram = pShader;
        pipelineSettings.pVertexLayout = &vertexLayout;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;
        addPipeline(pRenderer, &desc, &pPipeline);
    }

    void removePipelines()
    {
        removePipeline(pRenderer, pPipeline);
    }

    void prepareDescriptorSets()
    {
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            DescriptorData params = {};
            params.mIndex = SRT_RES_IDX(SrtData, PerFrame, gUniformBlock);
            params.ppBuffers = &pUniformBuffer[i];
            updateDescriptorSet(pRenderer, i, pDescriptorSetUniforms, 1, &params);
        }
    }
};

DEFINE_APPLICATION_MAIN(RedCube)
