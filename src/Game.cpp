#include <GLFW/glfw3.h>

int main() {
    glfwInit();

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    GLFWwindow* window = glfwCreateWindow( 640, 480, "Test Window", NULL, NULL );

    while( !glfwWindowShouldClose( window ) ) {
        glfwPollEvents();
    }

    glfwDestroyWindow( window );
    glfwTerminate();

    return 0;
}