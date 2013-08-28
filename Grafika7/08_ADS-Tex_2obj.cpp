// dołączenie plików nagłówkowych z biblioteki J.Ganczarskiego

#include "shaders.h"
#include "vecmatquat.h"
#include "obj.h"
#include "targa.h"

// załadowanie pliku nagłówkowego biblioteki GLEW - znajdują się w nim wszystkie
// rozszerzenia biblioteki OpenGL również dla wersji wyższych niż 3.0
// Nie ma więc potrzeby wykorzysytwania pliku nagłówkowego gl3.h
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>

#include <GLTools.h>
#include <GLBatch.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <GLFrustum.h>

#include <iostream>

#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "gltools.lib")

using namespace std;

GLMatrixStack		modelViewMatrix;	// stos macierzy Widoku Modelu
GLMatrixStack		projectionMatrix;	// stos macierzy Projekcji
GLFrustum			viewFrustum;		// bryła obcięcia
GLGeometryTransform	transformPipeline;	// potok renderowania
GLFrame				cameraFrame;		// obiekt kamery

// kąty obrotu obiektu
GLfloat rotateX = 0.0f;
GLfloat rotateY = 0.0f;
GLfloat rotateZ = 0.0f;

// kierunek źródła światła
GLfloat lightDir[4] = {5.0f, 5.0f, 3.0f, 1.0f};

// identyfikator obiektu programu
GLuint toonShader, texShader;

// identyfikatory obiektów tablic wierchołków dla .obj i sześcianu
GLuint objVertexArray;
GLuint coneVertexArray;
GLuint cylinderVertexArray;
GLuint cubeVertexArray;
GLuint starVertexArray;

// obiekt reprezentujący bryłę wczytaną jako .obj
objShape obj;
objShape cone;
objShape cylinder;
objShape star;

// identyfiakator tekstury
GLuint tgaTex;

//=============================================================================
// załadowanie pliku TGA i zrobienie z niego tekstury
//=============================================================================
bool loadTGATexture(const char *texFileName)
{
	GLbyte *texPointer;
	int width, height, components;
	GLenum format;


	// Read the texture bits
	texPointer = gltReadTGABits(texFileName, &width, &height, &components, &format);
	if(texPointer == NULL) 
		return false;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, components, width, height, 0, format, GL_UNSIGNED_BYTE, texPointer);
	
	free(texPointer);

	return true;
}

//=============================================================================
// inicjalizacja stałych elementów maszyny stanu OpenGL
//=============================================================================

void readObj(char *objFileName)
{

}

int init(char *objFileName, char *texFileName)
{
// definicja sześcianu:
// współrzędne wierzchołków wierzchołki
GLfloat cubeVertices[4*3] = {
	-0.5f,  0.f,  0.5f,
	 0.5f,  0.f,  0.5f,
	-0.5f,  0.f, -0.5f,
	 0.5f,  0.f, -0.5f,
};

// normalne - nie ma potrzeby ich definiowania.
// Współrzędne wierzchołków są za razem wektorami nomralnymi (po ich unormowaniu).

// współrzędne tekstur dla ścian
GLfloat cubeTexCoords[4*2] = {
	// 1-sza ściana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};

// indeksy
GLuint cubeIndices[3*2] = {
	// 1-sza ściana
	0, 1, 2,
	2, 1, 3
};

	int objError;
	// identyfikatory obiektów buforów ze współrzędnymi, normalnymi i indeksami
	GLuint verticesBuffer;
	GLuint normalsBuffer;
	GLuint indicesBuffer;
	GLuint texturesBuffer;
	
	// indeksy atrybutów wierzchołków (współrzędne i normalne) z obiektu shadera
	GLuint verticesLocation;
	GLuint normalsLocation;
	GLuint texCoordsLocation;

	// wczytanie obiektu z pliku .obj i przygotowanie go
	if ((objError = obj.readFromFile(objFileName)) )
		return objError;
	
	// przeskalowanie wczytanego obj, tak aby był wpisany w jednostkowy sześcian
	// o środku w początku układu współrzędnych
	obj.scale();

	// sprawdzenie czy zostały poprawnie zdefiniowane normalne
	if (!obj.nNormals)
		// wygenerowanie uśrednionych normalnych
		obj.genSmoothNormals();
		// wygenerowanie normalnych dla ścianek
		//obj.genFacesNormals();
	else
		if (!obj.normIndGood)
			// gdy indeksy normalnych nie zgadzają się z indeksami wierzhołków
			// należy przebudować obie tablice, aby były tak samo indeksowane
			// przbudowanie indeksów normalnych i jeśli trzeba indeksów wierchołków
			obj.rebuildAttribTable('n');



	// ustawienie koloru tła
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// wczytanie shaderów i przygotowanie obsługi programu
	AttachVertexShader	(texShader, "ADS_light_tex_vs.glsl");
	AttachFragmentShader(texShader, "ADS_light_tex_fs.glsl");
	
	// wykonanie powiązania pomiędzy zmienną a indeksem ogólnych atrybutów wierzchołka
	// operacja ta nie jest konieczna i będzie wykonana automatycznie w czasie
	// konsolidacji shadera - przypisany zostanie "pierwszy wolny" indeks
	LinkProgram(texShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation	= glGetAttribLocation(texShader, "inVertex");
	normalsLocation		= glGetAttribLocation(texShader, "inNormal");
	texCoordsLocation	= glGetAttribLocation(texShader, "inTexCoord");

	// wygenerowanie i włączenie tablicy wierzchołków sześcianu
	glGenVertexArrays(1, &cubeVertexArray);
	glBindVertexArray(cubeVertexArray);

	// utworzenie obiektu bufora wierzchołków (VBO) i załadowanie danych
	//współrzędne
	glGenBuffers(1, &verticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(verticesLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// normalne:
	glGenBuffers(1, &normalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), (GLfloat*)cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(normalsLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// współrzędne tekstury:
	glGenBuffers(1, &texturesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeTexCoords), (GLfloat*)cubeTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(texCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// indeksy:
	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), (GLuint*)cubeIndices, GL_STATIC_DRAW);

	// włączenie obiektów buforów wierchołków
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);
	glEnableVertexAttribArray(texCoordsLocation);

	AttachVertexShader(toonShader, "toon_light_vs.glsl");
	AttachFragmentShader(toonShader, "toon_light_fs.glsl");
	// wykonanie powiązania pomiędzy zmienną a indeksem ogólnych atrybutów wierzchołka
	// operacja ta nie jest konieczna i będzie wykonana automatycznie w czasie
	// konsolidacji shadera - przypisany zostanie "pierwszy wolny" indeks
	LinkProgram(toonShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation = glGetAttribLocation(toonShader, "inVertex");
	normalsLocation = glGetAttribLocation(toonShader, "inNormal");

	// wygenerowanie i włączenie tablicy wierzchołków .obj
	glGenVertexArrays(1, &objVertexArray);
	glBindVertexArray(objVertexArray);

	//współrzędne
	glGenBuffers(1, &verticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3*obj.nAttribs*sizeof(GLfloat), obj.vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(verticesLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// normalne:
	glGenBuffers(1, &normalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3*obj.nAttribs*sizeof(GLfloat), obj.normals, GL_STATIC_DRAW);
	glVertexAttribPointer(normalsLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// indeksy:
	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*obj.nFaces*sizeof(GLuint), obj.faces, GL_STATIC_DRAW);

	// włączenie obiektów buforów wierchołków
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);

	// wyłączenie tablicy wierchołków
	glBindVertexArray(0);

	// wygenerowanie i załadowanie tekstury 
	glGenTextures(1, &tgaTex);
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// załadowanie tekstury z pliku .tga
	
	if (! loadTGATexture(texFileName)) {
		cout << "Nie ma pliku z tekstura: \"" << texFileName << "\"" << endl;
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// włączenie wykorzystania bufora głębokości
	glEnable(GL_DEPTH_TEST);
	// włączenie pominięcia renderowania tylnych stron wielokątów
	glEnable(GL_CULL_FACE);

	// ustawienie początkowego położenia kamery
	cameraFrame.SetOrigin(1.1f, 1.1f, 15.0f);
	// zdefiniowanie potoku renderowania składającego się z dwóch stosów macierzy
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
	return 0;
}

//=============================================================================
// zmiana rozmiaru okna
//=============================================================================
void reshape(int width, int height)
{
	// ustawienie obszaru renderingu - całe okno

	glViewport(0, 0, width, height);

	// utworzenie bryły obcięcia określającej perspektywę
	viewFrustum.SetPerspective(55.0f, float(width)/float(height), 1.0f, 100.0f);
	// załadowanie macierzy opisującej bryłę obcięcia do macierzy Projekcji
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
}

void drawTrunk(M3DVector4f lightEyeDir)
{
	
	//---------------------------------------------------------------------------------------------
	// === przekształcenia geometryczne i narysowanie obiektu w innym stanie układu ===
	// Odłożenie obiektu macierzy na stos
	modelViewMatrix.PopMatrix();

	// użycie obiektu shadera
	glUseProgram(texShader);

	glUniform3fv(glGetUniformLocation(texShader, "inLightDir"), 1, lightEyeDir);

	modelViewMatrix.Rotate(rotateX, 1.0f, 0.0f, 0.0f);
	modelViewMatrix.Rotate(rotateY, 0.0f, 1.0f, 0.0f);
	modelViewMatrix.Rotate(rotateZ, 0.0f, 0.0f, 1.0f);

	modelViewMatrix.Scale(1.0f, 2.0f, 1.0f);

	// przesunięcie układu
	modelViewMatrix.Translate(0.0f, 0.5f, 0.0f);

	// wykonanie obrotów układu


	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	// załadowanie zmiennej jednorodnej - identyfikatora tekstury
	glUniform1i(glGetUniformLocation(texShader ,"fileTexture"), 0);

	// włączenie tablicy wierzchołków sześcianu
	glBindVertexArray(cubeVertexArray);
	
	// włączenie tablicy wierzchołków .obj
	glBindVertexArray(objVertexArray);
	// narysowanie danych zawartych w tablicy wierzchołków .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*obj.nFaces, GL_UNSIGNED_INT, 0);

	// zdjęcie zapamiętanej macierzy widoku-mocelu ze stosu
	modelViewMatrix.PopMatrix();

	//---------------------------------------------------------------------------------------------
}

//=============================================================================
// wyświetlenie sceny
//=============================================================================
void display(void)
{
	// czyszczenie bufora koloru
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	M3DMatrix44f mCamera;
	// pobranie macierzy 4x4 (z obiektu kamery) zawierającej przekształcenia kamery
	cameraFrame.GetCameraMatrix(mCamera);
	// załadowanie przekształceń kamery do stosu macierzy Widoku Modelu
	// (nie ma zatem potrzeby uprzedniego ładowania macierzy jednostkowej)
	modelViewMatrix.LoadMatrix(mCamera);


	// użycie obiektu shadera
	glUseProgram(toonShader);

	M3DVector4f lightEyeDir;
	// przetransformowanie kierunku światła do współrzędnych obserwatora
	m3dTransformVector4(lightEyeDir, lightDir, mCamera);
	// załadowanie zmiennej jednorodnej - kierunku światla
	// dalsze przekształcenia zmieniają macierz Widoku Modelu, ale nie wpływają
	// na światło - obracamy obiektami a nie światłem
	float light[3] = {5.0, 5.0, 2.0};
	glUniform3fv(glGetUniformLocation(toonShader, "inLightDir"), 1, lightEyeDir);
	glUniform3fv(glGetUniformLocation(toonShader, "inLightDir"), 1, light);

	// === przekształcenia geometryczne i narysowanie obiektu w innym stanie układu ===
	// Odłożenie obiektu macierzy na stos
	modelViewMatrix.PushMatrix();

	// użycie obiektu shadera
	glUseProgram(texShader);

	glUniform3fv(glGetUniformLocation(texShader, "inLightDir"), 1, lightEyeDir);

	// przesunięcie układu
	modelViewMatrix.Translate(0.0f, 0.0f, 0.0f);
	modelViewMatrix.Scale(10.0f, 10.0f, 10.0f);
	// wykonanie obrotów układu
	modelViewMatrix.Rotate(rotateX, 1.0f, 0.0f, 0.0f);
	modelViewMatrix.Rotate(rotateY, 0.0f, 1.0f, 0.0f);
	modelViewMatrix.Rotate(rotateZ, 0.0f, 0.0f, 1.0f);

	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	// załadowanie zmiennej jednorodnej - identyfikatora tekstury
	glUniform1i(glGetUniformLocation(texShader ,"fileTexture"), 0);

	// włączenie tablicy wierzchołków sześcianu
	glBindVertexArray(cubeVertexArray);
	
	// włączenie aktywnej tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// narysowanie danych zawartych w tablicy wierchołków sześcianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

	// zdjęcie zapamiętanej macierzy widoku-mocelu ze stosu
	//modelViewMatrix.PopMatrix();

	drawTrunk(lightEyeDir);
	// wyłączenie tablic wierzhołków
	glBindVertexArray(0);

	// wyłączenie tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);

	// wyłączenie shadera
	glUseProgram(0);

	// wyrenderowanie sceny
	glFlush();
}

//=============================================================================
// obsługa klawiatury - klawisze standardowe
//=============================================================================
void standardKbd(unsigned char key, int x, int y)
{
	GLint viewport[4]; // aktualne parametry okna

	// pobranie własności bieżącego okna - współrzędne x,y okna oraz jego
	// szerokość i wysokość
	glGetIntegerv(GL_VIEWPORT,viewport);

	// obsługa standardowych klawiszy
	switch (key) {
		// zdefiniowanie obrotów kamery wokół osi x, y, z
		case 'x': cameraFrame.RotateWorld(0.1f, 1.0f, 0.0f, 0.0f);
			break;
		case 'X': cameraFrame.RotateWorld(-0.1f, 1.0f, 0.0f, 0.0f);
			break;
		case 'y': cameraFrame.RotateWorld(0.1f, 0.0f, 1.0f, 0.0f);
			break;
		case 'Y': cameraFrame.RotateWorld(-0.1f, 0.0f, 1.0f, 0.0f);
			break;
		case 'z': cameraFrame.RotateWorld(0.1f, 0.0f, 0.0f, 1.0f);
			break;
		case 'Z': cameraFrame.RotateWorld(-0.1f, 0.0f, 0.0f, 1.0f);
			break;
		// zdefiniowanie obrotów układu wokół osi x, y, z
		case 'j': rotateX += 10.0f;
			break;
		case 'J': rotateX -= 10.0f;
			break;
		case 'k': rotateY += 10.0f;
			break;
		case 'K': rotateY -= 10.0f;
			break;
		case 'l': rotateZ += 10.0f;
			break;
		case 'L': rotateZ -= 10.0f;
			break;
		// zdefiniowanie wektora kierunku światła
		case 'u': lightDir[0] += 0.1;
			break;
		case 'U': lightDir[0] -= 0.1;
			break;
		case 'i': lightDir[1] += 0.1;
			break;
		case 'I': lightDir[1] -= 0.1;
			break;
		case 'o': lightDir[2] += 0.1;
			break;
		case 'O': lightDir[2] -= 0.1;
			break;

		case 27: exit(0);
	}
	// wymuszenie odrysowania okna
	// (wywołanie zarejestrowanej funcji do obsługi tego zdarzenia)
	glutPostRedisplay();
}

//=============================================================================
// obsługa klawiatury - klawisze specjalne
//=============================================================================
void specialKbd (int key, int x, int y)
{
	// obsługa klawiszy funkcyjnych - analogicznie jak podstawowych
	switch (key) {
		case GLUT_KEY_RIGHT: cameraFrame.TranslateWorld(-0.1f, 0.0f, 0.0f);
			break;
		case GLUT_KEY_LEFT: cameraFrame.TranslateWorld(0.1f, 0.0f, 0.0f);
			break;
		case GLUT_KEY_UP: cameraFrame.TranslateWorld(0.0f, -0.1f, 0.0f);
			break;
		case GLUT_KEY_DOWN: cameraFrame.TranslateWorld(0.0f, 0.1f, 0.0f);
			break;
		case GLUT_KEY_PAGE_UP: cameraFrame.TranslateWorld(0.0f, 0.0f, -0.1f);
			break;
		case GLUT_KEY_PAGE_DOWN: cameraFrame.TranslateWorld(0.0f, 0.0f, 0.1f);
			break;
	}
	glutPostRedisplay();
}

//=============================================================================
// główna funkcja programu
//=============================================================================

int main(int argc, char** argv)
{
	if (argc != 3) {
		cout << "usage:" << endl;
		cout << "   " << argv[0] << " <obj file> <tex file>" << endl;
		return 1;
		int d;
		cin >> d;
	}

	gltSetWorkingDirectory(argv[0]);
	// ustalenie odpowiedniego kontekstu renderowania
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_DEBUG);
	// określenie wykorzystywanego profilu - profil CORE pełna zgodność z v3.2
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// inicjalizacja biblioteki GLUT
	glutInit(&argc, argv);
	// określenie trybu pracy biblioteki - kolor w formacie RGB
	glutInitDisplayMode(GLUT_RGB);
	// rozmiar tworzonego okna (w pikselach)
	glutInitWindowSize(600, 600);
	// położenie okna na ekranie (względem lewego dolnego rogu)
	glutInitWindowPosition(100, 100);
	// stworzenie okna programu
	glutCreateWindow("Choinka lol");

	// inicjalizacja biblioteki GLEW
	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	// sprawdzenie poprawności inicjalizacji GLEWa
	if (GLEW_OK != glewErr) {
		// nieudana inicjalizacja biblioteki
		cout << "Blad glewInit: " << glewGetErrorString(glewErr) << endl;
		return 2;
	}
	cout << "Wersja biblioteki GLEW: " << glewGetString(GLEW_VERSION) << endl;


	// wykonanie czynności przygotowawczych programu
	if (init("resources\\obj\\cylinder.obj", "resources\\tga\\grass.tga"))
		return 3;


	// ======================   funkcje callback ==================================
	// funkcja obsługująca zdarzenie konieczności odrysowania okna
	glutDisplayFunc(display);
	// funkcja obsługująca zdarzenie związane ze zmianą rozmiaru okna
	glutReshapeFunc(reshape);
	// funkcja obsługująca naciśnięcie standardowego klawisza z klawiatury
	glutKeyboardFunc(standardKbd);
	// funkcja obsługująca naciśnięcie klawisza specjalnego z klawiatury
	glutSpecialFunc(specialKbd);
 	//=============================================================================
	// główna pętla programu
	glutMainLoop();

	return 0;
}


