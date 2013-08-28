// do³¹czenie plików nag³ówkowych z biblioteki J.Ganczarskiego
#include "shaders.h"
#include "vecmatquat.h"
#include "obj.h"
#include "targa.h"

// za³adowanie pliku nag³ówkowego biblioteki GLEW - znajduj¹ siê w nim wszystkie
// rozszerzenia biblioteki OpenGL równie¿ dla wersji wy¿szych ni¿ 3.0
// Nie ma wiêc potrzeby wykorzysytwania pliku nag³ówkowego gl3.h
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
GLFrustum			viewFrustum;		// bry³a obciêcia
GLGeometryTransform	transformPipeline;	// potok renderowania
GLFrame				cameraFrame;		// obiekt kamery

// k¹ty obrotu obiektu
GLfloat rotateX = 10.0f;
GLfloat rotateY = 20.0f;
GLfloat rotateZ = 30.0f;

// kierunek Ÿród³a œwiat³a
GLfloat lightDir[4] = {5.0f, 5.0f, 3.0f, 1.0f};

// identyfikator obiektu programu
GLuint toonShader, texShader;

// identyfikatory obiektów tablic wiercho³ków dla .obj i szeœcianu
GLuint objVertexArray;
GLuint cubeVertexArray;

// obiekt reprezentuj¹cy bry³ê wczytan¹ jako .obj
objShape obj;

// identyfiakator tekstury
GLuint tgaTex;

//=============================================================================
// za³adowanie pliku TGA i zrobienie z niego tekstury
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
// inicjalizacja sta³ych elementów maszyny stanu OpenGL
//=============================================================================
int init(char *objFileName, char *texFileName)
{
// definicja szeœcianu:
// wspó³rzêdne wierzcho³ków wierzcho³ki
GLfloat cubeVertices[24*3] = {
	-0.5f, -0.5f,  0.5f,//0
	 0.5f, -0.5f,  0.5f,//1
	-0.5f,  0.5f,  0.5f,//2
	 0.5f,  0.5f,  0.5f,//3

	 0.5f, -0.5f,  0.5f,//1
	 0.5f, -0.5f, -0.5f,//5
	 0.5f,  0.5f,  0.5f,//3
	 0.5f,  0.5f, -0.5f,//7

	 0.5f, -0.5f, -0.5f,//5
	-0.5f, -0.5f, -0.5f,//4
	 0.5f,  0.5f, -0.5f,//7
	-0.5f,  0.5f, -0.5f,//6

	-0.5f, -0.5f, -0.5f,//4
	-0.5f, -0.5f,  0.5f,//0
	-0.5f,  0.5f, -0.5f,//6
	-0.5f,  0.5f,  0.5f,//2

	-0.5f,  0.5f,  0.5f,//2
	 0.5f,  0.5f,  0.5f,//3
	-0.5f,  0.5f, -0.5f,//6
	 0.5f,  0.5f, -0.5f,//7

	 0.5f, -0.5f,  0.5f,//1
	-0.5f, -0.5f,  0.5f,//0
	 0.5f, -0.5f, -0.5f,//5
	-0.5f, -0.5f, -0.5f //4
};

// normalne - nie ma potrzeby ich definiowania.
// Wspó³rzêdne wierzcho³ków s¹ za razem wektorami nomralnymi (po ich unormowaniu).

// wspó³rzêdne tekstur dla œcian
GLfloat cubeTexCoords[24*2] = {
	// 1-sza œciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 2-ga œciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 3-cia œciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 4-ta œciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 5-ta œciana
    0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 6-ta œciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};

// indeksy
GLuint cubeIndices[3*6*2] = {
	// 1-sza œciana
	0, 1, 2,
	2, 1, 3,
	// 1-ga œciana
	4, 5, 6,
	6, 5, 7,
	// 3-cia œciana
	8, 9, 10,
	10, 9, 11,
	// 4-ta œciana
	12, 13, 14,
	14, 13, 15,
	// 5-ta œciana
	16, 17, 18,
	18, 17, 19,
	// 6-ta œciana
	20, 21, 22,
	22, 21, 23
};

	int objError;
	// identyfikatory obiektów buforów ze wspó³rzêdnymi, normalnymi i indeksami
	GLuint verticesBuffer;
	GLuint normalsBuffer;
	GLuint indicesBuffer;
	GLuint texturesBuffer;
	// indeksy atrybutów wierzcho³ków (wspó³rzêdne i normalne) z obiektu shadera
	GLuint verticesLocation;
	GLuint normalsLocation;
	GLuint texCoordsLocation;

	// wczytanie obiektu z pliku .obj i przygotowanie go
	if ( (objError = obj.readFromFile(objFileName)) )
		return objError;
	
	// przeskalowanie wczytanego obj, tak aby by³ wpisany w jednostkowy szeœcian
	// o œrodku w pocz¹tku uk³adu wspó³rzêdnych
	obj.scale();

	// sprawdzenie czy zosta³y poprawnie zdefiniowane normalne
	if (!obj.nNormals)
		// wygenerowanie uœrednionych normalnych
		obj.genSmoothNormals();
		// wygenerowanie normalnych dla œcianek
		//obj.genFacesNormals();
	else
		if (!obj.normIndGood)
			// gdy indeksy normalnych nie zgadzaj¹ siê z indeksami wierzho³ków
			// nale¿y przebudowaæ obie tablice, aby by³y tak samo indeksowane
			// przbudowanie indeksów normalnych i jeœli trzeba indeksów wiercho³ków
			obj.rebuildAttribTable('n');

	// ustawienie koloru t³a
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// wczytanie shaderów i przygotowanie obs³ugi programu
	AttachVertexShader	(texShader, "ADS_light_tex_vs.glsl");
	AttachFragmentShader(texShader, "ADS_light_tex_fs.glsl");
	
	// wykonanie powi¹zania pomiêdzy zmienn¹ a indeksem ogólnych atrybutów wierzcho³ka
	// operacja ta nie jest konieczna i bêdzie wykonana automatycznie w czasie
	// konsolidacji shadera - przypisany zostanie "pierwszy wolny" indeks
	LinkProgram(texShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation	= glGetAttribLocation(texShader, "inVertex");
	normalsLocation		= glGetAttribLocation(texShader, "inNormal");
	texCoordsLocation	= glGetAttribLocation(texShader, "inTexCoord");

	// wygenerowanie i w³¹czenie tablicy wierzcho³ków szeœcianu
	glGenVertexArrays(1, &cubeVertexArray);
	glBindVertexArray(cubeVertexArray);

	// utworzenie obiektu bufora wierzcho³ków (VBO) i za³adowanie danych
	//wspó³rzêdne
	glGenBuffers(1, &verticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(verticesLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// normalne:
	glGenBuffers(1, &normalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), (GLfloat*)cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(normalsLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// wspó³rzêdne tekstury:
	glGenBuffers(1, &texturesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeTexCoords), (GLfloat*)cubeTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(texCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// indeksy:
	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), (GLuint*)cubeIndices, GL_STATIC_DRAW);

	// w³¹czenie obiektów buforów wiercho³ków
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);
	glEnableVertexAttribArray(texCoordsLocation);

	AttachVertexShader(toonShader, "toon_light_vs.glsl");
	AttachFragmentShader(toonShader, "toon_light_fs.glsl");
	// wykonanie powi¹zania pomiêdzy zmienn¹ a indeksem ogólnych atrybutów wierzcho³ka
	// operacja ta nie jest konieczna i bêdzie wykonana automatycznie w czasie
	// konsolidacji shadera - przypisany zostanie "pierwszy wolny" indeks
	LinkProgram(toonShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation = glGetAttribLocation(toonShader, "inVertex");
	normalsLocation = glGetAttribLocation(toonShader, "inNormal");

	// wygenerowanie i w³¹czenie tablicy wierzcho³ków .obj
	glGenVertexArrays(1, &objVertexArray);
	glBindVertexArray(objVertexArray);

	//wspó³rzêdne
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

	// w³¹czenie obiektów buforów wiercho³ków
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);

	// wy³¹czenie tablicy wiercho³ków
	glBindVertexArray(0);

	// wygenerowanie i za³adowanie tekstury 
	glGenTextures(1, &tgaTex);
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// za³adowanie tekstury z pliku .tga
	
	cout << "ale jaja\n";
	if (! loadTGATexture(texFileName)) {
		cout << "Nie ma pliku z tekstura: \"" << texFileName << "\"" << endl;
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// w³¹czenie wykorzystania bufora g³êbokoœci
	glEnable(GL_DEPTH_TEST);
	// w³¹czenie pominiêcia renderowania tylnych stron wielok¹tów
	glEnable(GL_CULL_FACE);

	// ustawienie pocz¹tkowego po³o¿enia kamery
	cameraFrame.SetOrigin(0.0f, 0.0f, 4.0f);
	// zdefiniowanie potoku renderowania sk³adaj¹cego siê z dwóch stosów macierzy
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
	return 0;
}

//=============================================================================
// zmiana rozmiaru okna
//=============================================================================
void reshape(int width, int height)
{
	// ustawienie obszaru renderingu - ca³e okno

	glViewport(0, 0, width, height);

	// utworzenie bry³y obciêcia okreœlaj¹cej perspektywê
	viewFrustum.SetPerspective(55.0f, float(width)/float(height), 1.0f, 100.0f);
	// za³adowanie macierzy opisuj¹cej bry³ê obciêcia do macierzy Projekcji
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
}

//=============================================================================
// wyœwietlenie sceny
//=============================================================================
void display(void)
{
	// czyszczenie bufora koloru
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	M3DMatrix44f mCamera;
	// pobranie macierzy 4x4 (z obiektu kamery) zawieraj¹cej przekszta³cenia kamery
	cameraFrame.GetCameraMatrix(mCamera);
	// za³adowanie przekszta³ceñ kamery do stosu macierzy Widoku Modelu
	// (nie ma zatem potrzeby uprzedniego ³adowania macierzy jednostkowej)
	modelViewMatrix.LoadMatrix(mCamera);


	// u¿ycie obiektu shadera
	glUseProgram(toonShader);

	M3DVector4f lightEyeDir;
	// przetransformowanie kierunku œwiat³a do wspó³rzêdnych obserwatora
	m3dTransformVector4(lightEyeDir, lightDir, mCamera);
	// za³adowanie zmiennej jednorodnej - kierunku œwiatla
	// dalsze przekszta³cenia zmieniaj¹ macierz Widoku Modelu, ale nie wp³ywaj¹
	// na œwiat³o - obracamy obiektami a nie œwiat³em
	float light[3] = {5.0, 5.0, 2.0};
	glUniform3fv(glGetUniformLocation(toonShader, "inLightDir"), 1, lightEyeDir);
//	glUniform3fv(glGetUniformLocation(toonShader, "inLightDir"), 1, light);

	// Od³o¿enie obiektu macierzy na stos
	modelViewMatrix.PushMatrix();

	// === przekszta³cenia geometryczne i narysowanie obiektów w danym stanie uk³adu ===
	// przesuniêcie uk³adu
	modelViewMatrix.Translate(-1.0f, 0.0f, 0.0f);
	// wykonanie obrotów uk³adu
	modelViewMatrix.Rotate(rotateX, 1.0f, 0.0f, 0.0f);
	modelViewMatrix.Rotate(rotateY, 0.0f, 1.0f, 0.0f);
	modelViewMatrix.Rotate(rotateZ, 0.0f, 0.0f, 1.0f);

	// za³adowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(toonShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// za³adowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(toonShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	// w³¹czenie tablicy wierzcho³ków szeœcianu
	glBindVertexArray(cubeVertexArray);
	// narysowanie danych zawartych w tablicy wiercho³ków szeœcianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, 3*6*2, GL_UNSIGNED_INT, 0);

	// w³¹czenie tablicy wierzcho³ków .obj
	glBindVertexArray(objVertexArray);
	// narysowanie danych zawartych w tablicy wierzcho³ków .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*obj.nFaces, GL_UNSIGNED_INT, 0);

	// zdjêcie zapamiêtanej macierzy widoku-mocelu ze stosu
	modelViewMatrix.PopMatrix();


	// === przekszta³cenia geometryczne i narysowanie obiektu w innym stanie uk³adu ===
	// Od³o¿enie obiektu macierzy na stos
	modelViewMatrix.PushMatrix();

	// u¿ycie obiektu shadera
	glUseProgram(texShader);

	glUniform3fv(glGetUniformLocation(texShader, "inLightDir"), 1, lightEyeDir);

	// przesuniêcie uk³adu
	modelViewMatrix.Translate(1.0f, 0.0f, 0.0f);
	// wykonanie obrotów uk³adu
	modelViewMatrix.Rotate(rotateX, 1.0f, 0.0f, 0.0f);
	modelViewMatrix.Rotate(rotateY, 0.0f, 1.0f, 0.0f);
	modelViewMatrix.Rotate(rotateZ, 0.0f, 0.0f, 1.0f);

	// za³adowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// za³adowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	// za³adowanie zmiennej jednorodnej - identyfikatora tekstury
	glUniform1i(glGetUniformLocation(texShader ,"fileTexture"), 0);

	// w³¹czenie tablicy wierzcho³ków szeœcianu
	glBindVertexArray(cubeVertexArray);
	
	// w³¹czenie aktywnej tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// narysowanie danych zawartych w tablicy wiercho³ków szeœcianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*6*2, GL_UNSIGNED_INT, 0);

	// wy³¹czenie tablic wierzho³ków
	glBindVertexArray(0);

	// wy³¹czenie tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);

	// zdjêcie zapamiêtanej macierzy widoku-mocelu ze stosu
	modelViewMatrix.PopMatrix();

	// wy³¹czenie shadera
	glUseProgram(0);

	// wyrenderowanie sceny
	glFlush();
}

//=============================================================================
// obs³uga klawiatury - klawisze standardowe
//=============================================================================
void standardKbd(unsigned char key, int x, int y)
{
	GLint viewport[4]; // aktualne parametry okna

	// pobranie w³asnoœci bie¿¹cego okna - wspó³rzêdne x,y okna oraz jego
	// szerokoœæ i wysokoœæ
	glGetIntegerv(GL_VIEWPORT,viewport);

	// obs³uga standardowych klawiszy
	switch (key) {
		// zdefiniowanie obrotów kamery wokó³ osi x, y, z
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
		// zdefiniowanie obrotów uk³adu wokó³ osi x, y, z
		case 'j': rotateX += 1.0f;
			break;
		case 'J': rotateX -= 1.0f;
			break;
		case 'k': rotateY += 1.0f;
			break;
		case 'K': rotateY -= 1.0f;
			break;
		case 'l': rotateZ += 1.0f;
			break;
		case 'L': rotateZ -= 1.0f;
			break;
		// zdefiniowanie wektora kierunku œwiat³a
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
	// (wywo³anie zarejestrowanej funcji do obs³ugi tego zdarzenia)
	glutPostRedisplay();
}

//=============================================================================
// obs³uga klawiatury - klawisze specjalne
//=============================================================================
void specialKbd (int key, int x, int y)
{
	// obs³uga klawiszy funkcyjnych - analogicznie jak podstawowych
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
// g³ówna funkcja programu
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
	// okreœlenie wykorzystywanego profilu - profil CORE pe³na zgodnoœæ z v3.2
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// inicjalizacja biblioteki GLUT
	glutInit(&argc, argv);
	// okreœlenie trybu pracy biblioteki - kolor w formacie RGB
	glutInitDisplayMode(GLUT_RGB);
	// rozmiar tworzonego okna (w pikselach)
	glutInitWindowSize(600, 600);
	// po³o¿enie okna na ekranie (wzglêdem lewego dolnego rogu)
	glutInitWindowPosition(100, 100);
	// stworzenie okna programu
	glutCreateWindow("Oswietlony obiekt .obj - freetlut i GLEW");

	// inicjalizacja biblioteki GLEW
	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	// sprawdzenie poprawnoœci inicjalizacji GLEWa
	if (GLEW_OK != glewErr) {
		// nieudana inicjalizacja biblioteki
		cout << "Blad glewInit: " << glewGetErrorString(glewErr) << endl;
		return 2;
	}
	cout << "Wersja biblioteki GLEW: " << glewGetString(GLEW_VERSION) << endl;


	// wykonanie czynnoœci przygotowawczych programu
	if (init(argv[1], argv[2]))
		return 3;


	// ======================   funkcje callback ==================================
	// funkcja obs³uguj¹ca zdarzenie koniecznoœci odrysowania okna
	glutDisplayFunc(display);
	// funkcja obs³uguj¹ca zdarzenie zwi¹zane ze zmian¹ rozmiaru okna
	glutReshapeFunc(reshape);
	// funkcja obs³uguj¹ca naciœniêcie standardowego klawisza z klawiatury
	glutKeyboardFunc(standardKbd);
	// funkcja obs³uguj¹ca naciœniêcie klawisza specjalnego z klawiatury
	glutSpecialFunc(specialKbd);
 	//=============================================================================
	// g³ówna pêtla programu
	glutMainLoop();

	return 0;
}


