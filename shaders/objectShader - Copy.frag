#version 330 core

in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexCoords;
in vec4 lightVSPosition;

// propiedades del material
uniform sampler2D colorTexture;

// propiedades de la luz
uniform float ambientStrength;
uniform vec3 lightColor;

out vec4 fragColor;




void main() {
	vec4 tex = texture(colorTexture,fragTexCoords);
	
	fragColor = vec4(vec3(tex),1);
	
	
}

