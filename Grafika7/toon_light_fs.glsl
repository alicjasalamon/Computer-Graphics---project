// GLSL 3.30
#version 330 core

// zmienne wejściowe - zwrócone przez shader wierzhołków
in vec3 normal;
in vec3 eye;
in vec3 lightDir;

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
	// Przejście z płynnej zmiany współczynnika intensywności na kilka ustalonych wartości
	if (diffuseIntensity < 0.25)
		diffuseIntensity = 0.25;
	else if (diffuseIntensity < 0.5)
		diffuseIntensity = 0.5;
	else if (diffuseIntensity < 0.75)
		diffuseIntensity = 0.75;
	else if (diffuseIntensity < 1.0)
		diffuseIntensity = 1.0;
	
	// ustalenie koloru "diffuse"
	vec4 diffuse = vec4(1.0, 0.0, 0.0, 1.0) * diffuseIntensity;
	
	// złożenie koloru fragmentu z obu składowych
	fragColor = ambient + diffuse;
}

