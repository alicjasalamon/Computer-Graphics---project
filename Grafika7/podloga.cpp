// do��czenie plik�w nag��wkowych z biblioteki J.Ganczarskiego
#include "shaders.h"
#include "vecmatquat.h"
#include "obj.h"
#include "targa.h"

// za�adowanie pliku nag��wkowego biblioteki GLEW - znajduj� si� w nim wszystkie
// rozszerzenia biblioteki OpenGL r�wnie� dla wersji wy�szych ni� 3.0
// Nie ma wi�c potrzeby wykorzysytwania pliku nag��wkowego gl3.h
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
GLFrustum			viewFrustum;		// bry�a obci�cia
GLGeometryTransform	transformPipeline;	// potok renderowania
GLFrame				cameraFrame;		// obiekt kamery

// k�ty obrotu obiektu
GLfloat rotateX = 10.0f;
GLfloat rotateY = 20.0f;
GLfloat rotateZ = 30.0f;

// kierunek �r�d�a �wiat�a
GLfloat lightDir[4] = {5.0f, 5.0f, 3.0f, 1.0f};

// identyfikator obiektu programu
GLuint toonShader, texShader;

// identyfikatory obiekt�w tablic wiercho�k�w dla .obj i sze�cianu
GLuint objVertexArray;
GLuint cubeVertexArray;

// obiekt reprezentuj�cy bry�� wczytan� jako .obj
objShape obj;

// identyfiakator tekstury
GLuint tgaTex;

//=============================================================================
// za�adowanie pliku TGA i zrobienie z niego tekstury
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
// inicjalizacja sta�ych element�w maszyny stanu OpenGL
//=============================================================================
int init(char *objFileName, char *texFileName)
{
// definicja sze�cianu:
// wsp�rz�dne wierzcho�k�w wierzcho�ki
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
// Wsp�rz�dne wierzcho�k�w s� za razem wektorami nomralnymi (po ich unormowaniu).

// wsp�rz�dne tekstur dla �cian
GLfloat cubeTexCoords[24*2] = {
	// 1-sza �ciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 2-ga �ciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 3-cia �ciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 4-ta �ciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 5-ta �ciana
    0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	// 6-ta �ciana
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};

// indeksy
GLuint cubeIndices[3*6*2] = {
	// 1-sza �ciana
	0, 1, 2,
	2, 1, 3,
	// 1-ga �ciana
	4, 5, 6,
	6, 5, 7,
	// 3-cia �ciana
	8, 9, 10,
	10, 9, 11,
	// 4-ta �ciana
	12, 13, 14,
	14, 13, 15,
	// 5-ta �ciana
	16, 17, 18,
	18, 17, 19,
	// 6-ta �ciana
	20, 21, 22,
	22, 21, 23
};

	int objError;
	// identyfikatory obiekt�w bufor�w ze wsp�rz�dnymi, normalnymi i indeksami
	GLuint verticesBuffer;
	GLuint normalsBuffer;
	GLuint indicesBuffer;
	GLuint texturesBuffer;
	// indeksy atrybut�w wierzcho�k�w (wsp�rz�dne i normalne) z obiektu shadera
	GLuint verticesLocation;
	GLuint normalsLocation;
	GLuint texCoordsLocation;

	// wczytanie obiektu z pliku .obj i przygotowanie go
	if ( (objError = obj.readFromFile(objFileName)) )
		return objError;
	
	// przeskalowanie wczytanego obj, tak aby by� wpisany w jednostkowy sze�cian
	// o �rodku w pocz�tku uk�adu wsp�rz�dnych
	obj.scale();

	// sprawdzenie czy zosta�y poprawnie zdefiniowane normalne
	if (!obj.nNormals)
		// wygenerowanie u�rednionych normalnych
		obj.genSmoothNormals();
		// wygenerowanie normalnych dla �cianek
		//obj.genFacesNormals();
	else
		if (!obj.normIndGood)
			// gdy indeksy normalnych nie zgadzaj� si� z indeksami wierzho�k�w
			// nale�y przebudowa� obie tablice, aby by�y tak samo indeksowane
			// przbudowanie indeks�w normalnych i je�li trzeba indeks�w wiercho�k�w
			obj.rebuildAttribTable('n');

	// ustawienie koloru t�a
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// wczytanie shader�w i przygotowanie obs�ugi programu
	AttachVertexShader	(texShader, "ADS_light_tex_vs.glsl");
	AttachFragmentShader(texShader, "ADS_light_tex_fs.glsl");
	
	// wykonanie powi�zania pomi�dzy zmienn� a indeksem og�lnych atrybut�w wierzcho�ka
	// operacja ta nie jest konieczna i b�dzie wykonana automatycznie w czasie
	// konsolidacji shadera - przypisany zostanie "pierwszy wolny" indeks
	LinkProgram(texShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation	= glGetAttribLocation(texShader, "inVertex");
	normalsLocation		= glGetAttribLocation(texShader, "inNormal");
	texCoordsLocation	= glGetAttribLocation(texShader, "inTexCoord");

	// wygenerowanie i w��czenie tablicy wierzcho�k�w sze�cianu
	glGenVertexArrays(1, &cubeVertexArray);
	glBindVertexArray(cubeVertexArray);

	// utworzenie obiektu bufora wierzcho�k�w (VBO) i za�adowanie danych
	//wsp�rz�dne
	glGenBuffers(1, &verticesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(verticesLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// normalne:
	glGenBuffers(1, &normalsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalsBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), (GLfloat*)cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(normalsLocation, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// wsp�rz�dne tekstury:
	glGenBuffers(1, &texturesBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texturesBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeTexCoords), (GLfloat*)cubeTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(texCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// indeksy:
	glGenBuffers(1, &indicesBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), (GLuint*)cubeIndices, GL_STATIC_DRAW);

	// w��czenie obiekt�w bufor�w wiercho�k�w
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);
	glEnableVertexAttribArray(texCoordsLocation);

	AttachVertexShader(toonShader, "toon_light_vs.glsl");
	AttachFragmentShader(toonShader, "toon_light_fs.glsl");
	// wykonanie powi�zania pomi�dzy zmienn� a indeksem og�lnych atrybut�w wierzcho�ka
	// operacja ta nie jest konieczna i b�dzie wykonana automatycznie w czasie
	// konsolidacji shadera - przypisany zostanie "pierwszy wolny" indeks
	LinkProgram(toonShader);

	// lokalizacja (indeksy) zmiennych w shaderze
	verticesLocation = glGetAttribLocation(toonShader, "inVertex");
	normalsLocation = glGetAttribLocation(toonShader, "inNormal");

	// wygenerowanie i w��czenie tablicy wierzcho�k�w .obj
	glGenVertexArrays(1, &objVertexArray);
	glBindVertexArray(objVertexArray);

	//wsp�rz�dne
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

	// w��czenie obiekt�w bufor�w wiercho�k�w
	glEnableVertexAttribArray(verticesLocation);
	glEnableVertexAttribArray(normalsLocation);

	// wy��czenie tablicy wiercho�k�w
	glBindVertexArray(0);

	// wygenerowanie i za�adowanie tekstury 
	glGenTextures(1, &tgaTex);
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// za�adowanie tekstury z pliku .tga
	
	cout << "ale jaja\n";
	if (! loadTGATexture(texFileName)) {
		cout << "Nie ma pliku z tekstura: \"" << texFileName << "\"" << endl;
		exit(1);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// w��czenie wykorzystania bufora g��boko�ci
	glEnable(GL_DEPTH_TEST);
	// w��czenie pomini�cia renderowania tylnych stron wielok�t�w
	glEnable(GL_CULL_FACE);

	// ustawienie pocz�tkowego po�o�enia kamery
	cameraFrame.SetOrigin(0.0f, 0.0f, 4.0f);
	// zdefiniowanie potoku renderowania sk�adaj�cego si� z dw�ch stos�w macierzy
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
	return 0;
}

//=============================================================================
// zmiana rozmiaru okna
//=============================================================================
void reshape(int width, int height)
{
	// ustawienie obszaru renderingu - ca�e okno

	glViewport(0, 0, width, height);

	// utworzenie bry�y obci�cia okre�laj�cej perspektyw�
	viewFrustum.SetPerspective(55.0f, float(width)/float(height), 1.0f, 100.0f);
	// za�adowanie macierzy opisuj�cej bry�� obci�cia do macierzy Projekcji
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
}

//=============================================================================
// wy�wietlenie sceny
//=============================================================================
void display(void)
{
	// czyszczenie bufora koloru
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	M3DMatrix44f mCamera;
	// pobranie macierzy 4x4 (z obiektu kamery) zawieraj�cej przekszta�cenia kamery
	cameraFrame.GetCameraMatrix(mCamera);
	// za�adowanie przekszta�ce� kamery do stosu macierzy Widoku Modelu
	// (nie ma zatem potrzeby uprzedniego �adowania macierzy jednostkowej)
	modelViewMatrix.LoadMatrix(mCamera);


	// u�ycie obiektu shadera
	glUseProgram(toonShader);

	M3DVector4f lightEyeDir;
	// przetransformowanie kierunku �wiat�a do wsp�rz�dnych obserwatora
	m3dTransformVector4(lightEyeDir, lightDir, mCamera);
	// za�adowanie zmiennej jednorodnej - kierunku �wiatla
	// dalsze przekszta�cenia zmieniaj� macierz Widoku Modelu, ale nie wp�ywaj�
	// na �wiat�o - obracamy obiektami a nie �wiat�em
	float light[3] = {5.0, 5.0, 2.0};
	glUniform3fv(glGetUniformLocation(toonShader, "inLightDir"), 1, lightEyeDir);
//	glUniform3fv(glGetUniformLocation(toonShader, "inLightDir"), 1, light);

	// Od�o�enie obiektu macierzy na stos
	modelViewMatrix.PushMatrix();

	// === przekszta�cenia geometryczne i narysowanie obiekt�w w danym stanie uk�adu ===
	// przesuni�cie uk�adu
	modelViewMatrix.Translate(-1.0f, 0.0f, 0.0f);
	// wykonanie obrot�w uk�adu
	modelViewMatrix.Rotate(rotateX, 1.0f, 0.0f, 0.0f);
	modelViewMatrix.Rotate(rotateY, 0.0f, 1.0f, 0.0f);
	modelViewMatrix.Rotate(rotateZ, 0.0f, 0.0f, 1.0f);

	// za�adowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(toonShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// za�adowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(toonShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	// w��czenie tablicy wierzcho�k�w sze�cianu
	glBindVertexArray(cubeVertexArray);
	// narysowanie danych zawartych w tablicy wiercho�k�w sze�cianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, 3*6*2, GL_UNSIGNED_INT, 0);

	// w��czenie tablicy wierzcho�k�w .obj
	glBindVertexArray(objVertexArray);
	// narysowanie danych zawartych w tablicy wierzcho�k�w .obj
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*obj.nFaces, GL_UNSIGNED_INT, 0);

	// zdj�cie zapami�tanej macierzy widoku-mocelu ze stosu
	modelViewMatrix.PopMatrix();


	// === przekszta�cenia geometryczne i narysowanie obiektu w innym stanie uk�adu ===
	// Od�o�enie obiektu macierzy na stos
	modelViewMatrix.PushMatrix();

	// u�ycie obiektu shadera
	glUseProgram(texShader);

	glUniform3fv(glGetUniformLocation(texShader, "inLightDir"), 1, lightEyeDir);

	// przesuni�cie uk�adu
	modelViewMatrix.Translate(1.0f, 0.0f, 0.0f);
	// wykonanie obrot�w uk�adu
	modelViewMatrix.Rotate(rotateX, 1.0f, 0.0f, 0.0f);
	modelViewMatrix.Rotate(rotateY, 0.0f, 1.0f, 0.0f);
	modelViewMatrix.Rotate(rotateZ, 0.0f, 0.0f, 1.0f);

	// za�adowanie zmiennej jednorodnej - iloczynu macierzy modelu widoku i projekcji
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewProjectionMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());

	// za�adowanie zmiennej jednorodnej - transponowanej macierzy modelu widoku
	glUniformMatrix4fv(glGetUniformLocation(texShader, "modelViewMatrix"),
		1, GL_FALSE, transformPipeline.GetModelViewMatrix());

	// za�adowanie zmiennej jednorodnej - identyfikatora tekstury
	glUniform1i(glGetUniformLocation(texShader ,"fileTexture"), 0);

	// w��czenie tablicy wierzcho�k�w sze�cianu
	glBindVertexArray(cubeVertexArray);
	
	// w��czenie aktywnej tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);
	// narysowanie danych zawartych w tablicy wiercho�k�w sze�cianu
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 3*6*2, GL_UNSIGNED_INT, 0);

	// wy��czenie tablic wierzho�k�w
	glBindVertexArray(0);

	// wy��czenie tekstury
	glBindTexture(GL_TEXTURE_2D, tgaTex);

	// zdj�cie zapami�tanej macierzy widoku-mocelu ze stosu
	modelViewMatrix.PopMatrix();

	// wy��czenie shadera
	glUseProgram(0);

	// wyrenderowanie sceny
	glFlush();
}

//=============================================================================
// obs�uga klawiatury - klawisze standardowe
//=============================================================================
void standardKbd(unsigned char key, int x, int y)
{
	GLint viewport[4]; // aktualne parametry okna

	// pobranie w�asno�ci bie��cego okna - wsp�rz�dne x,y okna oraz jego
	// szeroko�� i wysoko��
	glGetIntegerv(GL_VIEWPORT,viewport);

	// obs�uga standardowych klawiszy
	switch (key) {
		// zdefiniowanie obrot�w kamery wok� osi x, y, z
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
		// zdefiniowanie obrot�w uk�adu wok� osi x, y, z
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
		// zdefiniowanie wektora kierunku �wiat�a
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
	// (wywo�anie zarejestrowanej funcji do obs�ugi tego zdarzenia)
	glutPostRedisplay();
}

//=============================================================================
// obs�uga klawiatury - klawisze specjalne
//=============================================================================
void specialKbd (int key, int x, int y)
{
	// obs�uga klawiszy funkcyjnych - analogicznie jak podstawowych
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
// g��wna funkcja programu
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
	// okre�lenie wykorzystywanego profilu - profil CORE pe�na zgodno�� z v3.2
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// inicjalizacja biblioteki GLUT
	glutInit(&argc, argv);
	// okre�lenie trybu pracy biblioteki - kolor w formacie RGB
	glutInitDisplayMode(GLUT_RGB);
	// rozmiar tworzonego okna (w pikselach)
	glutInitWindowSize(600, 600);
	// po�o�enie okna na ekranie (wzgl�dem lewego dolnego rogu)
	glutInitWindowPosition(100, 100);
	// stworzenie okna programu
	glutCreateWindow("Oswietlony obiekt .obj - freetlut i GLEW");

	// inicjalizacja biblioteki GLEW
	glewExperimental = GL_TRUE;
	GLenum glewErr = glewInit();
	// sprawdzenie poprawno�ci inicjalizacji GLEWa
	if (GLEW_OK != glewErr) {
		// nieudana inicjalizacja biblioteki
		cout << "Blad glewInit: " << glewGetErrorString(glewErr) << endl;
		return 2;
	}
	cout << "Wersja biblioteki GLEW: " << glewGetString(GLEW_VERSION) << endl;


	// wykonanie czynno�ci przygotowawczych programu
	if (init(argv[1], argv[2]))
		return 3;


	// ======================   funkcje callback ==================================
	// funkcja obs�uguj�ca zdarzenie konieczno�ci odrysowania okna
	glutDisplayFunc(display);
	// funkcja obs�uguj�ca zdarzenie zwi�zane ze zmian� rozmiaru okna
	glutReshapeFunc(reshape);
	// funkcja obs�uguj�ca naci�ni�cie standardowego klawisza z klawiatury
	glutKeyboardFunc(standardKbd);
	// funkcja obs�uguj�ca naci�ni�cie klawisza specjalnego z klawiatury
	glutSpecialFunc(specialKbd);
 	//=============================================================================
	// g��wna p�tla programu
	glutMainLoop();

	return 0;
}


