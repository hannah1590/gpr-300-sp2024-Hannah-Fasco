#version 450
layout(location = 0) out vec4 FragColor1; //GL_COLOR_ATTACHMNENT0
in vec4 LightSpacePos;

in Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN;
	vec2 UV;
}fs_in;

uniform sampler2D _MainTex; 
uniform sampler2D normalMap; 
uniform sampler2D _ShadowMap;
uniform vec3 _EyePos;
uniform vec3 _LightDirection;// = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);
uniform float minBias;
uniform float maxBias;

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

float calcShadow(sampler2D shadowMap, vec4 lightSpacePos, float bias){
	//Homogeneous Clip space to NDC [-w,w] to [-1,1]
    vec3 sampleCoord = lightSpacePos.xyz / lightSpacePos.w;
    //Convert from [-1,1] to [0,1]
    sampleCoord = sampleCoord * 0.5 + 0.5;

	float myDepth = sampleCoord.z - bias; 
	if(myDepth > 1.0f)
		return 0.0f;

	float shadowMapDepth = texture(shadowMap, sampleCoord.xy).r;
	//step(a,b) returns 1.0 if a >= b, 0.0 otherwise
	float totalShadow = 0.0f;
	vec2 texelOffset = 1.0 /  textureSize(_ShadowMap,0);
	for(int y = -1; y <=1; y++){
		for(int x = -1; x <=1; x++){
			vec2 uv = sampleCoord.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow += step(texture(_ShadowMap,uv).r,myDepth);
		}
	}
	totalShadow /= 9.0;

	// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(sampleCoord.z > 1.0)
        totalShadow = 0.0;
	return totalShadow;
}

uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;

struct PointLight{
	vec3 position;
	float radius;
	vec4 color;
};
#define MAX_POINT_LIGHTS 64
uniform PointLight _PointLights[MAX_POINT_LIGHTS];

//Linear falloff
float attenuateLinear(float distance, float radius){
	return clamp((radius-distance)/radius,0.0,1.0);
}

//Exponential falloff
float attenuateExponential(float distance, float radius){
	float i = clamp(1.0 - pow(distance/radius,4.0),0.0,1.0);
	return i * i;
	
}

vec3 calcPointLight(PointLight light,vec3 normal){
	vec3 worldPos = texture(_gPositions,fs_in.UV).xyz;

	vec3 diff = light.position - worldPos;
	//Direction toward light position
	vec3 toLight = normalize(diff);
	//TODO: Usual blinn-phong calculations for diffuse + specular
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Calculate specularly reflected light
	vec3 toEye = normalize(_EyePos - worldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);

	vec3 lightColor = (diffuseFactor + specularFactor) * vec3(light.color);
	//Attenuation
	float d = length(diff); //Distance to light

	lightColor *= attenuateLinear(d,light.radius); //See below for attenuation options
	return lightColor;
}

void main(){
	//vec3 normal = normalize(fs_in.WorldNormal);
	vec3 normal = texture(_gNormals,fs_in.UV).xyz;

	PointLight mainLight;
	mainLight.position = vec3(LightSpacePos);
	mainLight.radius = 5;
	mainLight.color = vec4(_LightColor, 1);

	vec3 totalLight = vec3(0);
	totalLight+=calcPointLight(mainLight,normal);
	for(int i=0;i<MAX_POINT_LIGHTS;i++){
		totalLight+=calcPointLight(_PointLights[i],normal);
	}
	vec3 albedo = texture(_gAlbedo,fs_in.UV).xyz;
	FragColor1 = vec4(albedo * totalLight,1.0);
}