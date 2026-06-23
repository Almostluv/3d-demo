/**
 * @file
 * @brief AttackEffect の DirectX 11 描画処理を定義する。
 */

#include "attack_effect.h"
#include "attack_effect_internal.h"
#include "dx11_context.h"
#include "resource_manager.h"

#include <cstring>
#include <fstream>

using namespace AttackEffectInternal;

namespace
{
AttackEffect::Dx11Vertex makeDx11Vertex(VECTOR pos, COLOR_U8 color, float u = 0.0f, float v = 0.0f)
{
    /// DxLib の VECTOR/COLOR_U8 を、独自 DirectX 11 頂点バッファ用の形式へ詰め替える。
    AttackEffect::Dx11Vertex vertex;
    vertex.x = pos.x;
    vertex.y = pos.y;
    vertex.z = pos.z;
    vertex.r = color.r / 255.0f;
    vertex.g = color.g / 255.0f;
    vertex.b = color.b / 255.0f;
    vertex.a = color.a / 255.0f;
    vertex.u = u;
    vertex.v = v;
    return vertex;
}

std::vector<char> loadBinaryFile(const char* path)
{
    /// DirectX 11 シェーダーのコンパイル済みバイナリを丸ごと読み込む。
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

MATRIX multiplyMatrix(const MATRIX& a, const MATRIX& b)
{
    /// DxLib の MATRIX を使って、ワールド・ビュー・射影行列の合成を行う。
    MATRIX result = {};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            for (int k = 0; k < 4; ++k)
            {
                result.m[row][col] += a.m[row][k] * b.m[k][col];
            }
        }
    }

    return result;
}
}

bool AttackEffect::setupDx11()
{
    if (!Dx11Context::isAvailable())
    {
        /// DirectX 11 が使えない環境では DxLib 描画へフォールバックする。
        return false;
    }

    ID3D11Device* device = Dx11Context::device();
    std::vector<char> vsCode = loadBinaryFile("Resource/Shader/attack_effect_dx11_vs.cso");
    std::vector<char> psCode = loadBinaryFile("Resource/Shader/attack_effect_dx11_ps.cso");

    if (vsCode.empty() || psCode.empty())
    {
        /// シェーダーバイナリが無い場合もフォールバック描画で継続する。
        return false;
    }

    if (FAILED(device->CreateVertexShader(vsCode.data(), vsCode.size(), nullptr, &mDx11VertexShader)))
    {
        return false;
    }

    if (FAILED(device->CreatePixelShader(psCode.data(), psCode.size(), nullptr, &mDx11PixelShader)))
    {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        /// 頂点は位置、色、UV のみを渡し、行列と色味は定数バッファで渡す。
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if (FAILED(device->CreateInputLayout(layout, 3, vsCode.data(), vsCode.size(), &mDx11InputLayout)))
    {
        return false;
    }

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(Dx11Vertex) * 4096;
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&vertexBufferDesc, nullptr, &mDx11VertexBuffer)))
    {
        return false;
    }

    D3D11_BUFFER_DESC constantBufferDesc = {};
    /// 定数バッファは viewProj、params、tint の 3 つの float4 群だけを格納する。
    constantBufferDesc.ByteWidth = 96;
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(device->CreateBuffer(&constantBufferDesc, nullptr, &mDx11ConstantBuffer)))
    {
        return false;
    }

    D3D11_BLEND_DESC blendDesc = {};
    /// 攻撃エフェクトは発光表現が中心なので、加算合成用のブレンドステートを使う。
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    if (FAILED(device->CreateBlendState(&blendDesc, &mDx11BlendState)))
    {
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    /// 深度テストは行うが書き込まず、エフェクトが後続描画の Z を汚さないようにする。
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    if (FAILED(device->CreateDepthStencilState(&depthDesc, &mDx11DepthStencilState)))
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;

    if (FAILED(device->CreateRasterizerState(&rasterizerDesc, &mDx11RasterizerState)))
    {
        return false;
    }

    return true;
}
bool AttackEffect::isDx11Ready() const
{
    /// Dx11 描画に必要なリソースがすべて作成済みか確認する。
    return mDx11VertexShader != nullptr && mDx11PixelShader != nullptr && mDx11InputLayout != nullptr &&
        mDx11VertexBuffer != nullptr && mDx11ConstantBuffer != nullptr && mDx11BlendState != nullptr &&
        mDx11DepthStencilState != nullptr && mDx11RasterizerState != nullptr && Dx11Context::isAvailable();
}
void AttackEffect::releaseDx11()
{
    /// COM オブジェクトは生成と逆順に解放し、ポインタを null へ戻す。
    if (mDx11RasterizerState != nullptr)
    {
        mDx11RasterizerState->Release();
        mDx11RasterizerState = nullptr;
    }

    if (mDx11DepthStencilState != nullptr)
    {
        mDx11DepthStencilState->Release();
        mDx11DepthStencilState = nullptr;
    }

    if (mDx11BlendState != nullptr)
    {
        mDx11BlendState->Release();
        mDx11BlendState = nullptr;
    }

    if (mDx11ConstantBuffer != nullptr)
    {
        mDx11ConstantBuffer->Release();
        mDx11ConstantBuffer = nullptr;
    }

    if (mDx11VertexBuffer != nullptr)
    {
        mDx11VertexBuffer->Release();
        mDx11VertexBuffer = nullptr;
    }

    if (mDx11InputLayout != nullptr)
    {
        mDx11InputLayout->Release();
        mDx11InputLayout = nullptr;
    }

    if (mDx11PixelShader != nullptr)
    {
        mDx11PixelShader->Release();
        mDx11PixelShader = nullptr;
    }

    if (mDx11VertexShader != nullptr)
    {
        mDx11VertexShader->Release();
        mDx11VertexShader = nullptr;
    }
}

bool AttackEffect::drawDx11Batch(const std::vector<Dx11Vertex>& vertices, FLOAT4 params, FLOAT4 tint) const
{
    if (vertices.empty() || vertices.size() > 4096 || !Dx11Context::isAvailable())
    {
        return false;
    }

    /// 動的頂点バッファへ今回描く三角形リストを一括転送する。
    ID3D11DeviceContext* context = Dx11Context::context();
    D3D11_MAPPED_SUBRESOURCE mapped = {};

    if (FAILED(context->Map(mDx11VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return false;
    }

    memcpy(mapped.pData, vertices.data(), sizeof(Dx11Vertex) * vertices.size());
    context->Unmap(mDx11VertexBuffer, 0);

    struct ConstantBuffer
    {
        float viewProj[16];
        float params[4];
        float tint[4];
    };

    MATRIX view = GetCameraViewMatrix();
    MATRIX projection = GetCameraProjectionMatrix();
    /// DxLib の現在カメラ行列を使い、独自シェーダー側でも同じ視点に合わせる。
    MATRIX viewProj = multiplyMatrix(view, projection);

    ConstantBuffer constants = {};
    memcpy(constants.viewProj, viewProj.m, sizeof(constants.viewProj));
    constants.params[0] = params.x;
    constants.params[1] = params.y;
    constants.params[2] = params.z;
    constants.params[3] = params.w;
    constants.tint[0] = tint.x;
    constants.tint[1] = tint.y;
    constants.tint[2] = tint.z;
    constants.tint[3] = tint.w;

    if (FAILED(context->Map(mDx11ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return false;
    }

    memcpy(mapped.pData, &constants, sizeof(constants));
    context->Unmap(mDx11ConstantBuffer, 0);

    UINT stride = sizeof(Dx11Vertex);
    UINT offset = 0;
    /// このバッチ描画の間だけ、入力レイアウトとシェーダーをエフェクト用へ差し替える。
    context->IASetInputLayout(mDx11InputLayout);
    context->IASetVertexBuffers(0, 1, &mDx11VertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(mDx11VertexShader, nullptr, 0);
    context->PSSetShader(mDx11PixelShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &mDx11ConstantBuffer);
    context->PSSetConstantBuffers(0, 1, &mDx11ConstantBuffer);
    context->Draw(static_cast<UINT>(vertices.size()), 0);
    return true;
}

bool AttackEffect::drawDx11() const
{
    if (!isDx11Ready())
    {
        return false;
    }

    if (mSlashes.empty() && mImpacts.empty() && mSwordTrailSamples.empty())
    {
        return true;
    }

    /// DxLib 側の描画状態を壊さないよう、差し替える DirectX 11 state を退避する。
    ID3D11DeviceContext* context = Dx11Context::context();

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

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    /// ここから攻撃エフェクト用の加算合成・深度・ラスタライザ設定で描画する。
    context->OMSetBlendState(mDx11BlendState, blendFactor, 0xffffffff);
    context->OMSetDepthStencilState(mDx11DepthStencilState, 0);
    context->RSSetState(mDx11RasterizerState);

    drawSwordTrailDx11();

    for (const auto& slash : mSlashes)
    {
        /// 斬撃は扇形の帯を複数三角形で構成し、時間経過で薄くする。
        float rate = clamp01(slash.timer / slash.duration);
        float fade = 1.0f - rate;
        float stepScale = slash.attackStep == 3 ? 1.35f : 1.0f;
        float innerRadius = 6.0f * stepScale;
        float outerRadius = 24.0f * stepScale;
        float height = 7.0f + (slash.attackStep * 1.5f);
        float startAngle = -1.10f + (rate * 0.45f);
        float endAngle = 1.10f + (rate * 0.45f);
        int alpha = static_cast<int>(120.0f * fade);
        COLOR_U8 innerColor = GetColorU8(0, 45, 180, alpha / 2);
        COLOR_U8 outerColor = GetColorU8(15, 130, 255, alpha);
        FLOAT4 params = { slash.timer, fade, static_cast<float>(slash.attackStep), 0.0f };
        FLOAT4 tint = { 0.08f, 0.42f, 1.0f, 0.75f };
        std::vector<Dx11Vertex> vertices;
        vertices.reserve(SlashSegmentCount * 6);

        for (int i = 0; i < SlashSegmentCount; ++i)
        {
            float t0 = static_cast<float>(i) / SlashSegmentCount;
            float t1 = static_cast<float>(i + 1) / SlashSegmentCount;
            float a0 = startAngle + ((endAngle - startAngle) * t0);
            float a1 = startAngle + ((endAngle - startAngle) * t1);
            VECTOR i0 = VAdd(slash.pos, rotateLocal(sinf(a0) * innerRadius, cosf(a0) * innerRadius, slash.rotY));
            VECTOR o0 = VAdd(slash.pos, rotateLocal(sinf(a0) * outerRadius, cosf(a0) * outerRadius, slash.rotY));
            VECTOR i1 = VAdd(slash.pos, rotateLocal(sinf(a1) * innerRadius, cosf(a1) * innerRadius, slash.rotY));
            VECTOR o1 = VAdd(slash.pos, rotateLocal(sinf(a1) * outerRadius, cosf(a1) * outerRadius, slash.rotY));

            i0.y += height;
            o0.y += height + sinf(t0 * DX_PI_F) * 5.0f;
            i1.y += height;
            o1.y += height + sinf(t1 * DX_PI_F) * 5.0f;

            vertices.push_back(makeDx11Vertex(i0, innerColor, t0, 0.0f));
            vertices.push_back(makeDx11Vertex(o0, outerColor, t0, 1.0f));
            vertices.push_back(makeDx11Vertex(o1, outerColor, t1, 1.0f));
            vertices.push_back(makeDx11Vertex(i0, innerColor, t0, 0.0f));
            vertices.push_back(makeDx11Vertex(o1, outerColor, t1, 1.0f));
            vertices.push_back(makeDx11Vertex(i1, innerColor, t1, 0.0f));
        }

        drawDx11Batch(vertices, params, tint);
    }

    /// 退避した DirectX 11 state を戻し、以降の DxLib 描画へ影響を残さない。
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

    return true;
}
bool AttackEffect::drawSwordTrailDx11() const
{
    if (mSwordTrailSamples.size() < 2 || !isDx11Ready())
    {
        return true;
    }

    std::vector<Dx11Vertex> vertices;
    /// 剣軌跡は根元と先端のサンプル列を Catmull-Rom で補間して帯にする。
    vertices.reserve((mSwordTrailSamples.size() - 1) * SwordTrailSubdivisions * 6);

    for (std::size_t i = 0; i + 1 < mSwordTrailSamples.size(); ++i)
    {
        const SwordTrailSample& p0 = mSwordTrailSamples[i == 0 ? i : i - 1];
        const SwordTrailSample& p1 = mSwordTrailSamples[i];
        const SwordTrailSample& p2 = mSwordTrailSamples[i + 1];
        const SwordTrailSample& p3 = mSwordTrailSamples[i + 2 < mSwordTrailSamples.size() ? i + 2 : i + 1];

        for (int j = 0; j < SwordTrailSubdivisions; ++j)
        {
            float t0 = static_cast<float>(j) / SwordTrailSubdivisions;
            float t1 = static_cast<float>(j + 1) / SwordTrailSubdivisions;
            VECTOR base0 = catmullRomVector(p0.basePos, p1.basePos, p2.basePos, p3.basePos, t0);
            VECTOR tip0 = catmullRomVector(p0.tipPos, p1.tipPos, p2.tipPos, p3.tipPos, t0);
            VECTOR base1 = catmullRomVector(p0.basePos, p1.basePos, p2.basePos, p3.basePos, t1);
            VECTOR tip1 = catmullRomVector(p0.tipPos, p1.tipPos, p2.tipPos, p3.tipPos, t1);
            float ageRate0 = clamp01(lerpVector(VGet(p1.age, 0.0f, 0.0f), VGet(p2.age, 0.0f, 0.0f), t0).x / SwordTrailDuration);
            float ageRate1 = clamp01(lerpVector(VGet(p1.age, 0.0f, 0.0f), VGet(p2.age, 0.0f, 0.0f), t1).x / SwordTrailDuration);
            int alpha0 = static_cast<int>((1.0f - ageRate0) * 235.0f);
            int alpha1 = static_cast<int>((1.0f - ageRate1) * 235.0f);
            COLOR_U8 baseColor0 = GetColorU8(120, 190, 255, alpha0 / 5);
            COLOR_U8 tipColor0 = GetColorU8(220, 245, 255, alpha0);
            COLOR_U8 baseColor1 = GetColorU8(120, 190, 255, alpha1 / 5);
            COLOR_U8 tipColor1 = GetColorU8(220, 245, 255, alpha1);

            vertices.push_back(makeDx11Vertex(base0, baseColor0, 0.0f, ageRate0));
            vertices.push_back(makeDx11Vertex(tip0, tipColor0, 1.0f, ageRate0));
            vertices.push_back(makeDx11Vertex(tip1, tipColor1, 1.0f, ageRate1));
            vertices.push_back(makeDx11Vertex(base0, baseColor0, 0.0f, ageRate0));
            vertices.push_back(makeDx11Vertex(tip1, tipColor1, 1.0f, ageRate1));
            vertices.push_back(makeDx11Vertex(base1, baseColor1, 0.0f, ageRate1));
        }
    }

    if (vertices.empty())
    {
        return true;
    }

    for (std::size_t i = 1; i < mSwordTrailSamples.size(); ++i)
    {
        /// 先端側に薄い外側グローを追加し、軌跡の方向を読みやすくする。
        float ageRate = clamp01(mSwordTrailSamples[i].age / SwordTrailDuration);
        int alpha = static_cast<int>((1.0f - ageRate) * 185.0f);
        COLOR_U8 glowColor = GetColorU8(100, 175, 255, alpha);
        VECTOR prevTip = mSwordTrailSamples[i - 1].tipPos;
        VECTOR tip = mSwordTrailSamples[i].tipPos;
        VECTOR moveDir = VSub(tip, prevTip);
        if (VSize(moveDir) <= 0.0001f)
        {
            continue;
        }
        moveDir = VNorm(moveDir);
        VECTOR trailBack = VScale(moveDir, -1.0f);
        VECTOR lift = VGet(0.0f, 1.5f + 3.0f * (1.0f - ageRate), 0.0f);
        VECTOR outer0 = VAdd(prevTip, VAdd(lift, VScale(trailBack, 7.0f)));
        VECTOR outer1 = VAdd(tip, VAdd(lift, VScale(trailBack, 7.0f)));
        VECTOR inner0 = VAdd(prevTip, VAdd(lift, VScale(trailBack, 2.0f)));
        VECTOR inner1 = VAdd(tip, VAdd(lift, VScale(trailBack, 2.0f)));
        vertices.push_back(makeDx11Vertex(inner0, GetColorU8(80, 150, 255, alpha / 4), 0.0f, ageRate));
        vertices.push_back(makeDx11Vertex(outer0, glowColor, 1.0f, ageRate));
        vertices.push_back(makeDx11Vertex(outer1, glowColor, 1.0f, ageRate));
        vertices.push_back(makeDx11Vertex(inner0, GetColorU8(80, 150, 255, alpha / 4), 0.0f, ageRate));
        vertices.push_back(makeDx11Vertex(outer1, glowColor, 1.0f, ageRate));
        vertices.push_back(makeDx11Vertex(inner1, GetColorU8(80, 150, 255, alpha / 4), 0.0f, ageRate));
    }

    FLOAT4 params = { 0.0f, 1.0f, 0.0f, 0.0f };
    FLOAT4 tint = { 0.45f, 0.78f, 1.0f, 1.0f };
    return drawDx11Batch(vertices, params, tint);
}
