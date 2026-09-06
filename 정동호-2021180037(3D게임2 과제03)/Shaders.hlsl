struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular;
	float4					m_cEmissive;
};

cbuffer cbCameraInfo : register(b1)
{
	matrix					gmtxView : packoffset(c0);
	matrix					gmtxProjection : packoffset(c4);
	float3					gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix					gmtxGameObject : packoffset(c0);
	MATERIAL				gMaterial : packoffset(c4);
	uint					gnTexturesMask : packoffset(c8);

    float4x4 gmtxTextureTransforms[2]: packoffset(c9);
};
#include "Light.hlsl"
struct INSTANCE_DATA
{
    float4x4 World;

};

StructuredBuffer<INSTANCE_DATA> g_InstanceData : register(t18);

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

Texture2D gtxtAlbedoTexture : register(t6);
Texture2D gtxtSpecularTexture : register(t7);
Texture2D gtxtNormalTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtEmissionTexture : register(t10);
Texture2D gtxtDetailAlbedoTexture : register(t11);
Texture2D gtxtDetailNormalTexture : register(t12);
cbuffer cbShadowInfo : register(b14)
{
    matrix gmtxLightViewProj;
};

Texture2D gtxtShadowMap : register(t20);
SamplerState gssWrap : register(s0);
SamplerState gssPointClamp : register(s2);
SamplerComparisonState gssShadowCmp : register(s3);

struct VS_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normalW : NORMAL;
	float3 tangentW : TANGENT;
	float3 bitangentW : BITANGENT;
	float2 uv : TEXCOORD;
};
struct VS_SHADOW_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_SHADOW_OUTPUT VS_Shadow(VS_STANDARD_INPUT input)
{
    VS_SHADOW_OUTPUT o;

    float4 posW = mul(float4(input.position, 1.0f), gmtxGameObject);
    o.position = mul(posW, gmtxLightViewProj);

    return o;
}

float CalcShadow(float3 positionW)
{

    float4 lp = mul(float4(positionW, 1.0f), gmtxLightViewProj);
    lp.xyz /= lp.w;

    float2 uv;
    uv.x = lp.x * 0.5f + 0.5f;
    uv.y = -lp.y * 0.5f + 0.5f;

    float depth = lp.z;

    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        return 1.0f;

    float bias = 0.0015f;

    float lit = gtxtShadowMap.SampleCmpLevelZero(gssShadowCmp, uv, depth - bias);

    return lerp(0.75f, 1.0f, lit);
}
VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;

    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);

    output.tangentW = mul(input.tangent, (float3x3) gmtxGameObject);
    output.bitangentW = mul(input.bitangent, (float3x3) gmtxGameObject);

    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{

    float4 cAlbedoColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (gnTexturesMask & MATERIAL_ALBEDO_MAP)
        cAlbedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_SPECULAR_MAP)
        cSpecularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
        cNormalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_EMISSION_MAP)
        cEmissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);

    float4 cIllumination = float4(1, 1, 1, 1);
    float4 cColor = cAlbedoColor + cSpecularColor + cEmissionColor;

    float3 normalW = normalize(input.normalW);

    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
    {
        float3x3 TBN = float3x3(normalize(input.tangentW), normalize(input.bitangentW), normalize(input.normalW));
        float3 vNormal = normalize(cNormalColor.rgb * 2.0f - 1.0f);
        normalW = normalize(mul(vNormal, TBN));
    }

    cIllumination = Lighting(input.positionW, normalW);

    float shadow = CalcShadow(input.positionW);

    cIllumination.rgb *= shadow;

    cColor = lerp(cColor, cIllumination, 0.5f);

    return cColor;
}

struct VS_SKYBOX_CUBEMAP_INPUT
{
	float3 position : POSITION;
};

struct VS_SKYBOX_CUBEMAP_OUTPUT
{
	float3	positionL : POSITION;
	float4	position : SV_POSITION;
};

VS_SKYBOX_CUBEMAP_OUTPUT VSSkyBox(VS_SKYBOX_CUBEMAP_INPUT input)
{

    VS_SKYBOX_CUBEMAP_OUTPUT output;
    float3x3 viewRotation = (float3x3) gmtxView;
    float3 posV = mul(input.position, viewRotation);

    output.position = mul(float4(posV, 1.0f), gmtxProjection);

    output.position.z = output.position.w;
    output.positionL = input.position;

    return (output);
}

TextureCube gtxtSkyCubeTexture : register(t13);
SamplerState gssClamp : register(s1);

float4 PSSkyBox(VS_SKYBOX_CUBEMAP_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.positionL);

	return(cColor);
}

struct VS_SPRITE_TEXTURED_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct VS_SPRITE_TEXTURED_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_SPRITE_TEXTURED_OUTPUT VSTextured(VS_SPRITE_TEXTURED_INPUT input)
{
	VS_SPRITE_TEXTURED_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}

Texture2D gtxtTerrainTexture : register(t14);
Texture2D gtxtDetailTexture : register(t15);
Texture2D gtxtAlphaTexture : register(t16);

float4 PSTextured(VS_SPRITE_TEXTURED_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtTerrainTexture.Sample(gssWrap, input.uv);

	return(cColor);
}

struct VS_TERRAIN_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
    VS_TERRAIN_OUTPUT output;

    float4 worldPos = mul(float4(input.position, 1.0f), gmtxGameObject);
    float4 viewPos = mul(worldPos, gmtxView);
    float4 projPos = mul(viewPos, gmtxProjection);

    output.position = projPos;
    output.color = input.color;
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    output.worldPos = worldPos.xyz;

    return output;
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
    float4 baseC = gtxtTerrainTexture.Sample(gssWrap, input.uv0);
    float4 detailC = gtxtDetailTexture.Sample(gssWrap, input.uv1);
    float3 albedo = (baseC.rgb * 0.5f + detailC.rgb * 0.5f);

    float3 dx = ddx(input.worldPos);
    float3 dy = ddy(input.worldPos);

    float3 normalW = normalize(cross(dx, dy));
    if (normalW.y < 0.0f)
        normalW = -normalW;

    float3 litRGB = max(Lighting(input.worldPos, normalW).rgb, 0.0f);

    float shadow = CalcShadow(input.worldPos);

    float3 ambientFloor = 0.20f;
    float3 direct = litRGB * shadow;

    float3 finalRGB = albedo * (ambientFloor + direct);

    return float4(saturate(finalRGB), 1.0f);
}

struct HS_TERRAIN_CONSTANT_DATA_OUTPUT
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSPatchConstantsTerrain")]
VS_TERRAIN_OUTPUT HSTerrain(InputPatch<VS_TERRAIN_OUTPUT, 3> patch, uint i : SV_OutputControlPointID)
{
    return patch[i];
}
HS_TERRAIN_CONSTANT_DATA_OUTPUT HSPatchConstantsTerrain(InputPatch<VS_TERRAIN_OUTPUT, 3> patch)
{
    HS_TERRAIN_CONSTANT_DATA_OUTPUT o;

    float3 center = (patch[0].worldPos + patch[1].worldPos + patch[2].worldPos) / 3.0f;

    float dist = distance(center, gvCameraPosition);

    const float maxTess = 8.0f;
    const float minTess = 1.0f;
    const float startDistance = 50.0f;
    const float endDistance = 400.0f;

    float t = saturate((dist - startDistance) / (endDistance - startDistance));
    float tess = lerp(maxTess, minTess, t);

    o.EdgeTess[0] = tess;
    o.EdgeTess[1] = tess;
    o.EdgeTess[2] = tess;
    o.InsideTess = tess;

    return o;
}
[domain("tri")]
VS_TERRAIN_OUTPUT DSTerrain(HS_TERRAIN_CONSTANT_DATA_OUTPUT pc,const OutputPatch<VS_TERRAIN_OUTPUT, 3> patch,float3 bary : SV_DomainLocation)
{
    VS_TERRAIN_OUTPUT o;

    o.worldPos =
          patch[0].worldPos * bary.x
        + patch[1].worldPos * bary.y
        + patch[2].worldPos * bary.z;

    o.color =
          patch[0].color * bary.x
        + patch[1].color * bary.y
        + patch[2].color * bary.z;

    o.uv0 =
          patch[0].uv0 * bary.x
        + patch[1].uv0 * bary.y
        + patch[2].uv0 * bary.z;

    o.uv1 =
          patch[0].uv1 * bary.x
        + patch[1].uv1 * bary.y
        + patch[2].uv1 * bary.z;

    float4 viewPos = mul(float4(o.worldPos, 1.0f), gmtxView);
    o.position = mul(viewPos, gmtxProjection);

    return o;
}

struct VS_GS_BILLBOARD_INPUT
{
    float3 vPos : POSITION;
    float3 vPosW_Center : CENTER;
    float2 vSizeW : SIZE;
};
struct VS_GS_BILLBOARD_OUTPUT
{
    float3 vPosW_Center : POSITION;
    float2 vSizeW : SIZE;
};
struct GS_PS_BILLBOARD_OUTPUT
{
    float4 fPosH : SV_POSITION;
    float2 vTex : TEXCOORD;
    float3 vPosW : POSITION;
};
VS_GS_BILLBOARD_OUTPUT VS_GS_Billboard(VS_GS_BILLBOARD_INPUT input)
{
    VS_GS_BILLBOARD_OUTPUT output;
    output.vPosW_Center = input.vPosW_Center;
    output.vSizeW = input.vSizeW;
    return output;
}

[maxvertexcount(4)]
void GS_Billboard(point VS_GS_BILLBOARD_OUTPUT input[1],
                  inout TriangleStream<GS_PS_BILLBOARD_OUTPUT> outStream)
{
    float3 vPosW_Center = input[0].vPosW_Center;
    float2 vSizeW = input[0].vSizeW;

    float3 vUp = float3(0.0f, 1.0f, 0.0f);
    float3 vLook = normalize(gvCameraPosition.xyz - vPosW_Center);
    float3 vRight = normalize(cross(vUp, vLook));

    float fHalfW = vSizeW.x * 0.5f;
    float fHalfH = vSizeW.y * 0.5f;

    float4 pVertices[4];
    pVertices[0] = float4(vPosW_Center + vRight * fHalfW + vUp * fHalfH, 1.0f);
    pVertices[1] = float4(vPosW_Center + vRight * fHalfW - vUp * fHalfH, 1.0f);
    pVertices[2] = float4(vPosW_Center - vRight * fHalfW + vUp * fHalfH, 1.0f);
    pVertices[3] = float4(vPosW_Center - vRight * fHalfW - vUp * fHalfH, 1.0f);

    float2 pUVs[4] =
    {
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(0.0f, 1.0f)
    };

    GS_PS_BILLBOARD_OUTPUT o;
    for (int i = 0; i < 4; ++i)
    {
        o.vPosW = pVertices[i].xyz;
        o.fPosH = mul(pVertices[i], gmtxView);
        o.fPosH = mul(o.fPosH, gmtxProjection);
        o.vTex = pUVs[i];
        outStream.Append(o);
    }
}
float4 PS_GS_Billboard(GS_PS_BILLBOARD_OUTPUT input) : SV_TARGET
{
    float4 uvParams = gmtxTextureTransforms[0][0];

    float fUOffset = uvParams.x;
    float fVOffset = uvParams.y;
    float fUScale = uvParams.z;
    float fVScale = uvParams.w;

    if (fUScale == 0.0f)
        fUScale = 1.0f;
    if (fVScale == 0.0f)
        fVScale = 1.0f;

    float2 finalUV;
    finalUV.x = (input.vTex.x * fUScale) + fUOffset;
    finalUV.y = (input.vTex.y * fVScale) + fVOffset;

    float4 cAlbedo = gtxtAlbedoTexture.Sample(gssWrap, finalUV);

    if (cAlbedo.a < 0.1f)
        discard;

    return cAlbedo;
}

Texture2D gtxtTextures[3] : register(t17);

struct VS_TRANSFORMED_TEXTURED_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VS_TRANSFORMED_TEXTURED_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

VS_TRANSFORMED_TEXTURED_OUTPUT VSTextureTransform(VS_TRANSFORMED_TEXTURED_INPUT input)
{
    VS_TRANSFORMED_TEXTURED_OUTPUT output;

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.uv0 = mul(float3(input.uv, 1.0f), (float3x3) gmtxTextureTransforms[0]).xy;
    output.uv1 = mul(float3(input.uv, 1.0f), (float3x3) gmtxTextureTransforms[1]).xy;

    return (output);
}

float4 PSTextureTransform(VS_TRANSFORMED_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor0 = gtxtTextures[0].Sample(gssWrap, input.uv0);
    float4 cColor1 = gtxtTextures[1].Sample(gssWrap, input.uv1);
    float4 cColor = lerp(cColor1, cColor0, cColor0.a);

    return (cColor);
}

float4 PSTextureTransformDistortion(VS_TRANSFORMED_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor0 = gtxtTextures[0].Sample(gssWrap, input.uv0);
    float4 cColor2 = gtxtTextures[2].Sample(gssWrap, input.uv0);
    float2 uv = input.uv1 - cColor2.r * float2(gmtxTextureTransforms[1]._41, gmtxTextureTransforms[1]._42);
    float4 cColor1 = gtxtTextures[1].Sample(gssWrap, uv);
    float4 cColor = lerp(cColor1, cColor0, cColor0.a);

    return (cColor);
}

float4 PSTextureTransformDistortionTransparent(VS_TRANSFORMED_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor0 = gtxtTextures[0].Sample(gssWrap, input.uv0);
    float4 cColor2 = gtxtTextures[2].Sample(gssWrap, input.uv0);
    float2 uv = input.uv1 - cColor2.r * float2(gmtxTextureTransforms[1]._41, gmtxTextureTransforms[1]._42);
    float4 cColor1 = gtxtTextures[1].Sample(gssWrap, uv);
    float4 cColor = lerp(cColor1, cColor0, cColor0.a);

    return (cColor);
}

float4 PS_UI_Textured(VS_SPRITE_TEXTURED_OUTPUT input) : SV_Target
{

    return gtxtAlbedoTexture.Sample(gssWrap, input.uv);
}

struct VS_IN
{
    float3 vPos : POSITION;
    float4 vColor : COLOR;
};
struct VS_OUT
{
    float4 vPos : SV_POSITION;
    float4 vColor : COLOR;
};

float4 PS_UI_HealthBar(VS_SPRITE_TEXTURED_OUTPUT input) : SV_Target
{
    float fUOffset = gmtxTextureTransforms[0][0][0];
    if (input.uv.x > fUOffset)
    {
        discard;
    }

    return float4(0.0f, 1.0f, 0.0f, 1.0f);

}

float4 PS_UI_TextureAtlas(VS_SPRITE_TEXTURED_OUTPUT input) : SV_Target
{
    if (input.uv.y < 0.3f || input.uv.y > 0.7f)
    {
        discard;
    }

    float fUOffset = gmtxTextureTransforms[0][0][0];
    float fVOffset = gmtxTextureTransforms[0][1][0];
    float fUScale = gmtxTextureTransforms[0][2][0];
    float fVScale = gmtxTextureTransforms[0][3][0];

    float2 fUVFinal;
    fUVFinal.x = (( input.uv.x) * fUScale) + fUOffset;
    fUVFinal.y = (input.uv.y * fVScale) + fVOffset;

    float4 cColor = gtxtAlbedoTexture.Sample(gssPointClamp, fUVFinal);

    float fAlphaMask = cColor.a;
    return float4(cColor.rgb, fAlphaMask);
}

float4 PSMirrorGlass(VS_STANDARD_OUTPUT input) : SV_TARGET
{
    float3 normalW = normalize(input.normalW);

    float4 lightColor = Lighting(input.positionW, normalW);

    float4 diffuse = gMaterial.m_cDiffuse;
    float4 emissive = gMaterial.m_cEmissive;

    float3 baseColor = diffuse.rgb;
    float3 litColor = baseColor * lightColor.rgb + emissive.rgb;

    float alpha = saturate(diffuse.a * 1.3f);
    return float4(litColor, alpha);
}
struct VS_MIRROR_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_MIRROR_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float3 bitangentW : BITANGENT;
    float2 uv : TEXCOORD;
};

VS_MIRROR_OUTPUT VSMirror(VS_MIRROR_INPUT input)
{
    VS_MIRROR_OUTPUT output;

    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);

    output.uv = input.uv;
    float3 localNormal = float3(0.0f, 0.0f, -1.0f);
    output.normalW = normalize(mul(localNormal, (float3x3) gmtxGameObject));

    output.tangentW = float3(1.0f, 0.0f, 0.0f);
    output.bitangentW = float3(0.0f, 1.0f, 0.0f);

    return output;
}
VS_MIRROR_OUTPUT VSMirrorMask(VS_MIRROR_INPUT input)
{
    VS_MIRROR_OUTPUT output;

    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.position.z = output.position.w;

    output.uv = input.uv;
    output.normalW = float3(0, 0, 0);
    output.tangentW = float3(0, 0, 0);
    output.bitangentW = float3(0, 0, 0);

    return output;
}

VS_STANDARD_OUTPUT VSStandardInstanced(VS_STANDARD_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_STANDARD_OUTPUT output;

    float4x4 instanceWorld = g_InstanceData[instanceID].World;
    float4 posLocal = mul(float4(input.position, 1.0f), gmtxGameObject);
    output.positionW = (float3) mul(posLocal, instanceWorld);

    float3 normalLocal = normalize(mul(input.normal, (float3x3) gmtxGameObject));
    output.normalW = normalize(mul(normalLocal, (float3x3) instanceWorld));
    float3 tangentLocal = normalize(mul(input.tangent, (float3x3) gmtxGameObject));
    output.tangentW = normalize(mul(tangentLocal, (float3x3) instanceWorld));

    float3 bitangentLocal = normalize(mul(input.bitangent, (float3x3) gmtxGameObject));
    output.bitangentW = normalize(mul(bitangentLocal, (float3x3) instanceWorld));

    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return output;
}

struct VS_EXPLOSION_QUAD_INPUT
{
    float2 vLocalPos : LOCALPOS;
    float2 vTex : TEXCOORD0;
};

struct VS_EXPLOSION_INSTANCE
{
    float3 vCenter : INSTANCEPOS;
    float fSize : INSTANCESIZE;
    float4 uvParams : INSTANCEUV;
};

struct VS_EXPLOSION_OUTPUT
{
    float4 vPosH : SV_POSITION;
    float2 vTex : TEXCOORD0;
    float4 uvParams : TEXCOORD1;
};

VS_EXPLOSION_OUTPUT VS_Explosion_Instanced(VS_EXPLOSION_QUAD_INPUT vin,VS_EXPLOSION_INSTANCE inst)
{
    VS_EXPLOSION_OUTPUT vout;

    float3 vCenter = inst.vCenter;
    float fSize = inst.fSize;

    float3 vUp = float3(0.0f, 1.0f, 0.0f);
    float3 vLook = normalize(gvCameraPosition - vCenter);
    float3 vRight = normalize(cross(vUp, vLook));

    float fHalfW = fSize * vin.vLocalPos.x;
    float fHalfH = fSize * vin.vLocalPos.y;

    float3 vPosW3 = vCenter + vRight * fHalfW + vUp * fHalfH;
    float4 vPosW = float4(vPosW3, 1.0f);

    float4 vPosV = mul(vPosW, gmtxView);
    vout.vPosH = mul(vPosV, gmtxProjection);

    vout.vTex = vin.vTex;
    vout.uvParams = inst.uvParams;
    return vout;
}

float4 PS_Explosion_Instanced(VS_EXPLOSION_OUTPUT pin) : SV_TARGET
{
    float4 uv = pin.uvParams;

    float2 finalUV;
    finalUV.x = pin.vTex.x * uv.z + uv.x;
    finalUV.y = pin.vTex.y * uv.w + uv.y;

    float4 cAlbedo = gtxtAlbedoTexture.Sample(gssWrap, finalUV);

    if (cAlbedo.a < 0.1f)
        discard;

    return cAlbedo;
}

cbuffer cbMotionBlur : register(b0)
{
    float2 gScreenSize;
    float gBlurStrength;
    float gSampleCount;
}
Texture2D gtxMotionInput : register(t0);
RWTexture2D<float4> gtxMotionOutput : register(u0);

static const float GaussianWeights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

[numthreads(8, 8, 1)]
void CS_MotionBlur(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadID.xy;

    if (pixel.x >= (uint) gScreenSize.x || pixel.y >= (uint) gScreenSize.y)
        return;
    if (gBlurStrength <= 0.001f)
    {
        gtxMotionOutput[pixel] = gtxMotionInput.Load(int3(pixel, 0));
        return;
    }

    float2 texelSize = 1.0f / gScreenSize;
    float2 uv = (float2(pixel) + 0.5f) * texelSize;
    float2 offsetScale = texelSize * (gBlurStrength * 5.0f);

    int blurMode = (int) (gSampleCount + 0.5f);

    if (blurMode == 2)
    {
        float2 center = float2(0.5f, 0.5f);
        float2 dir = uv - center;

        float lenDir = length(dir);
        if (lenDir > 1e-6f)
            dir /= lenDir;
        else
            dir = float2(0.0f, 0.0f);

        float stepLen = max(offsetScale.x, offsetScale.y) * 0.5f;
        float2 stepUV = dir * stepLen;

        const int TAP = 8;

        float4 sum = gtxMotionInput.SampleLevel(gssWrap, uv, 0);
        float wsum = 1.0f;

    [unroll]
        for (int i = 1; i <= TAP; ++i)
        {
            float t = (float) i / (float) TAP;
            float w = (1.0f - t);

            float2 o = stepUV * (float) i;

            sum += gtxMotionInput.SampleLevel(gssWrap, uv + o, 0) * w;
            sum += gtxMotionInput.SampleLevel(gssWrap, uv - o, 0) * w;
            wsum += 2.0f * w;
        }

        gtxMotionOutput[pixel] = sum / wsum;
        return;
    }

    float4 color = gtxMotionInput.SampleLevel(gssWrap, uv, 0) * GaussianWeights[0];
    float totalWeight = GaussianWeights[0];

    for (int i = 1; i < 5; ++i)
    {
        float weight = GaussianWeights[i];
        float2 offset = float2(i, i) * offsetScale;

        color += gtxMotionInput.SampleLevel(gssWrap, uv + float2(offset.x, 0.0f), 0) * weight;
        color += gtxMotionInput.SampleLevel(gssWrap, uv - float2(offset.x, 0.0f), 0) * weight;

        color += gtxMotionInput.SampleLevel(gssWrap, uv + float2(0.0f, offset.y), 0) * weight;
        color += gtxMotionInput.SampleLevel(gssWrap, uv - float2(0.0f, offset.y), 0) * weight;

        totalWeight += weight * 4.0f;
    }
    gtxMotionOutput[pixel] = color / totalWeight;
}
