
#include <gl/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "windows.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>




GLuint Program;

struct Vertex
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct Texture {
    GLfloat x;
    GLfloat y;
};

struct Normale
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

struct PolygPoint {
    Vertex vertex;
    Texture tex;
    Normale norm;
};

GLint attribute_vertex;
GLint attribute_texture;
GLint attribute_normal;

GLint uniform_rotate;
GLint uniform_texture;
GLint uniform_move;
GLint uniform_scale;


GLuint busVBO;
GLuint busVAO;

GLint textureHandle;
sf::Texture bus_texture;


GLuint roadVBO;
GLuint roadVAO;

GLint textureHandle_road;
sf::Texture road_texture;


GLuint grassVBO;
GLuint grassVAO;

GLint textureHandle_grass;
sf::Texture grass_texture;


float viewPosition[3] = { 2.0, 2.0, 5.0 };

struct PointLight {
    float position[3] = { 0.0f, 8.0f, 0.0f };
};

PointLight light;


GLint trans_viewPos;

GLint light_pos;


const char* VertexShaderSource = R"(
    #version 330 core

    in vec3 vertex_pos;
    in vec2 texcoord;
    in vec3 normal;

	uniform vec3 rotate;
    uniform vec3 move;
    uniform vec3 scale;

    uniform vec3 viewPosition;

	uniform struct PointLight {
		vec3 position;
	} light;
    
	out Vertex {
		vec2 texcoord;
		vec3 normal;
		vec3 lightDir;
		vec3 viewDir;
		float distance;
	} Vert;

    void main() {
 
        vec3 vertex = vertex_pos * mat3(
          cos(3.14159), 0, -sin(3.14159),
            0, 1, 0,
         -sin(3.14159), 0, cos(3.14159)  
        );
        
        vertex *= mat3(
            1, 0, 0,
            0, cos(rotate[0]), -sin(rotate[0]),
            0, sin(rotate[0]), cos(rotate[0])
        ) 
        * mat3(
            cos(rotate[1]), 0, sin(rotate[1]),
            0, 1, 0,
            -sin(rotate[1]), 0, cos(rotate[1])
        ) 
        * mat3(
			cos(rotate[2]), -sin(rotate[2]), 0,
			sin(rotate[2]), cos(rotate[2]), 0,
			0, 0, 1
		) * mat3(
            scale[0], 0, 0,
            0, scale[1], 0,
            0, 0, scale[2]            
        );	

        vec4 newvertex = vec4(vertex, 1.0) * mat4(
            1, 0, 0, move[0],
            0, 1, 0, move[1],
            0, 0, 1, move[2],
            0, 0, 0, 1
        );

        vec3 lightDir = light.position - vec3(newvertex);
        
        float c = -1.0f;

        newvertex *= mat4 (
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, -1/c, 1
        );
        
        gl_Position = vec4(newvertex[0], newvertex[1], newvertex[2] + 0.5, newvertex[3] + 0.6);
        
        Vert.normal = normal * mat3(
            1, 0, 0,
            0, cos(rotate[0]), -sin(rotate[0]),
            0, sin(rotate[0]), cos(rotate[0])
        ) 
        * mat3(
            cos(rotate[1]), 0, sin(rotate[1]),
            0, 1, 0,
            -sin(rotate[1]), 0, cos(rotate[1])
        ) 
        * mat3(
			cos(rotate[2]), -sin(rotate[2]), 0,
			sin(rotate[2]), cos(rotate[2]), 0,
			0, 0, 1
		);

        Vert.texcoord = texcoord;
        Vert.lightDir = vec3(lightDir);
		Vert.viewDir = viewPosition - vec3(newvertex);
		Vert.distance = length(lightDir);
    }
)";


const char* FragShaderSource = R"(
    #version 330 core
    
    out vec4 color;

    uniform struct PointLight {
		vec3 position;
	} light;

    uniform sampler2D textureData;

    in Vertex {
		vec2 texcoord;
		vec3 normal;
		vec3 lightDir;
		vec3 viewDir;
		float distance;
	} Vert;
	
    void main() {
		vec3 n2 = normalize(Vert.normal);
		vec3 l2 = normalize(Vert.lightDir);
		vec3 v2 = normalize(Vert.viewDir);
	
		const float k = 1.0;
            
        float d1 = pow(max(dot(n2, l2), 0.0), 1.0 + k);
        float d2 = pow(1.0 - dot(n2, v2), 1.0 - k);        

        color = d1 * d2 * texture(textureData, Vert.texcoord);
    }
)";

void Init();
void Draw();
void Release();

float busrotate[3] = { -0.2f, 0.0f, 0.0f };
float busmove[3] = { 0.0f, -0.45f, 0.7f };
float busscale[3] = { 0.1f, 0.1f, 0.1f };

float roadrotate[3] = { -0.2f, 0.0f, 0.0f };
float roadmove[3] = { 0.0f, -0.7f, 0.0f };
float roadscale[3] = { 0.1f, 0.1f, 0.1f };

float grassleftrotate[3] = { -0.2f, 0.0f, 0.0f };
float grassleftmove[3] = { -11.5f, -0.7f, 0.0f };
float grassleftscale[3] = { 0.1f, 0.1f, 0.1f };

float grassrightrotate[3] = { 0.2f, 3.14f, 0.0f };
float grassrightmove[3] = { 11.5f, -0.7f, 0.0f };
float grassrightscale[3] = { 0.1f, 0.1f, 0.1f };

int main() {
    sf::Window window(sf::VideoMode(900, 900), "Bus and road", sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);

    window.setActive(true);

    
    glewInit();

    Init();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }
            // обработка нажатий клавиш
            else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case (sf::Keyboard::Right):
                    busrotate[1] += 0.04;
                    busmove[0] += 0.07;
                    break;
                case (sf::Keyboard::Left):
                    busrotate[1] -= 0.04;
                    busmove[0] -= 0.07;
                    break;
                case (sf::Keyboard::D):
                    light.position[0] += 0.1;  break;
                case (sf::Keyboard::S):
                    light.position[2] += 0.1;  break;
                case (sf::Keyboard::A):
                    light.position[0] -= 0.1;  break;
                case (sf::Keyboard::W):
                    light.position[2] -= 0.1;  break;
                case (sf::Keyboard::Q):
                    light.position[1] += 0.1;  break;
                case (sf::Keyboard::E):
                    light.position[1] -= 0.1;  break;
                case (sf::Keyboard::F):
                    light.position[1] = -99.0f;
                case (sf::Keyboard::G):
                    light.position[1] = 8.0f;
                default: break;
                }
            }
            else if (event.type == sf::Event::KeyReleased) {
                switch (event.key.code) {
                case (sf::Keyboard::Right):
                    busrotate[1] = 0.0f; break;
                case (sf::Keyboard::Left):
                    busrotate[1] = 0.0f; break;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Draw();

        window.display();
    }

    Release();
    return 0;
}



void checkOpenGLerror() {
    GLenum errCode;
    // Коды ошибок можно смотреть тут
    // https://www.khronos.org/opengl/wiki/OpenGL_Error
    if ((errCode = glGetError()) != GL_NO_ERROR)
        std::cout << "OpenGl error!: " << errCode << std::endl;
}


void ShaderLog(unsigned int shader)
{
    int infologLen = 0;
    int charsWritten = 0;
    char* infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
        infoLog = new char[infologLen];
        if (infoLog == NULL)
        {
            std::cout << "ERROR: Could not allocate InfoLog buffer" << std::endl;
            exit(1);
        }
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
        std::cout << "InfoLog: " << infoLog << "\n\n\n";
        delete[] infoLog;
    }
}




std::vector<Vertex> verticies;
std::vector<Normale> normales;
std::vector<Texture> textures;
std::vector<PolygPoint> polypoints;

void ReadFile(std::string fname) {
    verticies.clear();
    normales.clear();
    textures.clear();
    polypoints.clear();
    std::ifstream infile(fname);
    std::string dscrp;
    while (infile >> dscrp)
    {
        if (dscrp == "v")
        {
            float x, y, z;
            infile >> x >> y >> z;
            verticies.push_back({ x, y, z });
        }
        if (dscrp == "vt")
        {
            float x, y;
            infile >> x >> y;
            textures.push_back({ x, y });
        }
        if (dscrp == "vn")
        {
            float x, y, z;
            infile >> x >> y >> z;
            normales.push_back({ x, y, z });
        }
        if (dscrp == "f")
        {
            for (int i = 0; i < 3; i++)
            {
                char c;

                int vert_index;
                infile >> vert_index;
                infile >> c;

                int tex_index;
                infile >> tex_index;
                infile >> c;

                int norm_index;
                infile >> norm_index;

                polypoints.push_back({ verticies[vert_index - 1], textures[tex_index - 1], normales[norm_index - 1] });
            }
        }
    }
}

int bussize = 0;
int roadsize = 0;
int grasssize = 0;
int horizonsize = 0;

void InitVBO()
{
    std::vector<float> points;
    ReadFile("bus2.obj");

    for (int i = 0; i < polypoints.size(); i++)
    {
        points.push_back(polypoints[i].vertex.x);
        points.push_back(polypoints[i].vertex.y);
        points.push_back(polypoints[i].vertex.z);
        points.push_back(polypoints[i].tex.x);
        points.push_back(1 - polypoints[i].tex.y);
        points.push_back(polypoints[i].norm.x);
        points.push_back(polypoints[i].norm.y);
        points.push_back(polypoints[i].norm.z);
    }
    bussize = polypoints.size();
    glGenBuffers(1, &busVBO);
    glGenVertexArrays(1, &busVAO);

    glBindVertexArray(busVAO);

    glEnableVertexAttribArray(attribute_vertex);
    glEnableVertexAttribArray(attribute_texture);
    glEnableVertexAttribArray(attribute_normal);
    glBindBuffer(GL_ARRAY_BUFFER, busVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(attribute_vertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glVertexAttribPointer(attribute_texture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glVertexAttribPointer(attribute_normal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));

    points.clear();

    ReadFile("road.obj");

    for (int i = 0; i < polypoints.size(); i++)
    {
        points.push_back(polypoints[i].vertex.x);
        points.push_back(polypoints[i].vertex.y);
        points.push_back(polypoints[i].vertex.z);
        points.push_back(polypoints[i].tex.x);
        points.push_back(1 - polypoints[i].tex.y);
        points.push_back(polypoints[i].norm.x);
        points.push_back(polypoints[i].norm.y);
        points.push_back(polypoints[i].norm.z);
    }
    roadsize = polypoints.size();

    glGenBuffers(1, &roadVBO);
    glGenVertexArrays(1, &roadVAO);

    glBindVertexArray(roadVAO);

    glEnableVertexAttribArray(attribute_vertex);
    glEnableVertexAttribArray(attribute_texture);
    glEnableVertexAttribArray(attribute_normal);
    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(attribute_vertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glVertexAttribPointer(attribute_texture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glVertexAttribPointer(attribute_normal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));

    points.clear();
    
    ReadFile("grass.obj");

    for (int i = 0; i < polypoints.size(); i++)
    {
        points.push_back(polypoints[i].vertex.x);
        points.push_back(polypoints[i].vertex.y);
        points.push_back(polypoints[i].vertex.z);
        points.push_back(polypoints[i].tex.x);
        points.push_back(1 - polypoints[i].tex.y);
        points.push_back(polypoints[i].norm.x);
        points.push_back(polypoints[i].norm.y);
        points.push_back(polypoints[i].norm.z);
        
    }
    grasssize = polypoints.size();
    
    glGenBuffers(1, &grassVBO);
    glGenVertexArrays(1, &grassVAO);

    glBindVertexArray(grassVAO);

    glEnableVertexAttribArray(attribute_vertex);
    glEnableVertexAttribArray(attribute_texture);
    glEnableVertexAttribArray(attribute_normal);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GLfloat), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(attribute_vertex, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glVertexAttribPointer(attribute_texture, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glVertexAttribPointer(attribute_normal, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));

    points.clear();
    checkOpenGLerror();
}



void InitShader() {
    // Создаем вершинный шейдер
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    // Передаем исходный код
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    // Компилируем шейдер
    glCompileShader(vShader);
    std::cout << "vertex shader \n";
    ShaderLog(vShader);

    // Создаем фрагментный шейдер
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    // Передаем исходный код
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    // Компилируем шейдер
    glCompileShader(fShader);
    std::cout << "fragment shader \n";
    ShaderLog(fShader);

    // Создаем программу и прикрепляем шейдеры к ней
    Program = glCreateProgram();
    glAttachShader(Program, vShader);
    glAttachShader(Program, fShader);

    // Линкуем шейдерную программу
    glLinkProgram(Program);
    // Проверяем статус сборки
    int link_ok;
    glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        std::cout << "error attach shaders \n";
        return;
    }
    attribute_vertex = glGetAttribLocation(Program, "vertex_pos");
    if (attribute_vertex == -1)
    {
        std::cout << "could not bind attrib vertex_pos" << std::endl;
        return;
    }
    attribute_texture = glGetAttribLocation(Program, "texcoord");
    if (attribute_texture == -1)
    {
        std::cout << "could not bind attrib texcoord" << std::endl;
        return;
    }
    attribute_normal = glGetAttribLocation(Program, "normal");
    if (attribute_normal == -1)
    {
        std::cout << "could not bind attrib normal" << std::endl;
        return;
    }
    uniform_rotate = glGetUniformLocation(Program, "rotate");
    if (uniform_rotate == -1)
    {
        std::cout << "could not bind uniform rotate " << std::endl;
        return;
    }
    uniform_move = glGetUniformLocation(Program, "move");
    if (uniform_move == -1)
    {
        std::cout << "could not bind uniform move" << std::endl;
        return;
    }
    uniform_scale = glGetUniformLocation(Program, "scale");
    if (uniform_scale == -1)
    {
        std::cout << "could not bind uniform scale " << std::endl;
        return;
    }
    uniform_texture = glGetUniformLocation(Program, "textureData");
    if (uniform_texture == -1)
    {
        std::cout << "could not bind uniform textureData" << std::endl;
        return;
    }
    trans_viewPos = glGetUniformLocation(Program, "viewPosition");
    if (trans_viewPos == -1)
    {
        std::cout << "could not bind uniform viewPosition " << std::endl;
        return;
    }
    light_pos = glGetUniformLocation(Program, "light.position");
    if (light_pos == -1)
    {
        std::cout << "could not bind uniform light.position " << std::endl;
        return;
    }
    checkOpenGLerror();
}

void InitTexture()
{
    const char* filename = "bus2.png";
    if (!bus_texture.loadFromFile(filename))
    {
        return;
    }
    textureHandle = bus_texture.getNativeHandle();

    filename = "road.png";
    if (!road_texture.loadFromFile(filename))
    {
        return;
    }
    textureHandle_road = road_texture.getNativeHandle();

    filename = "grass.png";
    if (!grass_texture.loadFromFile(filename))
    {
        return;
    }
    textureHandle_grass = grass_texture.getNativeHandle();
}

void Init() {
    InitShader();
    InitVBO();
    InitTexture();
}




void Draw() {
    
    glUseProgram(Program);
    glEnable(GL_DEPTH_TEST);
    
    glUniform3fv(trans_viewPos, 1, viewPosition);
    glUniform3fv(light_pos, 1, light.position);

    glActiveTexture(GL_TEXTURE0);
    sf::Texture::bind(&bus_texture);
    glUniform1i(uniform_texture, 0);

    glUniform3fv(uniform_rotate, 1, busrotate);
    glUniform3fv(uniform_move, 1, busmove);
    glUniform3fv(uniform_scale, 1, busscale);

    glBindVertexArray(busVAO);
    glDrawArrays(GL_TRIANGLES, 0, bussize);

    glActiveTexture(GL_TEXTURE0);
    sf::Texture::bind(&road_texture);
    glUniform1i(uniform_texture, 0);

    glUniform3fv(uniform_rotate, 1, roadrotate);
    glUniform3fv(uniform_move, 1, roadmove);
    glUniform3fv(uniform_scale, 1, roadscale);

    glBindVertexArray(roadVAO);
    glDrawArrays(GL_TRIANGLES, 0, roadsize);

    glActiveTexture(GL_TEXTURE0);
    sf::Texture::bind(&grass_texture);
    glUniform1i(uniform_texture, 0);

    glUniform3fv(uniform_rotate, 1, grassleftrotate);
    glUniform3fv(uniform_move, 1, grassleftmove);
    glUniform3fv(uniform_scale, 1, grassleftscale);

    glBindVertexArray(grassVAO);
    glDrawArrays(GL_TRIANGLES, 0, grasssize);

    glUniform3fv(uniform_rotate, 1, grassrightrotate);
    glUniform3fv(uniform_move, 1, grassrightmove);
    glUniform3fv(uniform_scale, 1, grassrightscale);

    glDrawArrays(GL_TRIANGLES, 0, grasssize);

    glUseProgram(0);
    checkOpenGLerror();
}


// Освобождение шейдеров
void ReleaseShader() {
    // Передавая ноль, мы отключаем шейдрную программу
    glUseProgram(0);
    // Удаляем шейдерную программу
    glDeleteProgram(Program);
}

// Освобождение буфера
void ReleaseVBO()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteVertexArrays(1, &busVAO);
    glDeleteBuffers(1, &busVBO);
    glDeleteVertexArrays(1, &roadVAO);
    glDeleteBuffers(1, &roadVBO);
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteBuffers(1, &grassVBO);
}

void Release() {
    ReleaseShader();
    ReleaseVBO();
}