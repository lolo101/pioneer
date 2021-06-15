// Copyright © 2008-2021 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "attributes.glsl"

out vec4 v_color;
out vec2 v_texCoord0;

void main(void)
{
	gl_Position = matrixTransform();
	v_color = a_color;
	v_texCoord0 = a_uv0.xy;
}
