#version 100


precision mediump float;
// Input vertex attributes (from vertex shader)
// In GLSL 100, use 'varying' instead of 'in' for input from the vertex shader
varying vec4 fragColor;

// Output fragment color
// In GLSL 100, the output color is set to gl_FragColor
// There's no need to declare an output variable

void main()
{
    gl_FragColor = fragColor;
}
