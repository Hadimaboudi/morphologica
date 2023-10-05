#pragma once
#include <morph/VisualCommon.h>

namespace morph {
    namespace gl {

        struct compute_shaderprog
        {
            GLuint prog_id = 0;

            // Default constructor
            compute_shaderprog() {}

            /*
             * Construct with a structure of shader files looking like this:
             *
             * std::vector<morph::gl::ShaderInfo> shaders = {
             *   {GL_COMPUTE_SHADER, "my_compute_shader.glsl", morph::defaultComputeShader }
             * };
             */
            compute_shaderprog (const std::vector<morph::gl::ShaderInfo>& shaders)
            {
                this->load_shaders (shaders);
            }

            ~compute_shaderprog()
            {
                if (this->prog_id) {
                    glDeleteProgram (this->prog_id);
                    this->prog_id = 0;
                }
            }

            void load_shaders (const std::vector<morph::gl::ShaderInfo>& shaders)
            {
                this->prog_id = morph::gl::LoadShaders (shaders);
            }

            void use() const { glUseProgram (this->prog_id); }

            // Set a uniform variable into the OpenGL context associated with this shader program
            template <typename T>
            void set_uniform (const std::string& glsl_varname, const T& value)
            {
                GLint uloc = glGetUniformLocation (this->prog_id, static_cast<const GLchar*>(glsl_varname.c_str()));

                if constexpr (std::is_same<std::decay_t<T>, float>::value == true) {
                    if (uloc != -1) { glUniform1f (uloc, static_cast<GLfloat*>(&value)); }
                } else if constexpr (std::is_same<std::decay_t<T>, int>::value == true) {
                    if (uloc != -1) { glUniform1i (uloc, static_cast<GLint*>(&value)); }
                } else if constexpr (std::is_same<std::decay_t<T>, unsigned int>::value == true) {
                    if (uloc != -1) { glUniform1ui (uloc, static_cast<GLuint*>(&value)); }
                } else {
                    []<bool flag = false>() { static_assert(flag, "Can't set that type as a uniform in an OpenGL context"); }();
                }
            }

        };

    } // namespace gl
} // namespace morph
