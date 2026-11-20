#include "BattleScene.h"

#include <algorithm>
#include <cmath>
#include <cfloat>

namespace
{
    constexpr float kHudWidth = 1280.0f;
    constexpr float kHudHeight = 960.0f;
    constexpr float kCornerSize = 32.0f;
    constexpr float kAcquireSeconds = 0.22f;

    bool ProjectTargetBounds(CGameObject* target, CCamera* camera, XMFLOAT4& rectangle)
    {
        const XMFLOAT4X4 world = target->GetWorldMatrix();
        BoundingOrientedBox bounds;
        target->m_xmBigOOGG.Transform(bounds, XMLoadFloat4x4(&world));
        if (bounds.Extents.x <= 0.0f && bounds.Extents.y <= 0.0f && bounds.Extents.z <= 0.0f)
            bounds = target->m_xmWorldOOBB;

        const XMFLOAT4X4 view = camera->GetViewMatrix();
        const XMFLOAT4X4 projection = camera->GetProjectionMatrix();
        const XMMATRIX viewProjection = XMLoadFloat4x4(&view) * XMLoadFloat4x4(&projection);
        XMFLOAT4 centerClip;
        XMStoreFloat4(&centerClip, XMVector4Transform(
            XMVectorSet(bounds.Center.x, bounds.Center.y, bounds.Center.z, 1.0f), viewProjection));
        // Test the real projected center. An on-screen target may have corners outside the viewport.
        if (!std::isfinite(centerClip.x) || !std::isfinite(centerClip.y) ||
            !std::isfinite(centerClip.z) || !std::isfinite(centerClip.w) ||
            centerClip.w <= 0.001f || centerClip.z < 0.0f || centerClip.z > centerClip.w ||
            std::fabs(centerClip.x) > centerClip.w || std::fabs(centerClip.y) > centerClip.w)
            return false;
        XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT];
        bounds.GetCorners(corners);
        float left = FLT_MAX, top = FLT_MAX, right = -FLT_MAX, bottom = -FLT_MAX;
        for (const XMFLOAT3& corner : corners)
        {
            XMFLOAT4 clip;
            XMStoreFloat4(&clip, XMVector4Transform(XMVectorSet(corner.x, corner.y, corner.z, 1.0f), viewProjection));
            // Reject near-plane intersections and targets behind the camera; never pin them to an edge.
            if (!std::isfinite(clip.x) || !std::isfinite(clip.y) || !std::isfinite(clip.z) ||
                !std::isfinite(clip.w) || clip.w <= 0.001f || clip.z < 0.0f || clip.z > clip.w)
                return false;
            const float x = (clip.x / clip.w * 0.5f + 0.5f) * kHudWidth;
            const float y = (0.5f - clip.y / clip.w * 0.5f) * kHudHeight;
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x);
            bottom = std::max(bottom, y);
        }
        rectangle = XMFLOAT4(left, top, right, bottom);
        return true;
    }
}

void CBattleScene::BuildTargetDisplay(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    CMaterial* reticle = CreateHudMaterial(device, commandList, L"Image/UI/HiarCross.dds", m_pUIShader);
    m_pAimReticle = CreateHudQuad(device, commandList, "AimReticle", reticle,
        160.0f, 160.0f, kHudWidth * 0.5f, kHudHeight * 0.5f);

    CMaterial* designation = CreateHudMaterial(device, commandList, L"Image/UI/TargetDesignation.dds", m_pUIShader);
    for (int corner = 0; corner < 4; ++corner)
    {
        const XMFLOAT4 uv((corner * 64.0f + 0.5f) / 256.0f, 0.5f / 128.0f,
            63.0f / 256.0f, 63.0f / 128.0f);
        m_pTargetCorners[corner] = CreateHudQuad(device, commandList, "TargetCorner", designation,
            kCornerSize, kCornerSize, 0.0f, 0.0f, uv);
        m_pTargetCorners[corner]->bRender = false;
    }
    m_pTargetLockCue = CreateHudQuad(device, commandList, "TargetLockCue", designation,
        128.0f, 32.0f, kHudWidth * 0.5f, kHudHeight * 0.5f + 75.0f,
        XMFLOAT4(0.5f / 256.0f, 64.5f / 128.0f, 255.0f / 256.0f, 63.0f / 128.0f));
    m_pTargetLockCue->bRender = false;
    m_pTargetDisplayTracked = nullptr;
    m_fTargetAcquiredAt = 0.0f;
}

void CBattleScene::UpdateTargetDisplay(CCamera* camera)
{
    if (m_pAimReticle) m_pAimReticle->bRender = camera && camera->GetMode() != THIRD_PERSON_CAMERA;
    for (CGameObject* corner : m_pTargetCorners) if (corner) corner->bRender = false;
    if (m_pTargetLockCue) m_pTargetLockCue->bRender = false;

    CGameObject* target = GetLiveLockTarget();
    // A live selected target stays locked even while it is outside the view.
    // This is a status cue only; the tracked corners are never moved to a screen edge.
    if (m_pTargetLockCue) m_pTargetLockCue->bRender = camera && target;
    XMFLOAT4 rectangle;
    if (!camera || !target || !m_pTargetLockCue || !ProjectTargetBounds(target, camera, rectangle))
    {
        m_pTargetDisplayTracked = nullptr;
        return;
    }

    const float centerX = (rectangle.x + rectangle.z) * 0.5f;
    const float centerY = (rectangle.y + rectangle.w) * 0.5f;
    const float halfWidth = std::max(30.0f, (rectangle.z - rectangle.x) * 0.5f + 10.0f);
    const float halfHeight = std::max(22.0f, (rectangle.w - rectangle.y) * 0.5f + 10.0f);
    if (target != m_pTargetDisplayTracked)
    {
        m_pTargetDisplayTracked = target;
        m_fTargetAcquiredAt = m_fHudTime;
    }
    const float progress = std::max(0.0f, std::min(1.0f, (m_fHudTime - m_fTargetAcquiredAt) / kAcquireSeconds));
    const float eased = progress * progress * (3.0f - 2.0f * progress);
    // Keep the true projected bounds; the hardware viewport clips any overflowing corner pixels.
    const float padding = 12.0f * (1.0f - eased);
    const float left = centerX - halfWidth - padding;
    const float right = centerX + halfWidth + padding;
    const float top = centerY - halfHeight - padding;
    const float bottom = centerY + halfHeight + padding;
    // Each 32 px tile's visible corner is 4 px from its outer edge.
    const XMFLOAT2 centers[4] = {
        { left + 12.0f, top + 12.0f }, { right - 12.0f, top + 12.0f },
        { left + 12.0f, bottom - 12.0f }, { right - 12.0f, bottom - 12.0f }
    };
    for (int corner = 0; corner < 4; ++corner)
    {
        if (!m_pTargetCorners[corner]) continue;
        m_pTargetCorners[corner]->SetPosition(centers[corner].x, kHudHeight - centers[corner].y, 0.0f);
        m_pTargetCorners[corner]->bRender = true;
    }
    m_pTargetLockCue->bRender = true;
}
