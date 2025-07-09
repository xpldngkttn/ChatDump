#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <array>

struct Cube {
    glm::vec3 color;
    float rotationSpeed;
    std::array<glm::vec3, 36> verts;
};

static Cube gCube = {
    glm::vec3(0.2f, 0.7f, 0.9f),
    50.0f,
    {   // positions for 12 triangles
        glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(0.5f,-0.5f,-0.5f), glm::vec3(0.5f,0.5f,-0.5f),
        glm::vec3(0.5f,0.5f,-0.5f), glm::vec3(-0.5f,0.5f,-0.5f), glm::vec3(-0.5f,-0.5f,-0.5f),
        glm::vec3(-0.5f,-0.5f,0.5f), glm::vec3(0.5f,-0.5f,0.5f), glm::vec3(0.5f,0.5f,0.5f),
        glm::vec3(0.5f,0.5f,0.5f), glm::vec3(-0.5f,0.5f,0.5f), glm::vec3(-0.5f,-0.5f,0.5f),
        glm::vec3(-0.5f,0.5f,0.5f), glm::vec3(-0.5f,0.5f,-0.5f), glm::vec3(-0.5f,-0.5f,-0.5f),
        glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(-0.5f,-0.5f,0.5f), glm::vec3(-0.5f,0.5f,0.5f),
        glm::vec3(0.5f,0.5f,0.5f), glm::vec3(0.5f,0.5f,-0.5f), glm::vec3(0.5f,-0.5f,-0.5f),
        glm::vec3(0.5f,-0.5f,-0.5f), glm::vec3(0.5f,-0.5f,0.5f), glm::vec3(0.5f,0.5f,0.5f),
        glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(0.5f,-0.5f,-0.5f), glm::vec3(0.5f,-0.5f,0.5f),
        glm::vec3(0.5f,-0.5f,0.5f), glm::vec3(-0.5f,-0.5f,0.5f), glm::vec3(-0.5f,-0.5f,-0.5f),
        glm::vec3(-0.5f,0.5f,-0.5f), glm::vec3(0.5f,0.5f,-0.5f), glm::vec3(0.5f,0.5f,0.5f),
        glm::vec3(0.5f,0.5f,0.5f), glm::vec3(-0.5f,0.5f,0.5f), glm::vec3(-0.5f,0.5f,-0.5f)
    }
};

static const char* vShaderSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 mvp;
void main(){ gl_Position = mvp * vec4(aPos,1.0); }
)";

static const char* fShaderSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main(){ FragColor = vec4(color,1.0); }
)";

GLuint compileShader(GLenum type, const char* src){
    GLuint s = glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    return s;
}

int main(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    GLFWwindow* win = glfwCreateWindow(800,600,"Cube",nullptr,nullptr);
    if(!win){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glewInit();

    GLuint vs = compileShader(GL_VERTEX_SHADER,vShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER,fShaderSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program,vs); glAttachShader(program,fs); glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);

    GLuint vao,vbo;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec3)*gCube.verts.size(),gCube.verts.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);

    float angle = 0.0f;
    while(!glfwWindowShouldClose(win)){
        glfwPollEvents();
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        angle += gCube.rotationSpeed * 0.001f; // assume ms
        glm::mat4 proj = glm::perspective(glm::radians(60.0f),800.f/600.f,0.1f,100.f);
        glm::mat4 view = glm::lookAt(glm::vec3(2,2,2),glm::vec3(0,0,0),glm::vec3(0,1,0));
        glm::mat4 model = glm::rotate(glm::mat4(1.0f),angle,glm::vec3(0,1,0));
        glm::mat4 mvp = proj*view*model;
        glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,&mvp[0][0]);
        glUniform3fv(glGetUniformLocation(program,"color"),1,&gCube.color[0]);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES,0,gCube.verts.size());

        glfwSwapBuffers(win);
    }
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
    glDeleteProgram(program);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
