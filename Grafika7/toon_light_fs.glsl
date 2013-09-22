// GLSL 3.30
#version 330 core

// zmienne wejściowe - zwrócone przez shader wierzhołków
in vec3 normal;
in vec3 eye;
in vec3 lightDir;

uniform vec4 color2;

// zmienna typu "out" z tego shadera jest ustalonym kolorem fragmentu
out vec4 fragColor;

void main(void)
{
	// normalizacja wektora kierunku światła
	vec3 light = normalize(lightDir);
	vec3 reflection = normalize(reflect(-light, normal));

	// ustalenie koloru "ambient"
	vec4 ambient = vec4(0.05, 0.05, 0.05, 1.0);

	// Obliczenie współczynnika intensywności oświetlenia
	float diffuseIntensity = pow(max(dot(normal, light), 0.0), 0.5);

	// ustalenie koloru "diffuse"
	vec4 diffuse = vec4(1.0, 0.8, 0.0, 1.0) * diffuseIntensity;

	float specularItensity;
	if (diffuseIntensity == 0)
		specularItensity = 0;
	else
		specularItensity = pow(max(dot(reflection, eye), 0.0), 40.0 );
	// ustalenie koloru "specular"
	vec4 specular = vec4(1.0, 0.0, 0.0, 1.0) * specularItensity;
	
	// złożenie koloru fragmentu z obu składowych
	fragColor = color2 + ambient + diffuse + specular;
}

