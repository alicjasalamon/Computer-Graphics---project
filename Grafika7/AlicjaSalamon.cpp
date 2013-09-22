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

// kierunek źródła światła
GLfloat lightDir[4];
M3DVector4f lightEyeDir;

// identyfikator obiektu programu
GLuint singleColorShader, texShader;

// identyfikatory obiektów tablic wierchołków dla .obj i sciany
GLuint objVertexArray;
GLuint coneVertexArray;
GLuint cylinderVertexArray;
GLuint groundVertexArray;
GLuint starVertexArray;

// obiekty reprezentujący bryły wczytane jako .obj
objShape obj;
objShape cone;
objShape cylinder;
objShape star;

// identyfiakator tekstury
GLuint tgaTex;
GLuint starTex;

//color
GLuint baseColorUnif;

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
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

// identyfikatory obiektów buforów ze współrzędnymi, normalnymi i indeksami
GLuint verticesBuffer;
GLuint normalsBuffer;
GLuint indicesBuffer;
GLuint texturesBuffer;
	
// indeksy atrybutów wierzchołków (współrzędne i normalne) z obiektu shadera
GLuint verticesLocation;
GLuint normalsLocation;
GLuint texCoordsLocation;

void prepareGround()
{
	
	// współrzędne wierzchołków
	GLfloat groundVertices[4*3] = {
		-0.5f,  0.f,  0.5f,
		 0.5f,  0.f,  0.5f,
		-0.5f,  0.f, -0.5f,
		 0.5f,  0.f, -0.5f
	};

	// normalne - nie ma potrzeby ich definiowania.
	// Współrzędne wierzchołków są za razem wektorami normalnymi (po ich unormowaniu).

	// współrzędne tekstur dla ścianki
	GLfloat groundTexCoords[4*2] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};

	// indeksy
	GLuint groundIndices[3*2] = {
		0, 1, 2,
		2, 1, 3
	};

	// wygenerowanie i włączenie tablicy wierzchołków sześcianu
	glGenVertexArrays(1, &groundVertexArray);
	glBindVertexArray(groundVertexArray);

	// utworzenie obiektu bufora wierzchołków (VBO) i załadowanie danych
	//współrzędne
	glGenBuffers(1, &verticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(verticesLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// normalne:
	glGenBuffers(1, &normalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), (GLfloat*)groundVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(normalsLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// współrzędne tekstury:
	glGenBuffers(1, &texturesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundTexCoords), (GLfloat*)groundTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(texCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// indeksy:
	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), (GLuint*)groundIndices, GL_STATIC_DRAW);

}

void readObj(objShape &obj, char *objFileName, GLuint &objVertexArray)
{
	// wczytanie obiektu z pliku .obj i przygotowanie go
	obj.readFromFile(objFileName);

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

}

int init()
{
	// ustawienie koloru tła na granatowy
	glClearColor(0.0f, 49.0/256.0 , 83.0f/256.0, 1.0f);

	// wczytanie shaderów i przygotowanie obsługi programu
	AttachVertexShader	(texShader,		"textures_vs.glsl");
	AttachFragmentShader(texShader,		"textures_fs.glsl");
	AttachVertexShader	(singleColorShader,	"nothing_special_vs.glsl");
	AttachFragmentShader(singleColorShader,	"nothing_special_fs.glsl");

	// wykonanie powiązania pomiędzy zmienną a indeksem ogólnych atrybutów wierzchołka
	LinkProgram(texShader);
	LinkProgram(singleColorShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation	= glGetAttribLocation(texShader, "inVertex");
	normalsLocation		= glGetAttribLocation(texShader, "inNormal");
	texCoordsLocation	= glGetAttribLocation(texShader, "inTexCoord");
	prepareGround();
	
	// włączenie obiektów buforów wierchołków
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);
	glEnableVertexAttribArray(texCoordsLocation);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation	= glGetAttribLocation(singleColorShader, "inVertex");
	normalsLocation		= glGetAttribLocation(singleColorShader, "inNormal");

	// wygenerowanie i włączenie tablicy wierzchołków .obj
	readObj(star,		"resources\\obj\\star.obj",	starVertexArray); 
	readObj(cone,		"resources\\obj\\cone.obj",		coneVertexArray); 
	readObj(cylinder,	"resources\\obj\\cylinder.obj",	cylinderVertexArray); 

	// wyłączenie tablicy wierchołków
	glBindVertexArray(0);

	// wygenerowanie i załadowanie tekstury 
	glGenTextures(1, &tgaTex);
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// załadowanie tekstury z pliku .tga
	if (! loadTGATexture("resources\\tga\\sciolka.tga")) {
		cout << "Nie ma pliku z teksturą" << endl;
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// wygenerowanie i załadowanie tekstury 
	glGenTextures(1, &starTex);
	glBindTexture(GL_TEXTURE_2D, starTex);
	// załadowanie tekstury z pliku .tga
	if (! loadTGATexture("resources\\tga\\niebo.tga")) {
		cout << "Nie ma pliku z teksturą" << endl;
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// włączenie wykorzystania bufora głębokości
	glEnable(GL_DEPTH_TEST);
	// włączenie pominięcia renderowania tylnych stron wielokątów
	glEnable(GL_CULL_FACE);

	// ustawienie początkowego położenia kamery
	cameraFrame.SetOrigin(35.0f, 18.0f, 25.0f);

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
	viewFrustum.SetPerspective(55.0f, float(width)/float(height), 1.0f, 1000.0f);
	// załadowanie macierzy opisującej bryłę obcięcia do macierzy Projekcji
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
}

void drawTrunk()
{
	modelViewMatrix.Scale(1.0f, 2.0f, 1.0f);
	modelViewMatrix.Translate(0.0f, 0.5f, 0.0f);

	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewProjectionMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	//zaladowanie zmiennej jednorodnej - kolor pnia
	glUniform4f(glGetUniformLocation(singleColorShader ,"color2"), 0.0f, 0.0f, 0.0f, 1.0f);	
	
	// włączenie tablicy wierzchołków .obj
	glBindVertexArray(cylinderVertexArray);
	// narysowanie danych zawartych w tablicy wierzchołków .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*cylinder.nFaces, GL_UNSIGNED_INT, 0);

}


void drawStar()
{
	modelViewMatrix.Scale(0.8f, 1.0f, 0.5f);
	modelViewMatrix.Translate(0.0f, 0.7f, 0.0f);

	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewProjectionMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	//zaladowanie zmiennej jednorodnej - kolor gornej gwiazdki
	glUniform4f(glGetUniformLocation(singleColorShader ,"color2"), 254.0/256.0, 254.0/256.0, 51.0/256.0, 1.0f);	

	// włączenie tablicy wierzchołków .obj
	glBindVertexArray(starVertexArray);
	// narysowanie danych zawartych w tablicy wierzchołków .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*star.nFaces, GL_UNSIGNED_INT, 0);

}

double angle = 0.0;
double R=1.0, G=1.0, B=1.0;
void drawStars(double x, double y, double z, double s)
{
	modelViewMatrix.PushMatrix();

	modelViewMatrix.Scale(s, s, s);
	modelViewMatrix.Translate(x, y, z);
	modelViewMatrix.Rotate(angle, 0.0, 1.0, 0.0);
	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewProjectionMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	//zaladowanie zmiennej jednorodnej - kolor bombek
	glUniform4f(glGetUniformLocation(singleColorShader ,"color2"), R, G, B, 1.0f);	

	// włączenie tablicy wierzchołków .obj
	glBindVertexArray(starVertexArray);
	// narysowanie danych zawartych w tablicy wierzchołków .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*star.nFaces, GL_UNSIGNED_INT, 0);

	// zdjęcie zapamiętanej macierzy widoku-mocelu ze stosu
	modelViewMatrix.PopMatrix();
	
}

void drawLeaves(double x, double y, double z, double scale_x, double scale_y, double scale_z)
{
	modelViewMatrix.Translate(x,y,z);
	modelViewMatrix.Scale(scale_x, scale_y, scale_z);

	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewProjectionMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(singleColorShader, "modelViewMatrix"), 1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	//zaladowanie zmiennej jednorodnej - kolor drzewka
	glUniform4f(glGetUniformLocation(singleColorShader ,"color2"), 0.0f, 102.0f/256.0, 51.0f/256.0, 1.0f);
	
	// włączenie tablicy wierzchołków .obj
	glBindVertexArray(coneVertexArray);
	// narysowanie danych zawartych w tablicy wierzchołków .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*cone.nFaces, GL_UNSIGNED_INT, 0);

}

void drawGround()
{
	modelViewMatrix.PushMatrix();
	glUniform3fv(glGetUniformLocation(texShader, "inLightDir"), 1, lightEyeDir);

	modelViewMatrix.Scale(200.0f, 200.0f, 200.0f);
	
	
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
	glBindVertexArray(groundVertexArray);
	
	// włączenie aktywnej tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);

	// narysowanie danych zawartych w tablicy wierchołków sześcianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);

	modelViewMatrix.PopMatrix();

}


void drawSky(double a, double xR, double yR, double zR, double xT, double yT, double zT)
{
	modelViewMatrix.PushMatrix();
	
	// przesunięcie układu
	modelViewMatrix.Rotate(a, xR, yR, zR);
	modelViewMatrix.Translate(xT, yT, zT);
	modelViewMatrix.Scale(200.0f, 200.0f, 200.0f);	

	// wykonanie obrotów układu
	// załadowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// załadowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	glBindTexture(GL_TEXTURE_2D, starTex);

	// narysowanie danych zawartych w tablicy wierchołków sześcianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0);


	modelViewMatrix.PopMatrix();
}

void drawTree()
{

	//////////////////////////////////////////////////////////////////////////////////////
	//									DÓŁ												//
	//////////////////////////////////////////////////////////////////////////////////////

	drawLeaves(0.0f, 1.5f, 0.0f, 8.0f, 2.0f, 8.0f);

	drawStars( 2.5, -1.0, -2.5, 0.1);
	drawStars(-2.5, -1.0, -3.0, 0.1);
	drawStars(-2.5, -1.0,  2.5, 0.1);
	drawStars( 2.5, -1.0,  2.5, 0.1);

	//////////////////////////////////////////////////////////////////////////////////////
	//									ŚRODEK											//
	//////////////////////////////////////////////////////////////////////////////////////

	drawLeaves(0.0f, 0.5f, 0.0f, 0.7f, 0.8f, 0.7f);

	drawStars( 2.5, -1.0,  0.0, 0.15);
	drawStars(-2.5, -1.0,  0.0, 0.15);
	drawStars( 0.0, -1.0, -2.5, 0.15);
	drawStars( 0.0, -1.0,  2.5, 0.15);

	//////////////////////////////////////////////////////////////////////////////////////
	//									GÓRA											//
	//////////////////////////////////////////////////////////////////////////////////////

	drawLeaves(0.0f, 0.5f, 0.0f, 0.6f, 0.6f, 0.6f);

	drawStars( 1.5f, -1.0f, -1.5f, 0.2f);
	drawStars(-1.5f, -1.0f, -1.5f, 0.2f);
	drawStars(-1.5f, -1.0f,  1.5f, 0.2f);
	drawStars( 1.5f, -1.0f,  1.5f, 0.2f);

}

void drawChristmasTree(double x, double z, double scale = 1.0)
{
	modelViewMatrix.PushMatrix();
	modelViewMatrix.Scale(scale, scale, scale);
	modelViewMatrix.Translate(x, 0.0, z);
	drawTrunk();
	drawTree();
	drawStar();
	modelViewMatrix.PopMatrix();
}

//=============================================================================
// wyświetlenie sceny
//=============================================================================
double rotation = 0.0f;
double lightRot = 0.0f;
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

	angle += 1.0f;
	rotation += 0.1;

	lightDir[0]=50*sin(lightRot);
	lightDir[1]=50*cos(lightRot);
	
	// przetransformowanie kierunku światła do współrzędnych obserwatora
	m3dTransformVector4(lightEyeDir, lightDir, mCamera);
	// załadowanie zmiennej jednorodnej - kierunku światla
	// dalsze przekształcenia zmieniają macierz Widoku Modelu, ale nie wpływają
	// na światło - obracamy obiektami a nie światłem
	glUniform3fv(glGetUniformLocation(singleColorShader, "inLightDir"), 1, lightEyeDir);

	// === przekształcenia geometryczne i narysowanie obiektu w innym stanie układu ===
	// Odłożenie obiektu macierzy na stos
	modelViewMatrix.Rotate(rotation, 0.0, 1.0, 0.0);
	//modelViewMatrix.Translate(5.0, 0.0, 0.0);
	//rotation += 0.1;

	//modelViewMatrix.PushMatrix();

	// użycie obiektu shadera
	glUseProgram(texShader);
	

	drawGround();
	drawSky(90.0, 0.0, 0.0, 1.0, 0.0, -100.0, 0.0);
	drawSky(180.0, 0.0, 0.0, 1.0, 0.0, -100.0, 0.0);
	drawSky(270.0, 0.0, 0.0, 1.0, 0.0, -100.0, 0.0);
	drawSky(90.0, 1.0, 0.0, 0.0, 0.0, -100.0, 0.0);
	drawSky(270.0, 1.0, 0.0, 0.0, 0.0, -100.0, 0.0);

	// zdjęcie zapamiętanej macierzy widoku-mocelu ze stosu
	
	//modelViewMatrix.PopMatrix();

	// użycie obiektu shadera
	glUseProgram(singleColorShader);
	glUniform3fv(glGetUniformLocation(singleColorShader, "inLightDir"), 1, lightEyeDir);
	
	drawChristmasTree(40, -40, 2.0);
	drawChristmasTree(40, 40, 2.0);
	drawChristmasTree(-40, -40, 2.0);
	drawChristmasTree(-40, 40, 2.0);
	drawChristmasTree(-13, -46, 2.0);
	drawChristmasTree(24, -35, 2.0);
	drawChristmasTree(29, 46, 2.0);
	drawChristmasTree(-27, -39, 2.0);
	drawChristmasTree(-39, -9, 2.0);
	drawChristmasTree(-34, 31, 2.0);
	drawChristmasTree(24, 28, 2.0);
	drawChristmasTree(40, -20, 2.0);
	drawChristmasTree(40, 20, 2.0);
	drawChristmasTree(-20, -40, 2.0);
	drawChristmasTree(-20, 40, 2.0);


	// wyłączenie tablic wierzhołków
	glBindVertexArray(0);
	// wyłączenie tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// wyłączenie shadera
	glUseProgram(0);
	// wyrenderowanie sceny
	glFlush();

	glutSwapBuffers();
	glutPostRedisplay();
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
		case '1': { R=1.0; G=1.0; B=1.0;}
			break;
		case '2': { R=1.0; G=1.0; B=0.0;}
			break;
		case '3': { R=0.0; G=1.0; B=0.0;}
			break;
		case '4': { R=0.0; G=1.0; B=1.0;}
			break;
		case 'k': { lightRot += 0.05;}
			break;
		case 'l': { lightRot -= 0.05;}
			break;
		case 27: exit(0);
	}
	// wymuszenie odrysowania okna
	// (wywołanie zarejestrowanej funcji do obsługi tego zdarzenia)
	glutPostRedisplay();
}


//=============================================================================
// główna funkcja programu
//=============================================================================

int main(int argc, char** argv)
{
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
	glutCreateWindow("Alicja Salamon i super choinki");

	// inicjalizacja biblioteki GLEW
	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	// sprawdzenie poprawności inicjalizacji GLEWa
	if (GLEW_OK != glewErr) {
		// nieudana inicjalizacja biblioteki
		cout << "Blad glewInit: " << glewGetErrorString(glewErr) << endl;
		return 2;
	}

	init();

	// ======================   funkcje callback ==================================
	// funkcja obsługująca zdarzenie konieczności odrysowania okna
	glutDisplayFunc(display);
	// funkcja obsługująca zdarzenie związane ze zmianą rozmiaru okna
	glutReshapeFunc(reshape);
	// funkcja obsługująca naciśnięcie standardowego klawisza z klawiatury
	glutKeyboardFunc(standardKbd);
 	//=============================================================================
	// główna pętla programu
	glutMainLoop();

	return 0;
}