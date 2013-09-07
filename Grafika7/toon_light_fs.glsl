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

	// ustalenie koloru "ambient"
	vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);

	// Obliczenie współczynnika intensywności oświetlenia
	float diffuseIntensity = max(dot(normal, light), 0.0);

	// ustalenie koloru "diffuse"
	vec4 diffuse = vec4(254.0/256.0, 254.0/256.0, 51.0/256.0,  1.0) * diffuseIntensity;
	
	// złożenie koloru fragmentu z obu składowych
	fragColor = color2 + ambient + diffuse;
}

