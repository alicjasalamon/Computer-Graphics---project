// GLSL 3.30
#version 330 core

// iloczyn maciezy Widoku Modelu oraz Projekcji
uniform mat4 modelViewProjectionMatrix;
// macierz Widoku Modelu
uniform mat4 modelViewMatrix;
// kierunek źródła światła
uniform vec3 inLightDir;

// współrzędne wierzchołka
in vec3 inVertex;
// współrzędne wektora normalnego w wierzchołku
in vec3 inNormal;
// współrzędne tekstury 2D w wierchołku
in vec2 inTexCoord;

// wektor normalny we współrzędnych obserwatora
out vec3 normal;
// wektork kierunku światła we współrzędnych obserwatora
out vec3 lightDir;
// wektor kierunku "do obserwatora" we współrzędnych obserwatora
out vec3 eye;
// współrzędne tekstury 2D
out vec2 texCoord;

void main(void)
{
	// Przekształcenie trójelementowego wektora współrzędnych do czteroelemntowego
	vec4 vertex = vec4(inVertex,1.0);

	// Przekształcenie wektora normalnego do układu obserwatora.
	normal = normalize((modelViewMatrix * vec4(inNormal, 0.0)).xyz);
	// Przekształcenie wektora w kierunku "do obserwatora" do układu obserwatora
	eye = - normalize((modelViewMatrix * vertex).xyz);

	//Przepisanie zmiennych wejściowych na wyjściowe, aby przekazać je do kolejnego shadera.
	lightDir = inLightDir;
	texCoord = inTexCoord;

	// Obliczenie i ustawienie współrzędnych wierzchołka - w układzie obserwatora po
	// wykonaniu operacji rzutowania - macirz Projekcji
	gl_Position = modelViewProjectionMatrix * vertex;
}

