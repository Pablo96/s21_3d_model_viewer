#include "loader.hpp"

#include "../utils/utils.hpp"

#define READ_FILE(file, output_block, output_block_size, output_block_count, break_stmt) \
({                                                                                        \
if (feof(file) != 0) {                                                                   \
    break_stmt;                                                                          \
}                                                                                        \
auto amount_read = fread(output_block, output_block_size, output_block_count, file);     \
if (ferror(file) != 0) {                                                                 \
    printf("There was an error reading file: '%s'", filename);                           \
    fclose(file);                                                                        \
    return;                                                                              \
}                                                                                        \
if (amount_read == 0) {                                                                  \
    fclose(file);                                                                        \
    break_stmt;                                                                          \
}                                                                                        \
amount_read;                                                                             \
})

#define SEEK_FILE(file, offset, position, break_stmt)          \
({                                                             \
if (feof(file) != 0) {                                         \
    break_stmt;                                                \
}                                                              \
if (fseek(file, offset, position) != 0) {                      \
    printf("There was an error seeking file: '%s'", filename); \
    fclose(file);                                              \
    return;                                                    \
}                                                              \
if (ferror(file) != 0) {                                       \
    printf("There was an error reading file: '%s'", filename); \
    fclose(file);                                              \
    return;                                                    \
}                                                              \
})

static void load_obj(const char *filename, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count);

static void load_pure_model(const char *filename, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count);

static void calculate_size_and_center(std::vector<glm::vec3> const &vertices, size_t &model_size, glm::vec3 &model_center);

static float convert_float16_to_float32(short float16_value);

void load_model(std::string const &path, std::vector<glm::vec3> &vertices,
                size_t &faces_count, size_t &vertices_count, size_t &model_size, glm::vec3 &model_center)
{
    auto file_ext = get_file_extension(path);
    if (file_ext == "obj") {
        load_obj(path.c_str(), vertices, faces_count, vertices_count);
    } else if (file_ext == "model") {
        load_pure_model(path.c_str(), vertices, faces_count, vertices_count);
    } else {
        std::cout << "Cant open file of type: " << file_ext << std::endl;
        exit(EXIT_FAILURE);
    }

    if (vertices.size() >= 3) {
        calculate_size_and_center(vertices, model_size, model_center);
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

typedef glm::u16vec3 hvec3;
typedef glm::u16vec2 hvec2;

void load_pure_model(const char *filename, std::vector<glm::vec3> &vertices,
                     size_t &faces_count, size_t &vertices_count)
{
    FILE *file = fopen(filename, "rb");
    if (file == nullptr) {
        printf("There was an error opening file: '%s'", filename);
        return;
    }

    uint32_t magic_number;
    READ_FILE(file, &magic_number, sizeof(uint32_t), 1, return);
    if (magic_number != 5) {
        printf("Not a model file. Needs the magic number 5 at the beginning");
        return;
    }

    uint32_t weird_header;
    READ_FILE(file, &weird_header, sizeof(uint32_t), 1, return);

    // Final vertex layout struct??
    // Size = 28 Bytes
    struct Vertex
    {
        hvec3 point;
        hvec3 normal;
        hvec2 uv;
        hvec3 binormals[2];
    };
    typedef Vertex VERTEX_TYPE;
    assert(sizeof(VERTEX_TYPE) == 28);

    std::vector<VERTEX_TYPE> tmp_vertices;
    constexpr uint64_t VERTICES_OFFSET = 0x08;
    constexpr uint64_t VERTEX_COUNT = 2006;
    SEEK_FILE(file, VERTICES_OFFSET, SEEK_SET, return);
    
    for (uint64_t vertex_count = 0; vertex_count != VERTEX_COUNT; vertex_count++)
    {
        VERTEX_TYPE vertex;
        READ_FILE(file, &vertex, sizeof(VERTEX_TYPE), 1, break);
        tmp_vertices.push_back(vertex);
    }

    std::vector<uint16_t> indices;
    constexpr uint64_t INDICES_OFFSET = 0xF630;
    constexpr uint64_t INDICES_COUNT = 4566;
    SEEK_FILE(file, INDICES_OFFSET, SEEK_SET, return);

    for (uint64_t index_count = 0; index_count <= INDICES_COUNT; index_count++)
    {
        uint16_t index;
        READ_FILE(file, &index, sizeof(uint16_t), 1, break);
        indices.push_back(index);
    }
    fclose(file);

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        size_t const max_index = i + 2;
        for (size_t j = max_index; j >= i && j <= max_index; j--)
        {
            auto const short_index = indices[j];
            auto const index = static_cast<size_t>(short_index);
            auto tmp_vertex = tmp_vertices[index];
            auto tmp_x = convert_float16_to_float32(tmp_vertex.point.x);
            auto tmp_y = convert_float16_to_float32(tmp_vertex.point.y);
            auto tmp_z = convert_float16_to_float32(tmp_vertex.point.z);
            vertices.push_back(glm::vec3(tmp_x, tmp_y, tmp_z));
        }
    }

    vertices_count = vertices.size();
    faces_count = static_cast<size_t>(INDICES_COUNT / 3);
}

float convert_float16_to_float32(short float16_value) {
  // Implementation code by milhidaka@github
  // MSB -> LSB
  // float16=1bit: sign, 5bit: exponent, 10bit: fraction
  // float32=1bit: sign, 8bit: exponent, 23bit: fraction
  // for normal exponent(1 to 0x1e): value=2**(exponent-15)*(1.fraction)
  // for denormalized exponent(0): value=2**-14*(0.fraction)
  uint32_t sign = float16_value >> 15;
  uint32_t exponent = (float16_value >> 10) & 0x1F;
  uint32_t fraction = (float16_value & 0x3FF);
  uint32_t float32_value;
  if (exponent == 0)
  {
    if (fraction == 0)
    {
      // zero
      float32_value = (sign << 31);
    }
    else
    {
      // can be represented as ordinary value in float32
      // 2 ** -14 * 0.0101
      // => 2 ** -16 * 1.0100
      // int int_exponent = -14;
      exponent = 127 - 14;
      while ((fraction & (1 << 10)) == 0)
      {
        //int_exponent--;
        exponent--;
        fraction <<= 1;
      }
      fraction &= 0x3FF;
      // int_exponent += 127;
      float32_value = (sign << 31) | (exponent << 23) | (fraction << 13);  
    }    
  }
  else if (exponent == 0x1F)
  {
    /* Inf or NaN */
    float32_value = (sign << 31) | (0xFF << 23) | (fraction << 13);
  }
  else
  {
    /* ordinary number */
    float32_value = (sign << 31) | ((exponent + (127-15)) << 23) | (fraction << 13);
  }
  
  return *((float*)&float32_value);
}

void calculate_size_and_center(std::vector<glm::vec3> const &vertices, size_t &model_size, glm::vec3 &model_center) {
    glm::vec3 min_vert;
    glm::vec3 max_vert;
    for (auto vertex : vertices)
    {
        if (vertex.x < min_vert.x) {
            min_vert.x = vertex.x;
        }
        if (vertex.y < min_vert.y) {
            min_vert.y = vertex.y;
        }
        if (vertex.z < min_vert.z) {
            min_vert.z = vertex.z;
        }

        if (vertex.x > max_vert.x) {
            max_vert.x = vertex.x;
        }
        if (vertex.y > min_vert.y) {
            max_vert.y = vertex.y;
        }
        if (vertex.z > max_vert.z) {
            max_vert.z = vertex.z;
        }
    }

    auto size_vec = max_vert - min_vert;
    model_size = size_vec.length();
    model_center = size_vec * 0.5f;
}

#undef READ_FILE
