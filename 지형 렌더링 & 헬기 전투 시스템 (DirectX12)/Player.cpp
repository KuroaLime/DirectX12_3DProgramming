//-----------------------------------------------------------------------------
// File: CPlayer.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Player.h"
#include "Shader.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPlayer

CPlayer::CPlayer()
{
	//m_bSkipFrustum = true;
	m_pCamera = NULL;

	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fMaxVelocityXZ = 0.0f;
	m_fMaxVelocityY = 0.0f;
	m_fFriction = 0.0f;

	m_fPitch = 0.0f;
	m_fRoll = 0.0f;
	m_fYaw = 0.0f;

	m_pPlayerUpdatedContext = NULL;
	m_pCameraUpdatedContext = NULL;
	m_nCbvRootParameterIndex = 1;

	m_xmLocalOOBB = BoundingOrientedBox();
	SetLayer(EObjectLayer::Player);
}

CPlayer::~CPlayer()
{
	ReleaseShaderVariables();

	if (m_pCamera) delete m_pCamera;

	if (m_pShader) m_pShader->Release();
}

void CPlayer::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pCamera) m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CPlayer::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
}

void CPlayer::ReleaseShaderVariables()
{
	if (m_pCamera) m_pCamera->ReleaseShaderVariables();
}

void CPlayer::Move(DWORD dwDirection, float fDistance, bool bUpdateVelocity)
{
    XMFLOAT3 direction(0.0f, 0.0f, 0.0f);
    if (dwDirection & DIR_FORWARD) direction = Vector3::Add(direction, m_xmf3Look);
    if (dwDirection & DIR_BACKWARD) direction = Vector3::Add(direction, m_xmf3Look, -1.0f);
    if (dwDirection & DIR_RIGHT) direction = Vector3::Add(direction, m_xmf3Right);
    if (dwDirection & DIR_LEFT) direction = Vector3::Add(direction, m_xmf3Right, -1.0f);
    if (dwDirection & DIR_UP) direction = Vector3::Add(direction, m_xmf3Up);
    if (dwDirection & DIR_DOWN) direction = Vector3::Add(direction, m_xmf3Up, -1.0f);

    // Multiple held axes share one acceleration budget.
    if (Vector3::Length(direction) > 0.0f)
        Move(Vector3::ScalarProduct(direction, fDistance, true), bUpdateVelocity);
}

void CPlayer::Move(const XMFLOAT3& xmf3Shift, bool bUpdateVelocity)
{
    if (bUpdateVelocity)
    {
        m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Shift);
    }
    else
    {
        m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);
        CGameObject::SetPosition(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);
    }
}

void CPlayer::Rotate(float x, float y, float z)
{
	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	if ((nCurrentCameraMode == FIRST_PERSON_CAMERA) || (nCurrentCameraMode == THIRD_PERSON_CAMERA))
	{
		if (x != 0.0f)
		{
			m_fPitch += x;
			if (m_fPitch > +89.0f) { x -= (m_fPitch - 89.0f); m_fPitch = +89.0f; }
			if (m_fPitch < -89.0f) { x -= (m_fPitch + 89.0f); m_fPitch = -89.0f; }
		}
		if (y != 0.0f)
		{
			m_fYaw += y;
			if (m_fYaw > 360.0f) m_fYaw -= 360.0f;
			if (m_fYaw < 0.0f) m_fYaw += 360.0f;
		}
		if (z != 0.0f)
		{
			m_fRoll += z;
			if (m_fRoll > +20.0f) { z -= (m_fRoll - 20.0f); m_fRoll = +20.0f; }
			if (m_fRoll < -20.0f) { z -= (m_fRoll + 20.0f); m_fRoll = -20.0f; }
		}
		m_pCamera->Rotate(x, y, z);

		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}
	else if (nCurrentCameraMode == SPACESHIP_CAMERA)
	{
		m_pCamera->Rotate(x, y, z);
		if (x != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		}
		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
		if (z != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(z));
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}

	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);
	m_xmf3Up = Vector3::CrossProduct(m_xmf3Look, m_xmf3Right, true);
}

void CPlayer::Update(float fTimeElapsed)
{
	if (fTimeElapsed <= 0.0f) return;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, m_xmf3Gravity, fTimeElapsed);
	float fLength = sqrtf(m_xmf3Velocity.x * m_xmf3Velocity.x + m_xmf3Velocity.z * m_xmf3Velocity.z);
	float fMaxVelocityXZ = m_fMaxVelocityXZ;
	if (fLength > m_fMaxVelocityXZ)
	{
		m_xmf3Velocity.x *= (fMaxVelocityXZ / fLength);
		m_xmf3Velocity.z *= (fMaxVelocityXZ / fLength);
	}
	float fMaxVelocityY = m_fMaxVelocityY;
	fLength = sqrtf(m_xmf3Velocity.y * m_xmf3Velocity.y);
	if (fLength > m_fMaxVelocityY) m_xmf3Velocity.y *= (fMaxVelocityY / fLength);

	XMFLOAT3 xmf3Velocity = Vector3::ScalarProduct(m_xmf3Velocity, fTimeElapsed, false);
	Move(xmf3Velocity, false);

	if (m_pPlayerUpdatedContext) OnPlayerUpdateCallback(fTimeElapsed);
	OnPrepareRender();

	// Camera placement is finalized after scene collision resolution.

	fLength = Vector3::Length(m_xmf3Velocity);

	// 입력이 있으면 기존 마찰(m_fFriction), 없으면 정지 마찰(m_fStopFriction)
	float friction = m_fFriction;
	if (!m_bMoveInput && (m_fStopFriction > 0.0f)) friction = m_fStopFriction;

	float fDeceleration = (friction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;

	m_xmf3Velocity = Vector3::Add(
		m_xmf3Velocity,
		Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true)
	);

}

CCamera *CPlayer::OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode)
{
	CCamera *pNewCamera = NULL;
	switch (nNewCameraMode)
	{
		case FIRST_PERSON_CAMERA:
			pNewCamera = new CFirstPersonCamera(m_pCamera);
			break;
		case THIRD_PERSON_CAMERA:
			pNewCamera = new CThirdPersonCamera(m_pCamera);
			break;
		case SPACESHIP_CAMERA:
			pNewCamera = new CSpaceShipCamera(m_pCamera);
			break;
	}
	if (nCurrentCameraMode == SPACESHIP_CAMERA)
	{
		m_xmf3Right = Vector3::Normalize(XMFLOAT3(m_xmf3Right.x, 0.0f, m_xmf3Right.z));
		m_xmf3Up = Vector3::Normalize(XMFLOAT3(0.0f, 1.0f, 0.0f));
		m_xmf3Look = Vector3::Normalize(XMFLOAT3(m_xmf3Look.x, 0.0f, m_xmf3Look.z));

		m_fPitch = 0.0f;
		m_fRoll = 0.0f;
		m_fYaw = Vector3::Angle(XMFLOAT3(0.0f, 0.0f, 1.0f), m_xmf3Look);
		if (m_xmf3Look.x < 0.0f) m_fYaw = -m_fYaw;
	}
	else if ((nNewCameraMode == SPACESHIP_CAMERA) && m_pCamera)
	{
		m_xmf3Right = m_pCamera->GetRightVector();
		m_xmf3Up = m_pCamera->GetUpVector();
		m_xmf3Look = m_pCamera->GetLookVector();
	}

	if (pNewCamera)
	{
		pNewCamera->SetMode(nNewCameraMode);
		pNewCamera->SetPlayer(this);
	}

	if (m_pCamera) delete m_pCamera;

	return(pNewCamera);
}

void CPlayer::OnPrepareRender()
{
	m_xmf4x4Transform._11 = m_xmf3Right.x; m_xmf4x4Transform._12 = m_xmf3Right.y; m_xmf4x4Transform._13 = m_xmf3Right.z;
	m_xmf4x4Transform._21 = m_xmf3Up.x; m_xmf4x4Transform._22 = m_xmf3Up.y; m_xmf4x4Transform._23 = m_xmf3Up.z;
	m_xmf4x4Transform._31 = m_xmf3Look.x; m_xmf4x4Transform._32 = m_xmf3Look.y; m_xmf4x4Transform._33 = m_xmf3Look.z;
	m_xmf4x4Transform._41 = m_xmf3Position.x; m_xmf4x4Transform._42 = m_xmf3Position.y; m_xmf4x4Transform._43 = m_xmf3Position.z;

	UpdateTransform(NULL);
}
void CPlayer::SetPosition(float x, float y, float z)
{
    m_xmf3Position = XMFLOAT3(x, y, z);
    CGameObject::SetPosition(x, y, z);
}

void CPlayer::SetPosition(XMFLOAT3 xmf3Position)
{
    SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

void CPlayer::UpdateCamera(float fTimeElapsed, bool bSnap)
{
    if (!m_pCamera) return;

    if (m_pCamera->GetMode() == THIRD_PERSON_CAMERA && !bSnap)
    {
        m_pCamera->Update(m_xmf3Position, fTimeElapsed);
    }
    else
    {
        const XMFLOAT3 offset = m_pCamera->GetOffset();
        XMFLOAT3 position = Vector3::Add(m_xmf3Position, m_xmf3Right, offset.x);
        position = Vector3::Add(position, m_xmf3Up, offset.y);
        position = Vector3::Add(position, m_xmf3Look, offset.z);
        m_pCamera->SetPosition(position);
        if (m_pCamera->GetMode() == THIRD_PERSON_CAMERA)
            m_pCamera->SetLookAt(m_xmf3Position);
    }

    if (m_pCameraUpdatedContext) OnCameraUpdateCallback(fTimeElapsed);
    m_pCamera->RegenerateViewMatrix();
    m_pCamera->UpdateFrustum();
}

void CPlayer::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	DWORD nCameraMode = (pCamera) ? pCamera->GetMode() : 0x00;

	// 1) 거울 반사용 패스(파이프라인 인덱스 강제 지정되어 들어오는 경우)는 그대로 사용
	if (nPipelineState == PLAYER_PSO_REFLECTION)
	{
		if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera, nPipelineState);
		CGameObject::Render(pd3dCommandList, pCamera, nPipelineState);
		return;
	}
	if (nPipelineState == PLAYER_PSO_OVERLAY)
	{
		m_pShader->Render(pd3dCommandList, pCamera, PLAYER_PSO_OVERLAY);
		CGameObject::Render(pd3dCommandList, pCamera, PLAYER_PSO_OVERLAY);
		return;
	}
	// 2) 일반 렌더(세계) – 카메라 모드에 따라 PSO 선택
	if (nCameraMode == THIRD_PERSON_CAMERA)
	{
		// 기존 그대로: 월드 안에 있는 플레이어 모델
		if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera, PLAYER_PSO_DEFAULT);
		CGameObject::Render(pd3dCommandList, pCamera, PLAYER_PSO_DEFAULT);
	}
	else if (nCameraMode == FIRST_PERSON_CAMERA)
	{
		// 1인칭: OM 블렌딩용 PSO로 렌더
		if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera, PLAYER_PSO_FIRSTPERSON);
		CGameObject::Render(pd3dCommandList, pCamera, PLAYER_PSO_FIRSTPERSON);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAirplanePlayer

CAirplanePlayer::CAirplanePlayer(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CScene* OwnerScene)
{
	//m_pCamera = ChangeCamera(/*SPACESHIP_CAMERA*/THIRD_PERSON_CAMERA, 0.0f);
	m_pCamera = ChangeCamera(FIRST_PERSON_CAMERA, 0.0f);

	m_pShader = new CPlayerShader();
	m_pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);

	CGameObject *pGameObject = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Mi24.bin", m_pShader, OwnerScene);
	SetChild(pGameObject);
	if (pGameObject)
	{
		pGameObject->BuildBroadphaseSphere();
		// 1. 자식의 로컬 OOBB를 가져옵니다 (LoadMeshFromFile에서 이미 계산됨).
		BoundingOrientedBox childLocalOOBB = pGameObject->m_xmLocalOOBB;
		BoundingSphere childLocalSphere = pGameObject->m_xmLocalSphere;
		m_xmLocalOOBB = pGameObject->m_xmLocalOOBB;;
		m_xmLocalSphere = pGameObject->m_xmLocalSphere;
		// 2. 자식의 로컬 변환 행렬을 가져옵니다.
		XMMATRIX xmmtxChildTransform = XMLoadFloat4x4(&pGameObject->m_xmf4x4Transform);

		// 3. 자식의 OOBB를 자식의 변환 행렬로 변환하여
		//    부모(CPlayer)의 로컬 공간 OOBB를 구합니다.
		childLocalOOBB.Transform(m_xmLocalOOBB, xmmtxChildTransform);
		childLocalSphere.Transform(m_xmLocalSphere, xmmtxChildTransform);
		// The grid uses the aggregate bounds for objects with a model hierarchy.
		pGameObject->m_xmBigOOGG.Transform(m_xmBigOOGG, xmmtxChildTransform);
		UpdateTransform(nullptr);
	}
	PrepareAnimate();

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CAirplanePlayer::~CAirplanePlayer()
{
}

void CAirplanePlayer::PrepareAnimate()
{
	m_pMainRotorFrame = FindFrame("Top_Rotor");
	m_pTailRotorFrame = FindFrame("Tail_Rotor");
}

void CAirplanePlayer::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}

	CPlayer::Animate(fTimeElapsed, pxmf4x4Parent);
}

void CAirplanePlayer::OnPrepareRender()
{
	CPlayer::OnPrepareRender();
}

CCamera *CAirplanePlayer::ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed)
{
	DWORD nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);
	switch (nNewCameraMode)
	{
		case FIRST_PERSON_CAMERA:
			SetFriction(20.5f);
			SetStopFriction(160.0f);
			SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
			SetMaxVelocityXZ(200.5f);
			SetMaxVelocityY(20.0f);
			m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.0f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f,-1.0f, 10.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
			m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
			break;
		case SPACESHIP_CAMERA:
			SetFriction(20.5f);
			SetStopFriction(160.0f);
			SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
			SetMaxVelocityXZ(200.5f);
			SetMaxVelocityY(20.0f);
			m_pCamera = OnChangeCamera(SPACESHIP_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.0f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
			m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
			break;
		case THIRD_PERSON_CAMERA:
			SetFriction(20.5f);
			SetStopFriction(160.0f);
			SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
			SetMaxVelocityXZ(200.5f);
			SetMaxVelocityY(20.0f);
			m_pCamera = OnChangeCamera(THIRD_PERSON_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.25f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 15.0f, -30.0f));
			m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
			m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
			break;
		default:
			break;
	}
	// Switching viewpoints must not advance movement or apply friction.
	UpdateCamera(0.0f, true);

	return(m_pCamera);
}

