uniform sampler2D gBaseColor;
uniform sampler2D gMetallicRoughnessOcclusion;
uniform sampler2D gNormal;
uniform sampler2D gPosition;

uniform vec2 resolution;

struct DirectionalLight
{
    vec3 color;
    float intensity;
    
    vec3 direction;
};

uniform DirectionalLight light;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    // https://www.desmos.com/calculator/s4vkjimp63
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

// TODO: make location a #define
layout(location=4) out vec4 fragColor;

void main()
{
    vec2 gBufferUV = gl_FragCoord.xy / resolution;
    vec4 baseColor = texture(gBaseColor, gBufferUV);
    vec3 surfaceToCamera = -texture(gPosition, gBufferUV).xyz;
    vec3 unitNormal = normalize(texture(gNormal, gBufferUV).xyz) * 2.0 - 1.0; // decode from [0, 1] - > [-1, 1]
    vec3 metallicRoughnessOcclusion = texture(gMetallicRoughnessOcclusion, gBufferUV).xyz;
    float metallic = metallicRoughnessOcclusion.x;
    float roughness = metallicRoughnessOcclusion.y;
    float occlusion = metallicRoughnessOcclusion.z;
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, baseColor.rgb, metallic);
    vec3 surfaceToLight = -light.direction;
    vec3 H = normalize(surfaceToCamera + surfaceToLight);
    float cosLightSurfaceAngle = dot(light.direction, surfaceToLight);
    vec3 radiance = light.color * light.intensity;
    float NDF = DistributionGGX(unitNormal, H, roughness);  
    float G = GeometrySmith(unitNormal, surfaceToCamera, surfaceToLight, roughness);
    vec3 F = FresnelSchlick(max(dot(H, surfaceToCamera), 0.0), F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(unitNormal, surfaceToCamera), 0.0) * max(dot(unitNormal, surfaceToLight), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    float geometryTerm = max(dot(surfaceToLight, unitNormal), 0.0);
    // float shadow = textureProj(depthMaps[i + MAX_NUM_SPOT_LIGHTS], fsIn.surfacePosShadowMapUVSpace[i + MAX_NUM_SPOT_LIGHTS]);
    vec3 color = geometryTerm * radiance /** shadow*/ * (kD * baseColor.rgb / PI + specular);
    fragColor = vec4(color, baseColor.a);
}