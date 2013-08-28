// GLSL 3.30
#version 330 core

// zmienne wejściowe - zwrócone przez shader wierzhołków
in vec3 normal;
in vec3 eye;
in vec3 lightDir;
in vec2 texCoord;

// zmienna wejściowa - identyfikator tekstury
uniform sampler2D fileTexture;

// zmienna typu "out" z tego shadera jest ustalonym kolorem fragmentu
out vec4 fragColor;

void main(void)
{
	float specularItensity;
	// normalizacja wektora kierunku światła
	vec3 light = normalize(lightDir);
	// Obliczenie unormowanego wektora odbitego od powierzchni i biegnącego od światła
	vec3 reflection = normalize(reflect(-light, normal));

	// ustalenie koloru "ambient"
	vec4 ambient = vec4(0.1, 0.2, 0.2, 1.0);

	// Obliczenie współczynnika intensywności oświetlenia
	float diffuseIntensity = pow(max(dot(normal, light), 0.0), 0.5);
	// ustalenie koloru "diffuse" i pomieszanie go z kolorem tekstury
	vec4 diffuse = vec4(1.0, 1.0, 1.0, 1.0) * diffuseIntensity;

	// Obliczenie współczynnika intensywności odbłysków
	// Iloczyn skalarny pomiędzy odbitym wektorem światła, a kierunkiem
	// "do obserwatora" podniesiony do odpowiednio wysokiej potęgi.
	if (diffuseIntensity == 0)
		specularItensity = 0;
	else
		specularItensity = pow(max(dot(reflection, eye), 0.0), 40.0 );
	// ustalenie koloru "specular"
	vec4 specular = vec4(0.8, 0.8, 0.8, 1.0) * specularItensity;

	// mapowanie tekstury
	vec4 texel = texture2D(fileTexture, texCoord);

	// złożenie koloru fragmentu z wszystkich składowych
	fragColor = (ambient + diffuse + specular) * texel;
}

