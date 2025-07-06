// Interfaces
#include "../../../../Common_3/Application/Interfaces/IApp.h"
#include "../../../../Common_3/Application/Interfaces/ICameraController.h"
#include "../../../../Common_3/Application/Interfaces/IFont.h"
#include "../../../../Common_3/OS/Interfaces/IInput.h"
#include "../../../../Common_3/Application/Interfaces/IProfiler.h"
#include "../../../../Common_3/Application/Interfaces/IScreenshot.h"
#include "../../../../Common_3/Application/Interfaces/IUI.h"
#include "../../../../Common_3/Game/Interfaces/IScripting.h"
#include "../../../../Common_3/Utilities/Interfaces/IFileSystem.h"
#include "../../../../Common_3/Utilities/Interfaces/ILog.h"
#include "../../../../Common_3/Utilities/Interfaces/ITime.h"

// Renderer
#include "../../../../Common_3/Graphics/Interfaces/IGraphics.h"
#include "../../../../Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "../../../../Common_3/Utilities/RingBuffer.h"

// Math
#include "../../../../Common_3/Utilities/Math/MathTypes.h"

#include "../../../../Common_3/Utilities/Interfaces/IMemory.h"

// fsl
#include "../../../../Common_3/Graphics/FSL/defaults.h"
#include "./Shaders/FSL/Global.srt.h"

struct Vertex
{
    float3 pos;
    float3 color;
};

STRUCT(UniformBlock)
{
    mat4 mvp;
};

static Vertex gPyramidVerts[12] = {
    { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
    { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
    { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
    { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
};

const uint32_t gDataBufferCount = 2;

Renderer*  pRenderer = NULL;
Queue*     pGraphicsQueue = NULL;
GpuCmdRing gGraphicsCmdRing = {};
SwapChain* pSwapChain = NULL;
Semaphore* pImageAcquiredSemaphore = NULL;
Shader*    pBasicShader = NULL;
Pipeline*  pBasicPipeline = NULL;
Buffer*    pVertexBuffer = NULL;
Buffer*    pUniformBuffer[gDataBufferCount] = { NULL };
DescriptorSet* pDescriptorSetUniforms = NULL;
UniformBlock gUniformData;
float gRotation = 0.0f;
uint32_t   gFrameIndex = 0;

class Hades3 : public IApp
{
public:
    bool Init()
    {
        RendererDesc settings = {};
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
        initRootSignature(pRenderer, &rootDesc);

        BufferLoadDesc vbDesc = {};
        vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        vbDesc.mDesc.mSize = sizeof(gPyramidVerts);
        vbDesc.pData = gPyramidVerts;
        vbDesc.ppBuffer = &pVertexBuffer;
        addResource(&vbDesc, NULL);

        BufferLoadDesc ubDesc = {};
        ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        ubDesc.mDesc.mSize = sizeof(UniformBlock);
        ubDesc.pData = NULL;
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
        {
            ubDesc.ppBuffer = &pUniformBuffer[i];
            addResource(&ubDesc, NULL);
        }

        return true;
    }

    void Exit()
    {
        waitQueueIdle(pGraphicsQueue);
        removeResource(pVertexBuffer);
        for (uint32_t i = 0; i < gDataBufferCount; ++i)
            removeResource(pUniformBuffer[i]);
        exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        exitSemaphore(pRenderer, pImageAcquiredSemaphore);
        exitRootSignature(pRenderer);
        exitResourceLoaderInterface(pRenderer);
        exitQueue(pRenderer, pGraphicsQueue);
        exitRenderer(pRenderer);
        exitGPUConfiguration();
        pRenderer = NULL;
    }

    bool Load(ReloadDesc* pReloadDesc)
    {
        if (!addSwapChain())
            return false;
        addDescriptorSets();
        addShaders();
        addPipelines();
        prepareDescriptorSets();
        return true;
    }

    void Unload(ReloadDesc* pReloadDesc)
    {
        waitQueueIdle(pGraphicsQueue);
        removePipelines();
        removeDescriptorSets();
        removeSwapChain(pRenderer, pSwapChain);
        removeShaders();
    }

    void Update(float deltaTime)
    {
        gRotation += deltaTime;
        float aspectInverse = (float)mSettings.mHeight / (float)mSettings.mWidth;
        CameraMatrix proj = CameraMatrix::perspectiveReverseZ(PI / 2.0f, aspectInverse, 0.1f, 10.0f);
        mat4 model = mat4::rotationY(gRotation);
        gUniformData.mvp = proj * model;
    }

    void Draw()
    {
        uint32_t swapchainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL, &swapchainImageIndex);
        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];

        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);
        FenceStatus       fenceStatus;
        getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &elem.pFence);

        resetCmdPool(pRenderer, elem.pCmdPool);
        Cmd* cmd = elem.pCmds[0];
        beginCmd(cmd);

        RenderTargetBarrier barrier = { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, &barrier);

        BindRenderTargetsDesc bindRenderTargets = {};
        bindRenderTargets.mRenderTargetCount = 1;
        bindRenderTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bindRenderTargets);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        BufferUpdateDesc ubUpdate = { pUniformBuffer[gFrameIndex] };
        ubUpdate.pData = &gUniformData;
        updateResource(&ubUpdate);

        cmdBindDescriptorSet(cmd, gFrameIndex, pDescriptorSetUniforms);
        cmdBindPipeline(cmd, pBasicPipeline);
        uint32_t stride = sizeof(Vertex);
        cmdBindVertexBuffer(cmd, 1, &pVertexBuffer, &stride, NULL);
        cmdDraw(cmd, 12, 0);

        cmdBindRenderTargets(cmd, NULL);
        barrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, &barrier);
        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);
        Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.ppCmds = &cmd;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.mWaitSemaphoreCount = 2;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = swapchainImageIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.ppWaitSemaphores = &elem.pSemaphore;
        presentDesc.mSubmitDone = true;
        queuePresent(pGraphicsQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
    }

    const char* GetName() { return "00_Hades3"; }

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
        return pSwapChain != NULL;
    }

    void addShaders()
    {
        ShaderLoadDesc shaderDesc = {};
        shaderDesc.mVert.pFileName = "basic.vert";
        shaderDesc.mFrag.pFileName = "basic.frag";
        addShader(pRenderer, &shaderDesc, &pBasicShader);
    }

    void removeShaders() { removeShader(pRenderer, pBasicShader); }

    void addPipelines()
    {
        VertexLayout vertexLayout = {};
        vertexLayout.mBindingCount = 1;
        vertexLayout.mAttribCount = 2;
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[0].mOffset = 0;
        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_COLOR;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 0;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[1].mOffset = sizeof(float) * 3;

        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

        DepthStateDesc depthStateDesc = {};

        PipelineDesc desc = {};
        PIPELINE_LAYOUT_DESC(desc, NULL, SRT_LAYOUT_DESC(SrtData, PerFrame), NULL, NULL);
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& pipelineSettings = desc.mGraphicsDesc;
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = &depthStateDesc;
        pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        pipelineSettings.pShaderProgram = pBasicShader;
        pipelineSettings.pVertexLayout = &vertexLayout;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;
        addPipeline(pRenderer, &desc, &pBasicPipeline);
    }

    void removePipelines() { removePipeline(pRenderer, pBasicPipeline); }

    void addDescriptorSets()
    {
        DescriptorSetDesc desc = SRT_SET_DESC(SrtData, PerFrame, gDataBufferCount, 0);
        addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
    }

    void removeDescriptorSets() { removeDescriptorSet(pRenderer, pDescriptorSetUniforms); }

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

int WindowsMain(int argc, char** argv, IApp* app);
extern "C"
{
__declspec(dllexport) extern const UINT  D3D12SDKVersion = 715;
__declspec(dllexport) extern const char* D3D12SDKPath = u8"";
}
int main(int argc, char** argv)
{
    IApp::argc = argc;
    IApp::argv = (const char**)argv;
    static Hades3 app = {};
    return WindowsMain(argc, argv, &app);
}
