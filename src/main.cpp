#define GL_SILENCE_DEPRECATION
#include "main.hpp"

static inline void init_imgui(GLFWwindow *window);
static inline void init_glfw();
static inline void init_glew();
static inline void process_input(GLFWwindow *window);
static inline void imgui_postprocess();
static inline void imgui_preprocess();
static inline GLFWwindow *create_window();
static inline ImGui::FileBrowser init_filebrowser();
static inline void framebuffer_size_callback(GLFWwindow *window, int width,
                                             int height);
static inline void glfw_error_callback(int error, const char *description);
static inline void calculate_camera_position(glm::vec3 &camera_position, float const distance_to_center, float const yaw_angle, float const pitch_angle);

int main(int const argc, char **argv)
{
    printf("Args:\n");
    for (int i = 0; i < argc; i++)
    {
        printf("\t%s\n", argv[i]);
    }
    printf("\n");

    srand(time(0));
    init_glfw();
    GLFWwindow *window = create_window();

    init_glew();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Init VAO (VAO is an array of data buffers used by OpenGL)
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    init_imgui(window);
    ImGui::FileBrowser fileDialog = init_filebrowser();

    // Imgui Defaults
    ImVec4 bg_color = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    int draw_type = GL_TRIANGLES;
    float fov = 45.0f, near = 0.1f, far = 100.0f, view_distance = 0.f, yaw_camera_angle = 0.f, pitch_camera_angle = 90.f;
    glm::vec3 camera_position;


    // Load shaders
    GLuint programID =
        LoadShaders("./shader/vshader.glsl", "./shader/fshader.glsl");
    glUseProgram(programID);

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    // Vertices for loading
    #if 1
    std::string path = "/home/llama/Documents/PureParts/Parts/SWINGARM/SWINGARM_03_LOD1.model";
    #else
    std::string path = "/home/llama/Documents/PureParts/Parts/SWINGARM/SWINGARM_01_LOD3.model";
    #endif
    size_t faces_count = 0;
    size_t vertices_count = 0;
    float model_size = 0;
    glm::vec3 model_center;
    std::vector<glm::vec3> vertices;
    load_model(path, vertices, faces_count, vertices_count, model_size, model_center);
    view_distance = model_size * 1.5f;

    // Vertex buffer to load data into it in the main loop
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);

    GLfloat *color_buffer_data = new GLfloat[vertices.size() * 3 * 3];
    generate_random_colors(color_buffer_data, vertices.size());

    // Color buffer to load colors
    GLuint color_buffer;
    glGenBuffers(1, &color_buffer);

    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window)) {
        // Background color
        glClearColor(bg_color.x * bg_color.w, bg_color.y * bg_color.w,
                     bg_color.z * bg_color.w, bg_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        process_input(window);
        imgui_preprocess();

        // Load colors
        glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * 3 * 3,
                     color_buffer_data, GL_STATIC_DRAW);

        // Load vertices
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                     &vertices[0], GL_STATIC_DRAW);

        // Projection & View
        calculate_camera_position(camera_position, view_distance, yaw_camera_angle, pitch_camera_angle);
        glm::mat4 MVP = compute_mvp(fov, camera_position + model_center, model_center, near, far);

        // Send our transformation to the currently bound shader,
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        //  Enable to use attributes in a vertex shader
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        // Drawing GL_LINE_STRIP GL_TRIANGLES
        glDrawArrays(draw_type, 0, vertices.size());

        // Disable to avoid OpenGL reading from arrays bound to an invalid ptr
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Main GUI window
        {
            if (ImGui::Begin("Main Menu")) {
                if (ImGui::Button("Open file"))
                    fileDialog.Open();

                ImGui::SameLine();
                ImGui::Text("%s", get_filename(path).c_str());
                ImGui::Text("Vertices: %zu", vertices_count);
                ImGui::SameLine();
                ImGui::Text("Edges: %zu", faces_count);
                ImGui::SameLine();
                ImGui::Text("ModelSize: %.2f", model_size);
                ImGui::ColorEdit4("Color", (float *)&bg_color);
                ImGui::RadioButton("GL_TRIANGLES", &draw_type, GL_TRIANGLES);
                ImGui::SameLine();
                ImGui::RadioButton("GL_LINE_STRIP", &draw_type, GL_LINE_STRIP);
                ImGui::SameLine();
                ImGui::RadioButton("GL_POINTS", &draw_type, GL_POINTS);
                
                if (ImGui::Button("Reset##ModelCenter")) {
                    model_center.x = 0.0f;
                    model_center.y = 0.0f;
                    model_center.z = 0.0f;
                }
                ImGui::SameLine();
                ImGui::DragFloat3("Model Center", &model_center.x, 0.1f, .0f, .0f, "%.2f");

                if (ImGui::Button("Reset View")) {
                    yaw_camera_angle = 0.0f;
                    pitch_camera_angle = 90.0f;
                    view_distance = static_cast<float>(model_size) * 0.5f;
                }
                ImGui::DragFloat("yaw", &yaw_camera_angle, 0.1f, .0f, 360.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat);
                ImGui::DragFloat("pitch", &pitch_camera_angle, 0.1f, 0.01f, 179.99f, "%.2f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat);
                ImGui::DragFloat("distance", &view_distance, 0.1f, .01f, 10000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat);

                if (ImGui::Button("Reset##Fov"))
                    fov = 45.0f;
                ImGui::SameLine();
                ImGui::SliderFloat("FOV", &fov, 4.20f, 120.0f);
                if (ImGui::Button("Reset##Near"))
                    near = 0.1f;
                ImGui::SameLine();
                ImGui::DragFloat("Near", &near, 0.1f, .01f, 1000.0f, "%.2f");
                if (ImGui::Button("Reset##Far"))
                    far = 100.0f;
                ImGui::SameLine();
                ImGui::DragFloat("Far", &far, 0.1f, 1.0f, 10000.0f, "%.2f");
            }
            ImGui::End();

            fileDialog.Display();

            if (fileDialog.HasSelected()) {
                path = fileDialog.GetSelected();

                vertices.clear();
                faces_count = 0;
                vertices_count = 0;

                load_model(path.c_str(), vertices, faces_count, vertices_count, model_size, model_center);
                view_distance = model_size * 1.5f;


                delete[] color_buffer_data;
                color_buffer_data = new GLfloat[vertices.size() * 3 * 3];
                generate_random_colors(color_buffer_data, vertices.size());

                fileDialog.ClearSelected();
            }
        }

        imgui_postprocess();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteBuffers(1, &color_buffer);
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

    glfwTerminate();

    delete[] color_buffer_data;
    exit(EXIT_SUCCESS);
}

void calculate_camera_position(glm::vec3 &camera_position, float const distance_to_center, float const yaw_angle, float const pitch_angle) {
    auto const theta = glm::radians(pitch_angle);
    auto const phi = glm::radians(yaw_angle);
    camera_position.x = distance_to_center * glm::cos(phi) * glm::sin(theta);
    camera_position.z = distance_to_center * glm::sin(phi) * glm::sin(theta);
    camera_position.y = distance_to_center * glm::cos(theta);
}

/**
 * @brief Setting up callback for errors
 *
 * @param error Error ID
 * @param description Current error text
 */
static inline void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

/**
 * @brief Setting up resize callback
 *
 * @param window Ptr to the current window
 * @param width Window width
 * @param height Window height
 */
static inline void framebuffer_size_callback(GLFWwindow *window, int width,
                                             int height) {
    glViewport(0, 0, width, height);
}

/**
 * @brief Wait for esc to close window
 *
 * @param window Ptr to current window
 */
static inline void process_input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

/**
 * @brief All the woodoo magic to initalize glfw
 */
static inline void init_glfw() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

/**
 * @brief Imgui initialization
 *
 * @param window Ptr to the current window
 */
static inline void init_imgui(GLFWwindow *window) {
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

/**
 * @brief Filebrowser initialization
 *
 * @return Returns the FileBrowser instance
 */
static inline ImGui::FileBrowser init_filebrowser() {
    ImGui::FileBrowser fileDialog;
    fileDialog.SetTitle("Select a 3d model to view:");
    fileDialog.SetTypeFilters({".obj", ".model"});
    return fileDialog;
}

/**
 * @brief Creates a window using glfw
 *
 * @return Returns the ptr to the window created
 */
static inline GLFWwindow *create_window() {
    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
                                          PROGRAM_TITLE, NULL, NULL);
    if (!window)
        exit(EXIT_FAILURE);

    // Adjust viewport on resize
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwMakeContextCurrent(window);
    // Enable v-sync
    glfwSwapInterval(1);
    return window;
}

/**
 * @brief Imgui preprocessing
 */
static inline void imgui_preprocess() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

/**
 * @brief Imgui postprocessing
 */
static inline void imgui_postprocess() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief Glew initialization & Error checking
 */
static inline void init_glew() {
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
        exit(EXIT_FAILURE);
}
