#include "loader.hpp"

#include "../utils/utils.hpp"

static void load_obj(const char *filename, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count);

static void load_pure_model(const char *filename, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count);

void load_model(std::string const &path, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count) {
    auto file_ext = get_file_extension(path);
    if (file_ext == "obj") {
        load_obj(path.c_str(), vertices, faces_count, vertices_count);
    } else if (file_ext == "model") {
        load_pure_model(path.c_str(), vertices, faces_count, vertices_count);
    } else {
        std::cout << "Cant open file of type: " << file_ext << std::endl;
        exit(EXIT_FAILURE);
    }
}

void load_obj(const char *filename, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count) {
    FILE *file = fopen(filename, "r");

    std::vector<size_t> vertex_idx;
    std::vector<glm::vec3> tmp_vertices;

    char *line = NULL;
    size_t linecap = 0;

    while (getline(&line, &linecap, file) > 0) {
        if (line[0] == 'v') {
            vertices_count++;
            glm::vec3 vertex;
            sscanf(line + 1, "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
            tmp_vertices.push_back(vertex);
        }
        if (line[0] == 'f') {
            faces_count++;
            size_t v_index[3];
            sscanf(line + 1, "%zu %zu %zu", &v_index[0], &v_index[1],
                   &v_index[2]);
            vertex_idx.push_back(v_index[0]);
            vertex_idx.push_back(v_index[1]);
            vertex_idx.push_back(v_index[2]);
        }
    }

    for (size_t i = 0; i < vertex_idx.size(); i++) {
        glm::vec3 vertex = tmp_vertices[vertex_idx[i] - 1];
        vertices.push_back(vertex);
    }

    free(line);
    fclose(file);
}

void load_pure_model(const char *filename, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count) {
    FILE *file = fopen(filename, "rb");

    std::vector<size_t> vertex_idx;

    struct Vertex {
        glm::vec3 point;
        glm::vec2 uv;
    };
    typedef Vertex VERTEX_TYPE;
    constexpr auto VERTEX_SIZE = sizeof(VERTEX_TYPE);
    std::vector<VERTEX_TYPE> tmp_vertices;
    VERTEX_TYPE vertex;

    while (true)
    {
        auto has_content = fread(&vertex, VERTEX_SIZE, 1, file);
        if (!has_content)
            break;

        tmp_vertices.push_back(vertex);
    }

    for (size_t i = 0; i < vertex_idx.size(); i++) {
        glm::vec3 vertex = tmp_vertices[vertex_idx[i] - 1].point;
        if (vertex.length() > 5)
            vertex = glm::normalize(vertex);
        vertices.push_back(vertex);
    }

    fclose(file);
}
