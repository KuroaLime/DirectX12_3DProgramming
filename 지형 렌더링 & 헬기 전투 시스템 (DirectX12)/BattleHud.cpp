#include "BattleScene.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float radarRange = 1500.0f;
    constexpr float radarX = 109.0f;
    constexpr float radarY = 850.0f;
    // Leave the border and half a contact symbol outside the plotting area.
    constexpr float radarPlotRadius = 52.0f;

    XMFLOAT4 PanelRegion(float x, float y, float w, float h)
    {
        return XMFLOAT4(x / 1024.0f, y / 512.0f, w / 1024.0f, h / 512.0f);
    }

    bool IsLiveEnemy(CGameObject* object)
    {
        return object && object->GetLayer() == EObjectLayer::Enemy &&
            object->m_bAlive && object->GetHealth() > 0.0f && object->IsCollidable();
    }
}

CMaterial* CBattleScene::CreateHudMaterial(ID3D12Device* device, ID3D12GraphicsCommandList* commands,
    const wchar_t* texturePath, CShader* shader)
{
    CTexture* texture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
    texture->LoadTextureFromDDSFile(device, commands, const_cast<wchar_t*>(texturePath), RESOURCE_TEXTURE2D, 0);
    CreateShaderResourceView(device, texture, 0, 3);
    CMaterial* material = new CMaterial();
    material->SetShader(shader);
    material->SetTexture(texture);
    material->SetMaterialType(MATERIAL_ALBEDO_MAP);
    return material;
}

CGameObject* CBattleScene::CreateHudQuad(ID3D12Device* device, ID3D12GraphicsCommandList* commands,
    const char* name, CMaterial* material, float width, float height, float centerX, float centerYFromTop,
    const XMFLOAT4& meshUv)
{
    CGameObject* quad = new CGameObject(1, 1);
    quad->SetOwnerScene(this);
    quad->m_bSkipFrustum = true;
    strcpy_s(quad->m_pstrFrameName, name);
    quad->SetMaterial(0, material);
    quad->SetMesh(0, new CTexturedRectMesh(device, commands, width, height, 0, 0, 0, 1, meshUv));
    quad->SetPosition(centerX, FRAME_BUFFER_HEIGHT - centerYFromTop, 0);
    quad->SetTextureTransformParams(XMFLOAT4(0, 0, 1, 1));
    quad->CreateShaderVariables(device, commands);
    m_vpUIObjects.push_back(quad);
    return quad;
}

void CBattleScene::SetHudNumber(CGameObject* const* digits, size_t count, int value, bool leadingZeros)
{
    for (size_t i = count; i > 0; --i)
    {
        CGameObject* digit = digits[i - 1];
        digit->bRender = leadingZeros || value > 0 || i == count;
        digit->SetTextureTransformParams(XMFLOAT4((value % 10) * 0.1f, 0, 0.1f, 1));
        value /= 10;
    }
}

CGameObject* CBattleScene::GetLiveLockTarget() const
{
    return IsLiveEnemy(m_pCurrentLockOnTarget) ? m_pCurrentLockOnTarget : nullptr;
}

void CBattleScene::BuildUIObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commands)
{
    m_pUIShader = new CUIShader();
    m_pUIShader->AddRef();
    m_pUIShader->CreateShader(device, commands, m_pd3dGraphicsRootSignature);
    m_pTextureAtlasShader = new CUITextureAtlasShader();
    m_pTextureAtlasShader->AddRef();
    m_pTextureAtlasShader->CreateShader(device, commands, m_pd3dGraphicsRootSignature);

    m_pUIMaterial = CreateHudMaterial(device, commands, L"Image/UI/HPBar01.dds", m_pUIShader);
    m_pUIMaterial->AddRef();
    m_pScoreMaterial = CreateHudMaterial(device, commands, L"Image/UI/Font.dds", m_pTextureAtlasShader);
    m_pScoreMaterial->AddRef();

    BuildTargetDisplay(device, commands);
    BuildFlightDisplay(device, commands);

    CreateHudQuad(device, commands, "RadarPanel", m_pUIMaterial, 354, 160, 207, 850,
        PanelRegion(0, 0, 708, 320));
    m_pRadarSweep = CreateHudQuad(device, commands, "RadarSweep", m_pUIMaterial, 116, 116, radarX, radarY,
        PanelRegion(736, 0, 232, 232));

    // One marker per scene-owned enemy. All markers share a texture and never allocate during flight.
    CMaterial* markerMaterial = nullptr;
    for (int i = 0; i < m_nShaders; ++i)
    {
        auto* squadron = dynamic_cast<CBattleShader*>(m_ppShaders[i]);
        if (!squadron) continue;
        for (int j = 0; j < squadron->GetNumberOfObjects(); ++j)
        {
            CGameObject* enemy = squadron->GetObject(j);
            if (!enemy || enemy->GetLayer() != EObjectLayer::Enemy) continue;
            if (!markerMaterial)
            {
                markerMaterial = new CMaterial();
                markerMaterial->SetShader(m_pTextureAtlasShader);
                markerMaterial->SetTexture(m_pUIMaterial->m_pTexture);
                markerMaterial->SetMaterialType(MATERIAL_ALBEDO_MAP);
            }
            CGameObject* marker = CreateHudQuad(device, commands, "RadarContact", markerMaterial, 12, 12, radarX, radarY);
            marker->bRender = false;
            m_radarContacts.push_back({ enemy, marker });
        }
    }

    auto* fillShader = new CHealthBarShader();
    fillShader->CreateShader(device, commands, m_pd3dGraphicsRootSignature);
    CMaterial* fillMaterial = new CMaterial();
    fillMaterial->SetShader(fillShader);
    m_pHullFill = CreateHudQuad(device, commands, "HullFill", fillMaterial, 178, 11, 274, 869.5f);
    for (int i = 0; i < 3; ++i)
        m_hullDigits[i] = CreateHudQuad(device, commands, "HullDigit", m_pScoreMaterial,
            18, 40.0f * 18.0f / 26.0f, 194.0f + 18 * i, 898);

    CreateHudQuad(device, commands, "ScorePanel", m_pUIMaterial, 232, 87, 1134, 886.5f,
        PanelRegion(0, 336, 464, 174));
    for (int i = 0; i < 6; ++i)
        m_scoreDigits[i] = CreateHudQuad(device, commands, "ScoreDigit", m_pScoreMaterial,
            26, 40, 1061.0f + i * 29, 898);
    SetHudNumber(m_scoreDigits, _countof(m_scoreDigits), m_nScore, true);
}

void CBattleScene::UpdateBattleHud(float elapsed)
{
    m_fHudTime += elapsed;
    SetHudNumber(m_scoreDigits, _countof(m_scoreDigits), m_nScore, true);
    UpdateFlightDisplay();
    if (!m_pPlayer) return;

    const float maxHealth = m_pPlayer->GetMaxHealth();
    const float health = maxHealth > 0 ? std::min(1.0f, std::max(0.0f, m_pPlayer->GetHealth() / maxHealth)) : 0;
    m_pHullFill->SetTextureTransformParams(XMFLOAT4(health, 0, 0, 0));
    m_pHullFill->m_ppMaterials[0]->m_xmf4AlbedoColor = health <= 0.25f
        ? XMFLOAT4(0.97f, 0.61f, 0.31f, 1) : XMFLOAT4(0.54f, 0.92f, 0.82f, 1);
    SetHudNumber(m_hullDigits, _countof(m_hullDigits), int(std::lround(health * 100)), false);

    // Clockwise on the screen, with the texture pivot at the centre of the dial.
    const float angle = -std::fmod(m_fHudTime, 4.0f) * XM_PIDIV2;
    XMStoreFloat4x4(&m_pRadarSweep->m_xmf4x4Transform, XMMatrixRotationZ(angle) *
        XMMatrixTranslation(radarX, FRAME_BUFFER_HEIGHT - radarY, 0));
    m_pRadarSweep->UpdateTransform(nullptr);

    const XMFLOAT3 position = m_pPlayer->GetPosition();
    const XMFLOAT3 look = m_pPlayer->GetLookVector();
    const float heading = std::atan2(look.x, look.z);
    const float sinHeading = std::sin(heading);
    const float cosHeading = std::cos(heading);
    const auto* locked = GetLiveLockTarget();
    const float scale = radarPlotRadius / radarRange;
    for (const RadarContact& contact : m_radarContacts)
    {
        CGameObject* marker = contact.marker;
        const XMFLOAT3 enemy = contact.enemy->GetPosition();
        const float dx = enemy.x - position.x;
        const float dz = enemy.z - position.z;
        marker->bRender = IsLiveEnemy(contact.enemy) && dx * dx + dz * dz <= radarRange * radarRange;
        if (!marker->bRender) continue;
        const float right = dx * cosHeading - dz * sinHeading;
        const float forward = dx * sinHeading + dz * cosHeading;
        marker->SetPosition(radarX + right * scale, FRAME_BUFFER_HEIGHT - radarY + forward * scale, 0);
        marker->SetTextureTransformParams(PanelRegion(contact.enemy == locked ? 768.0f : 736.0f, 264, 24, 24));
    }
}
