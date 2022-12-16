#include "utils.hpp"

std::string get_filename(std::string s) {
    return s.substr(s.find_last_of("/") + 1);
}

glm::mat4 compute_mvp(float const &fov, glm::vec3 const &camera_pos, float const &near, float const &far) {
    glm::mat4 Projection = glm::perspective(
        glm::radians(fov),
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
         near, far
    );

    glm::mat4 View =
        glm::lookAt(camera_pos,            // Camera is at in World Space
                    glm::vec3(0, 0, 0),  // Camera looking at (origin)
                    glm::vec3(0, 1, 0)   // Head (0,-1,0 to look upside-down)
        );

    // Models are always centered
    //glm::mat4 Model = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 MVP = Projection * View /* * Model*/;
    return MVP;
}

void generate_random_colors(GLfloat colors[], size_t size) {
    for (size_t v = 0; v < size * 3; v++) {
        colors[3 * v + 0] = float(rand()) / float(RAND_MAX);
        colors[3 * v + 1] = float(rand()) / float(RAND_MAX);
        colors[3 * v + 2] = float(rand()) / float(RAND_MAX);
    }
}

std::string get_file_extension(std::string const &s) {
    auto index = s.find_last_of('.');
    return s.substr(index + 1);
}
