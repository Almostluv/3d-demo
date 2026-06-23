/**
 * @file
 * @brief mutant_scene_ui.cpp 実装を定義する。
 */

#include "mutant_scene.h"
#include "player_param.h"
#include "enemy_param.h"
#include "game_config.h"
#include "resource_manager.h"
#include "dx11_context.h"
#include "DxLib.h"
#include "DxFunctionWin.h"

#include <d3d11.h>
#include <cstring>
#include <fstream>
#include <vector>

namespace
{
struct HealthBarVertex
{
    float x, y;
    float r, g, b, a;
    float u, v;
};

struct HealthBarConstants
{
    float topColor[4];
    float bottomColor[4];
};

struct UiRect
{
    int x;
    int y;
    int width;
    int height;
};

constexpr const char* BuffArrowGraph = "Resource/GameUI/buff_arrow.png";
constexpr int BuffArrowSize = 30;

void drawBuffArrow(int graphHandle, const UiRect& frameRect)
{
    if (graphHandle == -1)
    {
        return;
    }

    /// HP フレームの右側へ強化状態アイコンを置く。
    int x = frameRect.x + frameRect.width + 8;
    int y = frameRect.y + (frameRect.height - BuffArrowSize) / 2;
    DrawExtendGraph(x, y, x + BuffArrowSize, y + BuffArrowSize, graphHandle, TRUE);
}
float clampRate(float value, float maxValue)
{
    /// HP やスタミナを 0..1 の描画率へ変換する。
    if (maxValue <= 0)
    {
        return 0.0f;
    }

    float rate = static_cast<float>(value) / static_cast<float>(maxValue);

    if (rate < 0.0f)
    {
        return 0.0f;
    }

    if (rate > 1.0f)
    {
        return 1.0f;
    }

    return rate;
}

std::vector<char> loadBinaryFile(const char* path)
{
    /// HP バー用のコンパイル済みシェーダーバイナリを読み込む。
    std::string resolvedPath = ResourceManager::instance()->resolvePath(path);
    std::ifstream file(resolvedPath.c_str(), std::ios::binary);
    if (!file)
    {
        return std::vector<char>();
    }

    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> data(static_cast<size_t>(size));
    file.read(data.data(), size);
    return data;
}

float toNdcX(float x)
{
    /// スクリーン X 座標を DirectX の NDC 座標へ変換する。
    return (x / static_cast<float>(kWindowWidth)) * 2.0f - 1.0f;
}

float toNdcY(float y)
{
    /// スクリーン Y 座標を DirectX の NDC 座標へ変換する。
    return 1.0f - (y / static_cast<float>(kWindowHeight)) * 2.0f;
}

class HealthBarShader
{
public:
    HealthBarShader()
        : mVertexShader(nullptr)
        , mPixelShader(nullptr)
        , mInputLayout(nullptr)
        , mVertexBuffer(nullptr)
        , mConstantBuffer(nullptr)
        , mBlendState(nullptr)
        , mDepthStencilState(nullptr)
        , mRasterizerState(nullptr)
    {
        setup();
    }

    ~HealthBarShader()
    {
        release();
    }

    bool draw(const UiRect& rect, const float topColor[4], const float bottomColor[4])
    {
        if (!isReady() || rect.width <= 0 || rect.height <= 0)
        {
            return false;
        }

        /// HP バーは矩形 1 枚を 2 三角形で描く。
        ID3D11DeviceContext* context = Dx11Context::context();

        HealthBarVertex vertices[6] =
        {
            makeVertex(rect.x, rect.y, 0.0f, 0.0f),
            makeVertex(rect.x, rect.y + rect.height, 0.0f, 1.0f),
            makeVertex(rect.x + rect.width, rect.y + rect.height, 1.0f, 1.0f),
            makeVertex(rect.x, rect.y, 0.0f, 0.0f),
            makeVertex(rect.x + rect.width, rect.y + rect.height, 1.0f, 1.0f),
            makeVertex(rect.x + rect.width, rect.y, 1.0f, 0.0f),
        };

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(context->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            return false;
        }
        memcpy(mapped.pData, vertices, sizeof(vertices));
        context->Unmap(mVertexBuffer, 0);

        /// 上下グラデーション色を constant buffer 経由でシェーダーへ渡す。
        HealthBarConstants constants = {};
        for (int i = 0; i < 4; ++i)
        {
            constants.topColor[i] = topColor[i];
            constants.bottomColor[i] = bottomColor[i];
        }

        if (FAILED(context->Map(mConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            return false;
        }
        memcpy(mapped.pData, &constants, sizeof(constants));
        context->Unmap(mConstantBuffer, 0);

        ID3D11InputLayout* oldInputLayout = nullptr;
        ID3D11Buffer* oldVertexBuffer = nullptr;
        UINT oldStride = 0;
        UINT oldOffset = 0;
        D3D11_PRIMITIVE_TOPOLOGY oldTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        ID3D11VertexShader* oldVertexShader = nullptr;
        ID3D11PixelShader* oldPixelShader = nullptr;
        ID3D11Buffer* oldVsConstantBuffer = nullptr;
        ID3D11Buffer* oldPsConstantBuffer = nullptr;
        ID3D11BlendState* oldBlendState = nullptr;
        FLOAT oldBlendFactor[4] = {};
        UINT oldSampleMask = 0;
        ID3D11DepthStencilState* oldDepthStencilState = nullptr;
        UINT oldStencilRef = 0;
        ID3D11RasterizerState* oldRasterizerState = nullptr;

        /// DxLib の描画状態を壊さないよう、DirectX 11 の主要ステートを退避する。
        context->IAGetInputLayout(&oldInputLayout);
        context->IAGetVertexBuffers(0, 1, &oldVertexBuffer, &oldStride, &oldOffset);
        context->IAGetPrimitiveTopology(&oldTopology);
        context->VSGetShader(&oldVertexShader, nullptr, nullptr);
        context->PSGetShader(&oldPixelShader, nullptr, nullptr);
        context->VSGetConstantBuffers(0, 1, &oldVsConstantBuffer);
        context->PSGetConstantBuffers(0, 1, &oldPsConstantBuffer);
        context->OMGetBlendState(&oldBlendState, oldBlendFactor, &oldSampleMask);
        context->OMGetDepthStencilState(&oldDepthStencilState, &oldStencilRef);
        context->RSGetState(&oldRasterizerState);

        UINT stride = sizeof(HealthBarVertex);
        UINT offset = 0;
        context->IASetInputLayout(mInputLayout);
        context->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(mVertexShader, nullptr, 0);
        context->PSSetShader(mPixelShader, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &mConstantBuffer);
        context->PSSetConstantBuffers(0, 1, &mConstantBuffer);
        context->OMSetBlendState(mBlendState, nullptr, 0xffffffff);
        context->OMSetDepthStencilState(mDepthStencilState, 0);
        context->RSSetState(mRasterizerState);
        context->Draw(6, 0);

        /// HP バー描画後は退避したステートへ戻し、以降の DxLib 描画へ影響を残さない。
        context->IASetInputLayout(oldInputLayout);
        context->IASetVertexBuffers(0, 1, &oldVertexBuffer, &oldStride, &oldOffset);
        context->IASetPrimitiveTopology(oldTopology);
        context->VSSetShader(oldVertexShader, nullptr, 0);
        context->PSSetShader(oldPixelShader, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &oldVsConstantBuffer);
        context->PSSetConstantBuffers(0, 1, &oldPsConstantBuffer);
        context->OMSetBlendState(oldBlendState, oldBlendFactor, oldSampleMask);
        context->OMSetDepthStencilState(oldDepthStencilState, oldStencilRef);
        context->RSSetState(oldRasterizerState);

        if (oldInputLayout != nullptr) oldInputLayout->Release();
        if (oldVertexBuffer != nullptr) oldVertexBuffer->Release();
        if (oldVertexShader != nullptr) oldVertexShader->Release();
        if (oldPixelShader != nullptr) oldPixelShader->Release();
        if (oldVsConstantBuffer != nullptr) oldVsConstantBuffer->Release();
        if (oldPsConstantBuffer != nullptr) oldPsConstantBuffer->Release();
        if (oldBlendState != nullptr) oldBlendState->Release();
        if (oldDepthStencilState != nullptr) oldDepthStencilState->Release();
        if (oldRasterizerState != nullptr) oldRasterizerState->Release();

        RefreshDxLibDirect3DSetting();
        return true;
    }

private:
    HealthBarVertex makeVertex(int x, int y, float u, float v) const
    {
        HealthBarVertex vertex = {};
        vertex.x = toNdcX(static_cast<float>(x));
        vertex.y = toNdcY(static_cast<float>(y));
        vertex.r = 1.0f;
        vertex.g = 1.0f;
        vertex.b = 1.0f;
        vertex.a = 1.0f;
        vertex.u = u;
        vertex.v = v;
        return vertex;
    }

    bool isReady() const
    {
        return mVertexShader != nullptr && mPixelShader != nullptr && mInputLayout != nullptr &&
            mVertexBuffer != nullptr && mConstantBuffer != nullptr && mBlendState != nullptr &&
            mDepthStencilState != nullptr && mRasterizerState != nullptr && Dx11Context::isAvailable();
    }

    bool setup()
    {
        if (!Dx11Context::isAvailable())
        {
            return false;
        }

        /// DxLib だけでは表現しにくい HP バーのグラデーション用に専用シェーダーを作る。
        ID3D11Device* device = Dx11Context::device();
        std::vector<char> vsCode = loadBinaryFile("Resource/Shader/health_bar_vs.cso");
        std::vector<char> psCode = loadBinaryFile("Resource/Shader/health_bar_ps.cso");

        if (vsCode.empty() || psCode.empty())
        {
            return false;
        }

        if (FAILED(device->CreateVertexShader(vsCode.data(), vsCode.size(), nullptr, &mVertexShader)))
        {
            return false;
        }

        if (FAILED(device->CreatePixelShader(psCode.data(), psCode.size(), nullptr, &mPixelShader)))
        {
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (FAILED(device->CreateInputLayout(layout, 3, vsCode.data(), vsCode.size(), &mInputLayout)))
        {
            return false;
        }

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = sizeof(HealthBarVertex) * 6;
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(device->CreateBuffer(&vertexBufferDesc, nullptr, &mVertexBuffer)))
        {
            return false;
        }

        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.ByteWidth = sizeof(HealthBarConstants);
        constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if (FAILED(device->CreateBuffer(&constantBufferDesc, nullptr, &mConstantBuffer)))
        {
            return false;
        }

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(device->CreateBlendState(&blendDesc, &mBlendState)))
        {
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        if (FAILED(device->CreateDepthStencilState(&depthDesc, &mDepthStencilState)))
        {
            return false;
        }

        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.DepthClipEnable = FALSE;

        return SUCCEEDED(device->CreateRasterizerState(&rasterizerDesc, &mRasterizerState));
    }

    void release()
    {
        /// DirectX 11 オブジェクトは生成と逆順で Release する。
        if (mRasterizerState != nullptr) mRasterizerState->Release();
        if (mDepthStencilState != nullptr) mDepthStencilState->Release();
        if (mBlendState != nullptr) mBlendState->Release();
        if (mConstantBuffer != nullptr) mConstantBuffer->Release();
        if (mVertexBuffer != nullptr) mVertexBuffer->Release();
        if (mInputLayout != nullptr) mInputLayout->Release();
        if (mPixelShader != nullptr) mPixelShader->Release();
        if (mVertexShader != nullptr) mVertexShader->Release();
    }

    ID3D11VertexShader* mVertexShader;
    ID3D11PixelShader* mPixelShader;
    ID3D11InputLayout* mInputLayout;
    ID3D11Buffer* mVertexBuffer;
    ID3D11Buffer* mConstantBuffer;
    ID3D11BlendState* mBlendState;
    ID3D11DepthStencilState* mDepthStencilState;
    ID3D11RasterizerState* mRasterizerState;
};

HealthBarShader* gHealthBarShader = nullptr;

HealthBarShader& getHealthBarShader()
{
    if (gHealthBarShader == nullptr)
    {
        /// UI 用シェーダーは初回描画時に遅延生成する。
        gHealthBarShader = new HealthBarShader();
    }

    return *gHealthBarShader;
}

void drawFrame(int graphHandle, const UiRect& frameRect, int srcX, int srcY, int srcW, int srcH)
{
    DrawRectExtendGraph(
        frameRect.x,
        frameRect.y,
        frameRect.x + frameRect.width,
        frameRect.y + frameRect.height,
        srcX,
        srcY,
        srcW,
        srcH,
        graphHandle,
        TRUE);
}

void drawFrameReveal(int graphHandle, const UiRect& frameRect, int srcX, int srcY, int srcW, int srcH, float revealRate)
{
    /// ボス HP バー表示開始時に、フレーム画像も横方向へ徐々に表示する。
    int drawWidth = static_cast<int>(frameRect.width * revealRate);
    int drawSrcW = static_cast<int>(srcW * revealRate);
    if (drawWidth <= 0 || drawSrcW <= 0)
    {
        return;
    }

    DrawRectExtendGraph(
        frameRect.x,
        frameRect.y,
        frameRect.x + drawWidth,
        frameRect.y + frameRect.height,
        srcX,
        srcY,
        drawSrcW,
        srcH,
        graphHandle,
        TRUE);
}

void drawShaderHpBarReveal(
    HealthBarShader& shader,
    int frameHandle,
    const UiRect& frameRect,
    const UiRect& fillRect,
    int srcX,
    int srcY,
    int srcW,
    int srcH,
    float hpRate,
    float revealRate,
    const float topColor[4],
    const float bottomColor[4])
{
    UiRect actualFill = fillRect;
    actualFill.width = static_cast<int>(actualFill.width * hpRate * revealRate);

    if (actualFill.width > 0)
    {
        /// シェーダー描画に失敗した場合は単色矩形で HP 表示を維持する。
        if (!shader.draw(actualFill, topColor, bottomColor))
        {
            DrawBox(actualFill.x, actualFill.y, actualFill.x + actualFill.width, actualFill.y + actualFill.height, GetColor(190, 30, 34), TRUE);
        }
    }

    drawFrameReveal(frameHandle, frameRect, srcX, srcY, srcW, srcH, revealRate);
}
void drawShaderHpBar(
    HealthBarShader& shader,
    int frameHandle,
    const UiRect& frameRect,
    const UiRect& fillRect,
    int srcX,
    int srcY,
    int srcW,
    int srcH,
    float hpRate,
    const float topColor[4],
    const float bottomColor[4])
{
    UiRect actualFill = fillRect;
    actualFill.width = static_cast<int>(actualFill.width * hpRate);

    if (actualFill.width > 0)
    {
        if (!shader.draw(actualFill, topColor, bottomColor))
        {
            DrawBox(actualFill.x, actualFill.y, actualFill.x + actualFill.width, actualFill.y + actualFill.height, GetColor(190, 30, 34), TRUE);
        }
    }

    drawFrame(frameHandle, frameRect, srcX, srcY, srcW, srcH);
}
}

void MutantScene::drawGameUi()
{
    /// 戦闘 UI に必要なフレーム画像を取得し、足りない場合は UI 描画をスキップする。
    int playerFrameHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::PlayerHpFrame);
    int enemyFrameHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::EnemyHpFrame);
    int staminaFrameHandle = ResourceManager::instance()->getGraph(MutantSceneConfig::StaminaFrame);
    int buffArrowHandle = ResourceManager::instance()->getGraph(BuffArrowGraph);
    if (playerFrameHandle == -1 || enemyFrameHandle == -1)
    {
        return;
    }

    HealthBarShader& healthBarShader = getHealthBarShader();
    const float playerTopColor[4] = { 0.28f, 1.0f, 0.34f, 1.0f };
    const float playerBottomColor[4] = { 0.02f, 0.36f, 0.08f, 1.0f };
    const float delayedHpTopColor[4] = { 1.0f, 0.18f, 0.16f, 1.0f };
    const float delayedHpBottomColor[4] = { 0.42f, 0.02f, 0.02f, 1.0f };
    const float enemyTopColor[4] = { 1.0f, 0.18f, 0.15f, 1.0f };
    const float enemyBottomColor[4] = { 0.36f, 0.0f, 0.02f, 1.0f };
    const float staminaTopColor[4] = { 0.95f, 0.88f, 0.24f, 1.0f };
    const float staminaBottomColor[4] = { 0.46f, 0.34f, 0.02f, 1.0f };

    float playerHpRate = clampRate(static_cast<float>(mPlayer->getHp()), static_cast<float>(PlayerParam::Hp));
    float playerDelayedHpRate = clampRate(mPlayerDelayedHp, static_cast<float>(PlayerParam::Hp));
    drawShaderHpBar(
        healthBarShader,
        playerFrameHandle,
        { 24, 18, 390, 56 },
        { 40, 36, 358, 30 },
        153,
        401,
        1264,
        181,
        playerDelayedHpRate,
        delayedHpTopColor,
        delayedHpBottomColor);
    drawShaderHpBar(
        healthBarShader,
        playerFrameHandle,
        { 24, 18, 390, 56 },
        { 40, 36, 358, 30 },
        153,
        401,
        1264,
        181,
        playerHpRate,
        playerTopColor,
        playerBottomColor);
    if (mPlayer->isPowerUpActive())
    {
        drawBuffArrow(buffArrowHandle, { 24, 18, 390, 56 });
    }


    if (mCompanion != nullptr)
    {
        float companionHpRate = clampRate(static_cast<float>(mCompanion->getHp()), static_cast<float>(PlayerParam::Hp));
        float companionDelayedHpRate = clampRate(mCompanionDelayedHp, static_cast<float>(PlayerParam::Hp));
        drawShaderHpBar(
            healthBarShader,
            playerFrameHandle,
            { 24, 250, 300, 44 },
            { 36, 264, 276, 22 },
            153,
            401,
            1264,
            181,
            companionDelayedHpRate,
            delayedHpTopColor,
            delayedHpBottomColor);
        drawShaderHpBar(
            healthBarShader,
            playerFrameHandle,
            { 24, 250, 300, 44 },
            { 36, 264, 276, 22 },
            153,
            401,
            1264,
            181,
            companionHpRate,
            playerTopColor,
            playerBottomColor);
        if (mCompanion->isPowerUpActive())
        {
            drawBuffArrow(buffArrowHandle, { 24, 250, 300, 44 });
        }
        DrawString(38, 228, mPlayerType == PlayerCharacterType::Knight ? "WIZARD" : "KNIGHT", GetColor(215, 225, 235));
    }
    if (staminaFrameHandle != -1)
    {
        float staminaRate = clampRate(mPlayer->getStamina(), PlayerParam::Stamina);
        drawShaderHpBar(
            healthBarShader,
            staminaFrameHandle,
            { 24, 70, 340, 48 },
            { 38, 86, 312, 24 },
            153,
            401,
            1264,
            181,
            staminaRate,
            staminaTopColor,
            staminaBottomColor);
    }

    if (mEnemies.empty())
    {
        return;
    }

    float revealRate = clampRate(mEnemyHpBarRevealTimer, 1.0f);
    if (revealRate <= 0.0f)
    {
        return;
    }

    const Enemy* bossEnemy = mEnemies.front().get();
    float enemyHpRate = clampRate(static_cast<float>(bossEnemy->getHp()), static_cast<float>(EnemyParam::Hp));
    drawShaderHpBarReveal(
        healthBarShader,
        enemyFrameHandle,
        { 230, 612, 820, 82 },
        { 300, 644, 700, 32 },
        51,
        333,
        1433,
        332,
        enemyHpRate,
        revealRate,
        enemyTopColor,
        enemyBottomColor);

    float shieldRevealRate = clampRate(mEnemyHpBarRevealTimer - 1.0f, 1.0f);
    if (bossEnemy->isBossShieldEnabled() && shieldRevealRate > 0.0f)
    {
        int shieldX = 300;
        int shieldY = 628;
        int shieldW = static_cast<int>(700.0f * shieldRevealRate);
        int shieldH = 10;
        float shieldRate = bossEnemy->getBossShieldRate() * shieldRevealRate;
        DrawBox(shieldX - 1, shieldY - 1, shieldX + shieldW + 1, shieldY + shieldH + 1, GetColor(18, 32, 62), TRUE);
        DrawBox(shieldX, shieldY, shieldX + shieldW, shieldY + shieldH, GetColor(7, 12, 26), TRUE);
        if (shieldRate > 0.0f)
        {
            DrawBox(shieldX, shieldY, shieldX + static_cast<int>(700.0f * shieldRate), shieldY + shieldH, GetColor(55, 170, 255), TRUE);
        }
    }
}

void MutantScene::releaseGameUi()
{
    delete gHealthBarShader;
    gHealthBarShader = nullptr;
}
