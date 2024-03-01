#version 450 core
layout(location = 0) out vec3 gPosition; //Worldspace position
layout(location = 1) out vec3 gNormal; //Worldspace normal 
layout(location = 2) out vec3 gAlbedo;
in vec4 LightSpacePos;

in Surface{
	vec3 WorldPos; 
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
	vec2 UV;
}fs_in;

uniform sampler2D _MainTex;
void main(){
	gPosition = fs_in.WorldPos;
	gAlbedo = texture(_MainTex,fs_in.TexCoord).rgb;
	gNormal = normalize(fs_in.WorldNormal);
}
