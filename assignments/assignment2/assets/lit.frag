#version 450
layout(location = 0) out vec4 FragColor1; //GL_COLOR_ATTACHMNENT0
in vec4 LightSpacePos;

in Surface{
	vec3 WorldPos; //Vertex position in world space
	vec3 WorldNormal; //Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

uniform sampler2D _MainTex; 
uniform sampler2D normalMap; 
uniform sampler2D _ShadowMap;
uniform vec3 _EyePos;
uniform vec3 _LightDirection;// = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

float calcShadow(sampler2D shadowMap, vec4 lightSpacePos){
	//Homogeneous Clip space to NDC [-w,w] to [-1,1]
    vec3 sampleCoord = lightSpacePos.xyz / lightSpacePos.w;
    //Convert from [-1,1] to [0,1]
    sampleCoord = sampleCoord * 0.5 + 0.5;
	float myDepth = sampleCoord.z; 
	float shadowMapDepth = texture(shadowMap, sampleCoord.xy).r;
	//step(a,b) returns 1.0 if a >= b, 0.0 otherwise
	return step(shadowMapDepth,myDepth);
}

void main(){
	float shadow = calcShadow(_ShadowMap, LightSpacePos); 

	//Make sure fragment normal is still length 1 after interpolation.
	vec3 normal = normalize(fs_in.WorldNormal);

	vec3 lightDir = _LightDirection;

	normal = texture(normalMap, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(fs_in.TBN * normal); 

	//Light pointing straight down
	vec3 toLight = -lightDir;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Calculate specularly reflected light
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	lightColor *= 1.0 - shadow;
	lightColor+=_AmbientColor * _Material.Ka;
	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
	FragColor1 = vec4(objectColor * lightColor,1.0);
}