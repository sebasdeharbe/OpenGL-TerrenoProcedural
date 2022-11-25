vec3 calcPhong(vec4 lightPosition, vec3 lightColor,
			   vec3 ambientColor, vec3 diffuseColor, 
			   vec3 specularColor, float shininess) 
{
	// ambient
	vec3 ambient = ambientColor * lightColor * ambientStrength;
	
	// diffuse
	vec3 norm = normalize(fragNormal.z<0? fragNormal : -fragNormal);
	vec3 lDir = vec3(lightPosition);
	lDir = normalize(lDir *-1.f);
	vec3 lightDir = lDir;
	vec3 diffuse = diffuseColor * max(pow(dot(norm,lightDir),5),0.f) * lightColor;
	
	// specular
	vec3 viewDir = normalize(-fragPosition);
	vec3 reflectDir = reflect(-lightDir,norm);
	vec3 specular = specularColor * pow(max(dot(viewDir,reflectDir),0.f),shininess) * lightColor;
	
	// result
	return ambient+diffuse+specular;
}
