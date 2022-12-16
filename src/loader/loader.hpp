#ifndef LOADER_H_
#define LOADER_H_

#include "../includes/common.h"
/**
 * @brief Loads any model type
 *
 * @param filename Path to the file
 * @param vertices Vector of vertices that we'll read from the file
 * @param faces_count Faces count
 * @param vertices_count Vertices count
 */
void load_model(std::string const &path, std::vector<glm::vec3> &vertices,
              size_t &faces_count, size_t &vertices_count, size_t &model_size, glm::vec3 &model_center);

#endif  // LOADER_H_
