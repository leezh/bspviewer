static const char * vertSrc = R"GLSL(
#version 120
uniform mat4 matrix;
attribute vec4 vertex;
attribute vec3 normal;
attribute vec4 texcoord;
attribute vec4 lmcoord;
varying vec3 fragNormal;

void main()
{
	gl_TexCoord[0] = texcoord;
	gl_TexCoord[1] = lmcoord;
	gl_Position = matrix * vertex;
	fragNormal = normal;
}
)GLSL";

static const char * fragSrc = R"GLSL(
#version 120
varying vec3 fragNormal;
uniform sampler2D texture;
uniform sampler2D lightmap;
uniform bool uselightmap;

void main() {
	vec4 texel = texture2D(texture, gl_TexCoord[0].st);
	if (uselightmap)
	{
		texel = texel * 3.0 * texture2D(lightmap, gl_TexCoord[1].st);
	}
	gl_FragColor = texel;
}
)GLSL";
