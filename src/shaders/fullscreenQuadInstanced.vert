//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = POSITION_ATTR) in vec2 position;

out gl_PerVertex
{
    vec4 gl_Position;
};

flat out int slice;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    slice = gl_InstanceID;
    gl_Position = vec4(position, 0.0, 1.0);
}