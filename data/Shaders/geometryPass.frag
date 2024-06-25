layout(location=0) out vec4 oBaseColor;
layout(location=1) out vec3 oMetallicRoughnessOcclusion;
layout(location=2) out vec3 oNormal;
layout(location=3) out vec3 oPosition;

#if defined(HAS_NORMALS) || defined(FLAT_SHADING)
    struct Material
    {
        vec4 baseColorFactor;
        sampler2D baseColorTexture;
        float metallicFactor;
        float roughnessFactor; 
        sampler2D metallicRoughnessTexture;
        sampler2D normalTexture;
        float normalScale;
        float occlusionStrength;
        sampler2D occlusionTexture;
    };

    uniform Material material;
#endif

in VS_OUT {
    vec3 surfacePosVS;

    #ifdef HAS_NORMALS
        #ifdef HAS_TANGENTS
            mat3 TBN;
        #else
            vec3 surfaceNormalVS;
        #endif // HAS_TANGENTS
    #endif // HAS_NORMALS

    #ifdef HAS_TEXCOORD
    vec2 texCoords;
    #endif // HAS_TEXCOORD

    #ifdef HAS_VERTEX_COLORS
    vec4 vertexColor;
    #endif // HAS_VERTEX_COLORS

#if defined(HAS_NORMALS) || defined(FLAT_SHADING)
    vec4 surfacePosShadowMapUVSpace[MAX_NUM_SPOT_LIGHTS + MAX_NUM_DIR_LIGHTS];
#endif

} fsIn;

void main()
{
    oPosition = fsIn.surfacePosVS;

#ifdef HAS_NORMALS
    #ifdef HAS_TANGENTS
        mat3 normalizedTBN = mat3(normalize(fsIn.TBN[0]), normalize(fsIn.TBN[1]), normalize(fsIn.TBN[2]));
        vec3 unitNormal = texture(material.normalTexture, fsIn.texCoords).rgb;
        unitNormal = unitNormal * 2.0 - 1.0;
        unitNormal *= vec3(material.normalScale, material.normalScale, 1.0);
        unitNormal = normalize(normalizedTBN * unitNormal);
    #else
        vec3 unitNormal = normalize(fsIn.surfaceNormalVS);
    #endif // HAS_TANGENTS
#elif defined(FLAT_SHADING)
    vec3 dxTangent = dFdx(fsIn.surfacePosVS);
    vec3 dyTangent = dFdy(fsIn.surfacePosVS);
    vec3 unitNormal = normalize(cross(dxTangent, dyTangent));
#endif // HAS_NORMALS

    oNormal = unitNormal + 1.0 * 0.5; // encode to [0, 1] range

#if defined(HAS_NORMALS) || defined(FLAT_SHADING)

    #ifdef HAS_TEXCOORD
        vec4 baseColor = texture(material.baseColorTexture, fsIn.texCoords) * material.baseColorFactor;
        vec2 metallicRoughness = texture(material.metallicRoughnessTexture, fsIn.texCoords).bg * vec2(material.metallicFactor, material.roughnessFactor);
        // TODO: fix occlusion bullshit. apparently this is wrong https://github.com/KhronosGroup/glTF/issues/884
        float occlusion = texture(material.occlusionTexture, fsIn.texCoords).r * material.occlusionStrength;
    #else
        vec4 baseColor = material.baseColorFactor;
        vec2 metallicRoughness = vec2(material.metallicFactor, material.roughnessFactor);
        float occlusion = material.occlusionStrength;
    #endif // HAS_TEXCOORD
    #ifdef HAS_VERTEX_COLORS
        baseColor = baseColor * fsIn.vertexColor;
    #endif // HAS_VERTEX_COLORS

    oBaseColor = baseColor;
    oMetallicRoughnessOcclusion = vec3(metallicRoughness, occlusion);
#endif
}