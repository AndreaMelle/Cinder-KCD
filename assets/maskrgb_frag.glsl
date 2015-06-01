#version 110

uniform sampler2D tex0;
uniform sampler2D tex1;

void main()
{
	gl_FragColor.rgb = texture2D(tex0, gl_TexCoord[0].st).rgb;
	gl_FragColor.a = texture2D(tex1, gl_TexCoord[0].st).r;
}