#include "stdafx.h"
#include "BattleScene.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kDashGlyph = 10;
    constexpr int kBlankGlyph = 11;
    constexpr float kGlyphCount = 12.0f;

    void SetFlightGlyph(CGameObject* digit, int glyph)
    {
        if (digit) digit->SetTextureTransformParams(
            XMFLOAT4(static_cast<float>(glyph) / kGlyphCount, 0.0f, 1.0f / kGlyphCount, 1.0f));
    }

    void SetFlightUnavailable(CGameObject* const* digits, int count)
    {
        for (int i = 0; i < count; ++i) SetFlightGlyph(digits[i], kDashGlyph);
    }

    void SetFlightNumber(CGameObject* const* digits, int count, int value, bool leadingZeros)
    {
        const bool negative = value < 0;
        int magnitude = negative ? -value : value;
        for (int i = count - 1; i >= 0; --i)
        {
            int glyph = kBlankGlyph;
            if (magnitude || i == count - 1 || leadingZeros)
            {
                glyph = magnitude % 10;
                magnitude /= 10;
            }
            SetFlightGlyph(digits[i], glyph);
        }
        if (negative)
        {
            int signIndex = count - 2;
            for (int remaining = -value; remaining >= 10; remaining /= 10) --signIndex;
            SetFlightGlyph(digits[signIndex], kDashGlyph);
        }
    }
}

void CBattleScene::BuildFlightDisplay(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    CMaterial* panelMaterial = CreateHudMaterial(device, commandList,
        L"Image/UI/FlightPanel.dds", m_pUIShader);
    CreateHudQuad(device, commandList, "FlightPanel", panelMaterial,
        680.0f, 88.0f, 640.0f, 67.0f);

    CMaterial* tapeMaterial = CreateHudMaterial(device, commandList,
        L"Image/UI/FlightTape.dds", m_pTextureAtlasShader);
    m_pHeadingTape = CreateHudQuad(device, commandList, "HeadingTape", tapeMaterial,
        412.0f, 38.0f, 640.0f, 85.0f);
    // The atlas has transparent outer rows. Hide telemetry until a player exists.
    m_pHeadingTape->SetTextureTransformParams(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));

    CMaterial* digitsMaterial = CreateHudMaterial(device, commandList,
        L"Image/UI/FlightDigits.dds", m_pTextureAtlasShader);
    for (int i = 0; i < 3; ++i)
    {
        m_ppHeadingDigits[i] = CreateHudQuad(device, commandList, "HeadingDigit", digitsMaterial,
            22.0f, 28.0f, 618.0f + 22.0f * i, 43.0f);
        m_ppSpeedDigits[i] = CreateHudQuad(device, commandList, "SpeedDigit", digitsMaterial,
            22.0f, 28.0f, 333.0f + 22.0f * i, 72.0f);
    }
    for (int i = 0; i < 4; ++i)
    {
        m_ppHeightDigits[i] = CreateHudQuad(device, commandList, "HeightDigit", digitsMaterial,
            20.0f, 28.0f, 870.0f + 20.0f * i, 72.0f);
    }
    SetFlightUnavailable(m_ppHeadingDigits, 3);
    SetFlightUnavailable(m_ppSpeedDigits, 3);
    SetFlightUnavailable(m_ppHeightDigits, 4);
}

void CBattleScene::UpdateFlightDisplay()
{
    if (!m_pHeadingTape) return;
    if (!m_pPlayer)
    {
        m_pHeadingTape->SetTextureTransformParams(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
        SetFlightUnavailable(m_ppHeadingDigits, 3);
        SetFlightUnavailable(m_ppSpeedDigits, 3);
        SetFlightUnavailable(m_ppHeightDigits, 4);
        return;
    }

    const XMFLOAT3 look = m_pPlayer->GetLookVector();
    const float horizontalLengthSquared = look.x * look.x + look.z * look.z;
    if (std::isfinite(horizontalLengthSquared) && horizontalLengthSquared > 1.0e-8f)
    {
        // World +Z is north and +X is east. Looking east slides the tape left.
        const float heading = std::fmod(std::atan2(look.x, look.z) * (180.0f / XM_PI) + 360.0f, 360.0f);
        const int roundedHeading = static_cast<int>(std::floor(heading + 0.5f)) % 360;
        SetFlightNumber(m_ppHeadingDigits, 3, roundedHeading, true);
        // The fixed quad shows 103 degrees. Its source covers -60..420 degrees,
        // so duplicate marks on both ends keep 359 -> 0 visually continuous.
        m_pHeadingTape->SetTextureTransformParams(XMFLOAT4((heading + 8.5f) / 480.0f,
            0.0f, 103.0f / 480.0f, 1.0f));
    }
    else
    {
        SetFlightUnavailable(m_ppHeadingDigits, 3);
        m_pHeadingTape->SetTextureTransformParams(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    // Velocity and terrain use game world units; no unsupported km/h or metres conversion.
    const float speed = m_pPlayer->GetSpeed();
    if (std::isfinite(speed) && speed >= 0.0f && speed < 999.5f)
        SetFlightNumber(m_ppSpeedDigits, 3, static_cast<int>(std::floor(speed + 0.5f)), false);
    else
        SetFlightUnavailable(m_ppSpeedDigits, 3);

    SetFlightUnavailable(m_ppHeightDigits, 4);
    if (!m_pTerrain) return;
    const XMFLOAT3 position = m_pPlayer->GetPosition();
    const XMFLOAT3 terrainPosition = m_pTerrain->GetPosition();
    const XMFLOAT3 scale = m_pTerrain->GetScale();
    const int mapWidth = m_pTerrain->GetRawImageWidth();
    const int mapLength = m_pTerrain->GetRawImageLength();
    if (mapWidth < 2 || mapLength < 2 || scale.x <= 0.0f || scale.z <= 0.0f) return;
    const float x = position.x - terrainPosition.x;
    const float z = position.z - terrainPosition.z;
    const float maxX = static_cast<float>(mapWidth - 1) * scale.x;
    const float maxZ = static_cast<float>(mapLength - 1) * scale.z;
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(position.y) ||
        x < 0.0f || z < 0.0f || x > maxX || z > maxZ) return;

    // GetHeight interpolates four samples, including x+1 and z+1. The final
    // vertex is sampled from inside its last valid cell; never query outside it.
    const float sampleX = std::min(x, std::nextafter(maxX, 0.0f));
    const float sampleZ = std::min(z, std::nextafter(maxZ, 0.0f));
    const float groundY = terrainPosition.y + m_pTerrain->GetHeight(sampleX, sampleZ);
    const float clearance = position.y - groundY;
    if (std::isfinite(clearance) && clearance > -999.5f && clearance < 9999.5f)
        SetFlightNumber(m_ppHeightDigits, 4, static_cast<int>(std::round(clearance)), false);
}
