#version 450

layout ( binding = 0 ) uniform sampler2D texSampler;

layout ( location = 0 ) in vec3 fragColor;
layout ( location = 1 ) in vec2 fragTexCoord;
layout ( location = 2 ) in flat uint fragIsTexture;

layout ( location = 0 ) out vec4 outColor;


void main()
{
	if ( fragIsTexture == 0 )
		outColor = vec4( fragColor, 1.0 );
	else
		outColor = texture( texSampler, fragTexCoord );
}